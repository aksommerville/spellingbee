/* sprite_foe.c
 * cmd 0x2f BATTLEID
 * spawnarg: u8:flag u24:reserved
 */

#include "game/bee.h"
#include "game/battle/battle.h"

struct sprite_foe {
  struct sprite hdr;
  int battleid;
  int flagid;
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
        case 0x2f: SPRITE->battleid=(argv[0]<<8)|argv[1]; break;
      }
    }
  }
  
  SPRITE->flagid=sprite->spawnarg>>24;
  if (flag_get(SPRITE->flagid)) return -1;
  
  return 0;
}

/* Update.
 */
 
static void _foe_update(struct sprite *sprite,double elapsed) {
  // Turn toward the hero horzontally if she's at least a tile away. Foe sprites naturally face right.
  if (GRP(HERO)->spritec>=1) {
    struct sprite *hero=GRP(HERO)->spritev[0];
    if (hero->x<sprite->x-0.25) sprite->xform=EGG_XFORM_XREV;
    else if (hero->x>sprite->x+0.25) sprite->xform=0;
  }
}

/* Bump.
 */
 
static void _foe_bump(struct sprite *sprite) {
  struct battle *battle=modal_battle_begin(SPRITE->battleid);
  if (!battle) return;
  if (world_cell_is_dark(&g.world,(int)sprite->x,(int)sprite->y)) {
    battle_set_dark(battle);
  }
  if (SPRITE->flagid) {
    sprite_kill_soon(sprite); // Even if this foe wins, we're not staying here.
    battle->flagid=SPRITE->flagid;
  } else {
    // No flag, the sprite sticks around after battle and you can do it again.
  }
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_foe={
  .name="foe",
  .objlen=sizeof(struct sprite_foe),
  .del=_foe_del,
  .init=_foe_init,
  .update=_foe_update,
  .bump=_foe_bump,
};
