#include "bee.h"

/* Default.
 */
 
void saved_game_default(struct saved_game *game) {
  memset(game,0,sizeof(struct saved_game)); // Most defaults are zero.
  game->hp=100;
  game->flags[0]|=2; // FLAG_one, index one, must be one.
}

/* Force valid.
 */

void saved_game_force_valid(struct saved_game *game) {
  #define RANGE(fldname,lo,hi) if (game->fldname<lo) game->fldname=lo; else if (game->fldname>hi) game->fldname=hi;
  RANGE(hp,1,100)
  RANGE(xp,0,32767)
  RANGE(gold,0,32767)
  game->inventory[0]=0;
  int i=1; for (;i<ITEM_COUNT;i++) RANGE(inventory[i],0,99)
  game->flags[0]=(game->flags[0]&0xfc)|0x02; // First two bits must be 0,1
  RANGE(gravep,0,255)
  RANGE(battlec,0,0xffffff)
  RANGE(wordc,0,0xffffff)
  RANGE(scoretotal,0,0xffffff)
  RANGE(bestscore,0,255)
  if (game->wordc>game->battlec) game->wordc=game->battlec;
  if (game->bestscore>game->scoretotal) game->bestscore=game->scoretotal;
  for (i=0;i<7;i++) {
    if ((game->bestword[i]>='A')&&(game->bestword[i]<='Z')) continue;
    if ((game->bestword[i]>='a')&&(game->bestword[i]<='z')) continue;
    memset(game->bestword+i,0,7-i);
    break;
  }
  RANGE(stepc,0,0xffffff)
  RANGE(flower_stepc,0,255)
  RANGE(bugspray,0,255)
  #undef RANGE
}

/* Validate.
 */
 
const char *saved_game_validate(const struct saved_game *game) {
  if (!game) return "null";
  #define RANGE(fldname,lo,hi) if ((game->fldname<lo)||(game->fldname>hi)) return #fldname;
  RANGE(hp,1,100)
  RANGE(xp,0,32767)
  RANGE(gold,0,32767)
  RANGE(inventory[0],0,0)
  int i=1; for (;i<ITEM_COUNT;i++) RANGE(inventory[i],0,99)
  if ((game->flags[0]&0x03)!=0x02) return "constant flags";
  RANGE(gravep,0,255)
  RANGE(battlec,0,0xffffff)
  RANGE(wordc,0,0xffffff)
  RANGE(scoretotal,0,0xffffff)
  RANGE(bestscore,0,255)
  if (game->wordc>game->battlec) return "wordc>battlec";
  if (game->bestscore>game->scoretotal) return "bestscore>scoretotal";
  int len=7;
  while (len&&!game->bestword[len]) len--;
  for (i=0;i<len;i++) {
    if ((game->bestword[i]>='A')&&(game->bestword[i]<='Z')) continue;
    if ((game->bestword[i]>='a')&&(game->bestword[i]<='z')) continue;
    return "bestword";
  }
  RANGE(stepc,0,0xffffff)
  RANGE(flower_stepc,0,255)
  RANGE(bugspray,0,255)
  #undef RANGE
  return 0;
}

/* Checksum.
 */
 
static uint32_t saved_game_checksum(const uint8_t *src,int srcc) {
  uint32_t checksum=0xc396a5e7;
  for (;srcc-->0;src++) {
    checksum=(checksum<<7)|(checksum>>25);
    checksum^=*src;
  }
  return checksum;
}

/* XOR filter, in place.
 */
 
static void saved_game_filter(uint8_t *v,int c) {
  int i=1; for (;i<c;i++) v[i]^=v[i-1];
}

static void saved_game_unfilter(uint8_t *v,int c) {
  int i=c; for (;i-->1;) v[i]^=v[i-1];
}

/* Base64ish encoding.
 * Basically base64 except:
 *  - Different alphabet: 0x23..0x5b => 0..56, 0x5d..0x63 => 57..63
 *  - No padding. Text length must be a multiple of 4, and binary length must be a multiple of 3.
 */
 
static int base64ish_encode(char *dst,int dsta,const uint8_t *src,int srcc) {
  if (srcc%3) return 0;
  int chunkc=srcc/3;
  int dstc=chunkc*4;
  if (dstc>dsta) return dstc;
  const char *alphabet="#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abc";
  int i=chunkc; for (;i-->0;dst+=4,src+=3) {
    dst[0]=alphabet[src[0]>>2];
    dst[1]=alphabet[((src[0]<<4)|(src[1]>>4))&0x3f];
    dst[2]=alphabet[((src[1]<<2)|(src[2]>>6))&0x3f];
    dst[3]=alphabet[src[2]&0x3f];
  }
  return dstc;
}

