#include "bee.h"
#include <opt/stdlib/egg-stdlib.h>

/* Begin.
 */
 
void encounter_begin(struct encounter *en) {
  fprintf(stderr,"%s\n",__func__);
  
  letterbag_reset(&en->letterbag);
  letterbag_draw(en->hand,&en->letterbag);
  foe_reset(&en->foe,en);
  
  en->cursor.x=0;
  en->cursor.y=2;
  
  en->active=1;
  en->phase=ENCOUNTER_PHASE_WELCOME;
  en->phaset=0.0;
  en->phaserate=1.0;
  en->display_foe_hp=en->foe.hp;
  en->display_hero_hp=g.hp;
}

/* Finish.
 */
 
static void encounter_finish(struct encounter *en) {
  en->active=0;
}

/* Enter GATHER phase.
 */
 
static void encounter_begin_GATHER(struct encounter *en) {

  // Sync displayed hp.
  if ((en->display_foe_hp=en->foe.hp)<0) en->display_foe_hp=0;
  if ((en->display_hero_hp=g.hp)<0) en->display_hero_hp=0;

  /* If either party is dead, go to the WIN or LOSE phase instead.
   */
  if (g.hp<=0) {
    en->phase=ENCOUNTER_PHASE_LOSE;
    return;
  }
  if (en->foe.hp<=0) {
    en->phase=ENCOUNTER_PHASE_WIN;
    return;
  }

  en->phase=ENCOUNTER_PHASE_GATHER;
  if (memcmp(en->hand,"\0\0\0\0\0\0\0",7)) return;
  if (memcmp(en->foe.hand,"\0\0\0\0\0\0\0",7)) return; // We can have an empty hand if the foe is not empty -- all we can do is fold.
  fprintf(stderr,"%s: Reshuffling letterbag.\n",__func__);
  letterbag_reset(&en->letterbag);
  letterbag_draw(en->hand,&en->letterbag);
}

/* Enter PLAY phase, committing the hero's staged word.
 */
 
static void encounter_begin_PLAY(struct encounter *en) {
  en->phase=ENCOUNTER_PHASE_PLAY;
  en->phaset=0.0;
  en->phaserate=0.333;
  int c=0;
  while ((c<7)&&en->stage[c]) c++;
  int valid=spellcheck(en->stage,c);
  en->efficacy=rate_word(en->stage,7);
  if (!valid) en->efficacy=-en->efficacy;
  memcpy(en->inplay,en->stage,7);
  memset(en->stage,0,sizeof(en->stage));
  if (en->efficacy>0) {
    en->foe.hp-=en->efficacy;
  } else if (en->efficacy<0) {
    g.hp+=en->efficacy;
  }
  letterbag_draw_partial(en->hand,&en->letterbag);
  en->cursor.y=2;
  en->cursor.x=0;
}

/* Enter REACT phase, which commits the foe's play.
 */
 
static void encounter_begin_REACT(struct encounter *en) {

  /* If either party is dead, go to the WIN or LOSE phase instead.
   */
  if (g.hp<=0) {
    en->phase=ENCOUNTER_PHASE_LOSE;
    return;
  }
  if (en->foe.hp<=0) {
    en->phase=ENCOUNTER_PHASE_WIN;
    return;
  }

  en->phase=ENCOUNTER_PHASE_REACT;
  en->phaset=0.0;
  en->phaserate=0.333;
  foe_play(&en->foe);
}

/* Activation in any non-GATHER phase: Skip to the next phase.
 * Also triggers on expiration of timed phases.
 */
 
static void encounter_skip(struct encounter *en) {
  switch (en->phase) {
    case ENCOUNTER_PHASE_PLAY: encounter_begin_REACT(en); break;
    case ENCOUNTER_PHASE_REACT: encounter_begin_GATHER(en); break;
    case ENCOUNTER_PHASE_WIN: encounter_finish(en); break;
    case ENCOUNTER_PHASE_LOSE: encounter_finish(en); break;
    case ENCOUNTER_PHASE_WELCOME: encounter_begin_GATHER(en); break;
  }
}

/* Update.
 */
 
