/* modal_type_pause.c
 * Very similar to 'hello', but we appear above the game, interrupting play.
 */
 
#include "game/bee.h"

#define PAUSE_OPTION_RESUME 10
#define PAUSE_OPTION_MENU 11
#define PAUSE_OPTION_LIMIT 2

/* Object definition.
 */
 
struct modal_pause {
  struct modal hdr;
  struct pause_option {
    int enable;
    int index; // PAUSE_OPTION_*, also the strings index.
    int texid;
    int x,y,w,h; // Label position.
  } optionv[PAUSE_OPTION_LIMIT]; // NB index is the tab order, not necessarily the option id.
  int optionc;
  int selp;
  int16_t dstx,dsty,dstw,dsth; // Full box surrounding all options.
};

#define MODAL ((struct modal_pause*)modal)

/* Cleanup.
 */
 
static void _pause_del(struct modal *modal) {
  while (MODAL->optionc-->0) {
    egg_texture_del(MODAL->optionv[MODAL->optionc].texid);
  }
}

/* Add option.
 * Position is initially zero, you must set later.
 */
 
static int pause_add_option(struct modal *modal,int index,int enable) {
  if (MODAL->optionc>=PAUSE_OPTION_LIMIT) return -1;
  struct pause_option *option=MODAL->optionv+MODAL->optionc++;
  option->enable=enable?1:0;
  option->index=index;
  if ((option->texid=font_texres_oneline(g.font,RID_strings_hello,index,g.fbw,enable?0xffffffff:0x808080ff))<1) return -1;
  egg_texture_get_status(&option->w,&option->h,option->texid);
  return 0;
}

/* Init.
 */
 
static int _pause_init(struct modal *modal) {
  modal->opaque=0;
  
  /* Decide which options are available.
   */
  pause_add_option(modal,PAUSE_OPTION_RESUME,1);
  pause_add_option(modal,PAUSE_OPTION_MENU,1);
  
  /* Position options.
   * For now, pack and center vertically.
   * Horizontally, maintain a flush left edge, centering the widest.
   */
  int wmax=0,hsum=0,i=0;
  for (;i<MODAL->optionc;i++) {
    struct pause_option *option=MODAL->optionv+i;
    if (option->w>wmax) wmax=option->w;
    hsum+=option->h;
  }
  int x=(g.fbw>>1)-(wmax>>1);
  int y=(g.fbh>>1)-(hsum>>1);
  for (i=0;i<MODAL->optionc;i++) {
    struct pause_option *option=MODAL->optionv+i;
    option->x=x;
    option->y=y;
    y+=option->h;
  }
  
  /* Initial selection is the first enabled option, or -1 if all disabled, which really shouldn't be possible.
   */
  MODAL->selp=-1;
  for (i=0;i<MODAL->optionc;i++) {
    if (MODAL->optionv[i].enable) {
      MODAL->selp=i;
      break;
    }
  }
  
  /* Dialogue box bounds cover all the options, plus a tasteful margin, and more margin on the left to cover the cursor.
   */
  MODAL->dstw=wmax+4+12;
  MODAL->dsth=hsum+3;
  MODAL->dstx=MODAL->optionv[0].x-2-12;
  MODAL->dsty=MODAL->optionv[0].y-2;
  
  return 0;
}

/* User actions.
 */
 
static void pause_do_resume(struct modal *modal) {
  egg_play_sound(RID_sound_ui_dismiss);
  modal_pop(modal);
}
 
static void pause_do_menu(struct modal *modal) {
  save_game();
  modal_pop(modal);
  modal_spawn(&modal_type_hello);
}

/* Activate.
 * May dismiss modal.
 */
 
static void pause_activate(struct modal *modal) {
  if ((MODAL->selp<0)||(MODAL->selp>=MODAL->optionc)) return;
  const struct pause_option *option=MODAL->optionv+MODAL->selp;
  if (!option->enable) return; // Shouldn't have been able to acquire focus, but whatever.
  switch (option->index) {
    case PAUSE_OPTION_RESUME: pause_do_resume(modal); break;
    case PAUSE_OPTION_MENU: pause_do_menu(modal); break;
  }
}

/* Move selection.
 */
 
static void pause_move(struct modal *modal,int d) {
  if (MODAL->optionc<1) return;
  egg_play_sound(RID_sound_ui_motion);
  int panic=MODAL->optionc;
  for (;;) {
    MODAL->selp+=d;
    if (MODAL->selp<0) MODAL->selp=MODAL->optionc-1;
    else if (MODAL->selp>=MODAL->optionc) MODAL->selp=0;
    if (MODAL->optionv[MODAL->selp].enable) return;
    if (--panic<0) {
      MODAL->selp=-1;
      return;
    }
  }
}

/* Input.
 */
 
static void _pause_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) pause_move(modal,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) pause_move(modal,1);
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) { pause_activate(modal); return; }
  if ((input&EGG_BTN_WEST)&&!(pvinput&EGG_BTN_WEST)) { modal_pop(modal); return; }
}

/* Update.
 */
 
static void _pause_update(struct modal *modal,double elapsed) {
}

/* Render.
 */
 
static void _pause_render(struct modal *modal) {
  graf_draw_rect(&g.graf,MODAL->dstx,MODAL->dsty,MODAL->dstw,MODAL->dsth,0x000000c0);
  struct pause_option *option=MODAL->optionv;
  int i=MODAL->optionc;
  for (;i-->0;option++) {
    graf_draw_decal(&g.graf,option->texid,option->x,option->y,0,0,option->w,option->h,0);
  }
  if ((MODAL->selp>=0)&&(MODAL->selp<MODAL->optionc)) {
    option=MODAL->optionv+MODAL->selp;
    graf_draw_tile(&g.graf,texcache_get_image(&g.texcache,RID_image_tiles),
      option->x-(TILESIZE>>1),
      option->y+(option->h>>1)-1,
      0x11,0
    );
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_pause={
  .name="pause",
  .objlen=sizeof(struct modal_pause),
  .del=_pause_del,
  .init=_pause_init,
  .input=_pause_input,
  .update=_pause_update,
  .render=_pause_render,
};