static int base64ish_decode(uint8_t *dst,int dsta,const char *src,int srcc) {
  if (srcc%4) return 0;
  int chunkc=srcc/4;
  int dstc=chunkc*3;
  if (dstc>dsta) return dstc;
  #define DECCHR(ch) ({ \
    int b; \
    if ((ch>=0x23)&&(ch<=0x5b)) b=ch-0x23; \
    else if ((ch>=0x5d)&&(ch<=0x63)) b=ch-0x5d+57; \
    else return 0; \
    b; \
  })
  int i=chunkc; for (;i-->0;dst+=3,src+=4) {
    uint8_t a=DECCHR(src[0]);
    uint8_t b=DECCHR(src[1]);
    uint8_t c=DECCHR(src[2]);
    uint8_t d=DECCHR(src[3]);
    dst[0]=(a<<2)|(b>>4);
    dst[1]=(b<<4)|(c>>2);
    dst[2]=(c<<6)|d;
  }
  #undef DECCHR
  return dstc;
}

/* Decode.
 */

int saved_game_decode(struct saved_game *game,const void *src,int srcc) {

  if (!game) return -1;
  saved_game_default(game);
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (((char*)src)[srcc]) srcc++; }

  /* Decode generically.
   */
  uint8_t bin[128]; // Currently can't exceed 78. We'll tolerate some additions.
  int binc=base64ish_decode(bin,sizeof(bin),src,srcc);
  if ((binc<37)||(binc>sizeof(bin))) return -1;
  saved_game_unfilter(bin,binc);
  
  /* Read the decoded binary field by field.
   */
  int binp=0;
  #define ABORT { saved_game_default(game); return -1; }
  #define RD32(lo,hi) ({ \
    if (binp>binc-4) ABORT \
    uint32_t v=(bin[binp]<<24)|(bin[binp+1]<<16)|(bin[binp+2]<<8)|bin[binp+3]; \
    binp+=4; \
    if ((v<lo)||(v>hi)) ABORT \
    v; \
  })
  #define RD24(lo,hi) ({ \
    if (binp>binc-3) ABORT \
    uint32_t v=(bin[binp]<<16)|(bin[binp+1]<<8)|bin[binp+2]; \
    binp+=3; \
    if ((v<lo)||(v>hi)) ABORT \
    v; \
  })
  #define RD16(lo,hi) ({ \
    if (binp>binc-2) ABORT \
    uint32_t v=(bin[binp]<<8)|bin[binp+1]; \
    binp+=2; \
    if ((v<lo)||(v>hi)) ABORT \
    v; \
  })
  #define RD8(lo,hi) ({ \
    if (binp>binc-1) ABORT \
    uint32_t v=bin[binp++]; \
    if ((v<lo)||(v>hi)) ABORT \
    v; \
  })
  uint32_t expectsum=RD32(0,UINT32_MAX);
  uint32_t actualsum=saved_game_checksum(bin+binp,binc-binp);
  if (expectsum!=actualsum) return -1;
  game->hp=RD8(1,100);
  game->xp=RD16(0,0xffff);
  game->gold=RD16(0,0xffff);
  game->gravep=RD8(0,0xff);
  game->playtime=RD24(0,0xffffff);
  game->battlec=RD24(0,0xffffff);
  game->wordc=RD24(0,0xffffff);
  game->scoretotal=RD24(0,0xffffff);
  game->bestscore=RD8(0,0xff);
  
  if (binp>binc-7) ABORT
  memcpy(game->bestword,bin+binp,7);
  binp+=7;
  
  game->stepc=RD24(0,0xffffff);
  game->flower_stepc=RD8(0,0xff);
  game->bugspray=RD8(0,0xff);
  
  int invc=RD8(0,0xff);
  if (binp>binc-invc) ABORT
  if (invc>=sizeof(game->inventory)) memcpy(game->inventory,bin+binp,sizeof(game->inventory));
  else memcpy(game->inventory,bin+binp,invc);
  binp+=invc;
  
  int flagc=RD8(0,0xff); // bytes
  if (binp>binc-flagc) ABORT
  if (flagc>sizeof(game->flags)) memcpy(game->flags,bin+binp,sizeof(game->flags));
  else memcpy(game->flags,bin+binp,flagc);
  binp+=flagc;
  
  // (binp) may be less than (binc) at this point due to padding. Don't worry about it.
  #undef ABORT
  #undef RD32
  #undef RD24
  #undef RD16
  #undef RD8
  
  if (0) {//XXX Log the decoded game.
    fprintf(stderr,"Decoded game...\n");
    #define _(name) fprintf(stderr,"  %s: %d\n",#name,game->name);
    _(hp)
    _(xp)
    _(gold)
    _(gravep)
    fprintf(stderr,"  playtime: %.0f\n",game->playtime);
    _(battlec)
    _(wordc)
    _(scoretotal)
    _(bestscore)
    fprintf(stderr,"  bestword: '%.7s'\n",game->bestword);
    _(stepc)
    _(flower_stepc)
    _(bugspray)
    fprintf(stderr,"  inventory:");
    int i=0; for (;i<sizeof(game->inventory);i++) fprintf(stderr," %d",game->inventory[i]);
    fprintf(stderr,"\n");
    fprintf(stderr,"  flags:");
    for (i=0;i<sizeof(game->flags);i++) fprintf(stderr," %02x",game->flags[i]);
    fprintf(stderr,"\n");
    #undef _
  }
  
  // We haven't validated everything. eg bestword format and all cross-field concerns are not checked yet.
  if (saved_game_validate(game)) {
    saved_game_default(game);
    return -1;
  }
  return 0;
}

