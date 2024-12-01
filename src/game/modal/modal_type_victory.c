/* modal_type_victory.c
 * Shows after the book modal, for the last book.
 */
 
#include "game/bee.h"
#include "game/flag_names.h"

/* Scrolling text, independent of the rest of the scene.
 */
 
static const char *credits[]={
  "Spelling Bee",
  "",
  "December 2024",
  "",
  "Starring Dot Vine",
  "",
  "Code, graphics, music:",
  "AK Sommerville",
  "",
  "Beta testers:",
  "Your Name Here!",//TODO
  "",
  "Thanks:",
  "Curtis Robinson",
  "GDEX Staff",
  "COGG",
  "",
  "Play more games:",
  "http://aksommerville.com",
  "",
  "Thanks for playing!",
  "-AK and Dot",
0};

/* Graphic panel from image:victory, and a text blurb, that display together.
 */
 
#define BIT_FADE_IN_TIME 0.500
#define BIT_FADE_OUT_TIME 0.500

static void cinema_dot_reading(struct modal *modal,double t,int texid);
static void cinema_stats(struct modal *modal,double t,int texid);
 
static struct cinema_bit {
  double dur; // Must be nonzero, first zero terminates the list. Durations should add up to about 71, the length of the song.
  int16_t x,y,w,h; // Upper image source. Drawn before (cb) if present.
  const char *text;
  void (*cb)(struct modal *modal,double t,int texid); // Generic render for upper part. Caller manages fade in/out.
} cinema_bitv[]={
  {  1.0,  0,  0,  0,  0,""}, // First one must be empty, it doesn't get a proper welcome.
  {  8.0,  1,  1,358, 81,"A great crowd assembled to hear the king's proclamation."},
  {  8.0,  1, 83,177,111,"With exemplary courage and wisdom, Dot Vine has saved the kingdom once again!"},
  {  7.0,0,0,0,0,"",cinema_dot_reading},
  {  5.0,293, 83, 66, 56,"Quit goofing off and fetch those overdue books!"},
  {  5.0,293,140, 66, 54,"I already finished!"},
  {  4.0,205,195,154, 36,"I fought off a swarm of bees..."},
  {  4.0,205,232,154, 36,"...I braved the Endurance Gauntlet..."},
  {  4.0,205,269,154, 36,"...I pacified the dreaded Sixclops..."},
  {  4.0,205,306,154, 36,"...I burrowed through the cemetery..."},
  {  4.0,205,343,154, 36,"...I outwitted the Scholar..."},
  {  4.0,205,380,154, 36,"...and I dethroned the evil Queen!"},
  {  5.0, 62,328,142, 88,"Oh! You've had a busy day. Let's review your performance."},
  { -1.0,0,0,0,0,"",cinema_stats},
{0}};
static int cinema_bitcount=0; // auto populates at first init

/* Object definition.
 */
 
struct modal_victory {
  struct modal hdr;
  double clock; // Absolute time, counts forever.
  int texid_credits;
  int creditsw,creditsh;
  int bitp;
  double bitclock; // Counts up
  int texid_dialogue;
  int dialoguew,dialogueh;
  int texid_stats;
  int statsw,statsh;
  int narrative;
};

#define MODAL ((struct modal_victory*)modal)

/* Cleanup.
 */
 
static void _victory_del(struct modal *modal) {
  egg_texture_del(MODAL->texid_credits);
  egg_texture_del(MODAL->texid_dialogue);
  egg_texture_del(MODAL->texid_stats);
}

/* Draw the credits texture.
 */
 
