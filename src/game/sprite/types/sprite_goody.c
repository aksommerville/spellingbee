/* sprite_goody.c
 * Located near the spawn point. Tells you about the next step, etc.
 */
 
#include "game/bee.h"
#include "game/flag_names.h"

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
  if (!flag_get(FLAG_goody_hello)) {
    flag_set(FLAG_goody_hello,1);
    strix=47;
  } else if (!flag_get(FLAG_book1)) strix=38;
  else if (!flag_get(FLAG_book2)) strix=39;
  else if (!flag_get(FLAG_book3)) strix=40;
  else if (!flag_get(FLAG_book4)) strix=41;
  else if (!flag_get(FLAG_book5)) strix=42;
  else if (!flag_get(FLAG_book6)) strix=43;
  else if (!flag_get(FLAG_flower_done)) strix=44;
  else if (!flag_get(FLAG_graverob5)) strix=45;
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
