#include "game/bee.h"

struct sprite_dummy {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_dummy*)sprite)

/* Delete.
 */
 
static void _dummy_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _dummy_init(struct sprite *sprite) {
  return 0;
}

/* Update.
 */
 
static void _dummy_update(struct sprite *sprite,double elapsed) {
}

/* Render.
 */
 
static void _dummy_render(struct sprite *sprite,int16_t addx,int16_t addy) {
  int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx;
  int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
  graf_set_image(&g.graf,sprite->imageid);
  graf_tile(&g.graf,dstx,dsty,sprite->tileid,sprite->xform);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_dummy={
  .name="dummy",
  .objlen=sizeof(struct sprite_dummy),
  .del=_dummy_del,
  .init=_dummy_init,
  .update=_dummy_update,
  .render=_dummy_render,
};