void encounter_update(struct encounter *en,double elapsed) {
  if ((en->cursor.animclock-=elapsed)<=0.0) {
    en->cursor.animclock+=0.200;
    if (++(en->cursor.animframe)>=4) en->cursor.animframe=0;
  }
  switch (en->phase) {
    // GATHER,WIN,LOSE: No clock, advance only by explicit player action.
    case ENCOUNTER_PHASE_GATHER: {
        foe_update(&en->foe,elapsed);
      } break;
    case ENCOUNTER_PHASE_WIN: break;
    case ENCOUNTER_PHASE_LOSE: break;
    // WELCOME,PLAY,REACT: Player can interrupt, but normally they run on a clock.
    case ENCOUNTER_PHASE_PLAY:
    case ENCOUNTER_PHASE_REACT: {
        // In PLAY and REACT, tick displayed HP toward the model state.
        // The word strikes at 0.400, hold off updating the display until then.
        if (en->phaset>=0.400) {
          en->display_hp_clock+=elapsed;
          double updinterval=0.080;
          while (en->display_hp_clock>=updinterval) {
            en->display_hp_clock-=updinterval;
            if (en->display_foe_hp>en->foe.hp) en->display_foe_hp--;
            else if (en->display_foe_hp<en->foe.hp) en->display_foe_hp++;
            if (en->display_foe_hp<0) en->display_foe_hp=0;
            if (en->display_hero_hp>g.hp) en->display_hero_hp--;
            else if (en->display_hero_hp<g.hp) en->display_hero_hp++;
            if (en->display_hero_hp<0) en->display_hero_hp=0;
          }
        }
      } // pass
    default: {
        if ((en->phaset+=elapsed*en->phaserate)>=1.0) {
          encounter_skip(en);
        }
      }
  }
}

/* Request to discard current hand, draw a fresh one, and forfeit the turn.
 */

static void request_new_hand(struct encounter *en) {
  // No impact to score. The penalty is that you lose these letters, and lose your turn.
  memset(en->stage,0,sizeof(en->stage));
  memset(en->hand,0,sizeof(en->hand));
  encounter_begin_PLAY(en);
}

/* Move the last tile from stage to anywhere in hand.
 */
 
static void unstage_last_tile(struct encounter *en) {
  int fromp=-1,top=-1,i;
  for (i=7;i-->0;) if (en->stage[i]) { fromp=i; break; }
  for (i=0;i<7;i++) if (!en->hand[i]) { top=i; break; }
  if ((fromp<0)||(top<0)) return;
  en->hand[top]=en->stage[fromp];
  en->stage[fromp]=0;
}

/* Move a tile from stage to hand.
 */

static void return_tile_to_hand(struct encounter *en,int p) {
  if ((p<0)||(p>=7)||!en->stage[p]) return;
  int dstp=0; for (;dstp<7;dstp++) {
    if (en->hand[dstp]) continue;
    en->hand[dstp]=en->stage[p];
    if ((en->hand[dstp]>='a')&&(en->hand[dstp]<='z')) en->hand[dstp]='@'; // Wildcard, unset on return.
    en->stage[p]=0;
    int i=p+1;
    for (;i<7;i++) en->stage[i-1]=en->stage[i];
    en->stage[6]=0;
    break;
  }
}

/* Move a tile from hand to stage, or trigger the wildcard modal.
 */

static void play_tile(struct encounter *en,int p) {
  if ((p<0)||(p>=7)||!en->hand[p]) return;
  if (en->hand[p]=='@') {
    en->wildcard_handp=p;
    en->wildcard_modal=1;
    en->wcx=4;
    en->wcy=1;
    return;
  }
  int dstp=0; for (;dstp<7;dstp++) {
    if (en->stage[dstp]) continue;
    en->stage[dstp]=en->hand[p];
    en->hand[p]=0;
    break;
  }
}

/* Move cursor.
 */
 
void encounter_move(struct encounter *en,int dx,int dy) {
  if (en->wildcard_modal) {
    en->wcx+=dx;
    en->wcy+=dy;
    if (en->wcx<0) en->wcx=8; else if (en->wcx>8) en->wcx=0;
    if (en->wcy<0) en->wcy=2; else if (en->wcy>2) en->wcy=0;
  } else if (en->phase==ENCOUNTER_PHASE_GATHER) {
    en->cursor.x+=dx;
    en->cursor.y+=dy;
    if (en->cursor.y<0) en->cursor.y=2;
    else if (en->cursor.y>2) en->cursor.y=0;
    switch (en->cursor.y) {
      case 0: if (dy) en->cursor.x=0; else if (en->cursor.x<0) en->cursor.x=1; else if (en->cursor.x>1) en->cursor.x=0; break;
      case 1: case 2: if (en->cursor.x<0) en->cursor.x=6; else if (en->cursor.x>6) en->cursor.x=0; break;
    }
  }
}

