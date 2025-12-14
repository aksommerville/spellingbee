/* modal_type_merchant.c
 * Merchants and kitchens are almost exactly the same thing.
 */

#include "game/bee.h"

#define HIGHLIGHT_TIME 0.333

/* Index is item ID, which is also the bits little-endianly of our init parameter.
 */
static const struct item {
  const char *name;
  int price; // Same prices everywhere per item. ...also there's only one merchant :P
} itemv[32]={
  [ITEM_BUGSPRAY]={"Bug Spray",30},
  [ITEM_UNFAIRIE]={"Unfairie",50},
  [ITEM_2XWORD]={"2x Word",20},
  [ITEM_3XWORD]={"3x Word",50},
  [ITEM_ERASER]={"Eraser",100},
};

struct modal_merchant {
  struct modal hdr;
  uint32_t items;
  int itemc;
  int texid_text;
  int textw,texth;
  int dstx,dsty;
  int focusx,focusy; // Place us near this, in screen coords. (presumably the merchant sprite)
  double animclock;
  int animframe;
  int sely;
  int highlighty;
  double highlightclock;
};

#define MODAL ((struct modal_merchant*)modal)

/* Delete.
 */
 
static void _merchant_del(struct modal *modal) {
  egg_texture_del(MODAL->texid_text);
}

/* Init.
 */
 
static int _merchant_init(struct modal *modal) {
  modal->opaque=0;
  return 0;
}

/* Move cursor.
 */
 
static void merchant_move(struct modal *modal,int d) {
  sb_sound(RID_sound_ui_motion);
  MODAL->sely+=d;
  if (MODAL->sely<0) MODAL->sely=MODAL->itemc;
  else if (MODAL->sely>MODAL->itemc) MODAL->sely=0;
}

/* Activate.
 */
 
static void merchant_activate(struct modal *modal) {
  
  if (!MODAL->sely) {
    sb_sound(RID_sound_ui_dismiss);
    modal_pop(modal);
    return;
  }
  
  if ((MODAL->sely>0)&&(MODAL->sely<=MODAL->itemc)) {
    const struct item *item=itemv;
    int i=0,q=MODAL->sely;
    uint32_t mask=MODAL->items;
    for (;i<32;i++,item++,mask>>=1) {
      if (!(mask&1)) continue;
      if (!--q) {
        if ((item->price>g.stats.gold)||(g.stats.inventory[i]>=99)) {
          TRACE("reject purchase of %s: %d<%d or %d>=99",item->name,g.stats.gold,item->price,g.stats.inventory[i])
          sb_sound(RID_sound_reject);
          return;
        }
        g.stats.gold-=item->price;
        g.stats.inventory[i]++;
        g.world.status_bar_dirty=1;
        sb_sound(RID_sound_purchase);
        save_game();
        MODAL->highlighty=MODAL->sely;
        MODAL->highlightclock=HIGHLIGHT_TIME;
        TRACE("purchase %s, inv now %d, gold now %d",item->name,g.stats.inventory[i],g.stats.gold)
        return;
      }
    }
  }
}

/* Input.
 */
 
static void _merchant_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) merchant_move(modal,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) merchant_move(modal,1);
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) {
    merchant_activate(modal);
    return; // activate may delete us
  }
  if ((input&EGG_BTN_WEST)&&!(pvinput&EGG_BTN_WEST)) {
    sb_sound(RID_sound_ui_dismiss);
    modal_pop(modal);
    return;
  }
}

/* Update.
 */
 
static void _merchant_update(struct modal *modal,double elapsed) {
  if ((MODAL->animclock-=elapsed)<=0.0) {
    MODAL->animclock+=0.250;
    if (++(MODAL->animframe)>=2) MODAL->animframe=0;
  }
  if (MODAL->highlightclock>0.0) {
    MODAL->highlightclock-=elapsed;
  }
}

/* Render.
 */
 
