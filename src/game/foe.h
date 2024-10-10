/* foe.h
 * The computer-controlled opponent in one encounter.
 */
 
#ifndef FOE_H
#define FOE_H

struct foe {
  struct encounter *en;
  char hand[7];
  int searchlen; // 2..7
  int searchp; // 0..6
  const char *word; // Best found so far.
  int wordc;
  int wordscore;
  double searchclock;
  char lastplay[7];
  int lastscore;
};

void foe_reset(struct foe *foe,struct encounter *en);
void foe_update(struct foe *foe,double elapsed);
void foe_play(struct foe *foe);

#endif
