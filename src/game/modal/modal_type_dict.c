/* modal_type_dict.c
 * Let the user search for words.
 */
 
#include "game/bee.h"
#include "game/battle/dict.h"

// There's some value in matching WCMODAL_{COL,ROW}C from battle.h
#define DICT_COLC 6
#define DICT_ROWC 5

struct modal_dict {
  struct modal hdr;
  char entry[7];
  int entryc;
  int valid;
  int selx,sely;
  double animclock;
  int animframe;
  int texid_list;
  int listw,listh;
};

#define MODAL ((struct modal_dict*)modal)

static void _dict_del(struct modal *modal) {
  egg_texture_del(MODAL->texid_list);
}

static int _dict_init(struct modal *modal) {
  modal->opaque=1;
  MODAL->selx=DICT_COLC>>1;
  MODAL->sely=DICT_ROWC>>1;
  return 0;
}

/* Given an array of buckets and a parallel array of positions, return which index is lowest.
 * Or which word *before* the index is highest.
 * We depend on buckets being sorted by length, which they are.
 */
 
static int dict_pick_lowest(const struct dict_bucket *bucketv,const int *pv,int c) {
  int best=-1;
  int i=0;
  const struct dict_bucket *q=bucketv;
  for (;i<c;i++,q++) {
    if (pv[i]>=q->c) continue;
    if (best<0) best=i;
    else if (memcmp(q->v+pv[i]*q->len,bucketv[best].v+pv[best]*bucketv[best].len,bucketv[best].len)<0) best=i;
  }
  return best;
}

static int dict_pick_highest(const struct dict_bucket *bucketv,const int *pv,int c) {
  int best=-1;
  int i=0;
  const struct dict_bucket *q=bucketv;
  for (;i<c;i++,q++) {
    if (pv[i]<=0) continue;
    if (best<0) best=i;
    else {
      const char *a=q->v+(pv[i]-1)*q->len;
      const char *b=bucketv[best].v+(pv[best]-1)*bucketv[best].len;
      int cmpc=bucketv[best].len;
      if (memcmp(a,b,cmpc)>=0) best=i;
    }
  }
  return best;
}

/* Draw the word list to a blank RGBA image.
 */
 
static void dict_draw_list(uint8_t *dst,int dstw,int dsth,const char *src,int srcc) {

  /* Get the whole dictionary and search for this word in each bucket.
   */
  if (srcc>7) srcc=7;
  char zword[8];
  memcpy(zword,src,srcc);
  zword[srcc]=0;
  struct dict_bucket bucketv[7];
  int bucketc=dict_get_all(bucketv,7,RID_dict_nwl2023);
  int pv[7];
  int i=bucketc;
  while (i-->0) {
    pv[i]=dict_bucket_search(bucketv+i,zword);
    if (pv[i]<0) pv[i]=-pv[i]-1;
    if (pv[i]<bucketv[i].c) { // Bump forward if this word comes before the query.
      int cmpc=bucketv[i].len;
      if (srcc<cmpc) cmpc=srcc;
      int cmp=memcmp(bucketv[i].v+pv[i]*bucketv[i].len,src,cmpc);
      if ((cmp<0)||(!cmp&&(bucketv[i].len<srcc))) pv[i]++;
    }
  }
  int pv0[7];
  memcpy(pv0,pv,sizeof(pv));
  
  /* Start at the center, and draw one word at a time forward and downward, whichever bucket is earliest lexically.
   */
  int rowh=font_get_line_height(g.font)+1;
  int dsty0=(dsth>>1)-(rowh>>1);
  int dsty=dsty0;
  for (;dsty<dsth;dsty+=rowh) {
    int choice=dict_pick_lowest(bucketv,pv,bucketc);
    if (choice<0) continue;
    const char *word=bucketv[choice].v+bucketv[choice].len*pv[choice];
    uint32_t color=((bucketv[choice].len==srcc)&&!memcmp(word,src,srcc))?0xffff00ff:0xc0c0c0ff;
    font_render_string(dst,dstw,dsth,dstw<<2,0,dsty,g.font,word,bucketv[choice].len,color);
    pv[choice]++;
  }
  
  /* Return to the center, restore search positions, and do a similar thing upward.
   * This time, we definitely won't see the word itself so no need to check.
   */
  dsty=dsty0;
  memcpy(pv,pv0,sizeof(pv));
  while (dsty>0) {
    dsty-=rowh;
    int choice=dict_pick_highest(bucketv,pv,bucketc);
    if (choice<0) continue;
    pv[choice]--;
    const char *word=bucketv[choice].v+bucketv[choice].len*pv[choice];
    font_render_string(dst,dstw,dsth,dstw<<2,0,dsty,g.font,word,bucketv[choice].len,0xc0c0c0ff);
  }
}

/* React to change of (entry).
 */
 
