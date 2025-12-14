#include "game/bee.h"

/* Delete.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->refc-->1) return;
  if (sprite->type->del) sprite->type->del(sprite);
  if (sprite->groupv) {
    free(sprite->groupv);
  }
  free(sprite);
}

/* Retain.
 */
 
int sprite_ref(struct sprite *sprite) {
  if (!sprite) return -1;
  if (sprite->refc<1) return -1;
  if (sprite->refc>=INT_MAX) return -1;
  sprite->refc++;
  return 0;
}

/* New.
 */
 
struct sprite *sprite_new(
  const struct sprite_type *type,
  double x,double y,
  uint32_t spawnarg,
  const uint8_t *def,int defc
) {
  if (!type) return 0;
  struct sprite *sprite=calloc(1,type->objlen);
  if (!sprite) return 0;
  
  sprite->type=type;
  sprite->refc=1;
  sprite->x=x;
  sprite->y=y;
  sprite->spawnarg=spawnarg;
  
  if (sprite_group_add(GRP(KEEPALIVE),sprite)<0) {
    sprite_del(sprite);
    return 0;
  }
  sprite_del(sprite);
  
  // Always add to VISIBLE, and add to UPDATE if the type has an update hook.
  // Likewise if there's a (bump) hook, put them in SOLID.
  if (type->update) sprite_group_add(GRP(UPDATE),sprite);
  sprite_group_add(GRP(VISIBLE),sprite);
  if (type->bump) sprite_group_add(GRP(SOLID),sprite);
  
  sprite->def=def;
  sprite->defc=defc;
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,def,defc)>=0) {
    struct cmdlist_entry cmd;
    int groups_set=0;
    while (cmdlist_reader_next(&cmd,&reader)>0) {
      switch (cmd.opcode) {
        case 0x20: sprite->imageid=(cmd.arg[0]<<8)|cmd.arg[1]; break;
        case 0x22: sprite->tileid=cmd.arg[0]; sprite->xform=cmd.arg[1]; break;
        case 0x40: {
            uint32_t mask=(cmd.arg[0]<<24)|(cmd.arg[1]<<16)|(cmd.arg[2]<<8)|cmd.arg[3];
            struct sprite_group *group=sprite_groupv;
            for (;mask;mask>>=1,group++) {
              if (mask&1) sprite_group_add(group,sprite);
            }
            groups_set=1;
          } break;
      }
    }
  }
  
  if (type->init&&(type->init(sprite)<0)) {
    sprite_kill_now(sprite);
    return 0;
  }
  
  return sprite;
}

/* Spawn sprite from map.
 */

struct sprite *sprite_spawn_from_map(uint16_t rid,uint8_t col,uint8_t row,uint32_t spawnarg) {
  const void *serial=0;
  int serialc=rom_get_res(&serial,EGG_TID_sprite,rid);
  struct cmdlist_reader reader;
  if (sprite_reader_init(&reader,serial,serialc)<0) return 0;
  struct cmdlist_entry cmd;
  const struct sprite_type *type=0;
  while (cmdlist_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case 0x21: type=sprite_type_by_id((cmd.arg[0]<<8)|cmd.arg[1]); break;
    }
  }
  double x=col+0.5,y=row+0.5;
  return sprite_new(type,x,y,spawnarg,serial,serialc);
}

/* Spawn sprite programmatically.
 */
 
struct sprite *sprite_spawn(const struct sprite_type *type,double x,double y) {
  return sprite_new(type,x,y,0,0,0);
}

/* Type registry.
 */
 
static const struct sprite_type *sprite_typev[]={
#define _(tag) &sprite_type_##tag,
SPRITE_TYPE_FOR_EACH
#undef _
};
 
const struct sprite_type *sprite_type_by_id(int id) {
  if (id<0) return 0;
  if (id>=sizeof(sprite_typev)/sizeof(void*)) return 0;
  return sprite_typev[id];
}
