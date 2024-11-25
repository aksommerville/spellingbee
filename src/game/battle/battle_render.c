#include "game/bee.h"
#include "battle.h"

/* Draw decimal integer.
 * Uses the minimum required digits, and centers on the point you provide.
 */
 
static void battle_draw_int(const struct battle *battle,int n,int x,int y,uint32_t rgba) {

  uint8_t digitv[16];
  int digitc=1;
  if (n<0) {
    digitv[0]='-';
    digitc=2;
    int limit=-10;
    while (n<=limit) { digitc++; if (limit<INT_MIN/10) break; limit*=10; }
    int i=digitc;
    for (;i-->1;n/=10) digitv[i]='0'-n%10;
  } else {
    digitc=1;
    int limit=10;
    while (n>=limit) { digitc++; if (limit>INT_MAX/10) break; limit*=10; }
    int i=digitc;
    for (;i-->0;n/=10) digitv[i]='0'+n%10;
  }
  
  const int xstride=7;
  int w=xstride*digitc;
  int xp=x-(w>>1)+(xstride>>1); // Center of leftmost tile.
  int texid=texcache_get_image(&g.texcache,RID_image_tiles);
  int i=0;
  graf_set_tint(&g.graf,rgba|0xff);
  graf_set_alpha(&g.graf,rgba&0xff);
  for (;i<digitc;i++,xp+=xstride) {
    graf_draw_tile(&g.graf,texid,xp,y,digitv[i],0);
  }
  graf_set_tint(&g.graf,0);
  graf_set_alpha(&g.graf,0xff);
}

/* Draw avatar.
 */
 
static void battle_draw_avatar(struct battle *battle,struct battler *battler,uint8_t xform) {
  int16_t dstx;
  if (xform) dstx=g.fbw-40-battler->avatar.w;
  else dstx=40;
  int16_t dsty=(g.fbh>>2)-(battler->avatar.h>>1);
  int16_t srcx=battler->avatar.x+battler->avatar.w*battler->avatar.face;
  int16_t srcy=battler->avatar.y;
  int16_t w=battler->avatar.w;
  int16_t h=battler->avatar.h;
  graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,battler->avatar.imageid),dstx,dsty,srcx,srcy,w,h,xform);
  
  /* Damage indicator.
   */
  if (battler->damageclock>0.0) {
    double t=1.0-battler->damageclock/BATTLE_DAMAGE_TIME;
    if ((t>0.0)&&(t<1.0)) {
      const double y0=(double)dsty+(double)h*0.666;
      const double y1=(double)dsty+(double)h*1.080;
      double v=cos(t*M_PI*2.0*2.0);
      if (v<0.0) v=-v;
      v*=((1.0-t)*(1.0-t)); // Taper off.
      int16_t y=(int16_t)(y0*v+y1*(1.0-v));
      battle_draw_int(battle,battler->damage,dstx+(w>>1),y,0xffffffff);
    }
  }
}

/* Draw one battler's hand.
 */
 
