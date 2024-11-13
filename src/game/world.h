/* world.h
 * Everything about the outer world is coordinated here.
 * Map, sprites, session-level state.
 */
 
#ifndef WORLD_H
#define WORLD_H

#define WORLD_BATTLE_LIMIT 16
#define WORLD_POI_LIMIT 256

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
  int recent_scroll_x,recent_scroll_y;
  
  struct world_battle {
    int rid,weight;
  } battlev[WORLD_BATTLE_LIMIT];
  int battlec;
  
  /* Points Of Interest that might be needed during play, eg doors.
   * (note that the editor's idea of "Interesting" is looser than ours, eg sprites are not included here.)
   * Sorted by (y,x) and duplicates at any position are allowed.
   */
  struct world_poi {
    uint8_t y,x;
    uint8_t opcode;
    const uint8_t *v; // Points into (mapcmdv). All arguments, but opcode is stripped.
    int c;
  } poiv[WORLD_POI_LIMIT];
  int poic;
};

int world_init(struct world *world,const char *save,int savec);

int world_save(char *dst,int dsta,const struct world *world);

/* For the dpad, world polls g.pvinput.
 * Act and Cancel are impulses, so main tracks them for us.
 */
void world_update(struct world *world,double elapsed);
void world_activate(struct world *world);
void world_cancel(struct world *world);

void world_render(struct world *world);

void world_load_map(struct world *world,int mapid);

/* Return a read-only pointer to all POI records at a given coordinate.
 */
int world_get_poi(struct world_poi **dstpp,struct world *world,int x,int y);

#endif
