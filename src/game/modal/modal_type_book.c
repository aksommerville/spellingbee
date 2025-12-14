/* modal_type_book.c
 * Spawned after a boss book, when the player recovered an overdue book.
 */

#include "game/bee.h"
#include "game/shared_symbols.h"

#define IMAGE_W 64 /* per tile */
#define IMAGE_H 64
#define IMAGE_COLC 3

struct modal_book {
  struct modal hdr;
  int bookid;
  int texid_text;
  int textw,texth;
  double clock; // Counts up forever.
};

#define MODAL ((struct modal_book*)modal)

/* Delete.
 */
 
static void _book_del(struct modal *modal) {
  egg_texture_del(MODAL->texid_text);
}

/* Init.
 */
 
static int _book_init(struct modal *modal) {
  modal->opaque=1;
  MODAL->texid_text=font_render_to_texture(0,g.font,"You recovered a book!",-1,g.fbw,font_get_line_height(g.font),0xffffffff);
  egg_texture_get_size(&MODAL->textw,&MODAL->texth,MODAL->texid_text);
  return 0;
}

/* Input.
 */
 
static void _book_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    modal_pop(modal);
    sprite_group_kill(GRP(HERO));
    world_load_map(&g.world,1);
    if (
      flag_get(NS_flag_book1)&&
      flag_get(NS_flag_book2)&&
      flag_get(NS_flag_book3)&&
      flag_get(NS_flag_book4)&&
      flag_get(NS_flag_book5)&&
      flag_get(NS_flag_book6)
    ) {
      modal_spawn(&modal_type_victory);
    }
    return;
  }
}

/* Update.
 */
 
static void _book_update(struct modal *modal,double elapsed) {
  MODAL->clock+=elapsed;
}

/* Render.
 */
 
static void _book_render(struct modal *modal) {
  graf_fill_rect(&g.graf,0,0,g.fbw,g.fbh,0x000000ff);
  
  // Message.
  if (MODAL->clock<1.0) {
    int alpha=(int)((MODAL->clock*255.0)/1.0);
    if (alpha<0) alpha=0; else if (alpha>0xff) alpha=0xff;
    graf_set_alpha(&g.graf,alpha);
  }
  graf_set_input(&g.graf,MODAL->texid_text);
  graf_decal(&g.graf,(g.fbw>>1)-(MODAL->textw>>1),20,0,0,MODAL->textw,MODAL->texth);
  graf_set_alpha(&g.graf,0xff);
  
  // Determine book's position.
  int bookzoneh=g.fbh-20-MODAL->texth;
  int dsty=20+MODAL->texth+(bookzoneh>>1)-(IMAGE_H>>1);
  if (MODAL->clock<2.0) {
    dsty+=(int)((2.0-MODAL->clock)*bookzoneh);
  }
  int dstx=(g.fbw>>1)-(IMAGE_W>>1);
  
  // Sunburst behind the book, after it reaches its resting position.
  graf_set_image(&g.graf,RID_image_books);
  if (MODAL->clock>2.0) {
    int frame=((int)(MODAL->clock*4.0))&1;
    if (frame) {
      graf_decal(&g.graf,dstx-IMAGE_W,dsty-(IMAGE_H>>1),0,IMAGE_H*2,IMAGE_W*3,IMAGE_H*2);
    }
  }
  
  // Book, sliding up from below.
  int srcx=(MODAL->bookid-1)%IMAGE_COLC;
  int srcy=(MODAL->bookid-1)/IMAGE_COLC;
  srcx*=IMAGE_W;
  srcy*=IMAGE_H;
  graf_decal(&g.graf,dstx,dsty,srcx,srcy,IMAGE_W,IMAGE_H);
}

/* Type definition.
 */
 
const struct modal_type modal_type_book={
  .name="book",
  .objlen=sizeof(struct modal_book),
  .del=_book_del,
  .init=_book_init,
  .input=_book_input,
  .update=_book_update,
  .render=_book_render,
};

/* New.
 */
 
void modal_book_begin(int bookid) {
  struct modal *modal=modal_spawn(&modal_type_book);
  if (!modal) return;
  MODAL->bookid=bookid;
  TRACE("bookid=%d",bookid)
}