static void battle_draw_hand(const struct battle *battle,const struct battler *battler,int faced,int x0,int highlight) {
  int texid=texcache_get_image(&g.texcache,RID_image_tiles);
  int16_t dstx0=x0?(g.fbw-TILESIZE*7-(TILESIZE>>1)):(TILESIZE+(TILESIZE>>1));//x0+(g.fbw>>2)-((TILESIZE*7)>>1)+(TILESIZE>>1);
  int16_t dsty=g.fbh-TILESIZE,dstx;
  int i=0;
  
  /* When facing the user, draw the rack first with 0x22,0x23, draw real tiles, and animate per hand_recent.
   */
  if (faced) {
    for (dstx=dstx0,i=7;i-->0;dstx+=TILESIZE) graf_draw_tile(&g.graf,texid,dstx,dsty+3,0x23,0);
    graf_draw_tile(&g.graf,texid,dstx0-TILESIZE,dsty+3,0x22,0);
    graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*7,dsty+3,0x22,EGG_XFORM_XREV);
    const char *hand=battler->hand;
    int16_t ry=dsty;
    const double rdist=TILESIZE*1.5;
    const double rtime=2.000;
    const double rcolortime=8.0; // They slide in fast, but retain a little highlight much longer.
    if (battler->hand_recent_clock<rtime) {
      double norm=1.0-(battler->hand_recent_clock/rtime);
      norm*=norm;
      ry-=(int16_t)(rdist*norm);
    }
    if (battler->hand_recent_clock<rcolortime) {
      double tintnorm=1.0-battler->hand_recent_clock/rcolortime;
      uint8_t alpha=(uint8_t)(tintnorm*128.0);
      graf_set_tint(&g.graf,0x00800000|alpha);
      uint8_t rmask=1;
      for (dstx=dstx0,i=0;i<7;i++,dstx+=TILESIZE,rmask<<=1) {
        if (!(battler->hand_recent&rmask)) continue;
        if (hand[i]) {
          graf_draw_tile(&g.graf,texid,dstx,ry,hand[i],0);
        }
      }
      graf_set_tint(&g.graf,0);
      for (dstx=dstx0,i=0,rmask=1;i<7;i++,dstx+=TILESIZE,rmask<<=1) {
        if (battler->hand_recent&rmask) continue;
        if (hand[i]) {
          graf_draw_tile(&g.graf,texid,dstx,dsty,hand[i],0);
        }
      }
    } else {
      for (dstx=dstx0,i=0;i<7;i++,dstx+=TILESIZE) {
        if (hand[i]) {
          graf_draw_tile(&g.graf,texid,dstx,dsty,hand[i],0);
        }
      }
    }
    
  /* Not facing user, draw the tiles first as blanks only, and the rack second with 0x20,0x21, and no animation.
   */
  } else {
    const char *hand=battler->hand;
    for (dstx=dstx0,i=0;i<7;i++,dstx+=TILESIZE) {
      if (hand[i]) {
        graf_draw_tile(&g.graf,texid,dstx,dsty,'@',0);
      }
    }
    for (dstx=dstx0,i=7;i-->0;dstx+=TILESIZE) graf_draw_tile(&g.graf,texid,dstx,dsty+3,0x21,0);
    graf_draw_tile(&g.graf,texid,dstx0-TILESIZE,dsty+3,0x20,0);
    graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*7,dsty+3,0x20,EGG_XFORM_XREV);
  }
  
  // If we're a robot, draw the charge meter. But not in the dark.
  if (!battler->human&&(battler->charge>battler->wakeup)&&!battle->dark) {
    if ((battle->stage==BATTLE_STAGE_GATHER)||(battle->stage==BATTLE_STAGE_ATTACK1)||(battle->stage==BATTLE_STAGE_ATTACK2)) {
      int16_t barw=g.fbw/3;
      int16_t barh=TILESIZE/2;
      int16_t barx=dstx0-10;
      int16_t bary=dsty-TILESIZE*3;
      int16_t fillw=((battler->gatherclock-battler->wakeup)*barw)/(battler->charge-battler->wakeup);
      if (fillw<=0) {
        graf_draw_rect(&g.graf,barx,bary,barw,barh,0x808080ff);
      } else {
        uint32_t fillcolor=0xff0000ff;
        const uint32_t bgcolor=0x402000ff;
        if (battler->gatherclock>=battler->charge) {
          fillcolor=(battle->cursorframe&2)?0xff8000ff:0xffc000ff;
        }
        if (fillw>=barw) {
          graf_draw_rect(&g.graf,barx,bary,barw,barh,fillcolor);
        } else {
          graf_draw_rect(&g.graf,barx,bary,fillw,barh,fillcolor);
          graf_draw_rect(&g.graf,barx+fillw,bary,barw-fillw,barh,bgcolor);
        }
      }
    }
  }
  
  // If highlight is not enabled (CPU or not in GATHER stage), we're done. Just the rack.
  if (!highlight) return;
  
  // Stage in middle row.
  const char *stage=battler->stage;
  for (dstx=dstx0,i=0;i<7;i++,dstx+=TILESIZE) {
    if (stage[i]) {
      uint8_t tileid=stage[i];
      graf_draw_tile(&g.graf,texid,dstx,dsty-TILESIZE*2,tileid,0);
    }
  }
  
  // Controls and items in top row.
  graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*0,dsty-TILESIZE*4,0x01,0); // commit
  if (battler->confirm_fold&&!battler->ready) {
    uint8_t tileid=0x0e;
    if (battle->cursorframe&2) tileid++;
    graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*1,dsty-TILESIZE*4,tileid,0);
  }
  graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*1,dsty-TILESIZE*4,0x02,0); // fold
  // Eraser is not quite like the modifier items.
  if (battler->erasing) {
    graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*3,dsty-TILESIZE*4,0x0f,0);
    graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*3,dsty-TILESIZE*4,0x03,0);
  } else if (battler->inventory[ITEM_ERASER]) {
    graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*3,dsty-TILESIZE*4,0x03,0);
  } else {
    graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*3,dsty-TILESIZE*4,0x10,0);
  }
  // Careful: When an item is selected, it has been removed from inventory already.
  #define ITEM(tag,col,unset_tileid) { \
    if (battler->modifier==ITEM_##tag) { \
      graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*col,dsty-TILESIZE*4,7+ITEM_##tag,0); \
      graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*col,dsty-TILESIZE*4,0x0d,0); \
    } else if (battler->inventory[ITEM_##tag]) { \
      graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*col,dsty-TILESIZE*4,7+ITEM_##tag,0); \
    } else { \
      graf_draw_tile(&g.graf,texid,dstx0+TILESIZE*col,dsty-TILESIZE*4,unset_tileid,0); \
    } \
  }
  // Column 2 is vacant.
  ITEM(UNFAIRIE,4,0x10)
  ITEM(2XWORD,5,0x0c)
  ITEM(3XWORD,6,0x0c)
  #undef ITEM
  
  // If highlight enabled and valid, draw it.
  if (highlight&&(battler->selx>=0)&&(battler->selx<7)&&(battler->sely>=0)&&(battler->sely<3)&&!battler->ready&&!battler->wcmodal) {
    int16_t hx=dstx0+battler->selx*TILESIZE;
    int16_t hy;
    switch (battler->sely) {
      case 0: hy=dsty-TILESIZE*4; break;
      case 1: hy=dsty-TILESIZE*2; break;
      case 2: hy=dsty; break;
    }
    uint8_t xform=0;
    switch (battle->cursorframe) {
      case 1: xform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
      case 2: xform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
      case 3: xform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
    }
    uint8_t tileid=battler->erasing?0x04:0x00;
    graf_draw_tile(&g.graf,texid,hx,hy,tileid,xform);
    
    // When hovering over an item, show its count above the cursor.
    switch (battler->sely) {
      case 0: switch (battler->selx) {
          case 3: battle_draw_int(battle,battler->inventory[ITEM_ERASER],hx,hy-TILESIZE,0xffffffff); break;
          case 4: battle_draw_int(battle,battler->inventory[ITEM_UNFAIRIE],hx,hy-TILESIZE,0xffffffff); break;
          case 5: battle_draw_int(battle,battler->inventory[ITEM_2XWORD],hx,hy-TILESIZE,0xffffffff); break;
          case 6: battle_draw_int(battle,battler->inventory[ITEM_3XWORD],hx,hy-TILESIZE,0xffffffff); break;
        } break;
    }
  }
}

