/* encounter.h
 * Abstraction of one spelling encounter.
 * Everything related to the actual spelling game lives under here.
 */
 
#ifndef ENCOUNTER_H
#define ENCOUNTER_H

#include "letterbag.h"
#include "foe.h"

#define ENCOUNTER_PHASE_GATHER  1 /* Collecting a word from the player, charging foe. */
#define ENCOUNTER_PHASE_PLAY    2 /* Hero's word attacks the foe. */
#define ENCOUNTER_PHASE_REACT   3 /* Foe's word attacks the hero. */
#define ENCOUNTER_PHASE_WIN     4 /* Report victory. */
#define ENCOUNTER_PHASE_LOSE    5 /* Report failure. */
#define ENCOUNTER_PHASE_WELCOME 6 /* Initial phase: "A foe draws near!" */

struct encounter {
  int active;
  int phase;
  double phaset; // 0=>1
  double phaserate; // t/sec
  struct letterbag letterbag;
  struct {
    double animclock;
    int animframe;
    int x,y;
  } cursor;
  char stage[7];
  char hand[7];
  char inplay[7]; // Hero or foe's word currently displayed travelling.
  int efficacy; // Word's score, during PLAY or REACT. Can be negative in PLAY, and can be zero.
  struct foe foe;
  int wildcard_modal;
  int wildcard_handp;
  int wcx,wcy;
  int display_foe_hp; // What we show on screen lags behind the true model, and never goes negative.
  int display_hero_hp;
  double display_hp_clock;
};

void encounter_begin(struct encounter *en);
void encounter_update(struct encounter *en,double elapsed);
void encounter_move(struct encounter *en,int dx,int dy);
void encounter_activate(struct encounter *en);
void encounter_cancel(struct encounter *en);
void encounter_render(struct encounter *en);

#endif
