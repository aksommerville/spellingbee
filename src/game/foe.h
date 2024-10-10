/* foe.h
 * The computer-controlled opponent in one encounter.
 */
 
#ifndef FOE_H
#define FOE_H

struct foe {
  struct encounter *en;
  char hand[7];
  /*XXX old coarse-grained search
  int searchlen; // 2..7
  int searchp; // 0..6
  const char *word; // Best found so far.
  int wordc;
  int wordscore;
  double searchclock;
  char lastplay[7];
  int lastscore;
  /**/
  
  // Progress in the current search, for reporting. (searchc) can be zero.
  int searchp,searchc;
  
  int search_len;
  int search_handp;
  const char *bucket;
  int bucketc; // words
  int bucketp; // words
  double searchclock;
  char bestword[7];
  int bestscore;
};

void foe_reset(struct foe *foe,struct encounter *en);
void foe_update(struct foe *foe,double elapsed);
void foe_play(struct foe *foe);

#endif
