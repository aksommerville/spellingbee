#include "bee.h"

/* Init.
 */
 
int world_init(struct world *world) {
  g.hp=100;
  memset(g.inventory,0,sizeof(g.inventory));
  world_load_map(world,3);
  return 0;
}

/* Update.
 */

void world_update(struct world *world,double elapsed) {
  sprite_group_update(GRP(UPDATE),elapsed);
  sprite_group_kill(GRP(DEATHROW));
}

/* A and B buttons.
 */
 
void world_activate(struct world *world) {
}

void world_cancel(struct world *world) {
}

/* Render.
 */

void world_render(struct world *world) {
  
  /* View centers on the hero, but clamps to map edges.
   * If the map is smaller than the viewport, lock it centered.
   */
  int focusx=0,focusy=0;
  if (GRP(HERO)->spritec>=1) {//TODO Should we plan for cases where there's no hero sprite?
    const struct sprite *hero=GRP(HERO)->spritev[0];
    focusx=(int)(hero->x*TILESIZE);
    focusy=(int)(hero->y*TILESIZE);
  }
  int worldw=world->mapw*TILESIZE;
  int worldh=world->maph*TILESIZE;
  int scrollx,scrolly,fill=0;
  if (worldw<=g.fbw) {
    scrollx=(worldw>>1)-(g.fbw>>1);
    fill=1;
  } else {
    scrollx=focusx-(g.fbw>>1);
    if (scrollx<0) scrollx=0;
    else if (scrollx>worldw-g.fbw) scrollx=worldw-g.fbw;
  }
  if (worldh<=g.fbh) {
    scrolly=(worldh>>1)-(g.fbh>>1);
    fill=1;
  } else {
    scrolly=focusy-(g.fbh>>1);
    if (scrolly<0) scrolly=0;
    else if (scrolly>worldh-g.fbh) scrolly=worldh-g.fbh;
  }
  if (fill) graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x000000ff);
  
  /* Draw tiles.
   */
  int cola=scrollx/TILESIZE; if (cola<0) cola=0;
  int colz=(scrollx+g.fbw-1)/TILESIZE; if (colz>=world->mapw) colz=world->mapw-1;
  int rowa=scrolly/TILESIZE; if (rowa<0) rowa=0;
  int rowz=(scrolly+g.fbh-1)/TILESIZE; if (rowz>=world->maph) rowz=world->maph-1;
  graf_draw_tile_buffer(
    &g.graf,texcache_get_image(&g.texcache,world->map_imageid),
    cola*TILESIZE+(TILESIZE>>1)-scrollx,
    rowa*TILESIZE+(TILESIZE>>1)-scrolly,
    world->map+rowa*world->mapw+cola,
    colz-cola+1,rowz-rowa+1,world->mapw
  );
  
  /* Draw sprites.
   */
  sprite_group_render(GRP(VISIBLE),-scrollx,-scrolly);
}

/* Clear map, if loading fails.
 */
 
static void world_clear_map(struct world *world) {
  world->mapid=0;
  world->mapw=0;
  world->maph=0;
  world->mapcmdc=0;
  world->map_imageid=0;
}

/* Spawn hero if there isn't one yet.
 */
 
static void world_spawn_hero(struct world *world,int col,int row) {
  if (GRP(HERO)->spritec>0) return; // Already got one, thanks
  struct sprite *hero=sprite_spawn(&sprite_type_hero,col+0.5,row+0.5);
  if (!hero) return;
  sprite_group_add(GRP(HERO),hero);
}

/* Load tilesheet.
 */
 
static void world_load_tilesheet(struct world *world) {
  memset(world->cellphysics,0,sizeof(world->cellphysics));
  const uint8_t *serial=0;
  int serialc=rom_get_res(&serial,EGG_TID_tilesheet,world->map_imageid);
  if ((serialc<4)||memcmp(serial,"\0TLS",4)) return;
  int serialp=4;
  while (serialp<serialc) {
    uint8_t tableid=serial[serialp++];
    if (!tableid) break;
    if (serialp>serialc-2) break;
    uint8_t tileid0=serial[serialp++];
    uint8_t tilec=serial[serialp++]+1;
    if (serialp>serialc-tilec) break;
    switch (tableid) {
      case 1: { // physics
          memcpy(world->cellphysics+tileid0,serial+serialp,tilec);
        } break;
    }
    serialp+=tilec;
  }
}

/* Load map.
 */
 
void world_load_map(struct world *world,int mapid) {
  sprite_group_kill(GRP(KEEPALIVE));//TODO Keep hero alive across map changes
  const uint8_t *serial=0;
  int serialc=rom_get_res(&serial,EGG_TID_map,mapid);
  if ((serialc<6)||memcmp(serial,"\0MAP",4)) { world_clear_map(world); return; }
  world->mapw=serial[4];
  world->maph=serial[5];
  if ((world->mapw<1)||(world->maph<1)) { world_clear_map(world); return; }
  int serialp=6;
  if (serialp>serialc-world->mapw*world->maph) { world_clear_map(world); return; }
  world->map=serial+serialp;
  serialp+=world->mapw*world->maph;
  world->mapcmdv=serial+serialp;
  world->mapcmdc=serialc-serialp;
  world->mapid=mapid;
  struct cmd_reader reader={.v=world->mapcmdv,.c=world->mapcmdc};
  uint8_t opcode;
  const uint8_t *argv;
  int argc;
  while ((argc=cmd_reader_next(&argv,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x20: egg_play_song((argv[0]<<8)|argv[1],0,1); break;
      case 0x21: world->map_imageid=(argv[0]<<8)|argv[1]; break;
      case 0x22: world_spawn_hero(world,argv[0],argv[1]); break;
      case 0x60: break; // TODO Door: u8:srcx u8:srcy u16:mapid u8:dstx u8:dsty u8:reserved1 u8:reserved2
      case 0x61: sprite_spawn_from_map((argv[0]<<8)|argv[1],argv[2],argv[3],(argv[4]<<24)|(argv[5]<<16)|(argv[6]<<8)|argv[7]); break;
    }
  }
  world_load_tilesheet(world);
}
