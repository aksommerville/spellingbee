#include "game/bee.h"

#define MESSAGE_MARGIN 10
#define MESSAGE_PADDING 4

struct modal_message {
  struct modal hdr;
  int texid,texw,texh;
};

#define MODAL ((struct modal_message*)modal)

/* Delete.
 */
 
static void _message_del(struct modal *modal) {
  if (MODAL->texid>0) egg_texture_del(MODAL->texid);
}

/* Init.
 */
 
static int _message_init(struct modal *modal) {
  modal->opaque=0;
  return 0;
}

/* Input.
 */
 
static void _message_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    egg_play_sound(RID_sound_ui_dismiss);
    modal_pop(modal);
    return;
  }
}

/* Update.
 */
 
static void _message_update(struct modal *modal,double elapsed) {
}

/* Render.
 */
 
static void _message_render(struct modal *modal) {
  int16_t boxx=MESSAGE_MARGIN;
  int16_t boxy=g.fbh>>1;
  graf_draw_rect(&g.graf,boxx,boxy,g.fbw-MESSAGE_MARGIN-boxx,g.fbh-MESSAGE_MARGIN-boxy,0x000000e0);
  graf_draw_decal(&g.graf,MODAL->texid,boxx+MESSAGE_PADDING,boxy+MESSAGE_PADDING,0,0,MODAL->texw,MODAL->texh,0);
}

/* Type definition.
 */
 
const struct modal_type modal_type_message={
  .name="message",
  .objlen=sizeof(struct modal_message),
  .del=_message_del,
  .init=_message_init,
  .input=_message_input,
  .update=_message_update,
  .render=_message_render,
};

/* Push new modal for single string.
 */
 
void modal_message_begin_single(int rid,int index) {
  int wlimit=g.fbw-(MESSAGE_MARGIN<<1)-(MESSAGE_PADDING<<1);
  int hlimit=(g.fbh>>1)-MESSAGE_MARGIN-(MESSAGE_PADDING<<1);
  int texid=font_texres_multiline(g.font,rid,index,wlimit,hlimit,0xffffffff);
  if (texid<=0) return;
  struct modal *modal=modal_spawn(&modal_type_message);
  if (!modal) {
    egg_texture_del(texid);
    return;
  }
  MODAL->texid=texid;
  egg_texture_get_status(&MODAL->texw,&MODAL->texh,texid);
}
