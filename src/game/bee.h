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

extern struct globals {
  void *rom;
  int romc;
  struct graf graf;
  struct texcache texcache;
  struct font *font;
  int fbw,fbh;
  struct encounter encounter;
  int pvinput;
} g;

#endif