static void _merchant_render(struct modal *modal) {
  // (texid_text) contains the background, static text, and options text. Everything but the cursor.
  graf_set_input(&g.graf,MODAL->texid_text);
  graf_decal(&g.graf,MODAL->dstx,MODAL->dsty,0,0,MODAL->textw,MODAL->texth);
  int cursorx=MODAL->dstx+7;
  int cursory=MODAL->dsty+16+MODAL->sely*10;
  graf_set_tint(&g.graf,MODAL->animframe?0xc0c0c0ff:0xffffffff);
  graf_set_image(&g.graf,RID_image_tiles);
  graf_tile(&g.graf,cursorx,cursory,0x11,0);
  graf_set_tint(&g.graf,0);
  if (MODAL->highlightclock>0.0) {
    int alpha=(int)((MODAL->highlightclock*0.500*255.0)/HIGHLIGHT_TIME);
    if (alpha>0) {
      graf_fill_rect(&g.graf,MODAL->dstx,MODAL->dsty+12+MODAL->highlighty*10,MODAL->textw,10,0xffff0000|alpha);
    }
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_merchant={
  .name="merchant",
  .objlen=sizeof(struct modal_merchant),
  .del=_merchant_del,
  .init=_merchant_init,
  .input=_merchant_input,
  .update=_merchant_update,
  .render=_merchant_render,
};

/* Calculate layout for new modal.
 */
 
static void modal_merchant_layout(struct modal *modal) {
  const int ymargin=3; // inside box
  const int xmargin=3; // inside box
  const int xspace=10; // outside box
  const int yspace=10; // bottom edge
  const int indent=10; // left column
  const int pricex=70;
  
  // First, how many items? ie how many bits set in (items)?
  uint32_t q=MODAL->items;
  const struct item *item=itemv;
  for (;q;q>>=1,item++) {
    if (!(q&1)) continue;
    if (!item->name) continue;
    MODAL->itemc++;
  }
  
  // Above the items there's a one-line prompt and one dummy option to exit.
  int rowc=2+MODAL->itemc;
  
  // Determine the dialogue box's bounds and allocate it.
  int lineh=font_get_line_height(g.font);
  lineh+=1; // Our 9-point font puts descenders directly against the top of the next line. It's a problem for merchants, "Bug Spray" and "Unfairie" touch inappropriately.
  int h=lineh*rowc+ymargin*2;
  h-=2; // ...and of course cheating lineh up becomes apparent at the very bottom.
  int w=g.fbw; // Limit; we'll trim after.
  int stride=w<<2;
  uint32_t *image=malloc(stride*h);
  if (!image) return;
  
  // Fill image initially with nearly-opaque black.
  int i=w*h;
  uint32_t *v=image;
  for (;i-->0;v++) *v=0xc0000000;
  
  // Top two lines are static. We assume that the prompt is wider than any other line.
  int promptw=font_render_string(image,w,h,stride,xmargin,ymargin,g.font,"Buy something will ya!",-1,0xffffffff);
  int clipw=xmargin+promptw+xmargin;
  if (clipw<w) w=clipw;
  font_render_string(image,w,h,stride,xmargin+indent,ymargin+lineh,g.font,"Exit",-1,0xff0000ff);
  
  // Write the name and price for each item.
  int row=2;
  for (item=itemv,i=0;i<32;i++,item++) {
    if (!(MODAL->items&(1<<i))) continue;
    if (!item->name) continue;
    
    font_render_string(image,w,h,stride,xmargin+indent,ymargin+lineh*row,g.font,item->name,-1,0x00ffffff);
    
    char tmp[8];
    int tmpc=0;
    tmp[tmpc++]='$';
    if (item->price>=100) tmp[tmpc++]='0'+(item->price/100)%10;
    if (item->price>=10) tmp[tmpc++]='0'+(item->price/10)%10;
    tmp[tmpc++]='0'+item->price%10;
    font_render_string(image,w,h,stride,xmargin+pricex,ymargin+lineh*row,g.font,tmp,tmpc,0x00ff00ff);
    
    row++;
  }
  
  MODAL->texid_text=egg_texture_new();
  egg_texture_load_raw(MODAL->texid_text,w,h,stride,image,stride*h);
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
 
void modal_merchant_begin(uint32_t items,int focusx,int focusy) {
  struct modal *modal=modal_spawn(&modal_type_merchant);
  if (!modal) return;
  MODAL->items=items;
  MODAL->focusx=focusx;
  MODAL->focusy=focusy;
  modal_merchant_layout(modal);
  TRACE("items=0x%08x",items)
}
