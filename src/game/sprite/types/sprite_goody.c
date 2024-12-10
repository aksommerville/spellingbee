/* sprite_goody.c
 * Located near the spawn point. Tells you about the next step, etc.
 */
 
#include "game/bee.h"
#include "game/shared_symbols.h"

struct sprite_goody {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_goody*)sprite)

static void _goody_del(struct sprite *sprite) {
}

static int _goody_init(struct sprite *sprite) {
  return 0;
}

static void _goody_bump(struct sprite *sprite) {
  int strix=0;
  
  // Before anything else, say hello. Would be rude not to.
  if (!flag_get(NS_flag_goody_hello)) {
    flag_set(NS_flag_goody_hello,1);
    strix=47;
  }
  
  // Books are numbered but meaninglessly. Report them roughly by difficulty or convenience.
  else if (!flag_get(NS_flag_book4)) strix=41; // garden
  else if (!flag_get(NS_flag_book2)) strix=39; // lab
  else if (!flag_get(NS_flag_book3)) strix=40; // gym
  else if (!flag_get(NS_flag_book6)) strix=43; // queen
  else if (!flag_get(NS_flag_book5)) strix=42; // cellar
  else if (!flag_get(NS_flag_book1)) strix=38; // cemetery
  
  // Side quests. Again, no particular order necessary. I'm aiming for short-to-long.
  else if (!flag_get(NS_flag_blackbelt)) strix=63;
  else if (!flag_get(NS_flag_mrclean)) strix=64;
  else if (!flag_get(NS_flag_flower_done)) strix=44;
  else if (!flag_get(NS_flag_englishprof)) strix=55;
  else if (!flag_get(NS_flag_graverob5)) strix=45;
  
  // And finally there's a "you're done!" message.
  else strix=46;
  
  modal_message_begin_single(RID_strings_dialogue,strix);
}

const struct sprite_type sprite_type_goody={
  .name="goody",
  .objlen=sizeof(struct sprite_goody),
  .del=_goody_del,
  .init=_goody_init,
  .bump=_goody_bump,
};