/* Activate cursor.
 */
 
void encounter_activate(struct encounter *en) {
  if (en->wildcard_modal) {
    char ch='A'+en->wcy*9+en->wcx;
    if ((ch>='A')&&(ch<='Z')) {
      en->hand[en->wildcard_handp]=ch+0x20;
      play_tile(en,en->wildcard_handp);
    }
    en->wildcard_modal=0;
  } else if (en->phase==ENCOUNTER_PHASE_GATHER) {
    switch (en->cursor.y) {
      case 0: switch (en->cursor.x) {
          case 0: encounter_begin_PLAY(en); break;
          case 1: request_new_hand(en); break;
        } break;
      case 1: return_tile_to_hand(en,en->cursor.x); break;
      case 2: play_tile(en,en->cursor.x); break;
    }
  } else {
    encounter_skip(en);
  }
}

/* Cancel (user presses B).
 */
 
void encounter_cancel(struct encounter *en) {
  if (en->wildcard_modal) {
    en->wildcard_modal=0;
  } else if (en->phase==ENCOUNTER_PHASE_GATHER) {
    unstage_last_tile(en);
  }
}

/* Render an animated travelling word.
 */
 
static void encounter_render_travelling_letter(
  struct encounter *en,int texid,
  char letter,
  int x0,int y0,double t0,
  int x1,int y1,double t1,
  int shift
) {
  double w1=(en->phaset-t0)/(t1-t0);
  w1-=0.125*shift;
  if (w1<0.0) return;
  else if (w1>1.0) return;
  double w0=1.0-w1;
  double arch=40.0;
  int dstx=(int)(x0*w0+x1*w1);
  int dsty=(int)(y0*w0+y1*w1-sin(w1*M_PI)*arch);
  graf_draw_tile(&g.graf,texid,dstx,dsty,letter,0);
}
 
static void encounter_render_travelling_word(
  struct encounter *en,int texid,
  const char *word/*7*/,
  int x0,int y0,double t0,
  int x1,int y1,double t1
) {
  if (x0<x1) { // Left to right: back letter in the lead.
    int shift=0;
    int i=7; while (i-->0) {
      if (!word[i]) continue;
      encounter_render_travelling_letter(en,texid,word[i],x0,y0,t0,x1,y1,t1,shift++);
    }
  } else { // Right to left: front letter in the lead.
    int i=0; for (;i<7;i++) {
      if (!word[i]) break;
      encounter_render_travelling_letter(en,texid,word[i],x0,y0,t0,x1,y1,t1,i);
    }
  }
}

/* Damage report: A number that drops down and bounces when someone gets hurt.
 */
 
static void encounter_render_damage_report(
  struct encounter *en,int texid,int v,
  int16_t dstx,int16_t dsty,
  double t0,double t1
) {
  if ((en->phaset<t0)||(en->phaset>t1)) return;
  int xstride=8;
  double t=(en->phaset-t0)/(t1-t0);
  double d=fabs(cos(t*M_PI*5.0))*(1.0-t)*(1.0-t);
  dsty-=(int)(d*32.0);
  char tmp[8];
  int tmpp=8;
  if (!v) {
    tmp[--tmpp]='0';
  } else {
    int negative=0;
    if (v<0) { v=-v; negative=1; }
    while ((tmpp>0)&&v) {
      int digit=v%10; if (digit<0) digit=-digit;
      v/=10;
      tmp[--tmpp]='0'+digit;
    }
    if (negative&&tmpp) tmp[--tmpp]='-';
  }
  char *tileid=tmp+tmpp;
  int digitc=8-tmpp;
  dstx-=(digitc*xstride)>>1;
  for (;digitc-->0;tileid++,dstx+=xstride) {
    graf_draw_tile(&g.graf,texid,dstx,dsty,*tileid,0);
  }
}

/* Render the player's hand.
 */
 
