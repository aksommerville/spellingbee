/* tool_main_dict.c
 * A complete separate operating mode where we read a dictionary resource and dump some stats.
 */
 
#include "tool_internal.h"
 
/* Globals.
 */
 
static struct {

  /* Buckets are indexed by length. Buckets 0 and 1 will always be empty.
   */
  struct bucket {
    int len;
    int stride; // (len) if binary, (len+1) if text.
    const char *v; // Start of first word.
    int c; // Count of words (not bytes).
  } bucketv[8];
  
} d={0};

/* Validate word. Must be uppercase ASCII of a length 2..7.
 */
 
static int validate_word(const char *src,int srcc,const char *path,int lineno) {
  if ((srcc<2)||(srcc>7)) {
    fprintf(stderr,"%s:%d: Invalid word length %d\n",path,lineno,srcc);
    return -2;
  }
  int i=srcc; while (i-->0) {
    if ((src[i]<'A')||(src[i]>'Z')) {
      fprintf(stderr,"%s:%d: Unexpected byte 0x%02x in word.\n",path,lineno,src[i]);
      return -2;
    }
  }
  return 0;
}

/* Rate a word the same way runtime will:
 *  - Use these letter_scores copied from src/game/battle/dict.c.
 *  - Apply length bonus 5, 10, or 50.
 *  - Obviously we're not considering validity, blanks, multipliers, per-foe modifiers, etc.
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

static const uint8_t letter_distribution[27]={
   9, // A
   2, // B
   2, // C
   4, // D
  12, // E
   2, // F
   3, // G
   2, // H
   9, // I
   1, // J
   1, // K
   4, // L
   2, // M
   6, // N
   8, // O
   2, // P
   1, // Q
   6, // R
   4, // S
   6, // T
   4, // U
   2, // V
   2, // W
   1, // X
   2, // Y
   1, // Z
   2, // Blank
};

// Assume every letter is real.
static int dict_rate_word_naive(const char *src,int srcc) {
  int score=(srcc>=7)?50:(srcc>=6)?10:(srcc>=5)?5:0;
  for (;srcc-->0;src++) {
    if ((*src<'A')||(*src>'Z')) return 0;
    score+=letter_scores[(*src)-'A'];
  }
  return score;
}

// Check whether the word is actually reachable given the distribution above, and 2 wildcards. Zero if not.
// Do not add points when a wildcard is used.
static int dict_rate_word_blank_savvy(const char *src,int srcc) {
  int score=(srcc>=7)?50:(srcc>=6)?10:(srcc>=5)?5:0;
  uint8_t hand[27];
  memcpy(hand,letter_distribution,sizeof(hand));
  for (;srcc-->0;src++) {
    int p=(*src)-'A';
    if ((p<0)||(p>=26)) return 0;
    if (hand[p]) {
      hand[p]--;
      score+=letter_scores[p];
    } else if (hand[26]) {
      hand[26]--;
    } else {
      return 0;
    }
  }
  return score;
}

/* Decode to globals, binary format.
 */
 
static int dict_decode_binary(const uint8_t *src,int srcc,const char *path) {

  // Read the fixed header.
  if (srcc<12) return -1;
  int lenv[8]={0};
  int lenp=2,srcp=0;
  for (;lenp<8;lenp++,srcp+=2) lenv[lenp]=(src[srcp]<<8)|src[srcp+1];
  
  // Populate buckets. This is a pretty format, we don't need to visit individual words.
  int i=0; for (;i<8;i++) {
    int addc=lenv[i]*i;
    if (srcp>srcc-addc) {
      fprintf(stderr,"%s: Unexpected EOF in bucket [%d], count %d, starting position %d\n",path,i,lenv[i],srcp);
      return -2;
    }
    struct bucket *bucket=d.bucketv+i;
    bucket->len=i;
    bucket->stride=i;
    bucket->v=(char*)(src+srcp);
    bucket->c=lenv[i];
    srcp+=addc;
    
    // But to ensure parity with dict_decode_text, do visit and validate each word.
    const char *word=bucket->v;
    int wordi=bucket->c;
    for (;wordi-->0;word+=i) {
      int err=validate_word(word,i,path,0);
      if (err<0) return err;
    }
  }
  
  if (srcp<srcc) fprintf(stderr,"%s:WARNING: %d bytes unused at end of file\n",path,srcc-srcp);
  return 0;
}

