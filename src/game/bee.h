#ifndef BEE_H
#define BEE_H

struct encounter;
struct foe;
struct letterbag;

#define TILESIZE 16

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "opt/rom/rom.h"
#include "egg_rom_toc.h"
#include "encounter.h"
#include "spellcheck.h"
#include "world.h"

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
  struct world world;
  
  int hp;
  int inventory[ITEM_COUNT];
  
} g;

int rom_get_res(void *dstpp,int tid,int rid);

struct cmd_reader {
  const uint8_t *v;
  int c;
  int p;
};
int cmd_reader_next(const uint8_t **argv,uint8_t *opcode,struct cmd_reader *reader);

#endif
