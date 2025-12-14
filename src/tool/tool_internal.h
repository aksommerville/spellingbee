/* tool used to be a bigger deal than it is.
 * Map, sprite, and tilesheet are now managed for us by Egg.
 * So our only responsibility here is dict, which is pretty simple.
 */

#ifndef TOOL_INTERNAL_H
#define TOOL_INTERNAL_H

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include "egg/egg.h"
#include "opt/serial/serial.h"
#include "opt/fs/fs.h"

#define EGG_TID_dict 33 /* We have to take this on faith, since we get built before egg_res_toc.h */

extern struct tool {
// argv:
  const char *exename;
  const char *srcpath;
  const char *dstpath;
  const char *tocpath;
// managed globally:
  char *src;
  int srcc;
  struct sr_encoder dst;
} tool;

int tool_main_dict();

int tool_compile_dict();

int tool_eval_tid(const char *src,int srcc);
int tool_eval_rid(const char *src,int srcc,int tid);

#endif