/* Encode.
 */
 
int saved_game_encode(void *dst,int dsta,const struct saved_game *kgame) {
  if ((dsta<0)||(dsta&&!dst)||!kgame) return 0;
  struct saved_game game=*kgame;
  saved_game_force_valid(&game);
  
  /* Total output length can be determined in advance.
   * If it won't fit, get out fast.
   */
  int itemc=sizeof(game.inventory);
  while (itemc&&!game.inventory[itemc-1]) itemc--;
  int flagc=sizeof(game.flags);
  while (flagc&&!game.flags[flagc-1]) flagc--;
  int binc=37+itemc+flagc; // 37 bytes constant.
  switch (binc%3) {
    case 1: binc+=2; break;
    case 2: binc+=1; break;
  }
  int dstc=(binc*4)/3;
  if (dstc>dsta) return dstc;
  
  /* Assemble the plaintext binary in a separate buffer.
   */
  uint8_t bin[128];
  if (binc>sizeof(bin)) return 0; // Should not exceed 75.
  int binp=4; // Skip the checksum, we'll be back.
  #define VERBATIM(fldname,len) { memcpy(bin+binp,game.fldname,len); binp+=len; }
  #define WR32(fldname) { bin[binp++]=game.fldname>>24; bin[binp++]=game.fldname>>16; bin[binp++]=game.fldname>>8; bin[binp++]=game.fldname; }
  #define WR24(fldname) { bin[binp++]=game.fldname>>16; bin[binp++]=game.fldname>>8; bin[binp++]=game.fldname; }
  #define WR16(fldname) { bin[binp++]=game.fldname>>8; bin[binp++]=game.fldname; }
  #define WR8(fldname) { bin[binp++]=game.fldname; }
  
  WR8(hp)
  WR16(xp)
  WR16(gold)
  WR8(gravep)
  int playtimes=(int)game.playtime; if (playtimes<0) playtimes=0; else if (playtimes>0xffffff) playtimes=0xffffff;
  bin[binp++]=playtimes>>16; bin[binp++]=playtimes>>8; bin[binp++]=playtimes;
  WR24(battlec)
  WR24(wordc)
  WR24(scoretotal)
  WR8(bestscore)
  VERBATIM(bestword,7)
  WR24(stepc)
  WR8(flower_stepc)
  WR8(bugspray)
  bin[binp++]=itemc;
  VERBATIM(inventory,itemc)
  bin[binp++]=flagc;
  VERBATIM(flags,flagc)
  if (binp<binc) bin[binp++]=0;
  if (binp<binc) bin[binp++]=0;
  
  if (binp!=binc) {
    fprintf(stderr,
      "!!! Serious internal error !!!\n%s:%d: binp=%d binc=%d\nI must have made a mistake encoding saved games.\n",
      __FILE__,__LINE__,binp,binc
    );
    return 0;
  }
  
  #undef VERBATIM
  #undef WR32
  #undef WR24
  #undef WR8
  
  // Compute and encode checksum.
  int checksum=saved_game_checksum(bin+4,binc-4);
  bin[0]=checksum>>24;
  bin[1]=checksum>>16;
  bin[2]=checksum>>8;
  bin[3]=checksum;
  
  // Apply XOR filter.
  saved_game_filter(bin,binc);
  
  // And finally, base64 the current mess.
  return base64ish_encode(dst,dsta,bin,binc);
}
