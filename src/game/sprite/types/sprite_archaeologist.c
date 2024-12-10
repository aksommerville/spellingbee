/* sprite_archaeologist.c
 * Manages the graverobbing treasure hunt.
 * No treasure exists until you talk to the archaeologist.
 */
 
#include "game/bee.h"
#include "game/shared_symbols.h"

struct sprite_archa {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_archa*)sprite)

static void _archa_del(struct sprite *sprite) {
}

static int _archa_init(struct sprite *sprite) {
  return 0;
}

// Select a grave command from map:cemetery. Doesn't touch global state.
static int archa_select_grave(struct sprite *sprite,int nextflag) {
  
  /* Acquire the cemetery map and validate lightly.
   */
  const uint8_t *serial=0;
  int serialc=rom_get_res(&serial,EGG_TID_map,RID_map_cemetery);
  struct rom_map rmap;
  if (rom_map_decode(&rmap,serial,serialc)<0) {
    fprintf(stderr,"map:%d(cemetery) not found or invalid! Can't generate graverobbing clues.\n",RID_map_cemetery);
    return 0;
  }
  
  /* Count "grave" commands.
   */
  struct rom_command_reader reader={.v=rmap.cmdv,.c=rmap.cmdc};
  struct rom_command cmd;
  int gravec=0;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode!=0xc0) continue;
    gravec++;
  }
  if (!gravec) {
    fprintf(stderr,"No graves in map:%d(cemetery)!\n",RID_map_cemetery);
    return 0;
  }
  
  /* Commands are ordered LRTB. Those at higher index are much more reachable than lower.
   * So we want the 5 treasures to generally move backward, but don't make it too predictable.
   * Proposal: The first two are in the last half, third anywhere, and last two in the first half.
   * Actually, for the very first one, use the last quarter instead of half. That way it's definitely reachable without tunnelling.
   */
  int candidatep=0,candidatec=gravec;
  switch (nextflag) {
    case NS_flag_graverob1: candidatep=(gravec*3)>>2; break;
    case NS_flag_graverob2: candidatep=gravec>>1; break;
    case NS_flag_graverob3: break;
    case NS_flag_graverob4: candidatec=gravec>>1; break;
    case NS_flag_graverob5: candidatec=gravec>>1; break;
  }
  if (candidatec<1) candidatec=1;
  if (candidatep>gravec-candidatec) candidatec=gravec-candidatep;
  
  /* Draw a random index from among the candidates.
   * And that's it. We don't need any more detail from the command or anything.
   */
  int choice=candidatep+rand()%candidatec;
  return choice;
}

// Compose the typical message: "There's treasure in so-and-so's grave."
// Do not return >dsta.
static int archa_describe_grave(char *dst,int dsta) {
  if (dsta<1) return 0;
  if (g.stats.gravep<1) return 0;
  
  const char *fmt=0;
  int fmtc=strings_get(&fmt,RID_strings_dialogue,27);
  
  const char *src=0; // Grave's raw text: "FIRSTNAME LASTNAME\nDOB - DOD"
  int srcc=0;
  const void *serial=0;
  int serialc=rom_get_res(&serial,EGG_TID_map,RID_map_cemetery);
  struct rom_map rmap;
  if (rom_map_decode(&rmap,serial,serialc)<0) return 0;
  struct rom_command_reader reader={.v=rmap.cmdv,.c=rmap.cmdc};
  struct rom_command cmd;
  int i=g.stats.gravep;
  while (rom_command_reader_next(&cmd,&reader)>0) {
    if (cmd.opcode!=0xc0) continue;
    if (--i) continue;
    if (cmd.argc<2) return 0;
    src=(char*)cmd.argv+2;
    srcc=cmd.argc-2;
    break;
  }
  if (!srcc) return 0;
  
  const char *name=src,*dates=0;
  int namec=0,datesc=0;
  for (i=0;i<srcc;i++) {
    if (src[i]==0x0a) {
      namec=i;
      dates=src+i+1;
      datesc=srcc-i-1;
      break;
    }
  }
  
  int dstc=0;
  for (i=0;i<fmtc;i++) {
    if (fmt[i]=='%') {
      if (dstc>dsta-namec) return 0;
      memcpy(dst+dstc,name,namec);
      dstc+=namec;
      if (dstc>dsta-2) return 0;
      memcpy(dst+dstc," (",2);
      dstc+=2;
      if (dstc>dsta-datesc) return 0;
      memcpy(dst+dstc,dates,datesc);
      dstc+=datesc;
      if (dstc>=dsta) return 0;
      dst[dstc++]=')';
    } else {
      if (dstc>=dsta) return 0;
      dst[dstc++]=fmt[i];
    }
  }
  
  return dstc;
}

static void _archa_bump(struct sprite *sprite) {
  
  int nextflag=0; // 0 if all the treasure is got.
       if (!flag_get(NS_flag_graverob1)) nextflag=NS_flag_graverob1;
  else if (!flag_get(NS_flag_graverob2)) nextflag=NS_flag_graverob2;
  else if (!flag_get(NS_flag_graverob3)) nextflag=NS_flag_graverob3;
  else if (!flag_get(NS_flag_graverob4)) nextflag=NS_flag_graverob4;
  else if (!flag_get(NS_flag_graverob5)) nextflag=NS_flag_graverob5;
  
  // If you got all the treasure, there's a congrats and nothing more.
  if (!nextflag) {
    modal_message_begin_single(RID_strings_dialogue,28);
    return;
  }
  
  // If there isn't a treasure index assigned yet, select one.
  // And if graverob1 is unset, deliver the introductory message.
  if (!g.stats.gravep) {
    if (!(g.stats.gravep=archa_select_grave(sprite,nextflag))) return;
    save_game();
    if (nextflag==NS_flag_graverob1) {
      modal_message_begin_single(RID_strings_dialogue,26);
      return;
    }
  }
  
  // Compose and deliver the message.
  char msg[256];
  int msgc=archa_describe_grave(msg,sizeof(msg));
  if (msgc<1) return;
  modal_message_begin_raw(msg,msgc);
}

const struct sprite_type sprite_type_archaeologist={
  .name="archaeologist",
  .objlen=sizeof(struct sprite_archa),
  .del=_archa_del,
  .init=_archa_init,
  .bump=_archa_bump,
};
