/* sprite_kitchen.c
 * spawnarg: 32 entree bits
 */

#include "game/bee.h"

struct sprite_kitchen {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_kitchen*)sprite)

/* Init.
 */
 
static int _kitchen_init(struct sprite *sprite) {
  if (!sprite->spawnarg) {
    fprintf(stderr,"WARNING: kitchen sprite at %d,%d has nothing on its menu\n",(int)sprite->x,(int)sprite->y);
  }
  return 0;
}

/* Update.
 */
 
static void _kitchen_update(struct sprite *sprite,double elapsed) {
  // Turn toward the hero horzontally if she's at least a tile away. Kitchen sprites naturally face right.
  if (GRP(HERO)->spritec>=1) {
    struct sprite *hero=GRP(HERO)->spritev[0];
    if (hero->x<sprite->x-0.25) sprite->xform=EGG_XFORM_XREV;
    else if (hero->x>sprite->x+0.25) sprite->xform=0;
  }
}

/* Bump.
 */
 
static void _kitchen_bump(struct sprite *sprite) {
  modal_kitchen_begin(sprite->spawnarg,(int)(sprite->x*TILESIZE)-g.world.recent_scroll_x,(int)(sprite->y*TILESIZE)-g.world.recent_scroll_y+STATUS_BAR_HEIGHT);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_kitchen={
  .name="kitchen",
  .objlen=sizeof(struct sprite_kitchen),
  .init=_kitchen_init,
  .update=_kitchen_update,
  .bump=_kitchen_bump,
};
