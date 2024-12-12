/* sprite_customer.c
 * Walks horizontally, and when there's a bookshelf north of us, randomly stop and read.
 * Mostly we're a decorative nuisance.
 * May contain dialogue.
 * spawnarg: u16 stringsid, u16 index
 */
 
#include "game/bee.h"

#define CUSTOMER_STAGE_PAUSE 0 /* Standing idle, facing horizontally. Timed. */
#define CUSTOMER_STAGE_WALK 1 /* Walking horizontally, stops at the next tile. */
#define CUSTOMER_STAGE_READ 2 /* Same as PAUSE but facing vertically, and a longer timer. */

#define CUSTOMER_WALK_SPEED 2.000 /* m/s */
#define CUSTOMER_PAUSE_MIN 0.125
#define CUSTOMER_PAUSE_MAX 0.750
#define CUSTOMER_READ_MIN 2.500
#define CUSTOMER_READ_MAX 8.000

/* Instance definition.
 */
 
struct sprite_customer {
  struct sprite hdr;
  uint8_t tileid0;
  int stage;
  double stageclock;
  uint8_t facedir; // DIR_W or DIR_E, or DIR_N in CUSTOMER_STAGE_READ. Tiles face left.
  double dx; // WALK stage only; speed is baked in. m/s
  double dstx; // WALK stage terminates and clamps when we reach this.
  double animclock; // WALK
  int animframe; // WALK, 0..1
};

#define SPRITE ((struct sprite_customer*)sprite)

/* Cleanup.
 */
 
static void _customer_del(struct sprite *sprite) {
}

/* Change stage.
 */
 
static void customer_begin_PAUSE(struct sprite *sprite) {
  SPRITE->stage=CUSTOMER_STAGE_PAUSE;
  SPRITE->stageclock=CUSTOMER_PAUSE_MIN+((rand()&0x7fff)*(CUSTOMER_PAUSE_MAX-CUSTOMER_PAUSE_MIN))/32768.0;
  sprite->tileid=SPRITE->tileid0;
  sprite->xform=(SPRITE->facedir==DIR_E)?EGG_XFORM_XREV:0;
}

static void customer_begin_READ(struct sprite *sprite) {
  SPRITE->stage=CUSTOMER_STAGE_READ;
  SPRITE->stageclock=CUSTOMER_READ_MIN+((rand()&0x7fff)*(CUSTOMER_READ_MAX-CUSTOMER_READ_MIN))/32768.0;
  sprite->tileid=SPRITE->tileid0+2;
  sprite->xform=0;
}

static void customer_begin_WALK(struct sprite *sprite) {
  SPRITE->stage=CUSTOMER_STAGE_WALK;
  SPRITE->stageclock=2.0/CUSTOMER_WALK_SPEED; // Travel should end the stage, but do impose a limit.
  sprite->tileid=SPRITE->tileid0;
  sprite->xform=(SPRITE->facedir==DIR_E)?EGG_XFORM_XREV:0;
  SPRITE->animclock=0.0;
  SPRITE->animframe=0;
  if (SPRITE->facedir==DIR_W) {
    SPRITE->dx=-CUSTOMER_WALK_SPEED;
    SPRITE->dstx=sprite->x-1.0;
  } else {
    SPRITE->dx=CUSTOMER_WALK_SPEED;
    SPRITE->dstx=sprite->x+1.0;
  }
}

/* Stage selection.
 */

