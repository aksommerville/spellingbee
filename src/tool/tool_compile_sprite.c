#include "tool_internal.h"
#include "game/sprite/sprite.h" /* macros only */

static const struct tool_compile_command sprite_commands[]={
  {0x20,"image"},
  {0x21,"type"},
  {0x22,"tile"},
  {0x40,"groups"},
{0}};

/* Sprite type names.
 */
 
static int sprtid_eval(const char *src,int srcc) {
  int sprtid=0;
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return sprtid; sprtid++;
  SPRITE_TYPE_FOR_EACH
  #undef _
  return -1;
}

/* Custom parameters.
 */
 
static int eval_sprite_param(struct sr_encoder *dst,const char *src,int srcc) {

  // Undecorated sprite type names are legal, they emit as 2 bytes.
  int sprtid=sprtid_eval(src,srcc);
  if (sprtid>=0) {
    if (sr_encode_intbe(dst,sprtid,2)<0) return -1;
    return 1;
  }
  
  return 0;
}

/* Compile sprite, main entry point.
 */
 
int tool_compile_sprite() {
  if (sr_encode_raw(&tool.dst,"\0SPR",4)<0) return -1;
  struct sr_decoder decoder={.v=tool.src,.c=tool.srcc};
  const char *line;
  int err,linec,lineno=1;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    if ((err=tool_compile_command(&tool.dst,line,linec,sprite_commands,eval_sprite_param,tool.srcpath,lineno))<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error compiling sprite command.\n",tool.srcpath,lineno);
      return -2;
    }
  }
  return 0;
}
