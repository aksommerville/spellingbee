#ifndef BEE_H
#define BEE_H

struct encounter;
struct foe;
struct letterbag;

#define TILESIZE 16
#define STATUS_BAR_HEIGHT 9

#include "egg/egg.h"
#include "opt/stdlib/egg-stdlib.h"
#include "opt/graf/graf.h"
#include "opt/text/text.h"
#include "opt/rom/rom.h"
#include "egg_rom_toc.h"
#include "world.h"
#include "sprite/sprite.h"
#include "modal/modal.h"

#define DIR_N 0x40
#define DIR_W 0x10
#define DIR_E 0x08
#define DIR_S 0x02

#define PHYSICS_VACANT 0
#define PHYSICS_SOLID 1
#define PHYSICS_WATER 2
#define PHYSICS_HOLE 3
#define PHYSICS_SAFE 4

#define ITEM_NOOP       0 /* Don't use zero. */
#define ITEM_BUGSPRAY   1
#define ITEM_UNFAIRIE   2
#define ITEM_2XWORD     3
#define ITEM_3XWORD     4
#define ITEM_ERASER     5
#define ITEM_COUNT      6

#define MODAL_LIMIT 8

/* Flag references will be 8-bit, so there's no point going over 32 here.
 * Don't go *under* 32 either! We assume all over the place that an 8-bit flag id can safely bitshift into a flags index.
 */
#define FLAGS_SIZE 32

extern struct globals {
  void *rom;
  int romc;
  struct graf graf;
  struct texcache texcache;
  struct font *font;
  int fbw,fbh;
  int pvinput;
  
  struct world world;
  struct modal *modalv[MODAL_LIMIT];
  int modalc;
  
  int hp;
  int xp;
  int gold;
  uint8_t inventory[ITEM_COUNT]; // 0..99 each
  uint8_t flags[FLAGS_SIZE]; // Bits indexed by FLAG_*, little-endianly.
  
} g;

int rom_get_res(void *dstpp,int tid,int rid);

struct cmd_reader {
  const uint8_t *v;
  int c;
  int p;
};
int cmd_reader_next(const uint8_t **argv,uint8_t *opcode,struct cmd_reader *reader);

void save_game();

int flag_get(int flagid);
int flag_set(int flagid,int v); // => nonzero if changed. We call world_recheck_flags() if so.
int flag_set_nofx(int flagid,int v); // No side effects. If you know it has no ramifications, or you're about to set another one.

#endif
