#include "bee.h"
#include "shared_symbols.h"

/* Init.
 */
 
int world_init(struct world *world,const char *save,int savec) {
  TRACE("save=%.*s",savec,save)
  
  /* This defaults the globals if input is empty or invalid, which is exactly what we want.
   */
  saved_game_decode(&g.stats,save,savec);
  
  /* Some extra state sanitization, business-aware.
   */
  flag_set(NS_flag_flower,0); // Can't start with the flower, since you could use death as a warp to cheat the step limit.
  g.stats.hp=100; // You start next to the free-heal point. Why bother even saving HP?
  
  if (!world->status_bar_texid) {
    if ((world->status_bar_texid=egg_texture_new())<0) return -1;
    if (egg_texture_load_raw(world->status_bar_texid,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,0,0)<0) return -1;
  }
  world->status_bar_dirty=1;
  
  sprite_group_kill(GRP(KEEPALIVE));
  world_load_map(world,1);
  
  return 0;
}

/* Update.
 */

void world_update(struct world *world,double elapsed) {
  sprite_group_update(GRP(UPDATE),elapsed);
  
  // Turn GRP(FACEHERO) to face the hero. Their natural orientation must be rightward.
  if (GRP(HERO)->spritec>=1) {
    double herox=GRP(HERO)->spritev[0]->x;
    int i=GRP(FACEHERO)->spritec;
    while (i-->0) {
      struct sprite *sprite=GRP(FACEHERO)->spritev[i];
      double dx=herox-sprite->x;
      if (dx>0.25) sprite->xform=0;
      else if (dx<-0.25) sprite->xform=EGG_XFORM_XREV;
    }
  }
  
  sprite_group_kill(GRP(DEATHROW));
}

/* A and B buttons.
 */
 
void world_activate(struct world *world) {
  if (g.stats.inventory[ITEM_BUGSPRAY]&&(g.stats.bugspray<BUG_SPRAY_SATURATION)) {
    TRACE("bugspray")
    egg_play_sound(RID_sound_bugspray);
    g.stats.inventory[ITEM_BUGSPRAY]--;
    world->status_bar_dirty=1;
    g.stats.bugspray+=BUG_SPRAY_DURATION;
    if (g.stats.bugspray>BUG_SPRAY_SATURATION) {
      g.stats.bugspray=BUG_SPRAY_SATURATION;
    }
  } else {
    TRACE("no bugspray")
    egg_play_sound(RID_sound_reject);
  }
}

void world_cancel(struct world *world) {
}

/* Render status bar content.
 */
 
void world_draw_status_bar_content(struct world *world) {
  uint32_t *bits=calloc(g.fbw<<2,STATUS_BAR_HEIGHT);
  if (!bits) return;
  const int xnudge=25;
  
  // Up to 5 digits.
  #define DECFLD(_dstx,lbl,lclr,_v) { \
    int v=_v; if (v<0) v=0; \
    char chv[16]; \
    int chc=0; \
    if (v>=10000) chv[chc++]='0'+(v/10000)%10; \
    if (v>= 1000) chv[chc++]='0'+(v/ 1000)%10; \
    if (v>=  100) chv[chc++]='0'+(v/  100)%10; \
    if (v>=   10) chv[chc++]='0'+(v/   10)%10; \
    chv[chc++]='0'+v%10; \
    int dstx=_dstx+xnudge; \
    dstx+=font_render_string(bits,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,dstx,1,g.font,lbl,-1,v?lclr:0x808080ff); \
    dstx+=2; \
    font_render_string(bits,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,dstx,1,g.font,chv,chc,v?0xffffffff:0x606060ff); \
  }
  
  // Our font includes icon glyphs for the status bar: 0x80..0x87 = HP, Gold, Bug spray, Unfairie, Double, Triple, XP, Eraser
  DECFLD(  0,"\xc2\x80",0xff0000ff,g.stats.hp)
  DECFLD( 40,"\xc2\x81",0xffff00ff,g.stats.gold)
  DECFLD( 80,"\xc2\x86",0x808080ff,g.stats.xp)
  DECFLD(120,"\xc2\x87",0xe95fc4ff,g.stats.inventory[ITEM_ERASER])
  DECFLD(160,"\xc2\x82",0x00ffffff,g.stats.inventory[ITEM_BUGSPRAY])
  DECFLD(200,"\xc2\x83",0xdcbe18ff,g.stats.inventory[ITEM_UNFAIRIE])
  DECFLD(240,"\xc2\x84",0xe080acff,g.stats.inventory[ITEM_2XWORD])
  DECFLD(280,"\xc2\x85",0xbb0a30ff,g.stats.inventory[ITEM_3XWORD])
  
  egg_texture_load_raw(world->status_bar_texid,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,bits,g.fbw*STATUS_BAR_HEIGHT*4);
  free(bits);
  #undef DECFLD
}

