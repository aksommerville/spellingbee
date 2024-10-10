#include "bee.h"

/* Begin.
 */
 
void encounter_begin(struct encounter *en) {
  
  letterbag_reset(&en->letterbag);
  letterbag_draw(en->hand,&en->letterbag);
  foe_reset(&en->foe,en);
  
  en->cursor.x=3;
  en->cursor.y=2;
  
  en->active=1;
}

/* Update.
 */
 
void encounter_update(struct encounter *en,double elapsed) {
  if ((en->cursor.animclock-=elapsed)<=0.0) {
    en->cursor.animclock+=0.200;
    if (++(en->cursor.animframe)>=4) en->cursor.animframe=0;
  }
  foe_update(&en->foe,elapsed);
}

/* Refill the hand from letterbag, and end the encounter if hand is empty.
 */

static void refill_hand(struct encounter *en) {
  if ((letterbag_draw_partial(en->hand,&en->letterbag)<1)&&!memcmp(en->hand,"\0\0\0\0\0\0\0",7)) {
    fprintf(stderr,"GAME OVER, final score %d\n",en->score);
    en->active=0;
  }
}

/* Request to discard current hand, draw a fresh one, and forfeit the turn.
 */

static void request_new_hand(struct encounter *en) {
  // No impact to score. The penalty is that you lose these letters, and lose your turn.
  en->recent_score=0;
  memset(en->stage,0,sizeof(en->stage));
  memset(en->hand,0,sizeof(en->hand));
  refill_hand(en);
  en->cursor.y=2;
  en->cursor.x=0;
  foe_play(&en->foe);
}

/* Cast the spell currently in stage.
 */

static void cast_spell(struct encounter *en) {
  int c=0;
  while ((c<7)&&en->stage[c]) c++;
  int valid=spellcheck(en->stage,c);
  int score=rate_word(en->stage,7);
  if (!valid) score=-score;
  fprintf(stderr,"CAST SPELL '%.*s': %s = %d\n",c,en->stage,valid?"VALID":"INVALID",score);
  // Drop the stage and refill hand, and update score. Whether it's valid or not (score is negative if invalid).
  memset(en->stage,0,sizeof(en->stage));
  en->score+=score;
  en->recent_score=score;
  refill_hand(en);
  en->cursor.y=2;
  en->cursor.x=0;
  foe_play(&en->foe);
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
  } else {
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
  } else {
    switch (en->cursor.y) {
      case 0: switch (en->cursor.x) {
          case 0: cast_spell(en); break;
          case 1: request_new_hand(en); break;
        } break;
      case 1: return_tile_to_hand(en,en->cursor.x); break;
      case 2: play_tile(en,en->cursor.x); break;
    }
  }
}

/* Render.
XXX I don't expect to keep any of this, really
 */

static void draw_integer(int texid,int16_t dstx,int16_t dsty,int v) {
  const int xstride=7;
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
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x406080ff);
  
  int texid_tiles=texcache_get_image(&g.texcache,RID_image_tiles);
  int i;
  int16_t dstx,dsty;
  
  // CPU charge meter.
  int16_t meterw=g.fbw-10;
  int progress=0;
  if (en->foe.searchc>0) progress=(en->foe.searchp*meterw)/en->foe.searchc;
  if (progress<meterw) {
    graf_draw_rect(&g.graf,5,100,meterw,10,0x808080ff);
    graf_draw_rect(&g.graf,5,100,progress,10,0xff0000ff);
  } else {
    graf_draw_rect(&g.graf,5,100,meterw,10,0xff0000ff);
  }

  /*XXX old coarse-grained search
  // CPU's last play.
  draw_integer(texid_tiles,5,120,en->foe.lastscore);
  for (i=0,dstx=10,dsty=140;i<7;i++,dstx+=18) {
    if (en->foe.lastplay[i]) graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,en->foe.lastplay[i],0);
  }
  /**/
  
  // Hand rack.
  dsty=83;
  graf_draw_tile(&g.graf,texid_tiles,24,dsty,0x03,0);
  for (i=8,dstx=40;i-->0;dstx+=16) {
    graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,0x04,0);
  }
  graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,0x03,EGG_XFORM_XREV);
  
  // Control buttons.
  graf_draw_tile(&g.graf,texid_tiles,40,40,0x01,0);
  graf_draw_tile(&g.graf,texid_tiles,58,40,0x02,0);
  
  // Stage and hand tiles.
  for (i=0,dstx=40,dsty=60;i<7;i++,dstx+=18) {
    if (en->stage[i]) graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,en->stage[i],0);
    if (en->hand[i]) graf_draw_tile(&g.graf,texid_tiles,dstx,dsty+20,en->hand[i],0);
  }
  
  // Cursor.
  uint8_t cursorxform=0;
  switch (en->cursor.animframe) {
    case 0: break;
    case 1: cursorxform=EGG_XFORM_XREV|EGG_XFORM_SWAP; break;
    case 2: cursorxform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
    case 3: cursorxform=EGG_XFORM_YREV|EGG_XFORM_SWAP; break;
  }
  if (!en->wildcard_modal) {
    int16_t cursorx=40+en->cursor.x*18;
    int16_t cursory=40+en->cursor.y*20;
    graf_draw_tile(&g.graf,texid_tiles,cursorx,cursory,0x00,cursorxform);
  }
  
  // Score and most recent play.
  draw_integer(texid_tiles,200,40,en->score);
  draw_integer(texid_tiles,200,56,en->recent_score);
  
  // Wildcard modal. 9x3, with Cancel in the extra slot.
  if (en->wildcard_modal) {
    const int margin=3;
    const int tilesize=16;
    const int colc=9;
    const int rowc=3;
    int w=colc*tilesize+margin*2;
    int h=rowc*tilesize+margin*2;
    int x=(g.fbw>>1)-(w>>1);
    int y=(g.fbh>>1)-(h>>1);
    graf_draw_rect(&g.graf,x,y,w,h,0xc0c0c0c0);
    char ch='a';
    int yi=rowc; for (dsty=y+margin+(tilesize>>1);yi-->0;dsty+=tilesize) {
      int xi=colc; for (dstx=x+margin+(tilesize>>1);xi-->0;dstx+=tilesize,ch++) {
        if (ch>'z') ch=0x05;
        graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,ch,0);
      }
    }
    graf_draw_tile(
      &g.graf,texid_tiles,
      x+margin+(tilesize>>1)+en->wcx*tilesize,
      y+margin+(tilesize>>1)+en->wcy*tilesize,
      0x00,cursorxform
    );
  }
}
