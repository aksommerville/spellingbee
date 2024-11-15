/* modal_type_battle.c
 * Our duty is to wrap battles in a generic modal, to present a uniform interface to main.
 * We don't get involved in the business logic of an battle.
 */

#include "game/bee.h"
#include "game/battle/battle.h"

struct modal_battle {
  struct modal hdr;
  struct battle battle;
  // We need to distinguish player 1 and player 2 inputs.
  // The game otherwise operates against player 0, the aggregate.
  int p1pv,p2pv;
  int started;
};

#define MODAL ((struct modal_battle*)modal)

/* Delete.
 */
 
static void _battle_del(struct modal *modal) {
  if (MODAL->started) battle_commit_to_globals(&MODAL->battle);
  battle_cleanup(&MODAL->battle);
}

/* Init.
 */
 
static int _battle_init(struct modal *modal) {
  modal->opaque=1;
  battle_init(&MODAL->battle);
  return 0;
}

/* Update.
 */
 
static void _battle_update(struct modal *modal,double elapsed) {
  if (battle_update(&MODAL->battle,elapsed)<=0) {
    modal_pop(modal);
    return;
  }
  // Most modal will use the 'input' hook. But we alone have to distinguish player 1 from 2.
  int p1=egg_input_get_one(1);
  int p2=egg_input_get_one(2);
  if (p1!=MODAL->p1pv) {
    #define BTN(tag) (p1&EGG_BTN_##tag)&&!(MODAL->p1pv&EGG_BTN_##tag)
    if (BTN(LEFT)) battle_move(&MODAL->battle,1,-1,0);
    if (BTN(RIGHT)) battle_move(&MODAL->battle,1,1,0);
    if (BTN(UP)) battle_move(&MODAL->battle,1,0,-1);
    if (BTN(DOWN)) battle_move(&MODAL->battle,1,0,1);
    if (BTN(SOUTH)) battle_activate(&MODAL->battle,1);
    if (BTN(WEST)) battle_cancel(&MODAL->battle,1);
    if (BTN(AUX1)) { modal_pop(modal); return; }//XXX Start to return to world, debug only.
    #undef BTN
    MODAL->p1pv=p1;
  }
  if (p2!=MODAL->p2pv) {
    #define BTN(tag) (p2&EGG_BTN_##tag)&&!(MODAL->p2pv&EGG_BTN_##tag)
    if (BTN(LEFT)) battle_move(&MODAL->battle,2,-1,0);
    if (BTN(RIGHT)) battle_move(&MODAL->battle,2,1,0);
    if (BTN(UP)) battle_move(&MODAL->battle,2,0,-1);
    if (BTN(DOWN)) battle_move(&MODAL->battle,2,0,1);
    if (BTN(SOUTH)) battle_activate(&MODAL->battle,2);
    if (BTN(WEST)) battle_cancel(&MODAL->battle,2);
    if (BTN(AUX1)) { modal_pop(modal); return; }//XXX Start to return to world, debug only.
    #undef BTN
    MODAL->p2pv=p2;
  }
}

/* Render.
 */
 
static void _battle_render(struct modal *modal) {
  battle_render(&MODAL->battle);
}

/* Type definition.
 */
 
const struct modal_type modal_type_battle={
  .name="battle",
  .objlen=sizeof(struct modal_battle),
  .del=_battle_del,
  .init=_battle_init,
  .update=_battle_update,
  .render=_battle_render,
};

/* Initialize battle and push a new modal for it.
 */
 
struct battle *modal_battle_begin(int rid) {
  const void *serial=0;
  int serialc=rom_get_res(&serial,EGG_TID_battle,rid);
  if (serialc<1) {
    fprintf(stderr,"battle:%d not found\n",rid);
    return 0;
  }
  struct modal *modal=modal_spawn(&modal_type_battle);
  if (!modal) return 0;
  if (battle_load(&MODAL->battle,serial,serialc,rid)<0) {
    fprintf(stderr,"battle:%d failed to load\n",rid);
    modal_pop(modal);
    return 0;
  }
  MODAL->started=1;
  return &MODAL->battle;
}