/* Decode to globals, text format.
 */
 
static int dict_decode_text(const char *src,int srcc,const char *path) {
  int srcp=0,lineno=1,len=2,err;
  while (srcp<srcc) {
    if (srcp>srcc-len) {
      fprintf(stderr,"%s:%d: Unexpected EOF\n",path,lineno);
      return -2;
    }
    if ((srcp+len<srcc)&&(src[srcp+len]!=0x0a)) { // No newline at the end of word? Bump expected length and stay here.
      len++;
      continue;
    }
    if ((err=validate_word(src+srcp,len,path,lineno))<0) return err;
    if (!d.bucketv[len].v) {
      d.bucketv[len].len=len;
      d.bucketv[len].stride=len+1;
      d.bucketv[len].v=src+srcp;
    }
    d.bucketv[len].c++;
    srcp+=len;
    if ((srcp<srcc)&&(src[srcp]==0x0a)) {
      srcp++;
      lineno++;
    }
  }
  return 0;
}

/* Search for words without AEIOU and report them.
 */

// (0,1,2)=(none,Y,AEIOU)
static int measure_vowelness(const char *src,int srcc) {
  int result=0;
  for (;srcc-->0;src++) {
    switch (*src) {
      case 'A': case 'E': case 'I': case 'O': case 'U': return 2;
      case 'Y': result=1; break;
    }
  }
  return result;
}
 
static void dict_report_vowelless_words(int allow_y) {
  fprintf(stderr,"Words without vowels (%s):\n",allow_y?"allowing Y":"even Y");
  const struct bucket *bucket=d.bucketv;
  int i=8;
  int wit=0,witout=0;
  for (;i-->0;bucket++) {
    const char *word=bucket->v;
    int wordi=bucket->c;
    for (;wordi-->0;word+=bucket->stride) {
      switch (measure_vowelness(word,bucket->len)) {
        case 0: witout++; break;
        case 1: wit++; if (!allow_y) continue; break;
        default: continue;
      }
      fprintf(stderr,"  %7.*s %2d\n",bucket->len,word,dict_rate_word_blank_savvy(word,bucket->len));
    }
  }
  fprintf(stderr,"Total %d words with no vowels, and %d with only Y\n",witout,wit);
}

/* Search for anagrams and report them.
 * The process is cheaper than you'd think.
 * But it does produce a ridiculous quantity of hits against the real dictionary.
 * So only enable when we're filtering the pidgin dictionary.
 */
 
struct hashed_word {
  uint64_t hash;
  int index; // word's index in bucket
};

static const uint64_t twenty_six_primes[26]={
   1, 2, 3, 5, 7,11,
  13,17,19,23,29,31,
  37,41,43,47,53,59,
  61,67,71,73,79,83,
  89,97,
};

/* Multiply a hash, starting at 1, against a prime number corresponding to each letter.
 * The range is about 97**7: well over 32 bits, but well under 64.
 * Collisions identify anagrams exactly.
 */
static uint64_t hash_word(const char *src,int srcc) {
  uint64_t hash=1;
  for (;srcc-->0;src++) {
    int p=(*src)-'A';
    if ((p<0)||(p>=26)) return 0;
    hash*=twenty_six_primes[p];
  }
  return hash;
}

static int hashed_word_cmp(const void *A,const void *B) {
  const struct hashed_word *a=A,*b=B;
  if (a->hash<b->hash) return -1;
  if (a->hash>b->hash) return 1;
  return 0;
}
 
static void dict_report_anagrams() {
  const struct bucket *bucket=d.bucketv;
  int i=8;
  for (;i-->0;bucket++) {
    if (bucket->c<2) continue;
    struct hashed_word *hashv=malloc(sizeof(struct hashed_word)*bucket->c);
    if (!hashv) continue;
    const char *word=bucket->v;
    int index=0;
    struct hashed_word *hash=hashv;
    for (;index<bucket->c;index++,word+=bucket->stride,hash++) {
      hash->hash=hash_word(word,bucket->len);
      hash->index=index;
    }
    qsort(hashv,bucket->c,sizeof(struct hashed_word),hashed_word_cmp);
    int p=0;
    while (p<bucket->c) {
      const struct hashed_word *first=hashv+p++;
      const struct hashed_word *next=first+1;
      int collided=0;
      while ((p<bucket->c)&&(first->hash==next->hash)) {
        if (!collided) {
          collided=1;
          fprintf(stderr,"Anagrams at length %d: %.*s",bucket->len,bucket->len,bucket->v+first->index*bucket->stride);
        }
        fprintf(stderr," %.*s",bucket->len,bucket->v+next->index*bucket->stride);
        p++;
        next++;
      }
      if (collided) fprintf(stderr,"\n");
    }
    free(hashv);
  }
}

