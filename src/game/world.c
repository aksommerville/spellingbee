#include "bee.h"

/* Init.
 */
 
int world_init(struct world *world) {
  g.hp=100;
  memset(g.inventory,0,sizeof(g.inventory));
  if (!world->status_bar_texid) {
    if ((world->status_bar_texid=egg_texture_new())<0) return -1;
    if (egg_texture_load_raw(world->status_bar_texid,EGG_TEX_FMT_RGBA,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,0,0)<0) return -1;
  }
  world->status_bar_dirty=1;
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

/* Render status bar content.
 */
 
void world_draw_status_bar_content(struct world *world) {
  uint32_t *bits=calloc(g.fbw<<2,STATUS_BAR_HEIGHT);
  if (!bits) return;
  const int xnudge=25;
  
  // Up to 3 digits, but we can trivially add more.
  #define DECFLD(_dstx,lbl,_v) { \
    int v=_v; if (v<0) v=0; \
    char chv[16]; \
    int chc=0; \
    if (v>=100) chv[chc++]='0'+(v/100)%10; \
    if (v>= 10) chv[chc++]='0'+(v/ 10)%10; \
    chv[chc++]='0'+v%10; \
    int dstx=_dstx+xnudge; \
    dstx+=font_render_string(bits,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,dstx,1,g.font,lbl,-1,0x808080ff); \
    dstx+=2; \
    font_render_string(bits,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,dstx,1,g.font,chv,chc,v?0xffffffff:0x606060ff); \
  }
  
  // TODO Colorful icons would be better than these text labels.
  DECFLD(  0,"HP",g.hp)
  DECFLD( 40,"G",g.gold)
  DECFLD( 80,"XP",g.xp)
  DECFLD(120,"E",g.inventory[ITEM_ERASER])
  DECFLD(160,"2L",g.inventory[ITEM_2XLETTER])
  DECFLD(200,"3L",g.inventory[ITEM_3XLETTER])
  DECFLD(240,"2W",g.inventory[ITEM_2XWORD])
  DECFLD(280,"3W",g.inventory[ITEM_3XWORD])
  
  egg_texture_load_raw(world->status_bar_texid,EGG_TEX_FMT_RGBA,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,bits,g.fbw*STATUS_BAR_HEIGHT*4);
  free(bits);
  #undef DECFLD
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
  world->recent_scroll_x=scrollx;
  world->recent_scroll_y=scrolly;
  
  /* Draw tiles.
   */
  int cola=scrollx/TILESIZE; if (cola<0) cola=0;
  int colz=(scrollx+g.fbw-1)/TILESIZE; if (colz>=world->mapw) colz=world->mapw-1;
  int rowa=scrolly/TILESIZE; if (rowa<0) rowa=0;
  int rowz=(scrolly+g.fbh-1)/TILESIZE; if (rowz>=world->maph) rowz=world->maph-1;
  graf_draw_tile_buffer(
    &g.graf,texcache_get_image(&g.texcache,world->map_imageid),
    cola*TILESIZE+(TILESIZE>>1)-scrollx,
    rowa*TILESIZE+(TILESIZE>>1)-scrolly+STATUS_BAR_HEIGHT,
    world->map+rowa*world->mapw+cola,
    colz-cola+1,rowz-rowa+1,world->mapw
  );
  
  /* Draw sprites.
   */
  sprite_group_render(GRP(VISIBLE),-scrollx,-scrolly+STATUS_BAR_HEIGHT);
  
  /* Draw status bar.
   */
  if (world->status_bar_dirty) {
    world->status_bar_dirty=0;
    world_draw_status_bar_content(world);
  }
  graf_draw_rect(&g.graf,0,0,g.fbw,STATUS_BAR_HEIGHT,0x000000ff);
  graf_draw_decal(&g.graf,world->status_bar_texid,0,0,0,0,g.fbw,STATUS_BAR_HEIGHT,0);
}

/* Clear map, if loading fails.
 */
 
static void world_clear_map(struct world *world) {
  world->mapid=0;
  world->mapw=0;
  world->maph=0;
  world->mapcmdc=0;
  world->map_imageid=0;
  world->battlec=0;
  world->poic=0;
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

/* Add battle.
 */
 
static void world_add_battle(struct world *world,int rid,int weight) {
  if (!rid||!weight) return;
  if (world->battlec>=WORLD_BATTLE_LIMIT) {
    fprintf(stderr,"%s:%d:WARNING: Ignoring random battle:%d with weight 0x%04x due to limit %d exceeded.\n",__FILE__,__LINE__,rid,weight,WORLD_BATTLE_LIMIT);
    return;
  }
  struct world_battle *battle=world->battlev+world->battlec++;
  battle->rid=rid;
  battle->weight=weight;
}

/* Search POI list. If there's more than one at a point, returns any of them, not necessarily the first.
 */
 
static int world_poi_search(const struct world *world,uint8_t x,uint8_t y) {
  int lo=0,hi=world->poic;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct world_poi *poi=world->poiv+ck;
         if (y<poi->y) hi=ck;
    else if (y>poi->y) lo=ck+1;
    else if (x<poi->x) hi=ck;
    else if (x>poi->x) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Add POI.
 */
 
static void world_add_poi(struct world *world,uint8_t opcode,uint8_t x,uint8_t y,const uint8_t *v,int c) {
  if (world->poic>=WORLD_POI_LIMIT) {
    fprintf(stderr,"map:%d:WARNING: Too many POI, limit %d\n",world->mapid,WORLD_POI_LIMIT);
    return;
  }
  int p=world_poi_search(world,x,y);
  if (p<0) p=-p-1;
  struct world_poi *poi=world->poiv+p;
  memmove(poi+1,poi,sizeof(struct world_poi)*(world->poic-p));
  world->poic++;
  poi->x=x;
  poi->y=y;
  poi->opcode=opcode;
  poi->v=v;
  poi->c=c;
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
  world->battlec=0;
  struct cmd_reader reader={.v=world->mapcmdv,.c=world->mapcmdc};
  uint8_t opcode;
  const uint8_t *argv;
  int argc;
  while ((argc=cmd_reader_next(&argv,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x20: egg_play_song((argv[0]<<8)|argv[1],0,1); break;
      case 0x21: world->map_imageid=(argv[0]<<8)|argv[1]; break;
      case 0x22: world_spawn_hero(world,argv[0],argv[1]); break;
      case 0x40: world_add_battle(world,(argv[0]<<8)|argv[1],(argv[2]<<8)|argv[3]); break;
      case 0x60: world_add_poi(world,opcode,argv[0],argv[1],argv,argc); break;
      case 0x61: sprite_spawn_from_map((argv[0]<<8)|argv[1],argv[2],argv[3],(argv[4]<<24)|(argv[5]<<16)|(argv[6]<<8)|argv[7]); break;
    }
  }
  world_load_tilesheet(world);
}

/* Get POI.
 */
 
int world_get_poi(struct world_poi **dstpp,struct world *world,int x,int y) {
  if ((x<0)||(x>0xff)||(y<0)||(y>0xff)) return 0;
  int p=world_poi_search(world,x,y);
  if (p<0) return 0;
  int c=1;
  struct world_poi *src=world->poiv+p;
  while (p&&(src[-1].x==x)&&(src[-1].y==y)) { p--; src--; c++; }
  while ((p+c<world->poic)&&(src[c].x==x)&&(src[c].y==y)) c++;
  *dstpp=src;
  return c;
}
