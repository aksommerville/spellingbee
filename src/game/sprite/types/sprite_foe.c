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
  
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,sprite->def,sprite->defc)>=0) {
    struct cmdlist_entry cmd;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case 0x2f: SPRITE->battleid=(cmd.arg[0]<<8)|cmd.arg[1]; break;
      }
    }
  }
  
  SPRITE->flagid=sprite->spawnarg>>24;
  if (flag_get(SPRITE->flagid)) return -1;
  
  return 0;
}

/* Bump.
 */
 
static void _foe_bump(struct sprite *sprite) {
  struct battle *battle=modal_battle_begin(SPRITE->battleid);
  if (!battle) return;
  if (g.stats.battlec<0xffffff) g.stats.battlec++;
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
  .bump=_foe_bump,
};