/* Wildcard modal.
 */
 
static void battle_draw_wildcard_modal(struct battle *battle,struct battler *battler,int x0) {
  int texid=texcache_get_image(&g.texcache,RID_image_tiles);
  const int margin=4;
  int modalw=WCMODAL_COLC*TILESIZE+margin*2;
  int modalh=WCMODAL_ROWC*TILESIZE+margin*2;
  int modalx=x0+(g.fbw>>2)-(modalw>>1);
  int modaly=(g.fbh>>1)-(modalh>>1);
  graf_draw_rect(&g.graf,modalx+1,modaly+1,modalw+2,modalh+2,0x00000080);
  graf_draw_rect(&g.graf,modalx-1,modaly-1,modalw+2,modalh+2,0xc0c0c0ff);
  graf_draw_rect(&g.graf,modalx,modaly,modalw,modalh,0x001020ff);
  
  int16_t xrow=modalx+margin+(TILESIZE>>1);
  int16_t y=modaly+margin+(TILESIZE>>1);
  int yi=WCMODAL_ROWC;
  uint8_t tileid='a';
  for (;yi-->0;y+=TILESIZE) {
    int16_t x=xrow;
    int xi=WCMODAL_COLC;
    for (;xi-->0;x+=TILESIZE,tileid++) {
      if (tileid>'z') break;
      graf_draw_tile(&g.graf,texid,x,y,tileid,0);
    }
  }
  
  if ((battler->wcx>=0)&&(battler->wcx<WCMODAL_COLC)&&(battler->wcy>=0)&&(battler->wcy<WCMODAL_ROWC)) {
    uint8_t xform=0;
    switch (battle->cursorframe) {
      case 1: xform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
      case 2: xform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
      case 3: xform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
    }
    graf_draw_tile(&g.graf,texid,modalx+margin+battler->wcx*TILESIZE+(TILESIZE>>1),modaly+margin+battler->wcy*TILESIZE+(TILESIZE>>1),0x00,xform);
  }
}