static int victory_generate_credits(struct modal *modal) {

  /* Allocate some things, take some measurements...
   */
  if ((MODAL->texid_credits=egg_texture_new())<=0) return -1;
  int lineh=font_get_line_height(g.font);
  int linec=0;
  int texw=0;
  while (credits[linec]) {
    int linew=font_measure_line(g.font,credits[linec],-1);
    if (linew>texw) texw=linew;
    linec++;
  }
  int texh=lineh*linec;
  if ((texw<1)||(texh<1)) return -1;
  texw+=2; texh+=2; // Leave a one-pixel margin on all sides.
  int texstride=texw<<2;
  uint8_t *rgba=calloc(texstride,texh);
  if (!rgba) return -1;
  
  /* Render to our temporary buffer line by line.
   */
  int i=0,y=1;
  for (;i<linec;i++,y+=lineh) {
    const char *src=credits[i];
    if (!src[0]) continue;
    int linew=font_measure_line(g.font,src,-1);
    int dstx=(texw>>1)-(linew>>1);
    font_render_string(rgba,texw,texh,texstride,dstx,y,g.font,src,-1,0xffffffff);
  }
  
  int err=egg_texture_load_raw(MODAL->texid_credits,EGG_TEX_FMT_RGBA,texw,texh,texstride,rgba,texstride*texh);
  MODAL->creditsw=texw;
  MODAL->creditsh=texh;
  free(rgba);
  return 0;
}

/* Init.
 */
 
static int _victory_init(struct modal *modal) {
  modal->opaque=1;
  if (victory_generate_credits(modal)<0) return -1;
  while ((cinema_bitv[cinema_bitcount].dur>0.25)||(cinema_bitv[cinema_bitcount].dur<-0.25)) cinema_bitcount++;
  egg_play_song(RID_song_deeper_than_shovels_can_dig,1,0);
  MODAL->narrative=1;
  return 0;
}

/* Input.
 */
 
static void _victory_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    //TODO Do we want it terminable?
    modal_pop(modal);
    modal_spawn_if_empty(&modal_type_hello);
    return;
  }
}

/* Replace the dialogue texture for current cinema bit.
 * Note that this does not happen for the first bit, that must have no dialogue.
 */
 
static void victory_replace_dialogue(struct modal *modal) {
  egg_texture_del(MODAL->texid_dialogue);
  MODAL->texid_dialogue=0;
  if (MODAL->bitp>=cinema_bitcount) return;
  const char *text=cinema_bitv[MODAL->bitp].text;
  if (!text||!text[0]) return;
  int hlimit=40;
  int wlimit=g.fbw-MODAL->creditsw-20;
  MODAL->texid_dialogue=font_tex_multiline(g.font,text,-1,wlimit,hlimit,0xffffffff);
  egg_texture_get_status(&MODAL->dialoguew,&MODAL->dialogueh,MODAL->texid_dialogue);
}

/* Update.
 */
 
static void _victory_update(struct modal *modal,double elapsed) {
  MODAL->clock+=elapsed;
  if (MODAL->narrative) {
    if (MODAL->bitp<cinema_bitcount) {
      double end=cinema_bitv[MODAL->bitp].dur;
      MODAL->bitclock+=elapsed;
      if ((end>0.0)&&(MODAL->bitclock>=end)) {
        MODAL->bitclock=0.0;
        MODAL->bitp++;
        victory_replace_dialogue(modal);
      }
    }
  } else if (MODAL->bitp!=cinema_bitcount-1) {
    MODAL->bitp=cinema_bitcount-1;
    victory_replace_dialogue(modal);
  } else {
    MODAL->bitclock+=elapsed;
  }
}

/* Cinema: Dot is reading on a stool and Goody calls her name.
 */
 
static void cinema_dot_reading(struct modal *modal,double t,int texid) {

  // One of two images on the right, same size.
  int16_t srcx=(t>=0.600)?103:1;
  int16_t srcy=195;
  int16_t w=101;
  int16_t h=132;
  int16_t dstx=200;
  int16_t dsty=0;
  graf_draw_decal(&g.graf,texid,dstx,dsty,srcx,srcy,w,h,0);

  // Word bubble, last half.
  if (t>=0.600) {
    graf_draw_decal(&g.graf,texid,20,10,179,83,113,77,0);
  }
}

/* Represent data points for stats report.
 */

static int victory_repr_int(char *dst,int dsta,int v,struct saved_game *save) {
  if (dsta<10) return 0;
  int dstc=0;
  if (v>=1000000) dst[dstc++]='0'+(v/1000000)%10;
  if (v>= 100000) dst[dstc++]='0'+(v/ 100000)%10;
  if (v>=  10000) dst[dstc++]='0'+(v/  10000)%10;
  if (v>=   1000) dst[dstc++]='0'+(v/   1000)%10;
  if (v>=    100) dst[dstc++]='0'+(v/    100)%10;
  if (v>=     10) dst[dstc++]='0'+(v/     10)%10;
  dst[dstc++]='0'+v%10;
  return dstc;
}
 
