/* modal_type_hello.c
 * The main menu, first thing a user sees.
 */
 
#include "game/bee.h"

#define HELLO_OPTION_CONTINUE 0
#define HELLO_OPTION_NEW 1
#define HELLO_OPTION_BATTLE 2
#define HELLO_OPTION_SETTINGS 3
#define HELLO_OPTION_QUIT 4
#define HELLO_OPTION_LIMIT 5

/* Object definition.
 */
 
struct modal_hello {
  struct modal hdr;
  struct hello_option {
    int enable;
    int index; // HELLO_OPTION_*, also the strings index.
    int texid;
    int x,y,w,h; // Label position.
  } optionv[HELLO_OPTION_LIMIT]; // NB index is the tab order, not necessarily the option id.
  int optionc;
  char save[256];
  int savec;
  int selp;
};

#define MODAL ((struct modal_hello*)modal)

/* Cleanup.
 */
 
static void _hello_del(struct modal *modal) {
  while (MODAL->optionc-->0) {
    egg_texture_del(MODAL->optionv[MODAL->optionc].texid);
  }
}

/* Add option.
 * Position is initially zero, you must set later.
 */
 
static int hello_add_option(struct modal *modal,int index,int enable) {
  if (MODAL->optionc>=HELLO_OPTION_LIMIT) return -1;
  struct hello_option *option=MODAL->optionv+MODAL->optionc++;
  option->enable=enable?1:0;
  option->index=index;
  if ((option->texid=font_texres_oneline(g.font,RID_strings_hello,index,g.fbw,enable?0xffffffff:0x808080ff))<1) return -1;
  egg_texture_get_status(&option->w,&option->h,option->texid);
  return 0;
}

/* Init.
 */
 
static int _hello_init(struct modal *modal) {
  modal->opaque=1;
  
  /* Acquire the encoded saved game.
   */
  MODAL->savec=egg_store_get(MODAL->save,sizeof(MODAL->save),"save",4);
  if ((MODAL->savec<0)||(MODAL->savec>sizeof(MODAL->save))) MODAL->savec=0;
  
  /* Decide which options are available.
   * TODO Can we optionally disable QUIT via some Egg runtime setting? Thinking about kiosks.
   * TODO Enable SETTINGS when ready.
   */
  hello_add_option(modal,HELLO_OPTION_CONTINUE,MODAL->savec);
  hello_add_option(modal,HELLO_OPTION_NEW,1);
  hello_add_option(modal,HELLO_OPTION_BATTLE,1);
  hello_add_option(modal,HELLO_OPTION_SETTINGS,0);
  hello_add_option(modal,HELLO_OPTION_QUIT,1);
  
  /* Position options.
   * For now, pack and center vertically.
   * Horizontally, maintain a flush left edge, centering the widest.
   */
  int wmax=0,hsum=0,i=0;
  for (;i<MODAL->optionc;i++) {
    struct hello_option *option=MODAL->optionv+i;
    if (option->w>wmax) wmax=option->w;
    hsum+=option->h;
  }
  int x=(g.fbw>>1)-(wmax>>1);
  int y=(g.fbh>>1)-(hsum>>1);
  for (i=0;i<MODAL->optionc;i++) {
    struct hello_option *option=MODAL->optionv+i;
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
  
  //egg_play_song(RID_song_open_arms,0,1);
  
  return 0;
}

/* User actions.
 */
 
static void hello_do_continue(struct modal *modal) {
  if (!MODAL->savec) return;
  if (world_init(&g.world,MODAL->save,MODAL->savec)<0) {
    fprintf(stderr,"world_init(continue) failed!\n");
    return;
  }
  modal_pop(modal);
}
 
static void hello_do_new(struct modal *modal) {
  if (world_init(&g.world,0,0)<0) {
    fprintf(stderr,"world_init(new) failed!\n");
    return;
  }
  save_game();
  modal_pop(modal);
}
 
static void hello_do_battle(struct modal *modal) {
  if (!modal_battle_begin(RID_battle_moonsong)) return;
  // Keep this modal open, why not.
  //TODO We'll need a notification when we return to the top of stack, to change the song.
}
 
static void hello_do_settings(struct modal *modal) {
  fprintf(stderr,"%s\n",__func__);
  //TODO Push settings modal, once there is such a thing.
}
 
static void hello_do_quit(struct modal *modal) {
  egg_terminate(0);
}

/* Activate.
 * May dismiss modal.
 */
 
static void hello_activate(struct modal *modal) {
  if ((MODAL->selp<0)||(MODAL->selp>=MODAL->optionc)) return;
  const struct hello_option *option=MODAL->optionv+MODAL->selp;
  if (!option->enable) return; // Shouldn't have been able to acquire focus, but whatever.
  switch (option->index) {
    case HELLO_OPTION_CONTINUE: hello_do_continue(modal); break;
    case HELLO_OPTION_NEW: hello_do_new(modal); break;
    case HELLO_OPTION_BATTLE: hello_do_battle(modal); break;
    case HELLO_OPTION_SETTINGS: hello_do_settings(modal); break;
    case HELLO_OPTION_QUIT: hello_do_quit(modal); break;
  }
}

/* Move selection.
 */
 
static void hello_move(struct modal *modal,int d) {
  if (MODAL->optionc<1) return;
  //TODO sound effect
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
 
static void _hello_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) hello_move(modal,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) hello_move(modal,1);
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) { hello_activate(modal); return; }
}

/* Update.
 */
 
static void _hello_update(struct modal *modal,double elapsed) {
  //TODO Animation everywhere.
  //TODO Long cutscene.
}

/* Render.
 */
 
static void _hello_render(struct modal *modal) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x004020ff);
  struct hello_option *option=MODAL->optionv;
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
 
const struct modal_type modal_type_hello={
  .name="hello",
  .objlen=sizeof(struct modal_hello),
  .del=_hello_del,
  .init=_hello_init,
  .input=_hello_input,
  .update=_hello_update,
  .render=_hello_render,
};
