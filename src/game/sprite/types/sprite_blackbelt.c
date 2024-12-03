/* sprite_blackbelt.c
 * Orchestrates the Black Belt Challenge: Beat a string of skeletons without taking any damage.
 */
 
#include "game/bee.h"
#include "game/flag_names.h"
#include "game/battle/battle.h"

#define BATTLE_COUNT 10

struct sprite_blackbelt {
  struct sprite hdr;
  int progress;
};

#define SPRITE ((struct sprite_blackbelt*)sprite)

static int _blackbelt_init(struct sprite *sprite) {
  return 0;
}

static void blackbelt_cb_finish(struct battle *battle,void *userdata) {
  struct sprite *sprite=userdata;
  if (battle->p2.hp>0) { // Skeleton wins.
    TRACE("lose")
    SPRITE->progress=0;
  } else { // Dot wins.
    TRACE("win")
    SPRITE->progress++;
    if (SPRITE->progress>=BATTLE_COUNT) {
      if (flag_get(FLAG_blackbelt)) {
        modal_message_begin_single(RID_strings_dialogue,62);
      } else {
        modal_message_begin_single(RID_strings_dialogue,61);
        if ((g.stats.gold+=200)>=32767) g.stats.gold=32767;
        g.world.status_bar_dirty=1;
        egg_play_sound(RID_sound_getpaid);
        flag_set(FLAG_blackbelt,1);
        save_game();
      }
      SPRITE->progress=0;
    } else {
      struct battle *battle=modal_battle_begin(RID_battle_blackbelt);
      if (!battle) return;
      battle_set_caption(battle,"Black Belt",SPRITE->progress+1,BATTLE_COUNT);
      battle_on_finish(battle,blackbelt_cb_finish,sprite);
    }
  }
}

static void blackbelt_cb_begin(int choice,void *userdata) {
  struct sprite *sprite=userdata;
  if (choice!=1) {
    TRACE("decline")
    return;
  }
  TRACE("begin")
  SPRITE->progress=0;
  struct battle *battle=modal_battle_begin(RID_battle_blackbelt);
  if (!battle) return;
  battle_set_caption(battle,"Black Belt",SPRITE->progress+1,BATTLE_COUNT);
  battle_on_finish(battle,blackbelt_cb_finish,sprite);
}

static int blackbelt_compose_prompt(char *dst,int dsta,struct sprite *sprite) {
  // Leaving the door open for formatting eg "Your last score was %..." but we're not doing that today.
  int promptix=flag_get(FLAG_blackbelt)?60:59;
  const char *src=0;
  int srcc=strings_get(&src,RID_strings_dialogue,promptix);
  if ((srcc<0)||(srcc>dsta)) return 0;
  memcpy(dst,src,srcc);
  return srcc;
}

static void _blackbelt_bump(struct sprite *sprite) {
  
  // Must complete the regular Endurance Gauntlet first.
  if (!flag_get(FLAG_book3)) {
    TRACE("not qualified")
    modal_message_begin_single(RID_strings_dialogue,58);
    return;
  }
  
  TRACE("")
  char prompt[256];
  int promptc=blackbelt_compose_prompt(prompt,sizeof(prompt),sprite);
  if ((promptc<=0)||(promptc>sizeof(prompt))) return;
  modal_prompt(prompt,promptc,blackbelt_cb_begin,sprite,"No","Yes");
}

const struct sprite_type sprite_type_blackbelt={
  .name="blackbelt",
  .objlen=sizeof(struct sprite_blackbelt),
  .init=_blackbelt_init,
  .bump=_blackbelt_bump,
};
