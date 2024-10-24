#include "bee.h"

// Extra delay at the start of each length bucket.
// Helps to keep the short lengths playable, otherwise like the entire search time is spent looking at 7-letter candidates.
#define FOE_HOLD_TIME 2.000

/* Call after changing (search_len) or (search_handp).
 * Advances those if necessary to the next valid combination, fetches the bucket, and positions us within the bucket.
 * Hand must be shuffled left -- all zeroes on the right.
 */
 
static void foe_prepare_search(struct foe *foe) {
  for (;;) {
    if (foe->search_len>7) {
      foe->searchp=foe->searchc;
      foe->bucketp=foe->bucketc;
      return;
    }
    if ((foe->search_handp>=7)||!foe->hand[foe->search_handp]) {
      foe->search_len++;
      foe->search_handp=0;
      foe->holdclock=FOE_HOLD_TIME;
      if (foe->search_len>7) {
        fprintf(stderr,"foe search exhausted\n");
        return;
      }
    } else if (foe->hand[foe->search_handp]=='@') {
      foe->search_handp++;
    } else {
      break;
    }
  }
  char fakeword[7]={foe->hand[foe->search_handp],' '};
  foe->bucketc=get_dictionary_bucket(&foe->bucket,foe->search_len);
  foe->bucketp=search_bucket(foe->bucket,foe->bucketc,fakeword,foe->search_len);
  if (foe->bucketp<0) foe->bucketp=-foe->bucketp-1;
}

/* Fill my hand, and restart the search.
 */
 
static void foe_draw_and_restart_search(struct foe *foe) {
  letterbag_draw_partial(foe->hand,&foe->en->letterbag);
  int handsize=7;
  int i=7; while (i-->0) if (!foe->hand[i]) { // Move zeroes to the right.
    memmove(foe->hand+i,foe->hand+i+1,6-i);
    foe->hand[6]=0;
    handsize--;
  }
  //fprintf(stderr,"%s hand='%.7s'\n",__func__,foe->hand);
  
  /* Determine the size of the search space.
   * We're going to look at all words 2..(handsize) starting with each of my real letters.
   * We never play wildcards in the first position, because that complicates searching.
   * If there are duplicates in the hand, dumbly check that letter twice.
   * It's possible to end up with zero candidates, regardless of hand size.
   */
  foe->searchp=0;
  foe->searchc=0;
  int len=2; for (;len<=handsize;len++) {
    char fakeword[7];
    const char *bucket=0;
    int bucketc=get_dictionary_bucket(&bucket,len);
    int i=handsize; while (i-->0) {
      if (foe->hand[i]=='@') continue;
      fakeword[0]=foe->hand[i];
      fakeword[1]=' ';
      int startp=search_bucket(bucket,bucketc,fakeword,len);
      if (startp<0) startp=-startp-1;
      fakeword[1]='[';
      int stopp=search_bucket(bucket,bucketc,fakeword,len);
      if (stopp<0) stopp=-stopp-1;
      if (startp>stopp) continue; // huh?
      //fprintf(stderr,"...%d of length %d starting with '%c'...\n",stopp-startp,len,foe->hand[i]);
      foe->searchc+=stopp-startp;
    }
  }
  //fprintf(stderr,"...search space %d\n",foe->searchc);
  
  foe->bestscore=0;
  memset(foe->bestword,0,7);
  foe->searchclock=0.0;
  foe->search_len=2;
  foe->search_handp=0;
  foe->holdclock=FOE_HOLD_TIME;
  foe_prepare_search(foe);
}

/* Reset.
 */
 
void foe_reset(struct foe *foe,struct encounter *en,int rules) {
  memset(foe,0,sizeof(struct foe));
  foe->en=en;
  foe->rules=rules;
  
  if (!foe->rules) {//XXX very temporary! the decision making should be higher up
    int choice=rand()%10;
    if (choice<=-1) foe->rules=FOE_RULES_SIXCLOPS;
    else if (choice<=3) foe->rules=FOE_RULES_IBALL;
    else foe->rules=FOE_RULES_NORMAL;
  }
  switch (foe->rules) {//TODO figure out how to communicate all this stuff. it should be just a single int coming in
    case FOE_RULES_SIXCLOPS: {
        foe->hp=15;
        foe->face_srcx=TILESIZE*6;
        foe->face_srcy=TILESIZE*8;
      } break;
    case FOE_RULES_IBALL: {
        foe->hp=25;
        foe->face_srcx=TILESIZE*9;
        foe->face_srcy=TILESIZE*8;
      } break;
    case FOE_RULES_NORMAL: default: {
        foe->hp=20;
        foe->face_srcx=TILESIZE*3;
        foe->face_srcy=TILESIZE*8;
      } break;
  }
  
  foe_draw_and_restart_search(foe);
}

