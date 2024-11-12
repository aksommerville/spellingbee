#include "game/bee.h"

/* Index is the little-endian bit index.
 * BEWARE: Our display logic assumes values will never exceed 99.
 */
static const struct entree {
  char name[7];
  uint8_t score;
  uint8_t price; // All kitchens have the same prices. Is that ok?
} entreev[32]={
  {"TEA    ", 3,  6},
  {"NOG    ", 4,  8},
  {"EGG    ", 5, 10},
  {"RYE    ", 6, 12},
  {"FLAN   ", 7, 14},
  {"HAM    ", 8, 16},
  {"OX     ", 9, 18},
  {"CAKE   ",10, 19},
  {"ZA     ",11, 20},
  {"MELON  ",12, 21},
  {"ZITI   ",13, 22},
  {"BACON  ",14, 23},
  {"CURRY  ",15, 24},
  {"SNAILS ",16, 25},
  {"GRAVY  ",17, 26},
  {"TOMATO ",18, 26},
  {"TAHINI ",19, 27},
  {"CREPES ",20, 27},
  {"HAGGIS ",21, 28},
  {"PEPPER ",22, 28},
  {"OXTAIL ",23, 29},
  {"COFFEE ",24, 29},
  {"WAFFLE ",25, 30},
  {"SQUIDS ",26, 30},
  {"PIZZA  ",30, 35},
  {"QUOKKA ",33, 36},
  {"LASAGNA",58, 40},
  {"TREACLE",59, 41},
  {"GUMDROP",63, 45},
  {"YOGHURT",64, 46},
  {"PRETZEL",68, 48},
  {"QUETZAL",75, 50},
};

struct modal_kitchen {
  struct modal hdr;
  uint32_t entrees;
  int entreec;
  int texid_text;
  int textw,texth;
  int dstx,dsty;
  int focusx,focusy; // Place us near this, in screen coords. (presumably the cook sprite)
  double animclock;
  int animframe;
  int sely;
};

#define MODAL ((struct modal_kitchen*)modal)

/* Delete.
 */
 
static void _kitchen_del(struct modal *modal) {
  egg_texture_del(MODAL->texid_text);
}

/* Init.
 */
 
static int _kitchen_init(struct modal *modal) {
  modal->opaque=0;
  return 0;
}

/* Move cursor.
 */
 
static void kitchen_move(struct modal *modal,int d) {
  //TODO sound effect
  MODAL->sely+=d;
  if (MODAL->sely<0) MODAL->sely=MODAL->entreec;
  else if (MODAL->sely>MODAL->entreec) MODAL->sely=0;
}

/* Activate.
 */
 
static void kitchen_activate(struct modal *modal) {
  
  if (!MODAL->sely) {
    //TODO sound effect for "Exit"
    modal_pop(modal);
    return;
  }
  
  if ((MODAL->sely>0)&&(MODAL->sely<=MODAL->entreec)) {
    const struct entree *entree=entreev;
    int i=32,q=MODAL->sely;
    uint32_t mask=MODAL->entrees;
    for (;i-->0;entree++,mask>>=1) {
      if (!(mask&1)) continue;
      if (!--q) {
        if (entree->price>g.gold) {
          //TODO rejection sound effect
          return;
        }
        g.gold-=entree->price;
        if ((g.hp+=entree->score)>100) g.hp=100;
        g.world.status_bar_dirty=1;
        //TODO purchase sound effect
        modal_pop(modal);
        return;
      }
    }
  }
}

/* Input.
 */
 
static void _kitchen_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) kitchen_move(modal,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) kitchen_move(modal,1);
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    kitchen_activate(modal);
    return; // activate may delete us
  }
  if ((input&EGG_BTN_WEST)&&!(pvinput&EGG_BTN_WEST)) {
    modal_pop(modal);
    return;
  }
}

/* Update.
 */
 
static void _kitchen_update(struct modal *modal,double elapsed) {
  if ((MODAL->animclock-=elapsed)<=0.0) {
    MODAL->animclock+=0.250;
    if (++(MODAL->animframe)>=2) MODAL->animframe=0;
  }
}

/* Render.
 */
 
static void _kitchen_render(struct modal *modal) {
  // (texid_text) contains the background, static text, and options text. Everything but the cursor.
  graf_draw_decal(&g.graf,MODAL->texid_text,MODAL->dstx,MODAL->dsty,0,0,MODAL->textw,MODAL->texth,0);
  int cursorx=MODAL->dstx+7;
  int cursory=MODAL->dsty+15+MODAL->sely*9;
  graf_set_tint(&g.graf,MODAL->animframe?0xc0c0c0ff:0xffffffff);
  graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_tiles),cursorx,cursory,0x11,0);
  graf_set_tint(&g.graf,0);
}

