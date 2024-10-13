#include "tool_internal.h"

static const struct tool_compile_command map_commands[]={
  {0x20,"song"},
  {0x21,"image"},
  {0x22,"hero"},
  {0x60,"door"},
  {0x61,"sprite"},
  {0x62,"message"},
{0}};

/* Compile map, main entry point.
 */
 
int tool_compile_map() {

  /* Encode signature and header with dummies.
   * We can start the map image as soon as we get it.
   */
  if (sr_encode_raw(&tool.dst,"\0MAP",4)<0) return -1;
  int dstwp=tool.dst.c;
  if (sr_encode_u8(&tool.dst,0)<0) return -1;
  int dsthp=tool.dst.c;
  if (sr_encode_u8(&tool.dst,0)<0) return -1;
  int colc=0,rowc=0;

  struct sr_decoder decoder={.v=tool.src,.c=tool.srcc};
  const char *line;
  int err,linec,lineno=1;
  int stage=0; // (0)=(init,image,commands)
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    switch (stage) {
    
      case 0: { // Initial. Skip blank lines. First non-blank line establishes map width.
          if (!linec) continue;
          colc=linec>>1;
          stage=1;
        } // pass
        
      case 1: { // Map image. First blank line terminates.
          if (!linec) {
            stage=2;
            continue;
          }
          if (linec!=(colc<<1)) {
            fprintf(stderr,"%s:%d: Expected blank line or %d hex digits, found length %d.\n",tool.srcpath,lineno,colc<<1,linec);
            return -2;
          }
          int linep=0; for (;linep<linec;linep+=2) {
            int hi=sr_digit_eval(line[linep]);
            int lo=sr_digit_eval(line[linep+1]);
            if ((hi<0)||(hi>15)||(lo<0)||(lo>15)) {
              fprintf(stderr,"%s:%d: Expected hexadecimal byte, found '%.2s'\n",tool.srcpath,lineno,line+linep);
              return -2;
            }
            if (sr_encode_u8(&tool.dst,(hi<<4)|lo)<0) return -1;
          }
          rowc++;
        } break;
        
      case 2: { // Commands.
          if ((err=tool_compile_command(&tool.dst,line,linec,map_commands,0,tool.srcpath,lineno))<0) {
            if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error compiling map command.\n",tool.srcpath,lineno);
            return -2;
          }
        } break;
        
      default: return -1;
    }
  }
  if ((colc<1)||(colc>0xff)||(rowc<1)||(rowc>0xff)) {
    fprintf(stderr,"%s: Invalid map dimensions %dx%d, must be in 1x1..255x255\n",tool.srcpath,colc,rowc);
    return -2;
  }
  ((uint8_t*)tool.dst.v)[dstwp]=colc;
  ((uint8_t*)tool.dst.v)[dsthp]=rowc;
  return 0;
}
