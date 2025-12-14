/* sprite_mrclean.c
 * Visible if lab and cellar are both completed, and our challenge hasn't been completed.
 * Prompts you to close all the lab doors and turn off all the cellar lights.
 */
 
#include "game/bee.h"
#include "game/shared_symbols.h"

struct sprite_mrclean {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_mrclean*)sprite)

static int _mrclean_init(struct sprite *sprite) {
  if (!flag_get(NS_flag_book2)) return -1; // Must finish lab.
  if (!flag_get(NS_flag_book5)) return -1; // Must finish cellar.
  if (flag_get(NS_flag_mrclean)) return -1; // Already finished my challenge.
  return 0;
}

static int mrclean_finished() {
  const int zero[]={
    NS_flag_lights1,
    NS_flag_lights2,
    NS_flag_lights3,
    NS_flag_lights4,
    NS_flag_lablock1,
    NS_flag_lablock2,
    NS_flag_lablock3,
    NS_flag_lablock4,
    NS_flag_lablock5,
    NS_flag_lablock6,
    NS_flag_lablock7,
    NS_flag_lablock8, // Supposed to be open, not closed, but the flag is still supposed to be zero.
  0};
  const int *v;
  for (v=zero;*v;v++) if (flag_get(*v)) return 0;
  //for (v=one;*v;v++) if (!flag_get(*v)) return 0;
  return 1;
}

// If we're not finished, describe the situation.
static int mrclean_compose_message(char *dst,int dsta) {
  #define LITERAL(src) { \
    const char *_src=src; \
    int srcc=0; while (_src[srcc]) srcc++; \
    if (dstc>dsta-srcc) return 0; \
    memcpy(dst+dstc,_src,srcc); \
    dstc+=srcc; \
  }
  #define DECUINT(v) { \
    if (dstc>dsta-3) return 0; \
    int _v=v; \
    if (_v>=100) dst[dstc++]='0'+(_v/100)%10; \
    if (_v>=10) dst[dstc++]='0'+(_v/10)%10; \
    dst[dstc++]='0'+_v%10; \
  }
  int dstc=0;
  LITERAL("You haven't cleaned up after yourself, Dot!\n")
  
  int lightc=0;
  if (flag_get(NS_flag_lights1)) lightc++;
  if (flag_get(NS_flag_lights2)) lightc++;
  if (flag_get(NS_flag_lights3)) lightc++;
  if (flag_get(NS_flag_lights4)) lightc++;
  if (lightc) {
    LITERAL("You left ")
    DECUINT(lightc)
    LITERAL(" light")
    if (lightc!=1) LITERAL("s")
    LITERAL(" on in the cellar.\n")
  }
  
  int doorc=0;
  if (flag_get(NS_flag_lablock1)) doorc++;
  if (flag_get(NS_flag_lablock2)) doorc++;
  if (flag_get(NS_flag_lablock3)) doorc++;
  if (flag_get(NS_flag_lablock4)) doorc++;
  if (flag_get(NS_flag_lablock5)) doorc++;
  if (flag_get(NS_flag_lablock6)) doorc++;
  if (flag_get(NS_flag_lablock7)) doorc++;
  if (doorc) {
    LITERAL("You left ")
    DECUINT(doorc)
    LITERAL(" door")
    if (doorc!=1) LITERAL("s")
    LITERAL(" open in the lab.\n")
  }
  
  if (flag_get(NS_flag_lablock8)) {
    LITERAL("The lab door that's supposed to stay open is closed!\n")
  }
  
  while (dstc&&(dst[dstc-1]==0x0a)) dstc--;
  #undef LITERAL
  #undef DECUINT
  return dstc;
}

static void _mrclean_bump(struct sprite *sprite) {
  if (flag_get(NS_flag_mrclean)) {
    modal_message_begin_single(RID_strings_dialogue,65);
  } else if (mrclean_finished()) {
    modal_message_begin_single(RID_strings_dialogue,66);
    if ((g.stats.gold+=100)>32767) g.stats.gold=32767;
    sb_sound(RID_sound_getpaid);
    g.world.status_bar_dirty=1;
    flag_set(NS_flag_mrclean,1);
    save_game();
    sprite->tileid=0x5f;
  } else {
    char msg[256];
    int msgc=mrclean_compose_message(msg,sizeof(msg));
    modal_message_begin_raw(msg,msgc);
  }
}

const struct sprite_type sprite_type_mrclean={
  .name="mrclean",
  .objlen=sizeof(struct sprite_mrclean),
  .init=_mrclean_init,
  .bump=_mrclean_bump,
};
