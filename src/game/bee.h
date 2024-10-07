#ifndef BEE_H
#define BEE_H

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "opt/rom/rom.h"
#include "egg_rom_toc.h"
#include "letterbag.h"

extern struct globals {
  void *rom;
  int romc;
  struct graf graf;
  struct texcache texcache;
  struct font *font;
  int fbw,fbh;
  struct {
    double animclock;
    int animframe;
    int x,y;
  } cursor;
  int pvinput;
  char stage[7];
  char hand[7];
  struct {
    char hand[7];
    int searchlen; // 2..7
    int searchp; // 0..6
    const char *word; // Best found so far.
    int wordc;
    int wordscore;
    double searchclock;
    char lastplay[7];
    int lastscore;
  } foe;
  struct letterbag letterbag;
  int score;
  int recent_score; // From the last play.
  
  //TODO The wildcard modal should be managed separately.
  int wildcard_modal;
  int wildcard_handp;
  int wcx,wcy;
} g;

/* Initializes globals if necessary, then looks up a word.
 * The spellchecker has its own globals outside of (g).
 * Returns 0 if invalid or 1 if valid, nothing else.
 * (src) must be composed of 2..7 letters, otherwise it's invalid and you don't need to call.
 * Lowercase letters are ok.
 */
int spellcheck(const char *src,int srcc);

/* Rating a word does not spellcheck it first; you'll want to do that separately.
 * Lowercase letters, and anything not a letter, safely counts as zero.
 */
int rate_letter(char letter);
int rate_word(const char *word,int wordc);

/* Call (cb) for each word of length (len) that begins with (first).
 * Stops if you return nonzero, and returns the same.
 */
int for_each_word(int len,char first,int (*cb)(const char *src,int srcc,void *userdata),void *userdata);

#endif