/* Type definition.
 */
 
const struct modal_type modal_type_kitchen={
  .name="kitchen",
  .objlen=sizeof(struct modal_kitchen),
  .del=_kitchen_del,
  .init=_kitchen_init,
  .input=_kitchen_input,
  .update=_kitchen_update,
  .render=_kitchen_render,
};

/* Calculate layout for new modal.
 */
 
static void modal_kitchen_layout(struct modal *modal) {
  const int ymargin=3; // inside box
  const int xmargin=3; // inside box
  const int xspace=10; // outside box
  const int yspace=10; // bottom edge
  const int indent=10; // left column
  const int scorex=60;
  const int pricex=80;
  
  // First, how many entrees? ie how many bits set in (entrees)?
  uint32_t q=MODAL->entrees;
  for (;q;q>>=1) if (q&1) MODAL->entreec++;
  
  // Above the entrees there's a one-line prompt and one dummy option to exit.
  int rowc=2+MODAL->entreec;
  
  // Determine the dialogue box's bounds and allocate it.
  int lineh=font_get_line_height(g.font);
  int h=lineh*rowc+ymargin*2;
  int w=g.fbw; // Limit; we'll trim after.
  int stride=w<<2;
  uint32_t *image=malloc(stride*h);
  if (!image) return;
  
  // Fill image initially with nearly-opaque black.
  int i=w*h;
  uint32_t *v=image;
  for (;i-->0;v++) *v=0xc0000000;
  
  // Top two lines are static. We assume that the prompt is wider than any other line.
  int promptw=font_render_string(image,w,h,stride,xmargin,ymargin,g.font,"Eat something to recover HP.",-1,0xffffffff);
  int clipw=xmargin+promptw+xmargin;
  if (clipw<w) w=clipw;
  font_render_string(image,w,h,stride,xmargin+indent,ymargin+lineh,g.font,"Exit",-1,0xff0000ff);
  
  // Write the name, score, and price for each entree.
  int row=2;
  const struct entree *entree=entreev;
  for (i=0;i<32;i++,entree++) {
    if (!(MODAL->entrees&(1<<i))) continue;
    
    font_render_string(image,w,h,stride,xmargin+indent,ymargin+lineh*row,g.font,entree->name,7,0x00ffffff);
    
    char tmp[8];
    int tmpc=0;
    tmp[tmpc++]='+';
    if (entree->score>=10) tmp[tmpc++]='0'+entree->score/10;
    tmp[tmpc++]='0'+entree->score%10;
    font_render_string(image,w,h,stride,xmargin+scorex,ymargin+lineh*row,g.font,tmp,tmpc,0xffff00ff);
    
    tmpc=0;
    tmp[tmpc++]='$';
    if (entree->price>=10) tmp[tmpc++]='0'+entree->price/10;
    tmp[tmpc++]='0'+entree->price%10;
    font_render_string(image,w,h,stride,xmargin+pricex,ymargin+lineh*row,g.font,tmp,tmpc,0x00ff00ff);
    
    row++;
  }
  
  MODAL->texid_text=egg_texture_new();
  egg_texture_load_raw(MODAL->texid_text,EGG_TEX_FMT_RGBA,w,h,stride,image,stride*h);
  free(image);
  MODAL->textw=w;
  MODAL->texth=h;
  
  /* Ideal horizontal position is centered on the speaker.
   */
  MODAL->dstx=MODAL->focusx-(MODAL->textw>>1);
  if (MODAL->dstx<0) MODAL->dstx=0;
  else if (MODAL->dstx+MODAL->textw>g.fbw) MODAL->dstx=g.fbw-MODAL->textw;
  
  /* Ideal horiztonal position is half a tile above the speaker.
   * We don't want that if it overlaps the status bar.
   * Next best is half a tile below the speaker (and if that goes OOB, clamp to bottom of screen).
   */
  MODAL->dsty=MODAL->focusy-(TILESIZE>>1)-MODAL->texth-1;
  if (MODAL->dsty<STATUS_BAR_HEIGHT) {
    MODAL->dsty=MODAL->focusy+(TILESIZE>>1)+1;
    if (MODAL->dsty+MODAL->texth>g.fbh) MODAL->dsty=g.fbh-MODAL->texth;
  }
}

/* Push new modal.
 */
 
void modal_kitchen_begin(uint32_t entrees,int focusx,int focusy) {
  struct modal *modal=modal_spawn(&modal_type_kitchen);
  if (!modal) return;
  MODAL->entrees=entrees;
  MODAL->focusx=focusx;
  MODAL->focusy=focusy;
  modal_kitchen_layout(modal);
}