/* Decorate one of the heroes during a fold.
 * I guess we don't do anything for this?
 */
 
static void battle_draw_fold_word(struct battle *battle,struct battler *battler,int dir,double t) {
}

/* Draw word during a backfire play.
 * It leaves the hero's hand like an attack, then turns around and hits the same hero.
 */
 
static void battle_draw_backfire_word(struct battle *battle,struct battler *battler,int dir,double t) {
  const double tdepart=0.050; // First tile leaves the attacker.
  const double tarrive=0.200; // First tile arrives at the center.
  const double treverse=0.450; // First tile leaves the center.
  const double treturn=0.600; // First tile arrives at attacker.
  const int16_t xmargin=80;
  const double arch=20.0;
  const double ybase=30.0;
  int texid=texcache_get_image(&g.texcache,RID_image_tiles);
  
  /* Select endpoints and put letters in chronological order.
   */
  uint8_t tileidv[8];
  int tileidc=0;
  double x1=g.fbw>>1;
  double x0;
  if (dir>0) { // Left to right. Throw the last letter first.
    x0=xmargin;
    int i=battler->attackc;
    while (i-->0) tileidv[tileidc++]=battler->attack[i];
  } else { // Right to left.
    x0=g.fbw-xmargin;
    memcpy(tileidv,battler->attack,battler->attackc);
    tileidc=battler->attackc;
  }
  if (tileidc<1) return;
  
  /* Visit each tile.
   */
  double dt; // Letter spacing in normal units, to yield visual separation of 16 pixels in motion.
  dt=((tarrive-tdepart)*TILESIZE)/(double)(x1-x0);
  if (dt>0.0) dt=-dt;
  int i=0,arrived=0;
  for (;i<tileidc;i++,t+=dt) {
    double subt; // How far along the curve? 0=attacker .. 1=peak
    if (t<tdepart) {
      continue;
    } else if (t<tarrive) {
      subt=(t-tdepart)/(tarrive-tdepart);
    } else if (t<treverse) {
      subt=1.0;
    } else if (t<treturn) {
      subt=1.0-(t-treverse)/(treturn-treverse);
    } else {
      arrived++;
      continue;
    }
    int16_t x=(int16_t)(x0*(1.0-subt)+x1*subt);
    int16_t y=(int16_t)(ybase-arch*(1.0-(1.0-subt)*(1.0-subt)));
    graf_draw_tile(&g.graf,texid,x,y,tileidv[i],0);
  }
  if (arrived) battle_begin_damage(battle,battler,-battler->force);
  if (arrived!=battle->last_arrived) {
    battle->last_arrived=arrived;
    egg_play_sound(RID_sound_letterslap);
  }
}

