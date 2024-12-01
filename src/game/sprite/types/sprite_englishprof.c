/* sprite_englishprof.c
 * Checks your best word and judges it.
 */
 
#include "game/bee.h"
#include "game/flag_names.h"

struct sprite_englishprof {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_englishprof*)sprite)

static int _englishprof_init(struct sprite *sprite) {
  return 0;
}

static int englishprof_format_string(char *dst,int dsta,const char *src,int srcc) {
  int dstc=0;
  for (;srcc-->0;src++) {
    if (*src=='%') {
      const char *word=g.stats.bestword;
      int wordc=0;
      while ((wordc<7)&&word[wordc]) wordc++;
      if (dstc>dsta-wordc) return 0;
      memcpy(dst+dstc,word,wordc);
      dstc+=wordc;
    } else if (*src=='#') {
      // 3-digit scores are rare but possible. (eg MUZJIKS with triple word is 137)
      if (dstc>dsta-3) return 0;
      if (g.stats.bestscore>=100) dst[dstc++]='0'+(g.stats.bestscore/100)%10;
      if (g.stats.bestscore>=10) dst[dstc++]='0'+(g.stats.bestscore/10)%10;
      dst[dstc++]='0'+g.stats.bestscore%10;
    } else {
      if (dstc>=dsta) return 0;
      dst[dstc++]=*src;
    }
  }
  return dstc;
}

static int englishprof_compose_message(char *dst,int dsta) {
  const char *src=0;
  int srcc=strings_get(&src,RID_strings_dialogue,51);
  int dstc=englishprof_format_string(dst,dsta,src,srcc);
  if (dstc>=dsta) return 0;
  dst[dstc++]=' ';
  int strix=0;
  if (flag_get(FLAG_englishprof)) strix=54; // Already collected.
  else if (g.stats.bestscore>=60) { // Win now.
    strix=53;
    if ((g.stats.gold+=200)>32767) g.stats.gold=32767;
    g.world.status_bar_dirty=1;
    //TODO sound effect
    flag_set(FLAG_englishprof,1);
  } else strix=52; // Insufficient.
  srcc=strings_get(&src,RID_strings_dialogue,strix);
  if (dstc>dsta-srcc) return 0;
  memcpy(dst+dstc,src,srcc);
  dstc+=srcc;
  return dstc;
}

static void _englishprof_bump(struct sprite *sprite) {

  // Zero score is a special case, and static. It's pretty hard to reach.
  if (!g.stats.bestscore) {
    modal_message_begin_single(RID_strings_dialogue,50);
    return;
  }
  
  // All others are string 51 with some insertions, concatenated with 52, 53, or 54.
  char msg[256];
  int msgc=englishprof_compose_message(msg,sizeof(msg));
  if ((msgc<1)||(msgc>sizeof(msg))) return;
  modal_message_begin_raw(msg,msgc);
}

const struct sprite_type sprite_type_englishprof={
  .name="englishprof",
  .objlen=sizeof(struct sprite_englishprof),
  .init=_englishprof_init,
  .bump=_englishprof_bump,
};
