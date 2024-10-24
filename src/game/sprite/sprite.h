/* sprite.h
 * Abstract framework for sprites.
 * Sprites have a visible presence on screen, physical presence in the model world, and can update at each cycle.
 * Can instantiate programmatically or from a "sprite" resource.
 * Every sprite has a "type", ie some C code and metadata, which can be identified with an integer.
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;
struct sprite_group;

/* Generic sprite.
 ***************************************************************************/
 
struct sprite {
  const struct sprite_type *type; // REQUIRED
  int refc;
  double x,y; // In map cells.
  int imageid;
  uint8_t tileid,xform;
  uint32_t spawnarg; // From map's spawn point, if applicable.
  const uint8_t *def; // From sprite resource, if applicable.
  int defc;
  struct sprite_group **groupv;
  int groupc,groupa;
};

struct sprite_type {
  const char *name;
  int objlen;
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite);
  void (*update)(struct sprite *sprite,double elapsed);
  void (*render)(struct sprite *sprite,int16_t addx,int16_t addy);
  void (*bump)(struct sprite *sprite);
};

/* It's unusual to call these.
 * Prefer "spawn" for creating a sprite and "kill" for deleting.
 */
void sprite_del(struct sprite *sprite);
int sprite_ref(struct sprite *sprite);
struct sprite *sprite_new(
  const struct sprite_type *type,
  double x,double y,
  uint32_t spawnarg,
  const uint8_t *def,int defc
);

struct sprite *sprite_spawn_from_map(uint16_t rid,uint8_t col,uint8_t row,uint32_t spawnarg);
struct sprite *sprite_spawn(const struct sprite_type *type,double x,double y);

/* Sprite groups.
 *****************************************************************************/
 
struct sprite_group {
  struct sprite **spritev;
  int spritec,spritea;
  int refc;
};

void sprite_group_del(struct sprite_group *group);
int sprite_group_ref(struct sprite_group *group);
struct sprite_group *sprite_group_new();

int sprite_group_has(const struct sprite_group *group,const struct sprite *sprite);
int sprite_group_add(struct sprite_group *group,struct sprite *sprite);
int sprite_group_remove(struct sprite_group *group,struct sprite *sprite);

void sprite_group_clear(struct sprite_group *group);
void sprite_kill_now(struct sprite *sprite);
void sprite_group_kill(struct sprite_group *group);

void sprite_group_update(struct sprite_group *group,double elapsed);
void sprite_group_render(struct sprite_group *group,int16_t addx,int16_t addy);

#define SPRITE_GROUP_KEEPALIVE 0
#define SPRITE_GROUP_DEATHROW  1
#define SPRITE_GROUP_VISIBLE   2
#define SPRITE_GROUP_UPDATE    3
#define SPRITE_GROUP_HERO      4
#define SPRITE_GROUP_SOLID     5

extern struct sprite_group sprite_groupv[32];
#define GRP(tag) (sprite_groupv+SPRITE_GROUP_##tag)

/* During init, a sprite can destroy itself by returning <0. That will be effective immediately.
 * Anywhere else you want to destroy a sprite, use sprite_kill_soon.
 * This will defer the actual deletion until the end of the update cycle.
 */
static inline void sprite_kill_soon(struct sprite *sprite) { sprite_group_add(GRP(DEATHROW),sprite); }

/* Registry of types.
 * To add a type, just append it to SPRITE_TYPE_FOR_EACH and create its struct sprite_type somewhere.
 * It will automatically get an id based on the order of SPRITE_TYPE_FOR_EACH.
 *******************************************************************************************/
 
#define SPRITE_TYPE_FOR_EACH \
  _(dummy) \
  _(hero) \
  _(foe)
  
#define _(tag) extern const struct sprite_type sprite_type_##tag;
SPRITE_TYPE_FOR_EACH
#undef _

const struct sprite_type *sprite_type_by_id(int id);

#endif