/* Draw the word flying across the action scene for a typical attack.
 */
 
static void battle_draw_attack_word(struct battle *battle,struct battler *battler,int dir,double t) {
  const double arrivet=0.300; // Time of the first letter's arrival at foe, in 0..1.
  const int16_t xmargin=80; // Horizontal start and end points in pixels.
  const double arch=20.0;
  const double ybase=30.0;
  int texid=texcache_get_image(&g.texcache,RID_image_tiles);
  
  /* Select endpoints and put letters in chronological order.
   */
  uint8_t tileidv[8];
  int tileidc=0;
  int16_t x0,x1;
  if (dir>0) { // Left to right. Throw the last letter first.
    x0=xmargin;
    x1=g.fbw-xmargin;
    int i=battler->attackc;
    while (i-->0) tileidv[tileidc++]=battler->attack[i];
  } else { // Right to left.
    x0=g.fbw-xmargin;
    x1=xmargin;
    memcpy(tileidv,battler->attack,battler->attackc);
    tileidc=battler->attackc;
  }
  if (tileidc<1) return;
  
  /* Visit each tile.
   */
  double subt=t/arrivet;
  int16_t subx=(int16_t)((double)x0*(1.0-subt)+x1*(double)subt);
  int16_t dx=(dir>0)?-TILESIZE:TILESIZE;
  double scale=1.0/((double)x1-(double)x0);
  int i=0,inflightc=0,arrived=0;
  for (;i<tileidc;i++,subx+=dx) {
    if (x0<x1) {
      if (subx<x0) { continue; }
      if (subx>x1) { arrived++; continue; }
    } else {
      if (subx<x1) { arrived++; continue; }
      if (subx>x0) { continue; }
    }
    inflightc++;
    double innert=((double)subx-(double)x0)*scale; // 0..1 relative to the arc
    innert-=0.5;
    innert*=2.0;
    innert=1.0-(innert*innert);
    int16_t y=(int16_t)(ybase-arch*innert);
    graf_draw_tile(&g.graf,texid,subx,y,tileidv[i],0);
  }
  if (arrived&&(battler->force>=0)) {
    if (battler==&battle->p1) battle_begin_damage(battle,&battle->p2,battler->force);
    else battle_begin_damage(battle,&battle->p1,battler->force);
    if (arrived!=battle->last_arrived) {
      battle->last_arrived=arrived;
      egg_play_sound(RID_sound_letterslap);
    }
  }
}

/* Draw the attack word centered toward the bottom of the action scene, for legibility.
 */
 
static void battle_draw_still_word(struct battle *battle,struct battler *battler,double t) {
  int16_t dstx=(g.fbw>>1)-((TILESIZE*battler->attackc)>>1)+(TILESIZE>>1); // center of leftmost tile
  int16_t dsty=(g.fbh>>1)-TILESIZE;
  int texid=texcache_get_image(&g.texcache,RID_image_tiles);
  int16_t x=dstx;
  int i=0;
  /* Our purpose is legibility, but we wouldn't want to be *too* legible, would we?
   * Make the letters jiggle a little.
   */
  double r=t*M_PI*2.0*6.0;
  for (;i<battler->attackc;i++,x+=TILESIZE,r+=2.5) {
    int16_t tilex=x+(int16_t)(cos(r)*1.5);
    int16_t tiley=dsty+(int16_t)(sin(r)*1.5);
    graf_draw_tile(&g.graf,texid,tilex,tiley,battler->attack[i],0);
  }
}