static void encounter_render_hand(struct encounter *en,int texid,int tilesize,int topy) {
  int rowmargin=8;
  int colmargin=2;
  int rowstride=tilesize+rowmargin;
  int colstride=tilesize+colmargin;
  int contentw=tilesize*7+colmargin*6;
  int contenth=tilesize*3+rowmargin*2;
  int contentx=(g.fbw>>1)-(contentw>>1); // outer bounds
  int contenty=topy+((g.fbh-topy)>>1)-(contenth>>1);
  int startx=contentx+(tilesize>>1); // first tile position
  int starty=contenty+(tilesize>>1);
  int16_t dstx,dsty,i;
  
  // Rack, behind hand. These must touch horizontally, so it's not the expected 9 columns.
  int rackcolc=10; // 7 letters, 1 extra letter due to horz spacing, 2 edges.
  int rackw=rackcolc*tilesize;
  dstx=(g.fbw>>1)-(rackw>>1)+(tilesize>>1);
  dsty=starty+rowstride*2+4;
  graf_draw_tile(&g.graf,texid,dstx,dsty,0x03,0);
  dstx+=tilesize;
  for (i=8;i-->0;dstx+=tilesize) {
    graf_draw_tile(&g.graf,texid,dstx,dsty,0x04,0);
  }
  graf_draw_tile(&g.graf,texid,dstx,dsty,0x03,EGG_XFORM_XREV);
  
  // Hand.
  for (dstx=startx,dsty=starty+rowstride*2,i=0;i<7;i++,dstx+=colstride) {
    if (!en->hand[i]) continue;
    graf_draw_tile(&g.graf,texid,dstx,dsty,en->hand[i],0);
  }
  
  // Cursor, if GATHER phase and no modal.
  if (!en->wildcard_modal&&(en->phase==ENCOUNTER_PHASE_GATHER)) {
    uint8_t cursorxform=0;
    switch (en->cursor.animframe) {
      case 0: break;
      case 1: cursorxform=EGG_XFORM_XREV|EGG_XFORM_SWAP; break;
      case 2: cursorxform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
      case 3: cursorxform=EGG_XFORM_YREV|EGG_XFORM_SWAP; break;
    }
    dstx=startx+en->cursor.x*colstride;
    dsty=starty+en->cursor.y*rowstride;
    graf_draw_tile(&g.graf,texid,dstx,dsty,0x00,cursorxform);
  }
}

/* Render bottom half of GATHER phase: Controls, stage, and hand.
 * This is essentially a 7x3 grid.
 * Actually, the hand will be elsewhere. It gets drawn in every phase.
 */
 
static void encounter_render_gather(struct encounter *en,int texid,int tilesize,int topy) {
  int rowmargin=8;
  int colmargin=2;
  int rowstride=tilesize+rowmargin;
  int colstride=tilesize+colmargin;
  int contentw=tilesize*7+colmargin*6;
  int contenth=tilesize*3+rowmargin*2;
  int contentx=(g.fbw>>1)-(contentw>>1); // outer bounds
  int contenty=topy+((g.fbh-topy)>>1)-(contenth>>1);
  int startx=contentx+(tilesize>>1); // first tile position
  int starty=contenty+(tilesize>>1);
  int16_t dstx,dsty,i;
  
  // Controls row.
  //TODO Show inventory for each stamp.
  //TODO Gray out if inventory zero.
  //TODO Highlight selected stamp.
  graf_draw_tile(&g.graf,texid,startx+colstride*0,starty,0x01,0);
  graf_draw_tile(&g.graf,texid,startx+colstride*1,starty,0x02,0);
  graf_draw_tile(&g.graf,texid,startx+colstride*3,starty,0x08,0);
  graf_draw_tile(&g.graf,texid,startx+colstride*4,starty,0x09,0);
  graf_draw_tile(&g.graf,texid,startx+colstride*5,starty,0x0a,0);
  graf_draw_tile(&g.graf,texid,startx+colstride*6,starty,0x0b,0);
  
  // Stage.
  for (dstx=startx,dsty=starty+rowstride,i=0;i<7;i++,dstx+=colstride) {
    if (!en->stage[i]) continue;
    graf_draw_tile(&g.graf,texid,dstx,dsty,en->stage[i],0);
  }
}

/* Render, PLAY phase.
 */
 