static int victory_repr_main(char *dst,int dsta,int dummy,struct saved_game *save) {
  int numer=0,denom=0;
  #define F(tag) if (save->flags[FLAG_##tag>>3]&(1<<(FLAG_##tag&7))) numer++; denom++;
  F(book1)
  F(book2)
  F(book3)
  F(book4)
  F(book5)
  F(book6)
  #undef F
  int dstc=victory_repr_int(dst,dsta,numer,save);
  if (dstc<dsta) dst[dstc++]='/';
  dstc+=victory_repr_int(dst+dstc,dsta-dstc,denom,save);
  return dstc;
}

static int victory_repr_side(char *dst,int dsta,int dummy,struct saved_game *save) {
  int numer=0,denom=0;
  #define F(tag) if (save->flags[FLAG_##tag>>3]&(1<<(FLAG_##tag&7))) numer++; denom++;
  F(graverob5)
  F(flower_done)
  F(englishprof)
  #undef F
  int dstc=victory_repr_int(dst,dsta,numer,save);
  if (dstc<dsta) dst[dstc++]='/';
  dstc+=victory_repr_int(dst+dstc,dsta-dstc,denom,save);
  return dstc;
}

static int victory_repr_time(char *dst,int dsta,double sf,struct saved_game *save) {
  if (dsta<8) return 0;
  int s=(int)sf;
  int m=s/60; s%=60;
  int h=m/60; m%=60;
  if (h>99) h=m=s=99;
  int dstc=0;
  if (h>0) {
    if (h>=10) dst[dstc++]='0'+h/10;
    dst[dstc++]='0'+h%10;
    dst[dstc++]=':';
  }
  dst[dstc++]='0'+m/10;
  dst[dstc++]='0'+m%10;
  dst[dstc++]=':';
  dst[dstc++]='0'+s/10;
  dst[dstc++]='0'+s%10;
  return dstc;
}

static int victory_repr_word(char *dst,int dsta,const char *src,struct saved_game *save) {
  int srcc=0;
  while ((srcc<7)&&src[srcc]) srcc++;
  if (srcc<=dsta) memcpy(dst,src,srcc);
  return srcc;
}

/* Generate stats texture.
 * This will contain its background and everything.
 */
 
static void victory_generate_stats(struct modal *modal) {
  int rowh=font_get_line_height(g.font)+1;
  MODAL->statsw=140;
  MODAL->statsh=rowh*8;
  int stride=MODAL->statsw<<2;
  uint8_t *rgba=calloc(stride,MODAL->statsh);
  if (!rgba) return;
  
  char encoded[256];
  int encodedc=egg_store_get(encoded,sizeof(encoded),"save",4);
  if ((encodedc<0)||(encodedc>sizeof(encoded))) encodedc=0;
  struct saved_game save;
  saved_game_decode(&save,encoded,encodedc);
  
  int dsty=0;
  #define ROW(label,repr,arg) { \
    font_render_string(rgba,MODAL->statsw,MODAL->statsh,stride,0,dsty,g.font,label,-1,0xffff00ff); \
    char tmp[256]; \
    int tmpc=repr(tmp,sizeof(tmp),arg,&save); \
    if ((tmpc>0)&&(tmpc<=sizeof(tmp))) { \
      font_render_string(rgba,MODAL->statsw,MODAL->statsh,stride,70,dsty,g.font,tmp,tmpc,0xffffffff); \
    } \
    dsty+=rowh; \
  }
  ROW("Main quest",victory_repr_main,0)
  ROW("Side quests",victory_repr_side,0)
  ROW("Experience",victory_repr_int,save.xp)
  ROW("Steps",victory_repr_int,save.stepc)
  ROW("Time",victory_repr_time,save.playtime)
  ROW("Best word",victory_repr_word,save.bestword)
  ROW("Score",victory_repr_int,save.bestscore)
  #undef ROW
  
  MODAL->texid_stats=egg_texture_new();
  egg_texture_load_raw(MODAL->texid_stats,EGG_TEX_FMT_RGBA,MODAL->statsw,MODAL->statsh,stride,rgba,stride*MODAL->statsh);
  free(rgba);
}

