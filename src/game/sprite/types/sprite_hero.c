#include "game/bee.h"
#include "game/battle/battle.h"
#include "game/flag_names.h"

#define HERO_SLIDE_TIME 0.250
#define HERO_SLIDE_DISTANCE 0.500 /* m */

struct sprite_hero {
  struct sprite hdr;
  uint8_t col,row;
  uint8_t motion; // DIR_* if we are moving right now. (col,row) update as soon as the move starts.
  uint8_t facedir; // For display purposes. Always a cardinal direction, never zero.
  int ckcol,ckrow; // Last cell we checked for POI, so we don't repeat it.
  double animclock;
  int animframe;
  int dpad_blackout; // Wait for dpad to go zero before resuming. Set during door travel.
  double bugclock; // 0..1
  double slideclock; // Counts down. If nonzero, everything is suspended and we're repeating the last few pixels of the last step, as a visual guide on return from battle.
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Delete.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  sprite->imageid=RID_image_hero;
  sprite->tileid=0x00;
  sprite->xform=0;
  SPRITE->facedir=DIR_S;
  SPRITE->col=(uint8_t)sprite->x;
  SPRITE->row=(uint8_t)sprite->y;
  SPRITE->ckcol=-1;
  SPRITE->ckrow=-1;
  return 0;
}

/* Tried to walk into a solid cell.
 * Look for "message" POI (et al), and if we find one, begin the dialogue.
 */
 
