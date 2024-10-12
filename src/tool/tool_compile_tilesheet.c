#include "tool_internal.h"

/* Evaluate table id.
 */
 
static int tableid_eval(const char *src,int srcc) {
  if ((srcc==7)&&!memcmp(src,"physics",7)) return 1;
  if ((srcc==7)&&!memcmp(src,"overlay",7)) return 2;
  if ((srcc==7)&&!memcmp(src,"animate",7)) return 3;
  // No need to acknowledge the editor-only tables.
  int v;
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>0)&&(v<=0xff)) return v;
  return -1;
}

/* Emit table.
 */
 
static int tilesheet_emit_table(struct sr_encoder *dst,int tableid,const uint8_t *src/*256*/) {
  int srcp=0;
  while (srcp<256) {
    if (!src[srcp]) { srcp++; continue; }
    int subp=srcp;
    int subc=0,zeroc=0;
    // Stop at the end of the table, or after 4 zeroes. (after the 4th, it makes sense to split entries).
    while (srcp<256) {
      subc++;
      if (src[srcp]) zeroc=0;
      else zeroc++;
      srcp++;
      if (zeroc>=4) break;
    }
    while (subc&&!src[subp+subc-1]) subc--;
    if (sr_encode_u8(dst,tableid)<0) return -1;
    if (sr_encode_u8(dst,subp)<0) return -1;
    if (sr_encode_u8(dst,subc-1)<0) return -1;
    if (sr_encode_raw(dst,src+subp,subc)<0) return -1;
  }
  return 0;
}

/* Compile tilesheet, main entry point.
 */
 
int tool_compile_tilesheet() {
  if (sr_encode_raw(&tool.dst,"\0TLS",4)<0) return -1;
  struct sr_decoder decoder={.v=tool.src,.c=tool.srcc};
  const char *line;
  // (tableid>0xff) to validate and skip
  int linec,lineno=1,tableid=0,tmpp=0;
  uint8_t tmp[256];
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { line++; linec--; }
    if (!linec) continue;
    
    if (!tableid) {
      if ((tableid=tableid_eval(line,linec))<1) {
        tableid=256;
      }
      memset(tmp,0,sizeof(tmp));
      tmpp=0;
      
    } else if (linec!=32) {
      fprintf(stderr,"%s:%d: Expected 32 hex digits for row %d of table %d, found length %d\n",tool.srcpath,lineno,tmpp>>4,tableid,linec);
      return -2;
      
    } else {
      int linep=0; for (;linep<linec;linep+=2) {
        int hi=sr_digit_eval(line[linep]);
        int lo=sr_digit_eval(line[linep+1]);
        if ((hi<0)||(lo<0)||(hi>15)||(lo>15)) {
          fprintf(stderr,"%s:%d: Expected hexadecimal byte, found '%.2s'\n",tool.srcpath,lineno,line+linep);
          return -2;
        }
        tmp[tmpp++]=(hi<<4)|lo;
      }
      if (tmpp>=sizeof(tmp)) {
        if (tableid<0x100) {
          if (tilesheet_emit_table(&tool.dst,tableid,tmp)<0) return -1;
        }
        tableid=0;
        tmpp=0;
      }
    }
  }
  if (tmpp) {
    fprintf(stderr,"%s: File ends with incomplete table\n",tool.srcpath);
    return -2;
  }
  return 0;
}
