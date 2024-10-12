/* world.h
 * Everything about the outer world is coordinated here.
 * Map, sprites, session-level state.
 */
 
#ifndef WORLD_H
#define WORLD_H

struct world {
  int mapid;
  int mapw,maph;
  const uint8_t *map;
  const uint8_t *mapcmdv;
  int mapcmdc;
  int map_imageid;
  uint8_t cellphysics[256];
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
