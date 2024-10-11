#ifndef BEE_H
#define BEE_H

struct encounter;
struct foe;
struct letterbag;

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "opt/rom/rom.h"
#include "egg_rom_toc.h"
#include "encounter.h"
#include "spellcheck.h"

#define ITEM_NOOP       0 /* Don't use zero. */
#define ITEM_2XLETTER   1
#define ITEM_3XLETTER   2
#define ITEM_2XWORD     3
#define ITEM_3XWORD     4
#define ITEM_COUNT      5

extern struct globals {
  void *rom;
  int romc;
  struct graf graf;
  struct texcache texcache;
  struct font *font;
  int fbw,fbh;
  int pvinput;
  
  struct encounter encounter;
  
  int hp;
  int inventory[ITEM_COUNT];
  
} g;

#endif
