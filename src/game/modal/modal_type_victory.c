/* modal_type_victory.c
 * Shows after the book modal, for the last book.
 */
 
#include "game/bee.h"

/* Scrolling text, independent of the rest of the scene.
 */
 
static const char *credits[]={
  "Spelling Bee",
  "",
  "32 Nev 202X",
  "",
  "Starring Dot Vine",
  "",
  "Code, graphics, music:",
  "AK Sommerville",
  "",
  "Beta testers:",
  "TODO Get beta tested",
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

//TODO I bet we'll want animation here too, eventually. Determine the big picture first.
 
static struct cinema_bit {
  double dur; // Must be nonzero, first zero terminates the list. Durations should add up to about 71, the length of the song.
  int16_t x,y,w,h;
  const char *text;
} cinema_bitv[]={
  {  1.0,  0,  0,  0,  0,""}, // First one must be empty, it doesn't get a proper welcome.
  {  6.0,  1,  1,125,111,"You collected all six overdue books!"},
  {  6.0,127,  1,176, 98,"Now we need to sit around quietly and pretend we had something to say, while the finale clock runs down."},//TODO
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
};

#define MODAL ((struct modal_victory*)modal)

/* Cleanup.
 */
 
static void _victory_del(struct modal *modal) {
  egg_texture_del(MODAL->texid_credits);
  egg_texture_del(MODAL->texid_dialogue);
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
  while (cinema_bitv[cinema_bitcount].dur>0.0) cinema_bitcount++;
  egg_play_song(RID_song_deeper_than_shovels_can_dig,1,0);
  return 0;
}

/* Input.
 */
 
static void _victory_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    //TODO Do we want it terminable?
    modal_pop(modal);
    modal_spawn(&modal_type_hello);
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
  if (MODAL->bitp<cinema_bitcount) {
    double end=cinema_bitv[MODAL->bitp].dur;
    if ((MODAL->bitclock+=elapsed)>=end) {
      MODAL->bitclock=0.0;
      MODAL->bitp++;
      victory_replace_dialogue(modal);
    }
  }
}

/* Render.
 */
 
static void _victory_render(struct modal *modal) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x000000ff);
  
  // Action scene on top, 360x139
  if (MODAL->bitp<cinema_bitcount) {
    const struct cinema_bit *bit=cinema_bitv+MODAL->bitp;
    int16_t dstx=(g.fbw>>1)-(bit->w>>1);
    int16_t dsty=((g.fbh-50)>>1)-(bit->h>>1);
    graf_draw_decal(&g.graf,texcache_get_image(&g.texcache,RID_image_victory),dstx,dsty,bit->x,bit->y,bit->w,bit->h,0);
    int alpha=0;
    if (MODAL->bitclock<BIT_FADE_IN_TIME) {
      alpha=(int)((1.0-MODAL->bitclock/BIT_FADE_IN_TIME)*256.0);
    } else if (MODAL->bitclock>bit->dur-BIT_FADE_OUT_TIME) {
      alpha=(int)((1.0-(bit->dur-MODAL->bitclock)/BIT_FADE_OUT_TIME)*256.0);
    }
    if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
    if (alpha) {
      graf_draw_rect(&g.graf,dstx,dsty,bit->w,bit->h,0x00000000|alpha);
    }
  }
  
  // Subtitles lower left.
  if (MODAL->texid_dialogue) {
    int16_t dstx=5;
    int16_t dsty=g.fbh-45;
    graf_draw_decal(&g.graf,MODAL->texid_dialogue,dstx,dsty,0,0,MODAL->dialoguew,MODAL->dialogueh,0);
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
  
  if (0) {//XXX draw the clock, seconds only
    int texid=texcache_get_image(&g.texcache,RID_image_tiles);
    int seconds=(int)MODAL->clock;
    graf_draw_tile(&g.graf,texid,100,100,'0'+(seconds/100)%10,0);
    graf_draw_tile(&g.graf,texid,108,100,'0'+(seconds/10 )%10,0);
    graf_draw_tile(&g.graf,texid,116,100,'0'+(seconds    )%10,0);
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
