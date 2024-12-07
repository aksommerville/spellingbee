/* sprite_lovers.c
 * One controller for both Romeo and Juliet, even though they're completely different.
 * 0x2f=role
 */
 
#include "game/bee.h"
#include "game/flag_names.h"

#define LOVERS_ROLE_ROMEO 1
#define LOVERS_ROLE_JULIET 2

struct sprite_lovers {
  struct sprite hdr;
  int role;
};

#define SPRITE ((struct sprite_lovers*)sprite)

static void _lovers_del(struct sprite *sprite) {
}

static int _lovers_init(struct sprite *sprite) {
  if ((sprite->defc>=4)&&!memcmp(sprite->def,"\0SPR",4)) {
    struct cmd_reader reader={.v=sprite->def+4,.c=sprite->defc-4};
    uint8_t opcode;
    const uint8_t *argv;
    int argc;
    while ((argc=cmd_reader_next(&argv,&opcode,&reader))>=0) {
      switch (opcode) {
        case 0x2f: SPRITE->role=(argv[0]<<8)|argv[1]; break;
      }
    }
  }
  return 0;
}

static void romeo_bump(struct sprite *sprite) {
  if (flag_get(FLAG_flower_done)) {
    modal_message_begin_single(RID_strings_dialogue,30); // "Thanks!"
  } else if (flag_get(FLAG_flower)) {
    modal_message_begin_single(RID_strings_dialogue,31); // "Deliver it"
  } else if (something_being_carried()) {
    modal_message_begin_single(RID_strings_dialogue,32); // "You don't have enough hands"
  } else {
    modal_message_begin_single(RID_strings_dialogue,33); // Canonical start text
    flag_set(FLAG_flower,1);
    g.stats.flower_stepc=0;
  }
}

static void juliet_bump(struct sprite *sprite) {
  if (flag_get(FLAG_flower_done)) {
    modal_message_begin_single(RID_strings_dialogue,34); // "Thanks!"
  } else if (flag_get(FLAG_flower)) {
    modal_message_begin_single(RID_strings_dialogue,35); // Canonical end text
    flag_set(FLAG_flower,0);
    flag_set(FLAG_flower_done,1);
    if ((g.stats.gold+=200)>32767) g.stats.gold=32767;
    g.world.status_bar_dirty=1;
    save_game();
    egg_play_sound(RID_sound_getpaid);
  } else {
    modal_message_begin_single(RID_strings_dialogue,36); // "Does he loves me or loves me not?"
  }
}

static void _lovers_bump(struct sprite *sprite) {
  switch (SPRITE->role) {
    case LOVERS_ROLE_ROMEO: romeo_bump(sprite); break;
    case LOVERS_ROLE_JULIET: juliet_bump(sprite); break;
  }
}

const struct sprite_type sprite_type_lovers={
  .name="lovers",
  .objlen=sizeof(struct sprite_lovers),
  .del=_lovers_del,
  .init=_lovers_init,
  .bump=_lovers_bump,
};
