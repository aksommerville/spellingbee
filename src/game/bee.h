#ifndef BEE_H
#define BEE_H

struct encounter;
struct foe;
struct letterbag;

#define TILESIZE 16
#define STATUS_BAR_HEIGHT 9

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/text/text.h"
#include "util/font/font.h"
#include "util/res/res.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"
#include "world.h"
#include "save.h"
#include "sprite/sprite.h"
#include "modal/modal.h"

// Disable music.
//#define egg_play_song(songid,force,repeat) 

#define DIR_N 0x40
#define DIR_W 0x10
#define DIR_E 0x08
#define DIR_S 0x02

#define PHYSICS_VACANT 0
#define PHYSICS_SOLID 1
#define PHYSICS_WATER 2
#define PHYSICS_HOLE 3
#define PHYSICS_SAFE 4

#define MODAL_LIMIT 8

// Enable for logging almost sufficient to recreate the play session.
//#define TRACE(fmt,...) fprintf(stderr,"%f %s "fmt"\n",egg_time_real(),__func__,##__VA_ARGS__);
#define TRACE(fmt,...)

extern struct globals {
  void *rom;
  int romc;
  struct graf graf;
  struct font *font;
  int fbw,fbh;
  int pvinput;
  int song_playing;
  
  struct world world;
  struct modal *modalv[MODAL_LIMIT];
  int modalc;
  
  struct saved_game stats;
  
} g;

int rom_get_res(void *dstpp,int tid,int rid);

void save_game();

int flag_get(int flagid);
int flag_set(int flagid,int v); // => nonzero if changed. We call world_recheck_flags() if so.
int flag_set_nofx(int flagid,int v); // No side effects. If you know it has no ramifications, or you're about to set another one.
int something_being_carried(); // Clearinghouse for all "carry something" flags.

void sb_song(int rid,int repeat);
void sb_sound(int rid);

#endif
