/* sprite_karate.c
 * The little karate students in the gym.
 * All karates in one scene coordinate with each other.
 */
 
#include "game/bee.h"
#include "game/battle/dict.h"

#define KARATE_ATTACK_TIME 0.500
#define KARATE_IDLE_TIME 1.000
#define KARATE_INITIAL_DELAY 1.500

/* Object definition.
 */
 
struct sprite_karate {
  struct sprite hdr;
  int principal; // One karate at a time is the principal, and it does most of the work.
  uint8_t tileid0;
  double clock;
  int attacker; // Which karate, counting backward thru GRP(KEEPALIVE), is currently attacking? If <0, nobody is.
  int otherc;
  int texid; // Text in word bubble.
  int texw,texh;
};

#define SPRITE ((struct sprite_karate*)sprite)
#define OTHER ((struct sprite_karate*)other)

/* Cleanup.
 */
 
static void _karate_del(struct sprite *sprite) {
  egg_texture_del(SPRITE->texid);
}

/* Init.
 */
 
static int _karate_init(struct sprite *sprite) {
  SPRITE->attacker=-1;
  SPRITE->clock=KARATE_INITIAL_DELAY;
  
  int otherc=0;
  int i=GRP(KEEPALIVE)->spritec;
  while (i-->0) {
    const struct sprite *other=GRP(KEEPALIVE)->spritev[i];
    if (other==sprite) continue;
    if (other->type==&sprite_type_karate) {
      otherc++;
    }
  }
  if (!otherc) {
    SPRITE->principal=1;
  } else {
    sprite->xform=(otherc&1)?EGG_XFORM_XREV:0;
    sprite->tileid+=(otherc&7)*2;
  }
  SPRITE->tileid0=sprite->tileid;
  
  return 0;
}

/* Pick one of the karate sprites at random, update local state, and return it.
 */
 
static struct sprite *karate_get_next_attacker(struct sprite *sprite) {
  SPRITE->attacker=-1;
  
  // Count the karate sprites, including myself. This is done lazy at the first update.
  if (SPRITE->otherc<1) {
    SPRITE->otherc=0;
    int i=GRP(KEEPALIVE)->spritec;
    while (i-->0) {
      struct sprite *other=GRP(KEEPALIVE)->spritev[i];
      if (other->type!=&sprite_type_karate) continue;
      SPRITE->otherc++;
    }
    if (SPRITE->otherc<1) return 0; // huh?
  }
  
  // Pick one. I think random is better than sequential.
  int p=rand()%SPRITE->otherc;
  int i=GRP(KEEPALIVE)->spritec;
  while (i-->0) {
    struct sprite *other=GRP(KEEPALIVE)->spritev[i];
    if (other->type!=&sprite_type_karate) continue;
    SPRITE->attacker++;
    if (!p--) return other;
  }
  
  // Oops.
  SPRITE->attacker=-1;
  return 0;
}

/* Current attacker.
 */
 
static struct sprite *karate_get_current_attacker(const struct sprite *sprite) {
  if (SPRITE->attacker<0) return 0;
  int p=SPRITE->attacker;
  int i=GRP(KEEPALIVE)->spritec;
  while (i-->0) {
    struct sprite *other=GRP(KEEPALIVE)->spritev[i];
    if (other->type!=&sprite_type_karate) continue;
    if (!p--) return other;
  }
  return 0;
}

/* Pick a 2-letter word and render it to SPRITE->texid.
 */
 
static void karate_refresh_word(struct sprite *sprite) {
  struct dict_bucket bucket={0};
  dict_get_bucket(&bucket,RID_dict_nwl2023,2);
  if (bucket.c<1) return;
  int p=rand()%bucket.c;
  char word[3]={0,0,'!'};
  memcpy(word,bucket.v+p*2,2);
  egg_texture_del(SPRITE->texid);
  SPRITE->texid=font_tex_oneline(g.font,word,3,TILESIZE*2,0x000000ff);
  egg_texture_get_status(&SPRITE->texw,&SPRITE->texh,SPRITE->texid);
}

/* Update.
 */
 
static void _karate_update(struct sprite *sprite,double elapsed) {
  if (!SPRITE->principal) return;
  if ((SPRITE->clock-=elapsed)>0.0) return;
  
  // Pick somebody to start attacking.
  if (SPRITE->attacker<0) {
    SPRITE->clock+=KARATE_ATTACK_TIME;
    struct sprite *other=karate_get_next_attacker(sprite);
    if (!other) return; // Should never happen; we are a karate too.
    OTHER->hdr.tileid=OTHER->tileid0+1;
    karate_refresh_word(sprite);
    return;
  }
  
  // End the current attack.
  SPRITE->clock+=KARATE_IDLE_TIME;
  struct sprite *other=karate_get_current_attacker(sprite);
  if (!other) return;
  OTHER->hdr.tileid=OTHER->tileid0;
  SPRITE->attacker=-1;
}

/* Bump.
 */
 
static void _karate_bump(struct sprite *sprite) {
  int index=20+rand()%5;
  modal_message_begin_single(RID_strings_dialogue,index);
}

/* Render word bubble.
 * Regular rendering uses the built-in facilities.
 */
 
static void _karate_render_post(struct sprite *sprite,int16_t addx,int16_t addy) {
  if (!SPRITE->principal) return;
  struct sprite *other=karate_get_current_attacker(sprite);
  if (!other) return;
  int texid=texcache_get_image(&g.texcache,sprite->imageid);
  int16_t dstx=(int16_t)(other->x*TILESIZE)+addx;
  int16_t dsty=(int16_t)(other->y*TILESIZE)+addy;
  graf_draw_tile(&g.graf,texid,dstx-(TILESIZE>>1),dsty-TILESIZE,SPRITE->tileid0+0x10,0);
  graf_draw_tile(&g.graf,texid,dstx+(TILESIZE>>1),dsty-TILESIZE,SPRITE->tileid0+0x11,0);
  graf_draw_decal(&g.graf,SPRITE->texid,dstx-(SPRITE->texw>>1),dsty-TILESIZE-(SPRITE->texh>>1)-1,0,0,SPRITE->texw,SPRITE->texh,0);
}

/* Type definition.
 */
 
const struct sprite_type sprite_type_karate={
  .name="karate",
  .objlen=sizeof(struct sprite_karate),
  .del=_karate_del,
  .init=_karate_init,
  .update=_karate_update,
  .render_post=_karate_render_post,
  .bump=_karate_bump,
};