static void dict_lookup(struct modal *modal) {
  
  /* First and most important, set (valid) whether it's in the dictionary.
   */
  struct rating_detail detail={0};
  int score=dict_rate_word(&detail,RID_dict_nwl2023,MODAL->entry,MODAL->entryc);
  if (detail.valid) {
    MODAL->valid=1;
  } else {
    MODAL->valid=0;
  }
  
  /* Then, painstakingly compose a list image centered on the entry, combining the buckets so it's purely alphabetical.
   */
  egg_texture_del(MODAL->texid_list);
  MODAL->texid_list=0;
  if (MODAL->entryc) {
    #define LISTW 80
    #define LISTH 189
    uint8_t *rgba=calloc(LISTW*LISTH,4);
    if (rgba) {
      dict_draw_list(rgba,LISTW,LISTH,MODAL->entry,MODAL->entryc);
      MODAL->texid_list=egg_texture_new();
      egg_texture_load_raw(MODAL->texid_list,EGG_TEX_FMT_RGBA,LISTW,LISTH,LISTW<<2,rgba,LISTW*LISTH*4);
      free(rgba);
      MODAL->listw=LISTW;
      MODAL->listh=LISTH;
    }
    #undef LISTW
    #undef LISTH
  }
}

/* Input.
 */

static void dict_activate(struct modal *modal) {
  if (MODAL->entryc>=7) {
    egg_play_sound(RID_sound_reject);
    return;
  }
  int ch=MODAL->sely*DICT_COLC+MODAL->selx;
  if ((ch<0)||(ch>=26)) {
    egg_play_sound(RID_sound_reject);
    return;
  }
  egg_play_sound(RID_sound_stage_letter);
  ch+='A';
  MODAL->entry[MODAL->entryc++]=ch;
  dict_lookup(modal);
}

static void dict_move(struct modal *modal,int dx,int dy) {
  MODAL->selx+=dx;
  if (MODAL->selx<0) MODAL->selx=DICT_COLC-1;
  else if (MODAL->selx>=DICT_COLC) MODAL->selx=0;
  MODAL->sely+=dy;
  if (MODAL->sely<0) MODAL->sely=DICT_ROWC-1;
  else if (MODAL->sely>=DICT_ROWC) MODAL->sely=0;
  egg_play_sound(RID_sound_ui_motion);
}

static void dict_cancel(struct modal *modal) {
  if (MODAL->entryc>0) {
    MODAL->entryc--;
    dict_lookup(modal);
    egg_play_sound(RID_sound_ui_dismiss);
  } else {
    modal_pop(modal);
  }
}

static void _dict_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_WEST)&&!(pvinput&EGG_BTN_WEST)) {
    dict_cancel(modal);
    return;
  }
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) dict_activate(modal);
  if ((input&EGG_BTN_LEFT)&&!(pvinput&EGG_BTN_LEFT)) dict_move(modal,-1,0);
  if ((input&EGG_BTN_RIGHT)&&!(pvinput&EGG_BTN_RIGHT)) dict_move(modal,1,0);
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) dict_move(modal,0,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) dict_move(modal,0,1);
}

static void _dict_update(struct modal *modal,double elapsed) {
  if ((MODAL->animclock-=elapsed)<=0.0) {
    MODAL->animclock+=0.200;
    if (++(MODAL->animframe)>=4) MODAL->animframe=0;
  }
}

/* Render.
 */

static void _dict_render(struct modal *modal) {
  int texid=texcache_get_image(&g.texcache,RID_image_tiles);
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x102030ff);
  
  /* Stage, upper left.
   */
  int16_t dstx=75,dsty=35;
  int i=0;
  graf_set_tint(&g.graf,0xffffffff);
  for (;i<MODAL->entryc;i++,dstx+=9) {
    graf_draw_tile(&g.graf,texid,dstx,dsty,MODAL->entry[i]+0x80,0);
  }
  graf_set_tint(&g.graf,0);
  
  /* Validity indicator, right of stage.
   */
  if (MODAL->entryc) {
    graf_draw_decal(&g.graf,texid,141,17,TILESIZE*(MODAL->valid?7:9),TILESIZE*10,TILESIZE*2,TILESIZE*2,0);
  }
  
  /* Letters grid, lower left.
   */
  int16_t gridx=70;
  int16_t gridy=70;
  graf_draw_rect(&g.graf,gridx,gridy,TILESIZE*DICT_COLC,TILESIZE*DICT_ROWC,0xc0c0c0ff);
  int row=0,ch='A'+0x80;
  for (dsty=gridy+(TILESIZE>>1);row<DICT_ROWC;row++,dsty+=TILESIZE) {
    int col=0;
    for (dstx=gridx+(TILESIZE>>1)-1;col<DICT_COLC;col++,dstx+=TILESIZE,ch++) {
      graf_draw_tile(&g.graf,texid,dstx,dsty,ch,0);
    }
  }
  int cursorxform=0;
  switch (MODAL->animframe) {
    case 1: cursorxform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
    case 2: cursorxform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
    case 3: cursorxform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
  }
  graf_draw_tile(&g.graf,texid,gridx+MODAL->selx*TILESIZE+(TILESIZE>>1),gridy+MODAL->sely*TILESIZE+(TILESIZE>>1),0x00,cursorxform);
  
  /* Word list, right.
   */
  if (MODAL->texid_list) {
    graf_draw_decal(&g.graf,MODAL->texid_list,190,(g.fbh>>1)-(MODAL->listh>>1),0,0,MODAL->listw,MODAL->listh,0);
  }
}

/* Type definition.
 */

const struct modal_type modal_type_dict={
  .name="dict",
  .objlen=sizeof(struct modal_dict),
  .del=_dict_del,
  .init=_dict_init,
  .input=_dict_input,
  .update=_dict_update,
  .render=_dict_render,
};
