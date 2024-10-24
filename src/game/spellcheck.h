/* spellcheck.h
 * Manages the static dictionary, and operations against it.
 * For the most part we don't touch the shared globals (we do read g.rom, that's it).
 */
 
#ifndef SPELLCHECK_H
#define SPELLCHECK_H

/* Initializes globals if necessary, then looks up a word.
 * The spellchecker has its own globals outside of (g).
 * Returns 0 if invalid or 1 if valid, nothing else.
 * (src) must be composed of 2..7 letters, otherwise it's invalid and you don't need to call.
 * Lowercase letters are ok.
 */
int spellcheck(const char *src,int srcc);

/* Rating a word does not spellcheck it first; you'll want to do that separately.
 * Lowercase letters, and anything not a letter, safely counts as zero.
 * (modifier) is ITEM_2XLETTER etc, or zero for none.
 */
int rate_letter(char letter);
int rate_word(const char *word,int wordc,int modifier);

/* Call (cb) for each word of length (len) that begins with (first).
 * Stops if you return nonzero, and returns the same.
 */
int for_each_word(int len,char first,int (*cb)(const char *src,int srcc,void *userdata),void *userdata);

/* Direct access to length buckets.
 * Returns the count of words (not bytes!).
 * Words are spaced (len+1) bytes apart, with an LF after each, are uppercase, and are in alphabetical order.
 */
int get_dictionary_bucket(const char **dstpp,int len);

/* Index or (-index-1) of the given word.
 * (qc) must be the bucket's word length.
 * We don't really care what's in (q), eg you can search "G[[" to find the end of 3-letter 'G' word or "G  " for the start.
 */
int search_bucket(const char *v,int wordc,const char *q,int qc);

#endif
