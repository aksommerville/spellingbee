#include "game/bee.h"
#include "dict.h"

/* Global cache.
 */
 
// There are two dict resources, and very unlikely to add any others.
// If we do, no problemo, just bump this up as needed.
#define DICT_CACHE_SIZE 2
 
static struct dict_cache {
  int rid;
  // Index is (wordlen-DICT_SHORTEST_WORD); we don't store lengths zero or one because they are always empty.
  struct dict_bucket bucketv[DICT_LONGEST_WORD-DICT_SHORTEST_WORD+1];
} dict_cache[DICT_CACHE_SIZE]={0};
static int dict_cachec=0;

/* Search cache entries.
 * These are a tad overkilly, since there won't be more than 2 entries, but whatever.
 */
 
static int dict_cache_search(int rid) {
  int lo=0,hi=dict_cachec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
         if (rid<dict_cache[ck].rid) hi=ck;
    else if (rid>dict_cache[ck].rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

// Provisions the new entry but does not populate it.
static struct dict_cache *dict_cache_insert(int p,int rid) {
  if ((p<0)||(p>=DICT_CACHE_SIZE)) return 0;
  struct dict_cache *cache=dict_cache+p;
  memmove(cache+1,cache,sizeof(struct dict_cache)*(dict_cachec-p));
  dict_cachec++;
  memset(cache,0,sizeof(struct dict_cache));
  cache->rid=rid;
  struct dict_bucket *bucket=cache->bucketv;
  int i=DICT_LONGEST_WORD-DICT_SHORTEST_WORD+1,len=2;
  for (;i-->0;bucket++,len++) bucket->len=len;
  return cache;
}

/* Decode dict resource into a fresh cache entry.
 * (rid) is provided only for logging.
 */
 
static void dict_cache_decode(struct dict_cache *cache,const char *src,int srcc,int rid) {
  // There are probably ways to do this without visiting every single word, but meh.
  int srcp=0,len=DICT_SHORTEST_WORD;
  struct dict_bucket *bucket=cache->bucketv;
  bucket->v=src;
  for (;;) {
    if (srcp+len>srcc) break;
    if (srcp+len==srcc) {
      bucket->c++;
      break;
    }
    if ((unsigned char)src[srcp+len]<=0x20) {
      bucket->c++;
      srcp+=len+1;
      continue;
    }
    len++;
    if (len>DICT_LONGEST_WORD) break;
    bucket++;
    bucket->v=src+srcp;
  }
}

/* Find a cache, or initialize it if absent.
 */
 
static struct dict_cache *dict_cache_require(int rid) {
  int p=dict_cache_search(rid);
  if (p>=0) return dict_cache+p;
  if (dict_cachec>=DICT_CACHE_SIZE) return 0;
  p=-p-1;
  const char *src=0;
  int srcc=rom_get_res(&src,EGG_TID_dict,rid);
  if (srcc<1) return 0;
  struct dict_cache *cache=dict_cache_insert(p,rid);
  if (!cache) return 0;
  dict_cache_decode(cache,src,srcc,rid);
  return cache;
}

/* Get all buckets from a dict resource.
 */
 
int dict_get_all(struct dict_bucket *dstv,int dsta,int rid) {
  if (!dstv||(dsta<0)) dsta=0;
  struct dict_cache *cache=dict_cache_require(rid);
  if (!cache) return 0;
  int dstc=DICT_LONGEST_WORD-DICT_SHORTEST_WORD+1;
  if (dstc>dsta) dstc=dsta;
  memcpy(dstv,cache->bucketv,sizeof(struct dict_bucket)*dstc);
  return dstc;
}

/* Get one bucket from a dict resource.
 */
 
void dict_get_bucket(struct dict_bucket *dst,int rid,int len) {
  dst->len=len;
  dst->v=0;
  dst->c=0;
  if ((len<DICT_SHORTEST_WORD)||(len>DICT_LONGEST_WORD)) return;
  struct dict_cache *cache=dict_cache_require(rid);
  if (!cache) return;
  memcpy(dst,cache->bucketv+len-DICT_SHORTEST_WORD,sizeof(struct dict_bucket));
}

/* Search within one bucket.
 */
 
int dict_bucket_search(const struct dict_bucket *bucket,const char *word) {
  if (!bucket||!word) return -1;
  int stride=bucket->len+1;
  int lo=0,hi=bucket->c;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int cmp=memcmp(word,bucket->v+ck*stride,bucket->len);
         if (cmp<0) hi=ck;
    else if (cmp>0) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Score per letter.
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

/* Rate a word.
 */

int dict_rate_word(struct rating_detail *detail,int rid,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  struct rating_detail _scratch={0};
  if (!detail) detail=&_scratch;
  
  // Calculate (basescore).
  {
    detail->basescore=0;
    const char *v=src;
    int i=srcc;
    for (;i-->0;v++) {
      if ((*v>='A')&&(*v<='Z')) detail->basescore+=letter_scores[(*v)-'A'];
    }
  }
  
  // Check dictionary, and set (valid).
  if (detail->force_valid) {
    detail->valid=1;
  } else {
    detail->valid=0;
    if ((srcc>=DICT_SHORTEST_WORD)&&(srcc<=DICT_LONGEST_WORD)) {
      char norm[DICT_LONGEST_WORD];
      int i=srcc; while (i-->0) {
        if ((src[i]>='a')&&(src[i]<='z')) norm[i]=src[i]-0x20;
        else if ((src[i]>='A')&&(src[i]<='Z')) norm[i]=src[i];
        else { norm[0]=0; break; }
      }
      if (norm[0]) {
        struct dict_bucket bucket={0};
        dict_get_bucket(&bucket,rid,srcc);
        if (dict_bucket_search(&bucket,norm)>=0) detail->valid=1;
      }
    }
  }
  
  // Apply modifier.
  detail->modbonus=0;
  switch (detail->modifier) {
    case ITEM_2XLETTER:
    case ITEM_3XLETTER: {
        const char *v=src;
        int i=srcc,best=0;
        for (;i-->0;v++) {
          if (*v<'A') continue;
          if (*v>'Z') continue;
          int lscore=letter_scores[(*v)-'A'];
          if (lscore>best) best=lscore;
        }
        if (detail->modifier==ITEM_2XLETTER) detail->modbonus=best;
        else detail->modbonus=best*2;
      } break;
    case ITEM_2XWORD: {
        detail->modbonus=detail->basescore;
      } break;
    case ITEM_3XWORD: {
        detail->modbonus=detail->basescore*2;
      } break;
  }
  
  // Apply super_effective.
  detail->superbonus=0;
  if (detail->super_effective&&detail->valid) {
    const char *q=detail->super_effective;
    for (;*q;q++) {
      if ((*q<'A')||(*q>'Z')) continue;
      const char *v=src;
      int i=srcc;
      for (;i-->0;v++) {
        if ((*v==*q)||((*v)-0x20==*q)) {
          detail->superbonus=10;
          goto _done_super_effective_;
        }
      }
    }
   _done_super_effective_:;
  }
  
  // Apply length bonus.
  switch (srcc) {
    case 5: detail->lenbonus=5; break;
    case 6: detail->lenbonus=10; break;
    case 7: detail->lenbonus=50; break;
    default: detail->lenbonus=0; break;
  }
  
  // If (lenonly), keep (lenbonus) but drop (basescore) and (modbonus).
  if (detail->lenonly) {
    detail->basescore=0;
    detail->modbonus=0;
  }
  
  // Determine penalty.
  detail->penalty=0;
  if (!detail->valid) {
    detail->penalty=(detail->basescore+detail->modbonus+detail->superbonus+detail->lenbonus)*-2;
  } else if (detail->forbidden) {
    const char *q=detail->forbidden;
    for (;*q;q++) {
      if ((*q<'A')||(*q>'Z')) continue;
      const char *v=src;
      int i=srcc;
      for (;i-->0;v++) {
        if ((*v==*q)||((*v)-0x20==*q)) {
          detail->penalty=-(detail->basescore+detail->modbonus+detail->superbonus+detail->lenbonus);
          goto _done_forbidden_;
        }
      }
    }
   _done_forbidden_:;
  }
  
  // And we now have the final score.
  return detail->basescore+detail->modbonus+detail->superbonus+detail->lenbonus+detail->penalty;
}
