#include "tool_internal.h"

/* Build TOC from text.
 */
 
static int tool_toc_parse(const char *src,int srcc,const char *path) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec,lineno=1;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    while (linec&&((unsigned char)line[linec-1]<=0x20)) linec--;
    while (linec&&((unsigned char)line[0]<=0x20)) { linec--; line++; }
    if (!linec) continue;
    
    if ((linec>=12)&&!memcmp(line,"#define RID_",12)) {
      int linep=12;
      const char *tname=line+linep;
      int tnamec=0;
      while ((linep<linec)&&(line[linep++]!='_')) tnamec++;
      const char *rname=line+linep;
      int rnamec=0;
      while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) rnamec++;
      const char *idstr=line+linep;
      int idstrc=linec-linep;
      int tid=tool_eval_tid(tname,tnamec);
      if (tid<1) {
        fprintf(stderr,"%s:%d:WARNING: Unknown type '%.*s'\n",path,lineno,tnamec,tname);
        continue;
      }
      if (rnamec<1) {
        fprintf(stderr,"%s:%d:WARNING: Malformed resource declaration, empty name.\n",path,lineno);
        continue;
      }
      int rid;
      if ((sr_int_eval(&rid,idstr,idstrc)<2)||(rid<1)||(rid>0xffff)) {
        fprintf(stderr,"%s:%d:WARNING: Invalid resource ID '%.*s'\n",path,lineno,idstrc,idstr);
        continue;
      }
      if (tool.tocc>=tool.toca) {
        int na=tool.toca+128;
        if (na>INT_MAX/sizeof(struct toc)) return -1;
        void *nv=realloc(tool.tocv,sizeof(struct toc)*na);
        if (!nv) return -1;
        tool.tocv=nv;
        tool.toca=na;
      }
      char *nv=malloc(rnamec+1);
      if (!nv) return -1;
      memcpy(nv,rname,rnamec);
      nv[rnamec]=0;
      struct toc *toc=tool.tocv+tool.tocc++;
      toc->tid=tid;
      toc->rid=rid;
      toc->name=nv;
      toc->namec=rnamec;
    }
  }
  return 0;
}

/* Acquire TOC.
 */
 
static int tool_toc_acquire() {
  tool.tocc=0;
  tool.toca=128;
  if (!(tool.tocv=malloc(sizeof(struct toc)*tool.toca))) return -1;
  if (!tool.tocpath) return 0;
  char *src=0;
  int srcc=file_read(&src,tool.tocpath);
  if (srcc<0) return 0;
  tool_toc_parse(src,srcc,tool.tocpath);
  free(src);
  return 0;
}

/* Parse flag_names.h
 */
 
static int tool_flags_parse(const char *src,int srcc,const char *path) {
  struct sr_decoder decoder={.v=src,.c=srcc};
  const char *line;
  int linec,lineno=1;
  for (;(linec=sr_decode_line(&line,&decoder))>0;lineno++) {
    if (linec<13) continue;
    if (memcmp(line,"#define FLAG_",13)) continue;
    int linep=13;
    const char *name=line+linep;
    int namec=0;
    while ((linep<linec)&&((unsigned char)line[linep++]>0x20)) namec++;
    while ((linep<linec)&&((unsigned char)line[linep]<=0x20)) linep++;
    const char *fidsrc=line+linep;
    int fidsrcc=0;
    while ((linep<linec)&&((unsigned char)line[linep]>0x20)&&(line[linep]!='/')) { linep++; fidsrcc++; }
    int fid;
    if ((sr_int_eval(&fid,fidsrc,fidsrcc)<2)||(fid<0)||(fid>0xff)) {
      fprintf(stderr,"%s:%d: Invalid flag id '%.*s'\n",path,lineno,fidsrcc,fidsrc);
      continue;
    }
    if (tool.flagc>=tool.flaga) {
      fprintf(stderr,"%s:%d: Too many flags. Ignoring '%.*s'=%d\n",path,lineno,namec,name,fid);
      continue;
    }
    struct flag *flag=tool.flagv+tool.flagc++;
    if (!(flag->name=malloc(namec+1))) { tool.flagc--; return -1; }
    memcpy(flag->name,name,namec);
    flag->name[namec]=0;
    flag->namec=namec;
    flag->fid=fid;
  }
  return 0;
}

/* Acquire flags.
 */
 
static int tool_flags_acquire() {
  tool.flagc=0;
  tool.flaga=256;
  if (!(tool.flagv=malloc(sizeof(struct flag)*tool.flaga))) return -1;
  const char *path="src/game/flag_names.h";
  char *src=0;
  int srcc=file_read(&src,path);
  if (srcc<0) return 0;
  tool_flags_parse(src,srcc,path);
  free(src);
  return 0;
}

/* tid
 */
 
int tool_eval_tid(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<1) return -1;
  #define _(tag) if ((srcc==sizeof(#tag)-1)&&!memcmp(src,#tag,srcc)) return EGG_TID_##tag;
  EGG_TID_FOR_EACH
  EGG_TID_FOR_EACH_CUSTOM
  #undef _
  int v;
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>0)&&(v<=0xff)) return v;
  return -1;
}

