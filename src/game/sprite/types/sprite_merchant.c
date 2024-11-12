/* sprite_merchant.c
 * spawnarg: ?
 */

#include "game/bee.h"

struct sprite_merchant {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_merchant*)sprite)

/* Init.
 */
 
static int _merchant_init(struct sprite *sprite) {
  if (!sprite->spawnarg) {
    fprintf(stderr,"WARNING: merchant sprite at %d,%d has nothing on its menu\n",(int)sprite->x,(int)sprite->y);
  }
  return 0;
}

/* Bump.
 */
 
static void _merchant_bump(struct sprite *sprite) {
  modal_merchant_begin(sprite->spawnarg,(int)(sprite->x*TILESIZE)-g.world.recent_scroll_x,(int)(sprite->y*TILESIZE)-g.world.recent_scroll_y+STATUS_BAR_HEIGHT);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_merchant={
  .name="merchant",
  .objlen=sizeof(struct sprite_merchant),
  .init=_merchant_init,
  .bump=_merchant_bump,
};
