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
#include "egg_rom_toc.h"

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
// cached toc content, lazy-loaded in tool_compile_command.c:
  struct toc { int tid,rid,namec; char *name; } *tocv;
  int tocc,toca;
  struct flag { int fid,namec; char *name; } *flagv;
  int flagc,flaga;
} tool;

int tool_compile_map();
int tool_compile_sprite();
int tool_compile_tilesheet();
int tool_compile_dict();

int tool_eval_tid(const char *src,int srcc);
int tool_eval_rid(const char *src,int srcc,int tid);
int tool_eval_flag(const char *src,int srcc);

/* Compile one command for map or sprite.
 * Caller must supply a list of valid commands, terminated by an entry with opcode zero.
 * Optionally provide (eval_extra) to process parameter tokens before general processing. Return >0 if you consumed it.
 * (src) must be one line of text.
 * Opcodes 0xe0..0xff are currently undefined and we allow any param length.
 */
struct tool_compile_command {
  uint8_t opcode;
  const char *name;
};
int tool_compile_command(
  struct sr_encoder *dst,
  const char *src,int srcc,
  const struct tool_compile_command *commandv,
  int (*eval_extra)(struct sr_encoder *dst,const char *token,int tokenc),
  const char *path,int lineno
);

#endif