static void customer_consider_next_move(struct sprite *sprite) {
  int col=(int)sprite->x,row=(int)sprite->y;
  
  /* Is walking forward an option?
   */
  int forwardok=0;
  if ((row>=0)&&(row<g.world.maph)) {
    int nextcol=col+((SPRITE->facedir==DIR_E)?1:-1);
    if ((nextcol>=0)&&(nextcol<g.world.mapw)) {
      uint8_t nexttile=g.world.map[row*g.world.mapw+nextcol];
      uint8_t physics=g.world.cellphysics[nexttile];
      if ((physics==PHYSICS_VACANT)||(physics==PHYSICS_SAFE)) {
        // Per grid it's fine.
        // Check the hero. Within like 2 or 3 meters of our target, call it blocked.
        forwardok=1;
        if (GRP(HERO)->spritec>=1) {
          struct sprite *hero=GRP(HERO)->spritev[0];
          double dx=hero->x-sprite->x;
          double dy=hero->y-sprite->y;
          if ((dy>=-2.0)&&(dy<=2.0)) {
            if (SPRITE->facedir==DIR_E) {
              if ((dx>0.0)&&(dx<=6.0)) forwardok=0;
            } else {
              if ((dx>=-6.0)&&(dx<0.0)) forwardok=0;
            }
          }
        }
        // Check other solid sprites. These aren't expected to move as much as hero, so we can tighten up a little.
        if (forwardok) {
          int i=GRP(SOLID)->spritec;
          while (i-->0) {
            struct sprite *other=GRP(SOLID)->spritev[i];
            if (other==sprite) continue;
            double dx=other->x-sprite->x;
            double dy=other->y-sprite->y;
            if ((dy<=1.0)||(dy>=1.0)) continue;
            if (SPRITE->facedir==DIR_E) {
              if ((dx<0.0)||(dx>=2.0)) continue;
            } else {
              if ((dx<=-2.0)||(dx>0.0)) continue;
            }
            forwardok=0;
            break;
          }
        }
      }
    }
  }
  
  /* Is the tile north of us a bookshelf?
   */
  int bookshelf=0;
  if ((col>=0)&&(col<g.world.mapw)&&(row>=1)&&(row<=g.world.maph)) {
    int qrow=row-1;
    uint8_t tileid=g.world.map[qrow*g.world.mapw+col];
    // Hard-coding the bookshelf tileids. I guess a different physics value would be appropriate, but meh.
    if ((tileid>=0x54)&&(tileid<=0x58)) bookshelf=1;
    else if ((tileid>=0x74)&&(tileid<=0x77)) bookshelf=1;
  }
  
  /* Options:
   * - 0: PAUSE. Always available.
   * - 1: WALK in the current direction. Check walls and hero, and other solid sprites.
   * - 2: Turn around, then pause. Always available. Weight heavily if we're facing a wall.
   * - 3: Read. Check tile north of us.
   */
  struct option { int id,weight; } optionv[4];
  int optionc=0,i;
  if (bookshelf) { // PAUSE is always an option, but when books absent, we de-emphasize.
    optionv[optionc++]=(struct option){0,100};
  } else {
    optionv[optionc++]=(struct option){0,10};
  }
  if (forwardok) {
    optionv[optionc++]=(struct option){1,500}; // Walking forward is a great option if possible.
    optionv[optionc++]=(struct option){2,50}; // Can also turn around whenever, but not preferred.
  } else {
    optionv[optionc++]=(struct option){2,500}; // When facing a wall, turning around is a great idea.
  }
  if (bookshelf) optionv[optionc++]=(struct option){3,100}; // Found something interesting? Read it.
  
  /* Pick an option at random.
   */
  int wsum=0;
  for (i=optionc;i-->0;) wsum+=optionv[i].weight;
  int choice=rand()%wsum;
  for (i=optionc;i-->0;) {
    if ((choice-=optionv[i].weight)<=0) {
      switch (optionv[i].id) {
        case 0: customer_begin_PAUSE(sprite); return;
        case 1: customer_begin_WALK(sprite); return;
        case 2: SPRITE->facedir=(SPRITE->facedir==DIR_E)?DIR_W:DIR_E; customer_begin_PAUSE(sprite); return;
        case 3: customer_begin_READ(sprite); return;
      }
    }
  }
  
  // And if I messed up the logic there, PAUSE is always an option.
  customer_begin_PAUSE(sprite);
}

/* Init.
 */
 
static int _customer_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  SPRITE->facedir=(rand()&1)?DIR_W:DIR_E;
  customer_begin_PAUSE(sprite);
  return 0;
}

/* Update.
 */
 
static void _customer_update(struct sprite *sprite,double elapsed) {

  /* When the stageclock expires, enter PAUSE.
   * Unless we're already in PAUSE, then pick a move from scratch.
   */
  if ((SPRITE->stageclock-=elapsed)<=0) {
    if (SPRITE->stage==CUSTOMER_STAGE_PAUSE) {
      customer_consider_next_move(sprite);
    } else {
      customer_begin_PAUSE(sprite);
    }
  }

  /* In WALK stage, there's motion, animation, and stage termination.
   */
  if (SPRITE->stage==CUSTOMER_STAGE_WALK) {
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=0.200;
      if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
      sprite->tileid=SPRITE->tileid0+SPRITE->animframe;
    }
    sprite->x+=SPRITE->dx*elapsed;
    if (((SPRITE->dx<0.0)&&(sprite->x<=SPRITE->dstx))||((SPRITE->dx>0.0)&&(sprite->x>=SPRITE->dstx))) {
      sprite->x=SPRITE->dstx;
      customer_begin_PAUSE(sprite);
    }
  }
}

/* Bump.
 */
 
static void _customer_bump(struct sprite *sprite) {
  uint16_t rid=sprite->spawnarg>>16,index=sprite->spawnarg;
  if (!rid) {
    // No action is fine, just do nothing.
  } else if (rid<0x40) {
    // strings are language-qualified, can't go above 63.
    modal_message_begin_single(rid,index);
    // When we start talking, if the hero approached horizontally, turn to face her and reenter PAUSE stage.
    if (GRP(HERO)->spritec>=1) {
      struct sprite *hero=GRP(HERO)->spritev[0];
      if ((hero->y>sprite->y-0.25)&&(hero->y<sprite->y+0.25)) {
        if (hero->x<sprite->x) SPRITE->facedir=DIR_W;
        else SPRITE->facedir=DIR_E;
        customer_begin_PAUSE(sprite);
      }
    }
  } else {
    // All other rid are reserved for fancier things in the future.
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_customer={
  .name="customer",
  .objlen=sizeof(struct sprite_customer),
  .del=_customer_del,
  .init=_customer_init,
  .update=_customer_update,
  .bump=_customer_bump,
};
