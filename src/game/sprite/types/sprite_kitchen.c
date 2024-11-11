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
  .bump=_kitchen_bump,
};