static void encounter_render_play(struct encounter *en,int texid,int tilesize,int topy) {
  //TODO Hero face attacking, foe face hurt at the right moment. Must manage higher up than here.
  //TODO Narration in the bottom?
  if (en->efficacy>=0) {
    encounter_render_travelling_word(
      en,texid,en->inplay,
      (g.fbw*4)/5-tilesize,65,0.000,
      (g.fbw*1)/5+tilesize,65,0.400
    );
    encounter_render_damage_report(
      en,texid,en->efficacy,
      (g.fbw*1)/5,topy-tilesize,
      0.300,1.000
    );
  } else {
    //TODO Word must turn around mid-flight.
    encounter_render_damage_report(
      en,texid,-en->efficacy,
      (g.fbw*4)/5,topy-tilesize,
      0.300,1.000
    );
  }
}

/* Render, REACT phase.
 */
 
static void encounter_render_react(struct encounter *en,int texid,int tilesize,int topy) {
  //TODO Hero face attacking, foe face hurt at the right moment. Must manage higher up than here.
  //TODO Narration in the bottom?
  encounter_render_travelling_word(
    en,texid,en->inplay,
    (g.fbw*1)/5+tilesize,65,0.000,
    (g.fbw*4)/5-tilesize,65,0.400
  );
  encounter_render_damage_report(
    en,texid,en->efficacy,
    (g.fbw*4)/5,topy-tilesize,
    0.300,1.000
  );
}

/* Render, WIN phase.
 */
 
static void encounter_render_win(struct encounter *en,int texid,int tilesize,int topy) {
  //TODO "Thou hast done well in defeating the orc, of gold thou hast gained 6."
  int16_t srcx=tilesize*6,srcy=tilesize,srcw=tilesize*3,srch=tilesize;
  int16_t dstx=(g.fbw>>1)-(srcw>>1);
  int16_t dsty=topy+((g.fbh-topy)>>1)-(srch>>1);
  graf_draw_decal(&g.graf,texid,dstx,dsty,srcx,srcy,srcw,srch,0);
}

/* Render, LOSE phase.
 */
 
static void encounter_render_lose(struct encounter *en,int texid,int tilesize,int topy) {
  //TODO How much of this is our problem, and how much global to the game?
  int16_t srcx=tilesize*9,srcy=tilesize,srcw=tilesize*3,srch=tilesize;
  int16_t dstx=(g.fbw>>1)-(srcw>>1);
  int16_t dsty=topy+((g.fbh-topy)>>1)-(srch>>1);
  graf_draw_decal(&g.graf,texid,dstx,dsty,srcx,srcy,srcw,srch,0);
}

/* Render, WELCOME phase.
 */
 
static void encounter_render_welcome(struct encounter *en,int texid,int tilesize,int topy) {
  //TODO Fade in, narrate who's drawing near, etc.
  int16_t srcx=0,srcy=tilesize,srcw=tilesize*6,srch=tilesize;
  int16_t dstx=(g.fbw>>1)-(srcw>>1);
  int16_t dsty=topy+((g.fbh-topy)>>1)-(srch>>1);
  graf_draw_decal(&g.graf,texid,dstx,dsty,srcx,srcy,srcw,srch,0);
}

/* Render the wildcard modal.
 */
 
static void encounter_render_wildcard(struct encounter *en,int texid,int tilesize) {
  const int margin=3;
  const int colc=9;
  const int rowc=3;
  int w=colc*tilesize+margin*2;
  int h=rowc*tilesize+margin*2;
  int x=(g.fbw>>1)-(w>>1);
  int y=(g.fbh>>1)-(h>>1);
  graf_draw_rect(&g.graf,x,y,w,h,0xc0c0c0c0);
  char ch='a';
  int16_t dstx,dsty;
  int yi=rowc; for (dsty=y+margin+(tilesize>>1);yi-->0;dsty+=tilesize) {
    int xi=colc; for (dstx=x+margin+(tilesize>>1);xi-->0;dstx+=tilesize,ch++) {
      if (ch>'z') ch=0x05;
      graf_draw_tile(&g.graf,texid,dstx,dsty,ch,0);
    }
  }
  uint8_t cursorxform=0;
  switch (en->cursor.animframe) {
    case 0: break;
    case 1: cursorxform=EGG_XFORM_XREV|EGG_XFORM_SWAP; break;
    case 2: cursorxform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
    case 3: cursorxform=EGG_XFORM_YREV|EGG_XFORM_SWAP; break;
  }
  graf_draw_tile(
    &g.graf,texid,
    x+margin+(tilesize>>1)+en->wcx*tilesize,
    y+margin+(tilesize>>1)+en->wcy*tilesize,
    0x00,cursorxform
  );
}

