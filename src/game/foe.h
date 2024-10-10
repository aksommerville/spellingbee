/* foe.h
 * The computer-controlled opponent in one encounter.
 */
 
#ifndef FOE_H
#define FOE_H

struct foe {
  struct encounter *en;
  char hand[7];
  
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
  
  int hp;
};

void foe_reset(struct foe *foe,struct encounter *en);
void foe_update(struct foe *foe,double elapsed);
void foe_play(struct foe *foe);

#endif