/* Render.
 */
 
void battle_render(struct battle *battle) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x001020ff);
  
  /* Log in the bottom half, centered.
   * We've tried to arrange for its width to be less than the combined player hands.
   */
  int16_t logy=((g.fbh*3)>>2)-(battle->logh>>1);
  int16_t logx=(g.fbw>>1)-(battle->logw>>1);
  graf_draw_rect(&g.graf,logx,logy,battle->logw,battle->logh,0x000000ff);
  if (!battle->log_texid) {
    battle->log_texid=egg_texture_new();
  }
  if (battle->logdirty) {
    egg_texture_load_raw(battle->log_texid,EGG_TEX_FMT_RGBA,battle->logw,battle->logh,battle->logw<<2,battle->log,battle->logw*4*battle->logh);
  }
  graf_draw_decal(&g.graf,battle->log_texid,logx,logy,0,0,battle->logw,battle->logh,0);
  
  /* Upper half of the screen shows the action scene.
   */
  int actionh=g.fbh>>1;
  if (battle->dark) {
    graf_draw_rect(&g.graf,0,0,g.fbw,actionh,0x000000ff);
    battle_draw_avatar(battle,&battle->p1,0);
    battle_draw_int(battle,battle->p1.disphp,20,g.fbh>>2,0xffffffff);
  } else {
    graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,RID_image_battlebg),0,0,0,actionh*g.world.battlebg,g.fbw,actionh,0);
    battle_draw_avatar(battle,&battle->p1,0);
    battle_draw_avatar(battle,&battle->p2,EGG_XFORM_XREV);
    battle_draw_int(battle,battle->p1.disphp,20,g.fbh>>2,0xffffffff);
    battle_draw_int(battle,battle->p2.disphp,g.fbw-20,g.fbh>>2,0xffffffff);
  }
  
  /* In the ATTACK stages, draw the word being flung across the top.
   */
  struct battler *attacker=0;
  switch (battle->stage) {
    case BATTLE_STAGE_ATTACK1: attacker=battle->first; break;
    case BATTLE_STAGE_ATTACK2: attacker=battle->second; break;
  }
  if (attacker) {
    double t=1.0-(battle->stageclock/BATTLE_ATTACK_TIME);
    if (t<0.0) t=0.0; else if (t>1.0) t=1.0;
    int dir=(attacker==&battle->p1)?1:-1;
    if (attacker->attackc&&!attacker->detail.valid) battle_draw_backfire_word(battle,attacker,dir,t);
    else if (attacker->confirm_fold) battle_draw_fold_word(battle,attacker,dir,t);
    else battle_draw_attack_word(battle,attacker,dir,t);
    battle_draw_still_word(battle,attacker,t);
  }
  
  /* "Bonus!" or "No effect!" at certain times during ATTACK.
   */
  if (attacker&&!(attacker->attackc&&!attacker->detail.valid)) {
    int message=0;
    if (battle->stageclock<BATTLE_ATTACK_TIME*0.700) { // appears halfway thru the stage (about when the damage is presented).
      if (attacker->confirm_fold) {
        // Folded: Nothing.
      } else if (attacker->force<0) {
        // Backfire: Nothing.
      } else if (!attacker->force) {
        message=1; // "No effect!"
      } else if (attacker->detail.superbonus) {
        message=2; // "Bonus!" eg for a "U" against the Eyeball.
      }
    }
    if (message) {
      int texid=texcache_get_image(&g.texcache,RID_image_tiles);
      int16_t w=TILESIZE*3,h=TILESIZE*2;
      int16_t dsty=20;
      int16_t dstx=(attacker==&battle->p1)?(g.fbw-90-w):90;
      int16_t srcy=TILESIZE*8;
      int16_t srcx=message*TILESIZE*3;
      if (message==1) {
        graf_set_tint(&g.graf,(battle->cursorframe&1)?0xe0e0e0ff:0xc0c0c0ff);
      } else {
        graf_set_tint(&g.graf,(battle->cursorframe&1)?0xff0000ff:0xffff00ff);
      }
      graf_draw_decal(&g.graf,texid,dstx,dsty,0,srcy,w,h,0);
      graf_set_tint(&g.graf,(message==1)?0x404040ff:0x004000ff);
      graf_draw_decal(&g.graf,texid,dstx,dsty,srcx,srcy,w,h,0);
      graf_set_tint(&g.graf,0);
    }
  }
  
  /* Show length bonus high centered, during ATTACK.
   * There can be a length bonus recorded even when the play is annulled -- don't show it in those cases.
   */
  if (attacker&&attacker->detail.lenbonus&&(battle->stageclock<BATTLE_ATTACK_TIME*0.700)&&(attacker->force>0)) {
    int16_t srcx,srcy,w=0,h=0;
    switch (attacker->attackc) {
      case 5: srcx=TILESIZE*9; srcy=TILESIZE*8; w=TILESIZE*3; h=TILESIZE; break;
      case 6: srcx=TILESIZE*9; srcy=TILESIZE*9; w=TILESIZE*3; h=TILESIZE; break;
      case 7: srcx=TILESIZE*12; srcy=TILESIZE*8; w=TILESIZE*4; h=TILESIZE; break;
    }
    if (w) {
      int16_t dstx=(g.fbw>>1);
      if (attacker==&battle->p2) dstx-=w+TILESIZE*2;
      else dstx+=TILESIZE*2;
      int16_t dsty=(g.fbh>>1)-h-4;
      graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,RID_image_tiles),dstx,dsty,srcx,srcy,w,h,0);
    }
  }
  
  /* If a 3xword item was just awarded, report it dead center.
   */
  if (battle->bonus3x>0.0) {
    int16_t w=TILESIZE*3,h=TILESIZE*2;
    int16_t dstx=(g.fbw>>1)-(w>>1);
    int16_t dsty=(g.fbh>>1)-(h>>1);
    int texid=texcache_get_image(&g.texcache,RID_image_tiles);
    graf_set_tint(&g.graf,(battle->cursorframe&1)?0xff0000ff:0xffff00ff);
    graf_set_alpha(&g.graf,0x80);
    graf_draw_decal(&g.graf,texid,dstx,dsty,0,TILESIZE*8,w,h,0);
    graf_set_tint(&g.graf,0);
    graf_set_alpha(&g.graf,0xff);
    graf_draw_decal(&g.graf,texid,dstx,dsty,0,TILESIZE*10,w,h,0);
  }
  
  /* Draw both hands along the bottom.
   * Humans' hands are always facing front.
   * Robots' only facing front if both battlers are robots.
   */
  int p1faced=battle->p1.human;
  int p2faced=battle->p2.human;
  if (!p1faced&&!p2faced) p1faced=p2faced=1;
  battle_draw_hand(battle,&battle->p1,p1faced,0,(battle->p1.human)&&(battle->stage==BATTLE_STAGE_GATHER));
  battle_draw_hand(battle,&battle->p2,p2faced,g.fbw>>1,(battle->p2.human)&&(battle->stage==BATTLE_STAGE_GATHER));
  
  /* If there's a message, display it a little up of center.
   */
  if ((battle->w_msg>0)&&(battle->h_msg>0)) {
    int16_t dstx=(g.fbw>>1)-(battle->w_msg>>1);
    int16_t dsty=(g.fbh>>1)-(battle->h_msg>>1);
    dsty-=battle->h_msg;
    graf_draw_decal(&g.graf,battle->texid_msg,dstx,dsty,0,0,battle->w_msg,battle->h_msg,0);
  }
  
  /* Draw wildcard modals if active.
   */
  if (battle->p1.wcmodal) battle_draw_wildcard_modal(battle,&battle->p1,0);
  if (battle->p2.wcmodal) battle_draw_wildcard_modal(battle,&battle->p2,g.fbw>>1);
}
