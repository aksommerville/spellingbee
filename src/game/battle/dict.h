/* dict.h
 * Global service providing access to the dictionary resources.
 */
 
#ifndef DICT_H
#define DICT_H

#define DICT_SHORTEST_WORD 2
#define DICT_LONGEST_WORD  7

struct dict_bucket {
  int len;       // Length of a word.
  const char *v; // Points to the first word. All same length, uppercase, in alphabetical order.
  int c;         // Count of words (not bytes!).
};

/* Copy one or multiple bucket headers from our cache, populating the cache if necessary.
 * Caller is free to modify these buckets, they're yours.
 * Content under (v) must not be modified.
 */
int dict_get_all(struct dict_bucket *dstv,int dsta,int rid);
void dict_get_bucket(struct dict_bucket *dst,int rid,int len);

/* (word) must agree with (bucket)'s word length.
 * Must be uppercase only; otherwise it won't match and will return an incorrect position.
 * Returns index or <0. Negative results are sensible: -n-1 is the first real word after the query.
 */
int dict_bucket_search(const struct dict_bucket *bucket,const char *word);

struct rating_detail {
// Input:
  int modifier; // ITEM_* (zero is noop)
  const char *forbidden; // Uppercase letters. If any is present, score is automatically zero. Wildcards do count.
  const char *super_effective; // Uppercase letters. 10-point bonus if any is present. Wildcards do count.
  int lenonly; // Nonzero to exclude (basescore,modbonus) from final score.
  int force_valid; // Pretend it's in the dictionary no matter what.
// Output:
  int basescore;  // Value of word without any adjustments, whether defined or not. Wildcards are still zero.
  int modbonus;   // Extra points due to modifier.
  int superbonus; // Extra points due to super_effective.
  int lenbonus;   // Extra points due to input length.
  int penalty;    // Negative of bonus-adjusted score if (forbidden) enforced, or twice negative if invalid.
  int valid;      // Nonzero if it's in the dictionary.
  // (basescore+modbonus+superbonus+lenbonus+penalty) is the adjusted score, exactly what dict_rate_word() returns.
};

/* Return the word's score.
 * If it's not present in the dictionary, we rate it and return negative.
 * (forbidden) and (super_effective) DO NOT apply to negative scores; other bonuses do.
 * (detail) is optional. Both input and output if present.
 * Lowercase letters do count for matching but do not contribute to the score.
 * There are additional battle rules that we can't enforce, eg required length or finisher letters.
 */
int dict_rate_word(struct rating_detail *detail,int rid,const char *src,int srcc);

#endif
