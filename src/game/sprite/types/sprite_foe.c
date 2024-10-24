/* sprite_foe.c
 * cmd 0x2f RULES ZERO
 */

#include "game/bee.h"

struct sprite_foe {
  struct sprite hdr;
  int rules;
};

#define SPRITE ((struct sprite_foe*)sprite)

/* Delete.
 */
 
static void _foe_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _foe_init(struct sprite *sprite) {
  
  if (sprite->defc>4) {
    struct cmd_reader reader={.v=sprite->def+4,.c=sprite->defc-4};
    uint8_t opcode;
    const uint8_t *argv;
    int argc;
    while ((argc=cmd_reader_next(&argv,&opcode,&reader))>0) {
      switch (opcode) {
        case 0x2f: SPRITE->rules=argv[0]; break;
      }
    }
  }
  
  return 0;
}

/* Bump.
 */
 
static void _foe_bump(struct sprite *sprite) {
  encounter_begin(&g.encounter,SPRITE->rules);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_foe={
  .name="foe",
  .objlen=sizeof(struct sprite_foe),
  .del=_foe_del,
  .init=_foe_init,
  .bump=_foe_bump,
};