static void hero_check_messages(struct sprite *sprite,int x,int y) {
  struct cmd_reader reader={.v=g.world.mapcmdv,.c=g.world.mapcmdc};
  uint8_t opcode;
  const uint8_t *argv;
  int argc,gravep=0;
  while ((argc=cmd_reader_next(&argv,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x43: { // flageffect
          if (x!=argv[0]) continue;
          if (y!=argv[1]) continue;
          if (flag_get(argv[2])) continue;
          if (!flag_get(argv[3])) continue;
          flag_set_nofx(argv[3],0);
          flag_set(argv[2],1);
          //TODO sound effect
        } break;
      case 0x44: { // toggle
          if (x!=argv[0]) continue;
          if (y!=argv[1]) continue;
          flag_set(argv[2],!flag_get(argv[2]));
          //TODO sound effect
        } break;
      case 0x62: { // message
          if ((argv[0]==x)&&(argv[1]==y)) {
            int rid=(argv[2]<<8)|argv[3];
            int index=(argv[4]<<8)|argv[5];
            int action=argv[6];
            int qualifier=argv[7];
            if (action==3) { // Action 3 is a dev-only cudgel to jump right to the victory modal. Don't bother displaying the text.
              modal_spawn(&modal_type_victory);
              return;
            }
            if (action==2) { // Action 2 means bump (index) by one if flag (qualifier) is set.
              if (flag_get(qualifier)) index++;
            }
            modal_message_begin_single(rid,index);
            switch (action) {
              case 1: g.hp=100; g.world.status_bar_dirty=1; break;
            }
            return;
          }
        } break;
      case 0x63: { // lights
          if ((argv[0]==x)&&(argv[1]==y)) {
            if (flag_set(argv[6],!flag_get(argv[6]))) {
              //TODO sound effect
            }
          }
        } break;
      case 0xc0: { // grave
          gravep++; // Grave index are one-based, so this is kosher.
          if (argc<2) continue;
          if ((argv[0]!=x)||(argv[1]!=y)) continue;
          if (gravep==g.gravep) {
            //TODO sound effect
            modal_message_begin_single(RID_strings_dialogue,29);
            g.gold+=500; // Ensure this agrees with strings:dialogue#29
            if (g.gold>32767) g.gold=32767;
            g.world.status_bar_dirty=1;
            g.gravep=0;
            if (!flag_get(FLAG_graverob1)) flag_set_nofx(FLAG_graverob1,1);
            else if (!flag_get(FLAG_graverob2)) flag_set_nofx(FLAG_graverob2,1);
            else if (!flag_get(FLAG_graverob3)) flag_set_nofx(FLAG_graverob3,1);
            else if (!flag_get(FLAG_graverob4)) flag_set_nofx(FLAG_graverob4,1);
            else if (!flag_get(FLAG_graverob5)) flag_set_nofx(FLAG_graverob5,1);
            save_game();
          } else {
            modal_message_begin_raw((char*)argv+2,argc-2);
          }
        } break;
    }
  }
}

/* End a step.
 */
 
static void hero_end_step(struct sprite *sprite) {
  SPRITE->motion=0;
  
  /* Update step count for the flower.
   */
  if (flag_get(FLAG_flower)) {
    g.flower_stepc++;
    if (g.flower_stepc>=120) { // Optimal 106. Without bridge 158.
      flag_set(FLAG_flower,0);
      modal_message_begin_single(RID_strings_dialogue,37);
    }
  }
  
  /* Check for doors and other POI.
   */
  struct world_poi *poi=0;
  int poic=world_get_poi(&poi,&g.world,SPRITE->col,SPRITE->row);
  for (;poic-->0;poi++) {
    switch (poi->opcode) {
      case 0x42: { // pickup: u8:x u8:y u8:itemflag u8:carryflag
          if (flag_get(poi->v[2])) continue; // already got (the poi continues to exist after).
          if (flag_get(poi->v[3])) continue; // already carrying something else, forget it. (XXX something_being_carried() now takes care of this)
          if (something_being_carried()) continue;
          flag_set_nofx(poi->v[2],1);
          flag_set(poi->v[3],1);
          //TODO sound effect
        } break;
      case 0x60: { // door: u8:srcx u8:srcy u16:mapid u8:dstx u8:dsty u8:reserved1 u8:reserved2
          int mapid=(poi->v[2]<<8)|poi->v[3];
          int dstx=poi->v[4];
          int dsty=poi->v[5];
          world_load_map(&g.world,mapid);
          sprite->x=dstx+0.5;
          sprite->y=dsty+0.5;
          SPRITE->facedir=DIR_S; // Our doors always face south. facedir is DIR_N right now, but turn around.
          SPRITE->col=dstx; // Very important not to trigger a step at the new position.
          SPRITE->row=dsty;
          SPRITE->dpad_blackout=1;
          return;
        } break;
    }
  }
  
  /* If the cell isn't SAFE, consider entering battle.
   */
  uint8_t physics=g.world.cellphysics[g.world.map[SPRITE->row*g.world.mapw+SPRITE->col]];
  if (physics!=PHYSICS_SAFE) {
    int rid=world_select_battle(&g.world);
    if (rid) {
      struct battle *battle=modal_battle_begin(rid);
      if (!battle) {
        fprintf(stderr,"battle:%d failed to launch\n",rid);
        return;
      }
      if (world_cell_is_dark(&g.world,SPRITE->col,SPRITE->row)) {
        battle_set_dark(battle);
      }
      SPRITE->slideclock=HERO_SLIDE_TIME;
      return;
    }
  }
}

/* Begin a step, if possible.
 */
 
static void hero_begin_step(struct sprite *sprite,int dx,int dy) {

  // Face changes even if we can't move.
  if (dx<0) SPRITE->facedir=DIR_W;
  else if (dx>0) SPRITE->facedir=DIR_E;
  else if (dy<0) SPRITE->facedir=DIR_N;
  else SPRITE->facedir=DIR_S;

  // Abort if the new cell is OOB, same as me, solid in map, or occupied by a solid sprite.
  // For solid sprites, call their (bump) handler if implemented.
  int col=SPRITE->col+dx;
  int row=SPRITE->row+dy;
  if ((col<0)||(row<0)||(col>=g.world.mapw)||(row>=g.world.maph)) return;
  if ((col==SPRITE->ckcol)&&(row==SPRITE->ckrow)) return;
  SPRITE->ckcol=col;
  SPRITE->ckrow=row;
  switch (g.world.cellphysics[g.world.map[row*g.world.mapw+col]]) {
    case PHYSICS_VACANT: 
    case PHYSICS_SAFE:
      break;
    case PHYSICS_SOLID: case PHYSICS_WATER: case PHYSICS_HOLE: hero_check_messages(sprite,col,row); return;
    default: return;
  }
  int i=GRP(SOLID)->spritec;
  while (i-->0) {
    struct sprite *brick=GRP(SOLID)->spritev[i];
    if ((int)brick->x!=col) continue;
    if ((int)brick->y!=row) continue;
    if (brick->type->bump) brick->type->bump(brick);
    return;
  }
  
  // OK, we're walking.
  SPRITE->col=col;
  SPRITE->row=row;
  if (dx<0) SPRITE->motion=DIR_W;
  else if (dx>0) SPRITE->motion=DIR_E;
  else if (dy<0) SPRITE->motion=DIR_N;
  else SPRITE->motion=DIR_S;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  SPRITE->bugclock+=elapsed*3.0;
  while (SPRITE->bugclock>=1.0) SPRITE->bugclock-=1.0;
  
  if (SPRITE->slideclock>0.0) {
    SPRITE->slideclock-=elapsed;
    return;
  }

  /* Continue walking.
   * We walk in discrete meter steps.
   */
  const double walkspeed=6.0; // m/s
  switch (SPRITE->motion) {
    case DIR_N: {
        double dsty=(double)SPRITE->row+0.5;
        if ((sprite->y-=walkspeed*elapsed)<=dsty) {
          sprite->y=dsty;
          hero_end_step(sprite);
        }
      } break;
    case DIR_S: {
        double dsty=(double)SPRITE->row+0.5;
        if ((sprite->y+=walkspeed*elapsed)>=dsty) {
          sprite->y=dsty;
          hero_end_step(sprite);
        }
      } break;
    case DIR_W: {
        double dstx=(double)SPRITE->col+0.5;
        if ((sprite->x-=walkspeed*elapsed)<=dstx) {
          sprite->x=dstx;
          hero_end_step(sprite);
        }
      } break;
    case DIR_E: {
        double dstx=(double)SPRITE->col+0.5;
        if ((sprite->x+=walkspeed*elapsed)>=dstx) {
          sprite->x=dstx;
          hero_end_step(sprite);
        }
      } break;
  }
  // If ending the step triggered an encounter, stop updating.
  if (g.modalc>0) return;
  
  /* Begin a new step if there's none in progress.
   * This can happen on the same cycle that the previous step ended, by design.
   */
  if (!SPRITE->motion) {
    if (SPRITE->dpad_blackout) {
      if ((g.pvinput&(EGG_BTN_LEFT|EGG_BTN_RIGHT|EGG_BTN_UP|EGG_BTN_DOWN))==0) {
        SPRITE->dpad_blackout=0;
      }
    } else {
      if (g.pvinput&EGG_BTN_LEFT) hero_begin_step(sprite,-1,0);
      else if (g.pvinput&EGG_BTN_RIGHT) hero_begin_step(sprite,1,0);
      else if (g.pvinput&EGG_BTN_UP) hero_begin_step(sprite,0,-1);
      else if (g.pvinput&EGG_BTN_DOWN) hero_begin_step(sprite,0,1);
      else SPRITE->ckcol=SPRITE->ckrow=-1;
    }
  }
  
  // Animate walking, or reset it.
  if (SPRITE->motion) {
    if ((SPRITE->animclock-=elapsed)>0.0) return;
    SPRITE->animclock+=0.200;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  } else {
    SPRITE->animclock=0.0;
    SPRITE->animframe=0;
  }
  
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  uint8_t tileid=sprite->tileid,xform=0;
  
  if (SPRITE->slideclock>0.0) {
    int add=(int)(SPRITE->slideclock*HERO_SLIDE_DISTANCE*TILESIZE);
    switch (SPRITE->facedir) {
      case DIR_S: dsty-=add; break;
      case DIR_N: dsty+=add; break;
      case DIR_W: dstx+=add; break;
      case DIR_E: dstx-=add; break;
    }
  }
  
  // Tiles are sourced in three columns: Down, Up, Left.
  switch (SPRITE->facedir) {
    case DIR_S: break;
    case DIR_N: tileid+=0x01; break;
    case DIR_W: tileid+=0x02; break;
    case DIR_E: tileid+=0x02; xform=EGG_XFORM_XREV; break;
  }
  
  // Walking animation is four frames from three tiles: +0, +16, +0, +32
  if (SPRITE->motion) {
    switch (SPRITE->animframe) {
      case 0: case 2: break;
      case 1: tileid+=0x10; break;
      case 3: tileid+=0x20; break;
    }
  }
  
  // If we're carrying the watercan and facing north, it draws first.
  int carry=something_being_carried();
  switch (carry) {
    case FLAG_watercan: carry=0x30; break;
    case FLAG_flower: carry=0x32; break;
    default: carry=0;
  }
  if (carry&&(SPRITE->facedir==DIR_N)) graf_draw_tile(&g.graf,texid,dstx+4,dsty,carry,EGG_XFORM_XREV);
  
  // Main tile.
  graf_draw_tile(&g.graf,texid,dstx,dsty,tileid,xform);
  
  // Watercan in any non-north direction draws after.
  if (carry) switch (SPRITE->facedir) {
    case DIR_S: case DIR_W: graf_draw_tile(&g.graf,texid,dstx-4,dsty,carry,0); break;
    case DIR_E: graf_draw_tile(&g.graf,texid,dstx+4,dsty,carry,EGG_XFORM_XREV); break;
  }
}

/* Render post.
 */
 
static void _hero_render_post(struct sprite *sprite,int16_t addx,int16_t addy) {
  if (!g.world.bugsprayc) return;
  if (SPRITE->bugclock>0.66) return;
  
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  
  if (SPRITE->slideclock>0.0) {
    int add=(int)(SPRITE->slideclock*HERO_SLIDE_DISTANCE*TILESIZE);
    switch (SPRITE->facedir) {
      case DIR_S: dsty-=add; break;
      case DIR_N: dsty+=add; break;
      case DIR_W: dstx+=add; break;
      case DIR_E: dstx-=add; break;
    }
  }
  
  // Bug spray indicator.
  graf_draw_tile(&g.graf,texid,dstx,dsty-TILESIZE,0x31,0);
  // Fill the can with some indication of the remaining step count. Interior is 4x7, 2 pixels off the bottom.
  // We're not showing anything special when it's overfilled. Should we? TODO
  int fillc=(g.world.bugsprayc*8)/BUG_SPRAY_DURATION;
  if (fillc>7) fillc=7; // zero is ok
  graf_draw_rect(&g.graf,dstx-2,dsty-(TILESIZE>>1)-2-fillc,4,fillc,0xff0000ff);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
  .render_post=_hero_render_post,
};