/* Find the most valuable word of each length.
 */
 
static void dict_report_best_plays() {
  const struct bucket *bucket=d.bucketv;
  int i=8;
  for (;i-->0;bucket++) {
    if (bucket->c<1) continue;
    int hiscore=0,tiec=0;
    const char *bestword="?";
    const char *word=bucket->v;
    int wordi=bucket->c;
    for (;wordi-->0;word+=bucket->stride) {
      int score=dict_rate_word_blank_savvy(word,bucket->len);
      if (score<=0) {
        fprintf(stderr,"%s:WARNING: Word '%.*s' is not reachable even with blanks.\n",tool.srcpath,bucket->len,word);
      } else if (score>hiscore) {
        hiscore=score;
        bestword=word;
        tiec=0;
      } else if (score==hiscore) {
        tiec++;
      }
    }
    fprintf(stderr,"Best play at length %d: %d for '%.*s' (and %d others)\n",bucket->len,hiscore,bucket->len,bestword,tiec);
  }
}
 
/* Main.
 * (srcpath) and (src) are set, caller ensures it.
 * Returns exit status.
 */
 
int tool_main_dict() {

  /* We'll accept both text and binary formats.
   * The binary format doesn't have a signature per se, but it's easy to detect.
   * The first 2 bytes are the big-endian count of 2-letter words, which is going to be about a hundred.
   * So the first byte will always be zero.
   */
  int err=-1;
  if ((tool.srcc>=12)&&!tool.src[0]) err=dict_decode_binary((uint8_t*)tool.src,tool.srcc,tool.srcpath);
  else err=dict_decode_text(tool.src,tool.srcc,tool.srcpath);
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Failed to decode dictionary.\n",tool.srcpath);
    return 1;
  }
  
  /* Dump TOC vitals.
   */
  fprintf(stderr,"%s: Parsed dictionary from %d bytes input:\n",tool.srcpath,tool.srcc);
  int len=0,total=0;
  for (;len<8;len++) {
    const struct bucket *bucket=d.bucketv+len;
    fprintf(stderr,"  [%d] %d\n",len,bucket->c);
    total+=bucket->c;
  }
  fprintf(stderr,"  %d words total\n",total);
  
  /* Issue a warning if any 2..7 bucket is empty, longer than 16 bit, or missorted.
   */
  for (len=2;len<=7;len++) {
    const struct bucket *bucket=d.bucketv+len;
    if (!bucket->c) {
      fprintf(stderr,"%s:WARNING: Bucket %d is empty.\n",tool.srcpath,len);
      continue;
    }
    if (bucket->c>0xffff) {
      fprintf(stderr,"%s:WARNING: Bucket %d length %d exceeds 16 bits. This dictionary will fail at compile.\n",tool.srcpath,len,bucket->c);
    }
    const char *word=bucket->v;
    int i=bucket->c-1;
    for (;i-->0;word+=bucket->stride) {
      int cmp=memcmp(word,word+bucket->stride,bucket->len);
      if (cmp>=0) {
        fprintf(stderr,"%s:WARNING: Words out of order: '%.*s' >= '%.*s'\n",tool.srcpath,bucket->len,word,bucket->len,word+bucket->stride);
      }
    }
  }
  
  /* And then we can do more interesting focussed tests...
   */
  fprintf(stderr,"%s:%d: Choice of reports is configured directly in the source.\n",__FILE__,__LINE__);
  //dict_report_vowelless_words(1); // 1 to include 'Y', 0 for only hardcore vowelless.
  //dict_report_anagrams(); // Noisy; enable as needed.
  dict_report_best_plays();

  return 0;
}
