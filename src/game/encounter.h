/* encounter.h
 * Abstraction of one spelling encounter.
 * Everything related to the actual spelling game lives under here.
 */
 
#ifndef ENCOUNTER_H
#define ENCOUNTER_H

#include "letterbag.h"
#include "foe.h"

struct encounter {
  int active;
  struct letterbag letterbag;
  struct {
    double animclock;
    int animframe;
    int x,y;
  } cursor;
  char stage[7];
  char hand[7];
  struct foe foe;
  int score;//XXX There won't be "score" per se; there's my hp and his.
  int recent_score; // From the last play.
  int wildcard_modal;
  int wildcard_handp;
  int wcx,wcy;
};

void encounter_begin(struct encounter *en);
void encounter_update(struct encounter *en,double elapsed);
void encounter_move(struct encounter *en,int dx,int dy);
void encounter_activate(struct encounter *en);
void encounter_render(struct encounter *en);

#endif