/* rid
 */
 
int tool_eval_rid(const char *src,int srcc,int tid) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<1) return -1;
  // Check numbers first; they're less painful.
  int v;
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>=0)&&(v<=0xffff)) return v;
  if (!tool.toca) tool_toc_acquire();
  const struct toc *toc=tool.tocv;
  int i=tool.tocc;
  for (;i-->0;toc++) {
    if (toc->tid!=tid) continue;
    if (toc->namec!=srcc) continue;
    if (memcmp(toc->name,src,srcc)) continue;
    return toc->rid;
  }
  return -1;
}

/* flag
 */
 
int tool_eval_flag(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  // Allowing integers for consistency, and because it's easy. But why did they bother with the "flag:" prefix?
  int v;
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>=0)&&(v<=0xff)) return v;
  if (!tool.flaga) tool_flags_acquire();
  const struct flag *flag=tool.flagv;
  int i=tool.flagc;
  for (;i-->0;flag++) {
    if (flag->namec!=srcc) continue;
    if (memcmp(flag->name,src,srcc)) continue;
    return flag->fid;
  }
  return -1;
}

/* Compile generic parameter.
 */
 
static int tool_compile_command_param(struct sr_encoder *dst,const char *src,int srcc,const char *path,int lineno) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!srcc) return -1;

  /* Hexadecimal integers with an even digit count emit the byte count indicated.
   * These are essentially hex dumps, and can be arbitrarily long.
   */
  if ((srcc>2)&&!(srcc&1)&&(src[0]=='0')&&((src[1]=='x')||(src[1]=='X'))) {
    int srcp=2;
    for (;srcp<srcc;srcp+=2) {
      int hi=sr_digit_eval(src[srcp]);
      int lo=sr_digit_eval(src[srcp+1]);
      if ((hi<0)||(hi>15)||(lo<0)||(lo>15)) {
        fprintf(stderr,"%s:%d: Unexpected digits '%.2s' in hexadecimal constant.\n",path,lineno,src+srcp);
        return -2;
      }
      if (sr_encode_u8(dst,(hi<<4)|lo)<0) return -1;
    }
    return 0;
  }
  
  /* "(uSIZE)N" are integers of explicit size: 8, 16, 24, 32
   */
  if (src[0]=='(') {
    int len;
         if ((srcc>=4)&&!memcmp(src,"(u8)",4)) { src+=4; srcc-=4; len=1; }
    else if ((srcc>=5)&&!memcmp(src,"(u16)",5)) { src+=5; srcc-=5; len=2; }
    else if ((srcc>=5)&&!memcmp(src,"(u24)",5)) { src+=5; srcc-=5; len=3; }
    else if ((srcc>=5)&&!memcmp(src,"(u32)",5)) { src+=5; srcc-=5; len=4; }
    else return -1;
    int v;
    if (sr_int_eval(&v,src,srcc)<2) return -1;
    if (sr_encode_intbe(dst,v,len)<0) return -1;
    return 0;
  }
  
  /* Plain integers in 0..255 emit one byte.
   * Anything outside that range, you have to tell us the length.
   */
  int v;
  if ((sr_int_eval(&v,src,srcc)>=2)&&(v>=0)&&(v<=0xff)) {
    if (sr_encode_u8(dst,v)<0) return -1;
    return 0;
  }
  
  /* "@X,Y[,W,H]" emits two or four bytes as if you had written separate args "X Y [W H]".
   * We'll generalize further and not insist on there being exactly 2 or 4.
   * In fact "@" alone will pass here ok and produce nothing.
   */
  if (src[0]=='@') {
    int srcp=1;
    while (srcp<srcc) {
      if (src[srcp]==',') { srcp++; continue; }
      const char *token=src+srcp;
      int tokenc=0;
      while ((srcp<srcc)&&(src[srcp++]!=',')) tokenc++;
      if ((sr_int_eval(&v,token,tokenc)<2)||(v<0)||(v>0xff)) {
        fprintf(stderr,"%s:%d: Expected integer in 0..255, found '%.*s'\n",path,lineno,tokenc,token);
        return -2;
      }
      if (sr_encode_u8(dst,v)<0) return -1;
    }
    return 0;
  }
  
  /* String tokens evaluate: Double-quote only.
   */
  if ((srcc>=2)&&(src[0]=='"')) {
    for (;;) {
      int err=sr_string_eval(((char*)dst->v)+dst->c,dst->a-dst->c,src,srcc);
      if (err<0) {
        fprintf(stderr,"%s:%d: Malformed string token\n",path,lineno);
        return -2;
      }
      if (dst->c<=dst->a-err) {
        dst->c+=err;
        break;
      }
      if (sr_encoder_require(dst,err)<0) return -1;
    }
    return 0;
  }
  
  /* "flag:NAME" => Flag ID in one byte.
   */
  if ((srcc>=5)&&!memcmp(src,"flag:",5)) {
    int fid=tool_eval_flag(src+5,srcc-5);
    if ((fid<0)||(fid>0xff)) {
      fprintf(stderr,"%s:%d: Invalid flag '%.*s'\n",path,lineno,srcc,src);
      return -2;
    }
    if (sr_encode_u8(dst,fid)<0) return -1;
    return 0;
  }
  
  /* "TYPE:NAME" => Resource ID in two bytes.
   */
  int sepp=0; for (;sepp<srcc;sepp++) {
    if (src[sepp]==':') {
      int tid=tool_eval_tid(src,sepp);
      if (tid<1) {
        fprintf(stderr,"%s:%d: Unknown resource type '%.*s'\n",path,lineno,sepp,src);
        return -2;
      }
      int rid=tool_eval_rid(src+sepp+1,srcc-sepp-1,tid);
      if (rid<1) {
        fprintf(stderr,"%s:%d: Failed to locate resource '%.*s'\n",path,lineno,srcc,src);
        return -2;
      }
      if (sr_encode_intbe(dst,rid,2)<0) return -1;
      return 0;
    }
  }
  
  fprintf(stderr,"%s:%d: Unexpected token '%.*s'\n",path,lineno,srcc,src);
  return -2;
}