/* Cinema: Show global stats.
 */
 
static void cinema_stats(struct modal *modal,double t,int texid) {
  if (!MODAL->texid_stats) victory_generate_stats(modal);
  int16_t dstx=(g.fbw>>1)-(MODAL->statsw>>1);
  int16_t dsty=(139>>1)-(MODAL->statsh>>1);
  graf_draw_decal(&g.graf,MODAL->texid_stats,dstx,dsty,0,0,MODAL->statsw,MODAL->statsh,0);
}

/* Render.
 */
 
static void _victory_render(struct modal *modal) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x000000ff);
  int texid=texcache_get_image(&g.texcache,RID_image_victory);
  int alpha=0; // How much blackout.
  
  // Action scene on top, 360x139
  if (MODAL->bitp<cinema_bitcount) {
    const struct cinema_bit *bit=cinema_bitv+MODAL->bitp;
    if ((bit->w>0)&&(bit->h>0)) {
      int16_t dstx=(g.fbw>>1)-(bit->w>>1);
      int16_t dsty=((g.fbh-50)>>1)-(bit->h>>1);
      graf_draw_decal(&g.graf,texid,dstx,dsty,bit->x,bit->y,bit->w,bit->h,0);
    }
    if (bit->cb) {
      double t=MODAL->bitclock/bit->dur;
      bit->cb(modal,t,texid);
    }
    if (MODAL->bitclock<BIT_FADE_IN_TIME) {
      alpha=(int)((1.0-MODAL->bitclock/BIT_FADE_IN_TIME)*256.0);
    } else if ((bit->dur>0.0)&&(MODAL->bitclock>bit->dur-BIT_FADE_OUT_TIME)) {
      alpha=(int)((1.0-(bit->dur-MODAL->bitclock)/BIT_FADE_OUT_TIME)*256.0);
    }
    if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
    if (alpha) {
      graf_draw_rect(&g.graf,0,0,g.fbw,139,0x00000000|alpha);
    }
  }
  
  // Subtitles lower left, or "the end" image eventually.
  if (MODAL->texid_dialogue) {
    int16_t dstx=5;
    int16_t dsty=g.fbh-45;
    graf_draw_decal(&g.graf,MODAL->texid_dialogue,dstx,dsty,0,0,MODAL->dialoguew,MODAL->dialogueh,0);
    if (alpha) graf_draw_rect(&g.graf,dstx,dsty,MODAL->dialoguew,MODAL->dialogueh,0x00000000|alpha);
  } else if (MODAL->narrative&&(MODAL->bitp>=cinema_bitcount-1)) {
    int16_t srcx=179;
    int16_t srcy=161;
    int16_t w=98;
    int16_t h=33;
    int16_t dstx=((g.fbw-MODAL->creditsw)>>1)-(w>>1);
    int16_t dsty=g.fbh-23-(h>>1);
    graf_draw_decal(&g.graf,texid,dstx,dsty,srcx,srcy,w,h,0);
  }
  
  { // Scrolling credits in the lower right corner.
    int credw=MODAL->creditsw+10;
    int credh=50;
    int dstx=g.fbw-credw+5;
    int dsty=g.fbh-credh;
    int srcx=0;
    int cpw=MODAL->creditsw;
    int cph=credh;
    double range=MODAL->creditsh+credh; // Starts and finishes just offscreen.
    double starttime=3.0;
    double endtime=71.0;
    double t=(MODAL->clock-starttime)/(endtime-starttime);
    if (t<0.0) t=0.0; else if (t>1.0) t=1.0;
    int srcy=-credh+(int)(t*range);
    graf_draw_decal(&g.graf,MODAL->texid_credits,dstx,dsty,srcx,srcy,cpw,cph,0);
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_victory={
  .name="victory",
  .objlen=sizeof(struct modal_victory),
  .del=_victory_del,
  .init=_victory_init,
  .input=_victory_input,
  .update=_victory_update,
  .render=_victory_render,
};

/* Launch modal without narrative bits.
 */
 
void modal_victory_begin_narrativeless() {
  struct modal *modal=modal_spawn(&modal_type_victory);
  if (!modal) return;
  MODAL->narrative=0;
}
