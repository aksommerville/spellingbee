/* sprite_merchant.c
 * spawnarg: 32 bits indexed little-endianly against item id.
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

/* Update.
 */
 
static void _merchant_update(struct sprite *sprite,double elapsed) {
  // Turn toward the hero horzontally if she's at least a tile away. Merchant sprites naturally face right.
  if (GRP(HERO)->spritec>=1) {
    struct sprite *hero=GRP(HERO)->spritev[0];
    if (hero->x<sprite->x-0.25) sprite->xform=EGG_XFORM_XREV;
    else if (hero->x>sprite->x+0.25) sprite->xform=0;
  }
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
  .update=_merchant_update,
  .bump=_merchant_bump,
};
