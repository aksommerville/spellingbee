/* foe.h
 * The computer-controlled opponent in one encounter.
 */
 
#ifndef FOE_H
#define FOE_H

#define FOE_RULES_NORMAL 0
#define FOE_RULES_SIXCLOPS 1
#define FOE_RULES_IBALL 2

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
  double holdclock;
  
  int hp;
  
  int face_srcx,face_srcy;
  int rules;
};

void foe_reset(struct foe *foe,struct encounter *en,int rules);
void foe_update(struct foe *foe,double elapsed);
void foe_play(struct foe *foe);

#endif
