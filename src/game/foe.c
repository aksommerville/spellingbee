#include "bee.h"

/* Reset.
 */
 
void foe_reset(struct foe *foe,struct encounter *en) {
  memset(foe,0,sizeof(struct foe));
  foe->en=en;
  foe->searchlen=2;
  foe->searchp=0;
  foe->searchclock=0.0;
  foe->word="";
  foe->wordc=0;
  foe->wordscore=0;
  letterbag_draw(foe->hand,&en->letterbag);
}

/* Advance search.
 */

static int foe_search_cb(const char *src,int srcc,void *userdata) {
  struct foe *foe=userdata;
  int score=rate_word(src,srcc);
  if (score<=foe->wordscore) return 0;
  char scratch[7];
  memcpy(scratch,foe->hand,7);
  int srcp=0; for (;srcp<srcc;srcp++) {
    int scratchp=0,ok=0,wcp=-1;
    for (;scratchp<7;scratchp++) {
      if (scratch[scratchp]==src[srcp]) {
        ok=1;
        scratch[scratchp]=0;
        break;
      } else if (scratch[scratchp]=='@') {
        wcp=scratchp;
      }
    }
    if (!ok&&(wcp>=0)) {
      ok=1;
      scratch[wcp]=0;
    }
    if (!ok) return 0;
  }
  //fprintf(stderr,">>> %.*s (%d)\n",srcc,src,score);
  //TODO The foe is getting points for his wildcards, that's not fair.
  foe->word=src;
  foe->wordc=srcc;
  foe->wordscore=score;
  return 0;
}

static void foe_search(struct foe *foe) {
  if (foe->searchp>=7) {
    foe->searchp=0;
    foe->searchlen++;
  }
  if (foe->searchlen>7) return;
  for_each_word(foe->searchlen,foe->hand[foe->searchp],foe_search_cb,foe);
  foe->searchp++;
}

/* Update.
 */
 
void foe_update(struct foe *foe,double elapsed) {
  // TODO Can we pay out the search at finer granularity?
  double interval=1.000;
  if ((foe->searchclock+=elapsed)>=interval) {
    foe->searchclock-=interval;
    foe_search(foe);
  }
}

/* Play.
 */
 
void foe_play(struct foe *foe) {
  if (!foe->wordc) {
    fprintf(stderr,"FOE FOLDS\n");
  } else {
    fprintf(stderr,"FOE PLAYS '%.*s' FOR %d POINTS\n",foe->wordc,foe->word,foe->wordscore);
    foe->en->score-=foe->wordscore;
  }
  memset(foe->lastplay,0,7);
  memcpy(foe->lastplay,foe->word,foe->wordc);
  foe->lastscore=foe->wordscore;
  letterbag_draw(foe->hand,&foe->en->letterbag);
  foe->searchclock=0.0;
  foe->searchp=0;
  foe->searchlen=2;
  foe->word="";
  foe->wordc=0;
  foe->wordscore=0;
}