/* Evaluate opcode.
 */
 
static int tool_compile_command_opcode(const char *src,int srcc,const struct tool_compile_command *commandv) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (!srcc) return -1;
  int opcode;
  if ((sr_int_eval(&opcode,src,srcc)>=2)&&(opcode>0)&&(opcode<=0xff)) return opcode;
  if (commandv) {
    for (;commandv->opcode;commandv++) {
      if (memcmp(commandv->name,src,srcc)) continue;
      if (commandv->name[srcc]) continue;
      return commandv->opcode;
    }
  }
  return -1;
}

/* Compile one command generically (map or sprite).
 */
 
int tool_compile_command(
  struct sr_encoder *dst,
  const char *src,int srcc,
  const struct tool_compile_command *commandv,
  int (*eval_extra)(struct sr_encoder *dst,const char *token,int tokenc),
  const char *path,int lineno
) {
  if (!dst||!commandv) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp>=srcc) return 0; // Empty lines are fine, they don't produce any output.
  const char *kw=src+srcp;
  int kwc=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) kwc++;
  int opcode=tool_compile_command_opcode(kw,kwc,commandv);
  if (opcode<0) {
    fprintf(stderr,"%s:%d: Unknown command '%.*s'\n",path,lineno,kwc,kw);
    return -2;
  }
  if (sr_encode_u8(dst,opcode)<0) return -1;
  if ((opcode>=0xc0)&&(opcode<=0xdf)) { // Explicit length. Emit placeholder.
    if (sr_encode_u8(dst,0)<0) return -1;
  }
  int dstargp=dst->c;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    const char *token=src+srcp;
    int tokenc=0;
    if (src[srcp]=='"') {
      if ((tokenc=sr_string_measure(src+srcp,srcc-srcp,0))<2) {
        fprintf(stderr,"%s:%d: Malformed string token.\n",path,lineno);
        return -2;
      }
      srcp+=tokenc;
    } else {
      while ((srcp<srcc)&&((unsigned char)src[srcp++]>0x20)) tokenc++;
    }
    if (eval_extra) {
      int err=eval_extra(dst,token,tokenc);
      if (err<0) {
        if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error at token '%.*s'\n",path,lineno,tokenc,token);
        return -2;
      }
      if (err>0) continue;
    }
    int err=tool_compile_command_param(dst,token,tokenc,path,lineno);
    if (err<0) {
      if (err!=-2) fprintf(stderr,"%s:%d: Unspecified error at token '%.*s'\n",path,lineno,tokenc,token);
      return -2;
    }
  }
  int arglen=dst->c-dstargp;
  if (arglen<0) return -1;
  if ((opcode>=0xc0)&&(opcode<=0xdf)) {
    if (arglen>0xff) {
      fprintf(stderr,"%s:%d: Body for '%.*s' command must be 0..255 bytes, found %d\n",path,lineno,kwc,kw,arglen);
      return -2;
    }
    ((uint8_t*)dst->v)[dstargp-1]=arglen;
  } else if ((opcode>=0xe0)&&(opcode<=0xff)) {
    // Anything goes.
  } else {
    int expectlen;
    switch (opcode&0xe0) {
      case 0x00: expectlen=0; break;
      case 0x20: expectlen=2; break;
      case 0x40: expectlen=4; break;
      case 0x60: expectlen=8; break;
      case 0x80: expectlen=12; break;
      case 0xa0: expectlen=16; break;
      default: return -1;
    }
    if (arglen!=expectlen) {
      fprintf(stderr,
        "%s:%d: '%.*s' command (0x%02x) must have %d bytes argument, found %d bytes\n",
        path,lineno,kwc,kw,opcode,expectlen,arglen
      );
      return -2;
    }
  }
  return 0;
}