/* If our hand can spell this word, return nonzero and rewrite it in (dst).
 * Rewrite with lowercase letters for wildcards if applicable.
 */
 
static int foe_can_spell_word(char *dst,struct foe *foe,const char *q,int qc) {
  char remaining[7];
  memcpy(remaining,foe->hand,7);
  int dstc=0,qi=0;
  for (;qi<qc;qi++) {
    char qletter=q[qi];
    int ok=0,wcp=-1;
    int ri=7; while (ri-->0) {
      if (remaining[ri]==qletter) {
        memmove(remaining+ri,remaining+ri+1,6-ri);
        remaining[6]=0;
        ok=1;
        break;
      } else if (remaining[ri]=='@') {
        wcp=ri;
      }
    }
    if (!ok&&(wcp>=0)) {
      memmove(remaining+wcp,remaining+wcp+1,6-wcp);
      remaining[6]=0;
      dst[dstc++]=qletter+0x20;
    } else if (ok) {
      dst[dstc++]=qletter;
    } else {
      return 0;
    }
  }
  return 1;
}

/* Advance search.
 */
 
static void foe_advance_search(struct foe *foe) {
  for (;;) {
    if (foe->searchp>=foe->searchc) return;
    if (foe->search_len>7) return;
    if (foe->bucketp<foe->bucketc) break;
    foe->search_handp++;
    foe_prepare_search(foe);
  }
  int stride=foe->search_len+1;
  const char *candidate=foe->bucket+foe->bucketp*stride;
  if (candidate[0]!=foe->hand[foe->search_handp]) {
    foe->search_handp++;
    foe_prepare_search(foe);
    return;
  }
  foe->bucketp++;
  foe->searchp++;
  char tmp[7]={0};
  if (foe_can_spell_word(tmp,foe,candidate,foe->search_len)) {
    int score=rate_word(tmp,foe->search_len,0);
    if (score>foe->bestscore) {
      //fprintf(stderr,"Foe candidate '%.*s' score=%d\n",foe->search_len,tmp,score);
      memcpy(foe->bestword,tmp,7);
      foe->bestscore=score;
      foe->searchclock-=0.250; // Experiment: Let the main interval be really fast, but apply a penalty at each real word found.
    }
  }
}

/* Update.
 */
 
void foe_update(struct foe *foe,double elapsed) {
  if (foe->holdclock>0.0) {
    if (foe->holdclock>=elapsed) {
      foe->holdclock-=elapsed;
      return;
    }
    elapsed-=foe->holdclock;
    foe->holdclock=0.0;
  }
  foe->searchclock+=elapsed;
  /* In a quick test, seems that 10k-20k is typical for a search space.
   * We impose an artificial delay (FOE_HOLD_TIME) at the start of each bucket.
   * Then at each word we find, impose a short penalty (0.250, see above).
   * Those account for most of the timing regulation.
   * Beyond that, a miniscule cost, 200 us, for each word examined.
   * If that small interval is too high, he spins for a long time reviewing non-matching 7-letter words.
   * With the current arrangement, he tends to exhausted the search space in about 15 seconds,
   * and the damage he does tends to grow more or less uniformly.
   * So it meets my main goal: Players that rush their next word get a bonus for it.
   */
  if (foe->searchp<foe->searchc) {
    double interval=0.0002;
    while (foe->searchclock>=interval) {
      foe->searchclock-=interval;
      foe_advance_search(foe);
    }
  }
}

/* Remove a word from the foe's hand, be mindful of wildcards.
 * Incoming word will be lowercase where a wildcard was played.
 */
 
static void foe_remove_word_from_hand(struct foe *foe,const char *src) {
  int i=7; while (i-->0) {
    if (!src[i]) continue;
    if ((src[i]>='a')&&(src[i]<='z')) {
      int ii=7; while (ii-->0) {
        if (foe->hand[ii]=='@') {
          foe->hand[ii]=0;
          break;
        }
      }
    } else {
      int ii=7; while (ii-->0) {
        if (foe->hand[ii]==src[i]) {
          foe->hand[ii]=0;
          break;
        }
      }
    }
  }
}

/* Play.
 */
 
void foe_play(struct foe *foe) {
  if (!foe->bestword[0]) {
    //fprintf(stderr,"FOE FOLDS\n");
    memset(foe->hand,0,7);
    foe->bestscore=0;
  } else {
    //fprintf(stderr,"FOE PLAYS '%.7s' FOR %d POINTS\n",foe->bestword,foe->bestscore);
    g.hp-=foe->bestscore;
  }
  memcpy(foe->en->inplay,foe->bestword,7);
  foe->en->inplay[7]=0;
  foe->en->efficacy=foe->bestscore;
  foe_remove_word_from_hand(foe,foe->bestword);
  foe_draw_and_restart_search(foe);
}
