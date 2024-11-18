#include "bee.h"
#include "flag_names.h"

/* Encode saved game.
 */
 
int world_save(char *dst,int dsta,const struct world *world) {
  int i;

  /* Assemble binary data.
   * Overkilling it by clamping to valid ranges. We'll always produce a valid save.
   */
  uint8_t bin[256];
  int binc=4; // skip checksum
  if (g.hp<1) bin[binc++]=1;
  else if (g.hp>100) bin[binc++]=100;
  else bin[binc++]=g.hp;
  if (g.xp<0) { bin[binc++]=0; bin[binc++]=0; }
  else if (g.xp>0x7fff) { bin[binc++]=0x7f; bin[binc++]=0xff; }
  else { bin[binc++]=g.xp>>8; bin[binc++]=g.xp; }
  if (g.gold<0) { bin[binc++]=0; bin[binc++]=0; }
  else if (g.gold>0x7fff) { bin[binc++]=0x7f; bin[binc++]=0xff; }
  else { bin[binc++]=g.gold>>8; bin[binc++]=g.gold; }
  bin[binc++]=ITEM_COUNT;
  bin[binc++]=0; // ITEM_NOOP must be zero even if it's not.
  for (i=1;i<ITEM_COUNT;i++) {
    if (g.inventory[i]>99) bin[binc++]=99;
    else bin[binc++]=g.inventory[i];
  }
  int flagc=FLAGS_SIZE;
  while (flagc&&!g.flags[flagc-1]) flagc--;
  bin[binc++]=flagc;
  memcpy(bin+binc,g.flags,flagc);
  binc+=flagc;
  switch (binc%3) {
    case 1: bin[binc++]=0; bin[binc++]=0; break;
    case 2: bin[binc++]=0; break;
  }
  
  /* We now can know the exact output size. Abort if too big.
   */
  int dstc=(binc/3)*4;
  if (dstc>dsta) {
    return -1;
  }
  
  /* Compute and encode checksum.
   */
  uint32_t cksum=0xc396a5e7;
  for (i=4;i<binc;i++) {
    cksum=(cksum>>25)|(cksum<<7);
    cksum^=bin[i];
  }
  bin[0]=cksum>>24;
  bin[1]=cksum>>16;
  bin[2]=cksum>>8;
  bin[3]=cksum;
  
  /* Recursive obfuscation filter.
   */
  for (i=1;i<binc;i++) bin[i]^=bin[i-1];
  
  /* Encode base64ish direct to the output.
   * Owing to the 3-byte padding, we don't need to worry about partial units.
   */
  const char *alphabet="#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abc";
  int dstp=0,binp=0;
  for (;binp<binc;dstp+=4,binp+=3) {
    uint8_t a=bin[binp]>>2;
    uint8_t b=((bin[binp]<<4)|(bin[binp+1]>>4))&0x3f;
    uint8_t c=((bin[binp+1]<<2)|(bin[binp+2]>>6))&0x3f;
    uint8_t d=bin[binp+2]&0x3f;
    dst[dstp]=alphabet[a];
    dst[dstp+1]=alphabet[b];
    dst[dstp+2]=alphabet[c];
    dst[dstp+3]=alphabet[d];
  }

  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Apply saved game, or default global state.
 */
 
static int world_apply_save(struct world *world,const char *save,int savec) {
  g.hp=100;
  g.xp=0;
  g.gold=0;
  memset(g.inventory,0,sizeof(g.inventory));
  memset(g.flags,0,sizeof(g.flags));
  g.flags[FLAG_one>>3]|=1<<(FLAG_one&7);
  
  /* No save file is fine, just report success now.
   */
  if (!save||(savec<1)) return 0;
  
  /* Input length must be a multiple of four, and 3/4 of it must fit in our temp buffer.
   */
  uint8_t bin[256];
  if ((savec&3)||((savec*3)>>2>sizeof(bin))) {
    fprintf(stderr,"%s: Invalid length %d\n",__func__,savec);
    return -1;
  }
  
  /* Decode base64ish.
   */
  int binc=0,savep=0;
  for (;savep<savec;binc+=3,savep+=4) {
    #define DECODE(ch) ({ \
      uint8_t byte; \
      if ((ch>=0x23)&&(ch<=0x5b)) byte=ch-0x23; \
      else if ((ch>=0x5d)&&(ch<=0x63)) byte=ch-0x5d+57; \
      else { \
        fprintf(stderr,"%s: Unexpected byte 0x%02x\n",__func__,ch); \
        return -1; \
      } \
      byte; \
    })
    uint8_t a=DECODE(save[savep]);
    uint8_t b=DECODE(save[savep+1]);
    uint8_t c=DECODE(save[savep+2]);
    uint8_t d=DECODE(save[savep+3]);
    #undef DECODE
    bin[binc]=(a<<2)|(b>>4);
    bin[binc+1]=(b<<4)|(c>>2);
    bin[binc+2]=(c<<6)|d;
  }
  
  /* Unfilter.
   */
  int i=binc;
  for (;i-->1;) bin[i]^=bin[i-1];
  
  /* Compute and compare checksum.
   */
  uint32_t cksum=0xc396a5e7;
  for (i=4;i<binc;i++) {
    cksum=(cksum>>25)|(cksum<<7);
    cksum^=bin[i];
  }
  uint32_t claim=(bin[0]<<24)|(bin[1]<<16)|(bin[2]<<8)|bin[3];
  if (cksum!=claim) {
    fprintf(stderr,"%s: Checksum mismatch! computed=0x%08x claimed=0x%08x\n",__func__,cksum,claim);
    return -1;
  }
  
  /* Validate fields in binary before touching global state.
   * It's OK at this point for the incoming inventory or flags to be too long.
   * Everything else must be perfect.
   */
  int q;
  if (binc<10) return -1; // Must go at least through (inventoryc)
  if ((bin[4]<1)||(bin[4]>100)) return -1; // hp 1..100
  q=(bin[5]<<8)|bin[6]; if ((q<0)||(q>0x7fff)) return -1; // xp 0..32767
  q=(bin[7]<<8)|bin[8]; if ((q<0)||(q>0x7fff)) return -1; // gold 0..32767
  if (10+bin[9]>binc) return -1; // inventory overflow
  for (i=bin[9];i-->0;) {
    if (bin[10+i]>99) return -1; // inventory 0..99
  }
  if (bin[9]&&bin[10]) return -1; // ITEM_NOOP must be zero if present
  int binp=10+bin[9];
  if (binp>=binc) return -1; // Missing flag count.
  if (binp+1+bin[binp]>binc) return -1; // flags overflow
  if (bin[binp]) {
    if ((bin[binp+1]&0x03)!=0x02) return -1; // flags zero and one not their expected constant value
  }
  
  /* Apply to global state.
   */
  g.hp=bin[4];
  g.xp=(bin[5]<<8)|bin[6];
  g.gold=(bin[7]<<8)|bin[8];
  q=bin[9]; if (q>sizeof(g.inventory)) q=sizeof(g.inventory); memcpy(g.inventory,bin+10,q);
  q=bin[10+bin[9]]; if (q>sizeof(g.flags)) q=sizeof(g.flags); memcpy(g.flags,bin+10+bin[9]+1,q);
  
  return 0;
}