/* Render.
 */

void world_render(struct world *world) {
  
  /* View centers on the hero, but clamps to map edges.
   * If the map is smaller than the viewport, lock it centered.
   * In the unlikely case that there's no hero sprite, stay where we were last time.
   */
  int scrollx=world->recent_scroll_x;
  int scrolly=world->recent_scroll_y;
  if (GRP(HERO)->spritec>=1) {
    int focusx=0,focusy=0;
    const struct sprite *hero=GRP(HERO)->spritev[0];
    focusx=(int)(hero->x*TILESIZE);
    focusy=(int)(hero->y*TILESIZE);
    int worldw=world->mapw*TILESIZE;
    int worldh=world->maph*TILESIZE;
    int fill=0;
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
  }
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
  
  /* Draw dark regions.
   * If at least one exists, draw the hero again after we're done.
   * All non-hero sprites are supposed to be occluded by the darkness, but the hero herself should render above it.
   * Rather than figuring out how to extract her and whether she's under a dark patch, whatever, just draw her twice.
   */
  if (world->darkc>0) {
    const struct world_dark *dark=world->darkv;
    int i=world->darkc;
    for (;i-->0;dark++) {
      graf_draw_rect(&g.graf,
        dark->x*TILESIZE-scrollx,
        dark->y*TILESIZE-scrolly+STATUS_BAR_HEIGHT,
        dark->w*TILESIZE,
        dark->h*TILESIZE,
        0x000000ff
      );
    }
    sprite_group_render(GRP(HERO),-scrollx,-scrolly+STATUS_BAR_HEIGHT);
  }
  
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
  world->songid=0;
  memset(world->battlebag,0xff,sizeof(world->battlebag));
  world->battlebagp=0;
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
  struct rom_tilesheet_reader reader;
  if (rom_tilesheet_reader_init(&reader,serial,serialc)<0) return;
  struct rom_tilesheet_entry entry;
  while (rom_tilesheet_reader_next(&entry,&reader)>0) {
    if (entry.tableid!=NS_tilesheet_physics) continue;
    memcpy(world->cellphysics+entry.tileid,entry.v,entry.c);
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

/* Apply the smoothing filter, read-only.
 * Returns <0 to move to the lower neighbor, >0 to the higher, or 0 to do nothing.
 */
 
static int world_smooth_battlebag_1(const uint8_t *bag,int p) {
  int d=1,lop=p,hip=p;
  for (;d<40;d++) { // 40 arbitrary, in theory WORLD_BATTLE_BAG_SIZE is the limit.
    lop--;
    hip++;
    int look=((lop>=0)&&(bag[lop]==0xff));
    int hiok=((hip<WORLD_BATTLE_BAG_SIZE)&&(bag[hip]==0xff));
    if (look&&hiok) continue;
    if (look) return -1;
    if (hiok) return 1;
    break;
  }
  return 0;
}

static void dump_bag(const struct world *world) {
  int i=0; for (;i<WORLD_BATTLE_BAG_SIZE;i++) fprintf(stderr,"%c",(world->battlebag[i]==0xff)?'.':('0'+world->battlebag[i]));
  fprintf(stderr,"\n");
}

/* Compose a fresh battle bag from (world->battlev).
 */
 
static void world_bag_battles(struct world *world) {
  memset(world->battlebag,0xff,sizeof(world->battlebag));
  world->battlebagp=0;
  
  /* Add up all the weights, and if there's too many, log it and do nothing.
   * Likewise if there's zero, just return.
   */
  int wsum=0,i=world->battlec;
  const struct world_battle *battle=world->battlev;
  for (;i-->0;battle++) wsum+=battle->weight;
  if (wsum<1) return; // No battles, perfectly normal (eg library)
  if (wsum>=WORLD_BATTLE_BAG_SIZE) { // sic >=, if they're equal we'll end up dividing by zero on the last one. (and honestly, even like 1/4 is way too many)
    fprintf(stderr,"map:%d: Battle weights %d greater than bag size %d. Creating an empty bag.\n",world->mapid,wsum,WORLD_BATTLE_BAG_SIZE);
    return;
  }
  
  /* Track the unassigned slots in battlebag.
   * This is what we'll draw from, and we'll shrink it incrementally.
   */
  uint8_t slotv[WORLD_BATTLE_BAG_SIZE];
  int slotc=0;
  for (;slotc<WORLD_BATTLE_BAG_SIZE;slotc++) slotv[slotc]=slotc;
  
  /* Run through the battles again, taking one slot for each unit of its weight.
   * Purely random at this point.
   */
  for (i=0,battle=world->battlev;i<world->battlec;i++,battle++) {
    int bi=battle->weight;
    while (bi-->0) {
      int slotp=rand()%slotc;
      world->battlebag[slotv[slotp]]=i;
      slotc--;
      memmove(slotv+slotp,slotv+slotp+1,slotc-slotp);
    }
  }
  
  /* If we leave it at that, it's not bad, but there is a tendency to clump.
   * It's annoying to have two battles really close to each other.
   * So make a few passes with a filter that nudges each battle left or right, whichever direction has more freedom.
   * This strategy operates in place so it's asymmetric: Battles move right by one slot, or left halfway to that neighbor.
   * Experimentally, one or two passes looks pretty good. After ten passes, it's almost perfectly uniform.
   */
  //dump_bag(world);
  for (i=2;i-->0;) { // 2 completely arbitrary
    int p=WORLD_BATTLE_BAG_SIZE;
    while (p-->0) {
      if (world->battlebag[p]==0xff) continue;
      int motion=world_smooth_battlebag_1(world->battlebag,p);
      if (motion<0) {
        world->battlebag[p-1]=world->battlebag[p];
        world->battlebag[p]=0xff;
      } else if (motion>0) {
        world->battlebag[p+1]=world->battlebag[p];
        world->battlebag[p]=0xff;
      }
    }
    //dump_bag(world);
  }
  //fprintf(stderr,"Rebagged battles for map:%d. wsum=%d\n",world->mapid,wsum);
}

/* Return the next battle bag item and rebuild it when needed.
 */
 
int world_select_battle(struct world *world) {
  if (world->battlec<1) return 0;
  if (world->battlebagp>=WORLD_BATTLE_BAG_SIZE) {
    // It seems overkill to recalculate the bag each time it empties, but it's cheap so whatever.
    world_bag_battles(world);
  }
  if (g.stats.bugspray>0) {
    g.stats.bugspray--;
    return 0;
  }
  int battlep=world->battlebag[world->battlebagp++];
  if (battlep>=world->battlec) return 0;
  return world->battlev[battlep].rid;
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

/* Add light switch and darkened region.
 */
 
static void world_add_lights(struct world *world,int switchx,int switchy,int roomx,int roomy,int roomw,int roomh,int flagid) {
  if (flag_get(flagid)) {
    if ((switchx<world->mapw)&&(switchy<world->maph)) {
      world->map[switchy*world->mapw+switchx]++;
    }
    return;
  }
  if (world->darkc<WORLD_DARK_LIMIT) {
    struct world_dark *dark=world->darkv+world->darkc++;
    dark->x=roomx;
    dark->y=roomy;
    dark->w=roomw;
    dark->h=roomh;
  }
}

/* Load map resource, private.
 */
 
static int world_load_map_res(struct world *world,int mapid) {

  world->darkc=0;

  const uint8_t *serial=0;
  int serialc=rom_get_res(&serial,EGG_TID_map,mapid);
  struct rom_map rmap;
  if (rom_map_decode(&rmap,serial,serialc)<0) return -1;
  world->mapw=rmap.w;
  world->maph=rmap.h;
  if ((world->mapw<1)||(world->maph<1)) return -1;
  
  int len=world->mapw*world->maph;
  if (len>world->mapa) {
    void *nv=realloc(world->map,world->mapw*world->maph);
    if (!nv) return -1;
    world->map=nv;
    world->mapa=len;
  }
  world->virginmap=rmap.v;
  memcpy(world->map,rmap.v,len);

  world->mapcmdv=rmap.cmdv;
  world->mapcmdc=rmap.cmdc;
  world->mapid=mapid;
  world->battlec=0;
  world->poic=0;
  world->songid=0;
  return 0;
}

/* Load map.
 */
 
void world_load_map(struct world *world,int mapid) {
  TRACE("map:%d",mapid)

  /* Kill the sprites.
   * If there's a hero, remove her from KEEPALIVE first, we want that sprite to survive the transition.
   */
  struct sprite *hero=0;
  if (GRP(HERO)->spritec>0) {
    hero=GRP(HERO)->spritev[0];
    if (sprite_ref(hero)<0) return;
    sprite_group_remove(GRP(KEEPALIVE),hero);
  }
  sprite_group_kill(GRP(KEEPALIVE));
  if (hero) {
    sprite_group_add(GRP(KEEPALIVE),hero);
    sprite_del(hero);
  }
  
  /* Acquire the resource and set up basic things.
   */
  if (world_load_map_res(world,mapid)<0) {
    world_clear_map(world);
    egg_play_song(world->songid,0,1);
    return;
  }
  
  /* Read and apply commands.
   */
  struct rom_command_reader reader={.v=world->mapcmdv,.c=world->mapcmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case 0x20: world->songid=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case 0x21: world->map_imageid=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case 0x22: world_spawn_hero(world,cmd.argv[0],cmd.argv[1]); break;
      case 0x23: world->battlebg=(cmd.argv[0]<<8)|cmd.argv[1]; break;
      case 0x40: world_add_battle(world,(cmd.argv[0]<<8)|cmd.argv[1],(cmd.argv[2]<<8)|cmd.argv[3]); break;
      case 0x41: if ((cmd.argv[0]<world->mapw)&&(cmd.argv[1]<world->maph)&&flag_get(cmd.argv[2])) world->map[cmd.argv[1]*world->mapw+cmd.argv[0]]++; break;
      case 0x42: {
          if ((cmd.argv[0]<world->mapw)&&(cmd.argv[1]<world->maph)&&flag_get(cmd.argv[2])) {
            world->map[cmd.argv[1]*world->mapw+cmd.argv[0]]++;
          }
          world_add_poi(world,cmd.opcode,cmd.argv[0],cmd.argv[1],cmd.argv,cmd.argc);
        } break;
      case 0x44: if ((cmd.argv[0]<world->mapw)&&(cmd.argv[1]<world->maph)&&flag_get(cmd.argv[2])) world->map[cmd.argv[1]*world->mapw+cmd.argv[0]]++; break;
      case 0x60: world_add_poi(world,cmd.opcode,cmd.argv[0],cmd.argv[1],cmd.argv,cmd.argc); break;
      case 0x61: sprite_spawn_from_map((cmd.argv[2]<<8)|cmd.argv[3],cmd.argv[0],cmd.argv[1],(cmd.argv[4]<<24)|(cmd.argv[5]<<16)|(cmd.argv[6]<<8)|cmd.argv[7]); break;
      case 0x63: world_add_lights(world,cmd.argv[0],cmd.argv[1],cmd.argv[2],cmd.argv[3],cmd.argv[4],cmd.argv[5],cmd.argv[6]); break;
    }
  }
  world_bag_battles(world);
  world_load_tilesheet(world);
  egg_play_song(world->songid,0,1);
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

/* Recheck flags.
 */
 
void world_recheck_flags(struct world *world) {
  struct rom_command_reader reader={.v=world->mapcmdv,.c=world->mapcmdc};
  struct rom_command cmd;
  world->darkc=0;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    switch (cmd.opcode) {
      case 0x41: case 0x42: case 0x44: if ((cmd.argv[0]<world->mapw)&&(cmd.argv[1]<world->maph)) { // flagtile,pickup,toggle
          int p=cmd.argv[1]*world->mapw+cmd.argv[0];
          if (flag_get(cmd.argv[2])) {
            world->map[p]=world->virginmap[p]+1;
          } else {
            world->map[p]=world->virginmap[p];
          }
        } break;
      case 0x63: if ((cmd.argv[0]<world->mapw)&&(cmd.argv[1]<world->maph)) {
          int p=cmd.argv[1]*world->mapw+cmd.argv[0];
          if (flag_get(cmd.argv[6])) {
            world->map[p]=world->virginmap[p]+1;
          } else {
            world->map[p]=world->virginmap[p];
            if (world->darkc<WORLD_DARK_LIMIT) {
              struct world_dark *dark=world->darkv+world->darkc++;
              dark->x=cmd.argv[2];
              dark->y=cmd.argv[3];
              dark->w=cmd.argv[4];
              dark->h=cmd.argv[5];
            }
          }
        } break;
    }
  }
}

/* Test darkness.
 */
 
int world_cell_is_dark(const struct world *world,int x,int y) {
  if (!world||(x<0)||(y<0)||(x>=world->mapw)||(y>=world->maph)) return 0;
  const struct world_dark *dark=world->darkv;
  int i=world->darkc;
  for (;i-->0;dark++) {
    if (x<dark->x) continue;
    if (y<dark->y) continue;
    if (x>=dark->x+dark->w) continue;
    if (y>=dark->y+dark->h) continue;
    return 1;
  }
  return 0;
}
