/* world.h
 * Everything about the outer world is coordinated here.
 * Map, sprites, session-level state.
 */
 
#ifndef WORLD_H
#define WORLD_H

#define WORLD_BATTLE_LIMIT 16

struct world {
  int mapid;
  int mapw,maph;
  const uint8_t *map;
  const uint8_t *mapcmdv;
  int mapcmdc;
  int map_imageid;
  uint8_t cellphysics[256];
  int status_bar_texid;
  int status_bar_dirty; // Other parties may set directly, when status changes.
  struct world_battle {
    int rid,weight;
  } battlev[WORLD_BATTLE_LIMIT];
  int battlec;
  int recent_scroll_x,recent_scroll_y;
};

int world_init(struct world *world);

/* For the dpad, world polls g.pvinput.
 * Act and Cancel are impulses, so main tracks them for us.
 */
void world_update(struct world *world,double elapsed);
void world_activate(struct world *world);
void world_cancel(struct world *world);

void world_render(struct world *world);

void world_load_map(struct world *world,int mapid);

#endif
