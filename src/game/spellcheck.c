#include "bee.h"

/* Private globals.
 */
 
static struct {
  int init;
  // Buckets are indexed by length. The first two will always be empty.
  struct bucket {
    const char *v; // Uppercase words, (wordlength+1) bytes apart.
    int c; // Count of words, not bytes.
  } bucketv[8];
} scg={0};

/* Initialize if we haven't yet.
 */
 
static inline void spellcheck_require_init() {
  if (scg.init) return;
  scg.init=1;
  const char *src=0;
  int srcc=0;
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)>=0) {
    struct rom_res *res;
    while (res=rom_reader_next(&reader)) {
      if (res->tid<EGG_TID_dict) continue;
      if (res->tid>EGG_TID_dict) break;
      src=res->v;
      srcc=res->c;
      break;
    }
  }
  
  /* The dictionary resource is composed of uppercase words separated by LF.
   * They are sorted by length then alphabetical, and only contain lengths 2..7.
   * This slicing could definitely be done more efficiently -- it shouldn't be necessary to scan the entire resource like we do.
   */
  int srcp=0,bucketp=2;
  scg.bucketv[bucketp].v=src;
  while (srcp<srcc) {
    if (srcp+bucketp>=srcc) {
      // Invalid! Resource has some trash at the end or doesn't end with an LF.
      break;
    } else if (src[srcp+bucketp]==0x0a) {
      // The next word is the expected length. Add to current bucket.
      scg.bucketv[bucketp].c++;
      srcp+=bucketp+1;
    } else {
      // The next word is longer than expected. There must be at least one of every length. Next bucket.
      bucketp++;
      if (bucketp>7) {
        // Invalid! Dictionary must not contain words longer than 7 letters.
        break;
      }
      scg.bucketv[bucketp].v=src+srcp;
      // Don't advance (srcp) or count this first word, let it hit at the next pass.
    }
  }
}

/* Validate word.
 */
 
int spellcheck(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<2) return 0;
  if (srcc>7) return 0;
  char norm[7];
  int i=srcc; while (i-->0) {
    if ((src[i]>='a')&&(src[i]<='z')) norm[i]=src[i]-0x20;
    else if ((src[i]>='A')&&(src[i]<='z')) norm[i]=src[i];
    else return 0;
  }
  spellcheck_require_init();
  int stride=srcc+1;
  const char *dict=scg.bucketv[srcc].v;
  int lo=0,hi=scg.bucketv[srcc].c;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const char *q=dict+ck*stride;
    int cmp=memcmp(norm,q,srcc);
         if (cmp<0) hi=ck;
    else if (cmp>0) lo=ck+1;
    else return 1;
  }
  return 0;
}

/* Score word.
 */
 
static const uint8_t letter_scores[26]={
   1, // A
   3, // B
   3, // C
   2, // D
   1, // E
   4, // F
   2, // G
   4, // H
   1, // I
   8, // J
   5, // K
   1, // L
   3, // M
   1, // N
   1, // O
   3, // P
  10, // Q
   1, // R
   1, // S
   1, // T
   1, // U
   4, // V
   4, // W
   8, // X
   4, // Y
  10, // Z
};
 
int rate_letter(char letter) {
  // Do not force uppercase. We use lowercase letters for those inserted by wildcard, which are always worth zero.
  if (letter<'A') return 0;
  if (letter>'Z') return 0;
  return letter_scores[letter-'A'];
}

int rate_word(const char *word,int wordc) {
  if (!word) return 0;
  if (wordc<0) { wordc=0; while (word[wordc]) wordc++; }
  int score=0,letterc=0;
  for (;wordc-->0;word++) {
    score+=rate_letter(*word);
    if (*word) letterc++;
  }
  if (letterc>=7) score+=50;
  else if (letterc>=6) score+=10;
  else if (letterc>=5) score+=5;
  return score;
}

/* Iterate words of a given length and starting letter.
 */
 
int for_each_word(int len,char first,int (*cb)(const char *src,int srcc,void *userdata),void *userdata) {
  if ((len<2)||(len>7)) return 0;
  if ((first<'A')||(first>'Z')) return 0;
  spellcheck_require_init();
  int stride=len+1;
  const char *dict=scg.bucketv[len].v;
  int lo=0,hi=scg.bucketv[len].c;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const char *q=dict+ck*stride;
         if (first<q[0]) hi=ck;
    else if (first>q[0]) lo=ck+1;
    else {
      while ((ck>lo)&&(q[-stride]==first)) { ck--; q-=stride; }
      int err;
      for (;(ck<hi)&&(q[0]==first);ck++,q+=stride) {
        if (err=cb(q,len,userdata)) return err;
      }
      break;
    }
  }
  return 0;
}

/* Direct access to buckets.
 */
 
int get_dictionary_bucket(const char **dstpp,int len) {
  if ((len<0)||(len>7)) return 0;
  spellcheck_require_init();
  *dstpp=scg.bucketv[len].v;
  return scg.bucketv[len].c;
}

/* Search bucket.
 */
 
int search_bucket(const char *v,int wordc,const char *q,int qc) {
  if (!v||(wordc<1)) return -1;
  if (!q) return -1;
  if (qc<0) { qc=0; while (q[qc]) qc++; }
  if (!qc) return -1;
  if (v[qc]!=0x0a) return -1; // Invalid word length for bucket.
  int stride=qc+1;
  int lo=0,hi=wordc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int cmp=memcmp(q,v+ck*stride,qc);
         if (cmp<0) hi=ck;
    else if (cmp>0) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}