/* Render, main.
 */

static void draw_integer(int texid,int16_t dstx,int16_t dsty,int v) {
  const int xstride=8;
  if (v==INT_MIN) v++;
  if (v<0) {
    v=-v;
    graf_draw_tile(&g.graf,texid,dstx,dsty,'-',0);
    dstx+=xstride;
  }
  if (v>=1000000000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+v/1000000000,0); dstx+=xstride; }
  if (v>= 100000000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/100000000)%10,0); dstx+=xstride; }
  if (v>=  10000000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/10000000)%10,0); dstx+=xstride; }
  if (v>=   1000000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/1000000)%10,0); dstx+=xstride; }
  if (v>=    100000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/100000)%10,0); dstx+=xstride; }
  if (v>=     10000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/10000)%10,0); dstx+=xstride; }
  if (v>=      1000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/1000)%10,0); dstx+=xstride; }
  if (v>=       100) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/100)%10,0); dstx+=xstride; }
  if (v>=        10) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/10)%10,0); dstx+=xstride; }
                     { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+v%10,0); dstx+=xstride; }
}
 
void encounter_render(struct encounter *en) {
  const int tilesize=32;
  int texid_tiles=texcache_get_image(&g.texcache,RID_image_tiles32);
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x201040ff);
  
  /* A large section at the top is the action scene.
   * Foe on the left, Dot on the right, with lots of animation.
   */
  int actionx=0,actiony=0,actionw=g.fbw;
  int actionh=g.fbh>>1;
  graf_draw_rect(&g.graf,actionx,actiony,actionw,actionh,0x80a0c0ff);
  int dotw=tilesize*3,doth=tilesize*4; // also the foe's dimensions
  graf_draw_decal(&g.graf,texid_tiles,actionx+(actionw*4)/5-(dotw>>1),actiony+(actionh>>1)-(doth>>1),0,256,dotw,doth,0);
  graf_draw_decal(&g.graf,texid_tiles,actionx+(actionw*1)/5-(dotw>>1),actiony+(actionh>>1)-(doth>>1),dotw,256,dotw,doth,0);
  
  /* The foe's charge meter appears left of him.
   * Composed of tiles 0x06(off) and 0x07(on), stacked vertically.
   */
  int meterc=9;
  int progress=0;
  if (en->foe.searchc>0) progress=(en->foe.searchp*meterc)/en->foe.searchc;
  int16_t dstx=actionx+actionw/5-60;
  int16_t dsty=actiony+(actionh>>1)-(doth>>1)+tilesize; // Top tile.
  int i=meterc;
  for (;i-->0;dsty+=5) {
    graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,(i<progress)?0x07:0x06,0);
  }
  
  /* The bottom section, and detail in the top, are different per phase.
   */
  switch (en->phase) {
    case ENCOUNTER_PHASE_GATHER: encounter_render_gather(en,texid_tiles,tilesize,actiony+actionh); break;
    case ENCOUNTER_PHASE_PLAY: encounter_render_play(en,texid_tiles,tilesize,actiony+actionh); break;
    case ENCOUNTER_PHASE_REACT: encounter_render_react(en,texid_tiles,tilesize,actiony+actionh); break;
    case ENCOUNTER_PHASE_WIN: encounter_render_win(en,texid_tiles,tilesize,actiony+actionh); break;
    case ENCOUNTER_PHASE_LOSE: encounter_render_lose(en,texid_tiles,tilesize,actiony+actionh); break;
    case ENCOUNTER_PHASE_WELCOME: encounter_render_welcome(en,texid_tiles,tilesize,actiony+actionh); break;
  }
  
  /* Show the hand at all times, so player can examine while other things are happening.
   */
  encounter_render_hand(en,texid_tiles,tilesize,actiony+actionh);
  
  /* HP for both parties, in all phases.
   */
  draw_integer(texid_tiles,20,20,en->display_foe_hp);
  draw_integer(texid_tiles,g.fbw-60,20,en->display_hero_hp);
  
  /* The wildcard modal only happens in GATHER phase, but for our purposes it's independent.
   */
  if (en->wildcard_modal) {
    encounter_render_wildcard(en,texid_tiles,tilesize);
  }
}
