#include "game/bee.h"

struct sprite_hero {
  struct sprite hdr;
  uint8_t col,row;
  uint8_t motion; // DIR_* if we are moving right now. (col,row) update as soon as the move starts.
  int ckcol,ckrow; // Last cell we checked for POI, so we don't repeat it.
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
  SPRITE->col=(uint8_t)sprite->x;
  SPRITE->row=(uint8_t)sprite->y;
  SPRITE->ckcol=-1;
  SPRITE->ckrow=-1;
  return 0;
}

/* Tried to walk into a solid cell.
 * Look for "message" POI, and if we find one, begin the dialogue.
 */
 
static void hero_check_messages(struct sprite *sprite,int x,int y) {
  struct cmd_reader reader={.v=g.world.mapcmdv,.c=g.world.mapcmdc};
  uint8_t opcode;
  const uint8_t *argv;
  int argc;
  while ((argc=cmd_reader_next(&argv,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x62: { // message
          if ((argv[0]==x)&&(argv[1]==y)) {
            int rid=(argv[2]<<8)|argv[3];
            int index=(argv[4]<<8)|argv[5];
            int action=argv[6];
            int qualifier=argv[7];
            modal_message_begin_single(rid,index);
            switch (action) {
              case 1: g.hp=100; break;
            }
            return;
          }
        } break;
    }
  }
}

/* End a step.
 */
 
static void hero_end_step(struct sprite *sprite) {
  SPRITE->motion=0;
  uint8_t physics=g.world.cellphysics[g.world.map[SPRITE->row*g.world.mapw+SPRITE->col]];
  if (physics!=PHYSICS_SAFE) {
    //TODO Random traps with parameters from the map.
    if (!(rand()%10)) {
      encounter_begin(&g.encounter,0);
    }
  }
}

/* Begin a step, if possible.
 */
 
static void hero_begin_step(struct sprite *sprite,int dx,int dy) {
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
    case PHYSICS_SOLID: hero_check_messages(sprite,col,row); return;
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
  if (g.encounter.active) return;
  
  /* Begin a new step if there's none in progress.
   * This can happen on the same cycle that the previous step ended, by design.
   */
  if (!SPRITE->motion) {
    if (g.pvinput&EGG_BTN_LEFT) hero_begin_step(sprite,-1,0);
    else if (g.pvinput&EGG_BTN_RIGHT) hero_begin_step(sprite,1,0);
    else if (g.pvinput&EGG_BTN_UP) hero_begin_step(sprite,0,-1);
    else if (g.pvinput&EGG_BTN_DOWN) hero_begin_step(sprite,0,1);
    else SPRITE->ckcol=SPRITE->ckrow=-1;
  }
  
  //TODO animation
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
  graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,sprite->imageid),dstx,dsty,sprite->tileid,sprite->xform);
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
};
