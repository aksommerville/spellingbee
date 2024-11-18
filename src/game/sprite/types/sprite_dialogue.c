/* sprite_dialogue.c
 * A dummy sprite that speaks when bumped.
 */
 
#include "game/bee.h"
 
struct sprite_dialogue {
  struct sprite hdr;
  int stringsid,index;
};

#define SPRITE ((struct sprite_dialogue*)sprite)

static int _dialogue_init(struct sprite *sprite) {
  SPRITE->stringsid=sprite->spawnarg>>16;
  SPRITE->index=sprite->spawnarg&0xffff;
  return 0;
}

static void _dialogue_bump(struct sprite *sprite) {
  modal_message_begin_single(SPRITE->stringsid,SPRITE->index);
}

const struct sprite_type sprite_type_dialogue={
  .name="dialogue",
  .objlen=sizeof(struct sprite_dialogue),
  .init=_dialogue_init,
  .bump=_dialogue_bump,
};
