/* modal_type_prompt.c
 * Presents a prompt and enumerated options.
 */

#include "game/bee.h"
#include <stdarg.h>

#define PROMPT_OPTION_LIMIT 8
#define MARGINL 2
#define MARGINT 2
#define MARGINR 1
#define MARGINB 0

struct modal_prompt {
  struct modal hdr;
  void (*cb)(int choice,void *userdata);
  void *userdata;
  int16_t dstx,dsty,dstw,dsth;
  int texid_prompt;
  int promptw,prompth;
  struct option {
    int texid;
    int w,h;
  } optionv[PROMPT_OPTION_LIMIT];
  int optionc;
  int optionp;
};

#define MODAL ((struct modal_prompt*)modal)

static void _prompt_del(struct modal *modal) {
  egg_texture_del(MODAL->texid_prompt);
  while (MODAL->optionc-->0) egg_texture_del(MODAL->optionv[MODAL->optionc].texid);
}

static int _prompt_init(struct modal *modal) {
  modal->opaque=0;
  return 0;
}

static void prompt_activate(struct modal *modal) {
  if ((MODAL->optionp>=0)&&(MODAL->optionp<MODAL->optionc)&&MODAL->cb) {
    MODAL->cb(MODAL->optionp,MODAL->userdata);
  }
  modal_pop(modal);
}

static void prompt_move(struct modal *modal,int dx,int dy) {
  if (MODAL->optionc<1) return;
  MODAL->optionp+=dy;
  if (MODAL->optionp<0) MODAL->optionp=MODAL->optionc-1;
  else if (MODAL->optionp>=MODAL->optionc) MODAL->optionp=0;
}

static void _prompt_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_WEST)&&!(pvinput&EGG_BTN_WEST)) {
    modal_pop(modal);
    return;
  }
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    prompt_activate(modal);
    return;
  }
  if ((input&EGG_BTN_LEFT)&&!(pvinput&EGG_BTN_LEFT)) prompt_move(modal,-1,0);
  if ((input&EGG_BTN_RIGHT)&&!(pvinput&EGG_BTN_RIGHT)) prompt_move(modal,1,0);
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) prompt_move(modal,0,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) prompt_move(modal,0,1);
}

static void _prompt_render(struct modal *modal) {
  int gutterw=12;
  graf_fill_rect(&g.graf,MODAL->dstx,MODAL->dsty,MODAL->dstw,MODAL->dsth,0x000000c0);
  graf_set_input(&g.graf,MODAL->texid_prompt);
  graf_decal(&g.graf,MODAL->dstx+MARGINL,MODAL->dsty+MARGINT,0,0,MODAL->promptw,MODAL->prompth);
  int16_t x=MODAL->dstx+MARGINL+gutterw;
  int16_t y=MODAL->dsty+MARGINT+MODAL->prompth;
  const struct option *option=MODAL->optionv;
  int i=MODAL->optionc,seli=MODAL->optionp;
  for (;i-->0;option++) {
    if (!seli--) {
      graf_set_image(&g.graf,RID_image_tiles);
      graf_tile(&g.graf,MODAL->dstx+MARGINL+(gutterw>>1),y+(option->h>>1),0x11,0);
    }
    graf_set_input(&g.graf,option->texid);
    graf_decal(&g.graf,x,y,0,0,option->w,option->h);
    y+=option->h;
  }
}

const struct modal_type modal_type_prompt={
  .name="prompt",
  .objlen=sizeof(struct modal_prompt),
  .del=_prompt_del,
  .init=_prompt_init,
  .input=_prompt_input,
  .render=_prompt_render,
};

static int prompt_set_prompt(struct modal *modal,const char *src,int srcc) {
  egg_texture_del(MODAL->texid_prompt);
  int wlimit=(g.fbw*3)>>2;
  int hlimit=g.fbh>>1;
  MODAL->texid_prompt=font_render_to_texture(0,g.font,src,srcc,wlimit,hlimit,0xffffffff);
  egg_texture_get_size(&MODAL->promptw,&MODAL->prompth,MODAL->texid_prompt);
  return 0;
}

static int prompt_add_option(struct modal *modal,const char *src) {
  if (MODAL->optionc>=PROMPT_OPTION_LIMIT) return -1;
  struct option *option=MODAL->optionv+MODAL->optionc++;
  int wlimit=(g.fbw*3)>>2;
  option->texid=font_render_to_texture(0,g.font,src,-1,wlimit,font_get_line_height(g.font),0xffffffff);
  egg_texture_get_size(&option->w,&option->h,option->texid);
  return 0;
}

static void prompt_layout(struct modal *modal) {
  int gutterw=12; // Space on the left of options for the cursor.
  MODAL->dstw=MODAL->promptw;
  MODAL->dsth=MODAL->prompth;
  const struct option *option=MODAL->optionv;
  int i=MODAL->optionc;
  for (;i-->0;option++) {
    int effw=gutterw+option->w;
    if (effw>MODAL->dstw) MODAL->dstw=effw;
    MODAL->dsth+=option->h;
  }
  MODAL->dstw+=MARGINL+MARGINR;
  MODAL->dsth+=MARGINT+MARGINB;
  MODAL->dstx=(g.fbw>>1)-(MODAL->dstw>>1);
  MODAL->dsty=(g.fbh>>1)-(MODAL->dsth>>1);
}

void _modal_prompt(const char *prompt,int promptc,void (*cb)(int choice,void *userdata),void *userdata,...) {
  struct modal *modal=modal_spawn(&modal_type_prompt);
  if (!modal) return;
  prompt_set_prompt(modal,prompt,promptc);
  va_list vargs;
  va_start(vargs,userdata);
  const char *option;
  while (option=va_arg(vargs,const char*)) {
    if (prompt_add_option(modal,option)<0) break;
  }
  prompt_layout(modal);
  MODAL->cb=cb;
  MODAL->userdata=userdata;
}