/* Init.
 */
 
int world_init(struct world *world,const char *save,int savec) {
  
  if (world_apply_save(world,save,savec)<0) {
    fprintf(stderr,"Error applying saved game! Starting a new one instead.\n");
  }
  
  if (!world->status_bar_texid) {
    if ((world->status_bar_texid=egg_texture_new())<0) return -1;
    if (egg_texture_load_raw(world->status_bar_texid,EGG_TEX_FMT_RGBA,g.fbw,STATUS_BAR_HEIGHT,g.fbw<<2,0,0)<0) return -1;
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
  
  // Up to 5 digits.
  #define DECFLD(_dstx,lbl,_v) { \
    int v=_v; if (v<0) v=0; \
    char chv[16]; \
    int chc=0; \
    if (v>=10000) chv[chc++]='0'+(v/10000)%10; \
    if (v>= 1000) chv[chc++]='0'+(v/ 1000)%10; \
    if (v>=  100) chv[chc++]='0'+(v/  100)%10; \
    if (v>=   10) chv[chc++]='0'+(v/   10)%10; \
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

/* Compose a fresh battle bag from (world->battlev).
 */
 
static void world_bag_battles(struct world *world) {
  memset(world->battlebag,0xff,sizeof(world->battlebag));
  world->battlebagp=0;
  if (world->battlec<1) return;
  
  /* Normalize battle weights, turn each into a count of bag slots.
   * None is allowed to be less than one, and their sum must be <=WORLD_BATTLE_BAG_SIZE.
   */
  int weightrange=0xffff; // Start with the "no battle" weight.
  const struct world_battle *battle=world->battlev;
  int i=world->battlec;
  for (;i-->0;battle++) weightrange+=battle->weight;
  int countv[WORLD_BATTLE_LIMIT],sum=0;
  for (i=world->battlec;i-->0;) {
    battle=world->battlev+i;
    countv[i]=(battle->weight*WORLD_BATTLE_BAG_SIZE)/weightrange;
    if (countv[i]<1) countv[i]=1;
    sum+=countv[i];
  }
  if (sum>WORLD_BATTLE_BAG_SIZE) {
    for (i=world->battlec;i-->0;) {
      int rmc=countv[i]-1;
      if (rmc>sum) rmc=sum;
      if (rmc<1) continue;
      countv[i]-=rmc;
      sum-=rmc;
      if (sum<1) break;
    }
  }
  
  /* Place so many entries in the bag, as recorded in (countv).
   */
  for (i=world->battlec;i-->0;) {
    int c=countv[i];
    while (c-->0) {
      int p=((uint32_t)rand())%WORLD_BATTLE_BAG_SIZE;
      while (world->battlebag[p]!=0xff) p=((uint32_t)rand())%WORLD_BATTLE_BAG_SIZE; // Might churn in overpopulated maps.... I doubt it's a problem.
      world->battlebag[p]=i;
    }
  }
}

/* Return the next battle bag item and rebuild it when needed.
 */
 
int world_select_battle(struct world *world) {
  if (world->battlec<1) return 0;
  if (world->battlebagp>=WORLD_BATTLE_BAG_SIZE) {
    // It seems overkill to recalculate the bag each time it empties, but it's cheap so whatever.
    world_bag_battles(world);
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

/* Load map resource, private.
 */
 
static int world_load_map_res(struct world *world,int mapid) {
  const uint8_t *serial=0;
  int serialc=rom_get_res(&serial,EGG_TID_map,mapid);
  if ((serialc<6)||memcmp(serial,"\0MAP",4)) return -1;
  world->mapw=serial[4];
  world->maph=serial[5];
  if ((world->mapw<1)||(world->maph<1)) return -1;
  int serialp=6;
  if (serialp>serialc-world->mapw*world->maph) return -1;
  
  int len=world->mapw*world->maph;
  if (len>world->mapa) {
    void *nv=realloc(world->map,world->mapw*world->maph);
    if (!nv) return -1;
    world->map=nv;
    world->mapa=len;
  }
  world->virginmap=serial+serialp;
  memcpy(world->map,serial+serialp,len);
  serialp+=len;

  world->mapcmdv=serial+serialp;
  world->mapcmdc=serialc-serialp;
  world->mapid=mapid;
  world->battlec=0;
  world->poic=0;
  world->songid=0;
  return 0;
}

/* Load map.
 */
 
void world_load_map(struct world *world,int mapid) {

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
  struct cmd_reader reader={.v=world->mapcmdv,.c=world->mapcmdc};
  uint8_t opcode;
  const uint8_t *argv;
  int argc;
  while ((argc=cmd_reader_next(&argv,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x20: world->songid=(argv[0]<<8)|argv[1]; break;
      case 0x21: world->map_imageid=(argv[0]<<8)|argv[1]; break;
      case 0x22: world_spawn_hero(world,argv[0],argv[1]); break;
      case 0x40: world_add_battle(world,(argv[0]<<8)|argv[1],(argv[2]<<8)|argv[3]); break;
      case 0x41: if ((argv[0]<world->mapw)&&(argv[1]<world->maph)&&(g.flags[argv[2]>>3]&(1<<(argv[2]&7)))) world->map[argv[1]*world->mapw+argv[0]]++; break;
      case 0x60: world_add_poi(world,opcode,argv[0],argv[1],argv,argc); break;
      case 0x61: sprite_spawn_from_map((argv[0]<<8)|argv[1],argv[2],argv[3],(argv[4]<<24)|(argv[5]<<16)|(argv[6]<<8)|argv[7]); break;
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
  struct cmd_reader reader={.v=world->mapcmdv,.c=world->mapcmdc};
  uint8_t opcode;
  const uint8_t *argv;
  int argc;
  while ((argc=cmd_reader_next(&argv,&opcode,&reader))>=0) {
    switch (opcode) {
      case 0x41: if ((argv[0]<world->mapw)&&(argv[1]<world->maph)) {
          int p=argv[1]*world->mapw+argv[0];
          if (g.flags[argv[2]>>3]&(1<<(argv[2]&7))) {
            world->map[p]=world->virginmap[p]+1;
          } else {
            world->map[p]=world->virginmap[p];
          }
        } break;
    }
  }
}
