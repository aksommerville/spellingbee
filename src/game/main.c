#include "bee.h"

struct globals g={0};

void egg_client_quit(int status) {
}

int egg_client_init() {
  egg_texture_get_status(&g.fbw,&g.fbh,1);
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1;
  if (font_add_image_resource(g.font,0x00a1,RID_image_font9_00a1)<0) return -1;
  if (font_add_image_resource(g.font,0x0400,RID_image_font9_0400)<0) return -1;
  // Also supplied by default: font6_0020, cursive_0020, witchy_0020
  
  srand_auto();
  
  letterbag_reset(&g.letterbag);
  if (letterbag_draw(g.hand,&g.letterbag)!=7) return -1;
  letterbag_draw(g.foe.hand,&g.letterbag);
  g.foe.searchlen=2;
  g.foe.searchp=0;
  g.foe.searchclock=0.0;
  g.foe.word="";
  g.foe.wordc=0;
  g.foe.wordscore=0;
  
  g.cursor.x=3;
  g.cursor.y=2;
  
  return 0;
}

static int foe_search_cb(const char *src,int srcc,void *userdata) {
  int score=rate_word(src,srcc);
  if (score<=g.foe.wordscore) return 0;
  char scratch[7];
  memcpy(scratch,g.foe.hand,7);
  int srcp=0; for (;srcp<srcc;srcp++) {
    int scratchp=0,ok=0,wcp=-1;
    for (;scratchp<7;scratchp++) {
      if (scratch[scratchp]==src[srcp]) {
        ok=1;
        scratch[scratchp]=0;
        break;
      } else if (scratch[scratchp]=='@') {
        wcp=scratchp;
      }
    }
    if (!ok&&(wcp>=0)) {
      ok=1;
      scratch[wcp]=0;
    }
    if (!ok) return 0;
  }
  //fprintf(stderr,">>> %.*s (%d)\n",srcc,src,score);
  //TODO The foe is getting points for his wildcards, that's not fair.
  g.foe.word=src;
  g.foe.wordc=srcc;
  g.foe.wordscore=score;
  return 0;
}

static void foe_search() {
  if (g.foe.searchp>=7) {
    g.foe.searchp=0;
    g.foe.searchlen++;
  }
  if (g.foe.searchlen>7) return;
  //fprintf(stderr,"%s len=%d p=%d(%c)\n",__func__,g.foe.searchlen,g.foe.searchp,g.foe.hand[g.foe.searchp]);
  for_each_word(g.foe.searchlen,g.foe.hand[g.foe.searchp],foe_search_cb,0);
  g.foe.searchp++;
}

static void foe_play() {
  //fprintf(stderr,"%s\n",__func__);
  if (!g.foe.wordc) {
    fprintf(stderr,"FOE FOLDS\n");
  } else {
    fprintf(stderr,"FOE PLAYS '%.*s' FOR %d POINTS\n",g.foe.wordc,g.foe.word,g.foe.wordscore);
    g.score-=g.foe.wordscore;
  }
  memset(g.foe.lastplay,0,7);
  memcpy(g.foe.lastplay,g.foe.word,g.foe.wordc);
  g.foe.lastscore=g.foe.wordscore;
  letterbag_draw(g.foe.hand,&g.letterbag);
  g.foe.searchclock=0.0;
  g.foe.searchp=0;
  g.foe.searchlen=2;
  g.foe.word="";
  g.foe.wordc=0;
  g.foe.wordscore=0;
}

static void foe_update(double elapsed) {
  // TODO Can we pay out the search at finer granularity?
  double interval=1.000;
  if ((g.foe.searchclock+=elapsed)>=interval) {
    g.foe.searchclock-=interval;
    foe_search();
  }
}

static void refill_hand() {
  if ((letterbag_draw_partial(g.hand,&g.letterbag)<1)&&!memcmp(g.hand,"\0\0\0\0\0\0\0",7)) {
    fprintf(stderr,"GAME OVER, final score %d\n",g.score);
    // Top: 279
    // 263 160 175 279 237
    // After adding 5 and 10 point bonuses: 235(no 7s) 250(no 7s) 219('') 201 228 178 234 240 130
    // Current scoring is not ideal.
    // Basically you're getting the total points in the bag, 187, offset by 50s, discards, and misplays.
    // If you can append a letter to your current word, there's very little reason to do so as opposed to saving it.
    // In the real game of course there's the additional challenge of trying to front-load your points, better to score them now than next turn.
    // But that's all marginal.
    // Let's try a 5-point bonus for 5-letter words, 10-point for 6-letter, and the existing 50-point for 7-letter.
    // I eventually want double and triple word scores, effected as an item you can play at your discretion.
    // ...bonus at 5 letters makes a big difference.
    // Let's add some kind of follow-up bonus too. After a 15+ point word, the next word is doubled?
  }
}

static void request_new_hand() {
  // No impact to score. The penalty is that you lose these letters, and lose your turn.
  g.recent_score=0;
  memset(g.stage,0,sizeof(g.stage));
  memset(g.hand,0,sizeof(g.hand));
  refill_hand();
  g.cursor.y=2;
  g.cursor.x=0;
  foe_play();
}

static void cast_spell() {
  int c=0;
  while ((c<7)&&g.stage[c]) c++;
  int valid=spellcheck(g.stage,c);
  int score=rate_word(g.stage,7);
  if (!valid) score=-score;
  fprintf(stderr,"CAST SPELL '%.*s': %s = %d\n",c,g.stage,valid?"VALID":"INVALID",score);
  // Drop the stage and refill hand, and update score. Whether it's valid or not (score is negative if invalid).
  memset(g.stage,0,sizeof(g.stage));
  g.score+=score;
  g.recent_score=score;
  refill_hand();
  g.cursor.y=2;
  g.cursor.x=0;
  foe_play();
}

static void return_tile_to_hand(int p) {
  if ((p<0)||(p>=7)||!g.stage[p]) return;
  int dstp=0; for (;dstp<7;dstp++) {
    if (g.hand[dstp]) continue;
    g.hand[dstp]=g.stage[p];
    if ((g.hand[dstp]>='a')&&(g.hand[dstp]<='z')) g.hand[dstp]='@'; // Wildcard, unset on return.
    g.stage[p]=0;
    int i=p+1;
    for (;i<7;i++) g.stage[i-1]=g.stage[i];
    g.stage[6]=0;
    break;
  }
}

static void play_tile(int p) {
  if ((p<0)||(p>=7)||!g.hand[p]) return;
  if (g.hand[p]=='@') {
    g.wildcard_handp=p;
    g.wildcard_modal=1;
    g.wcx=4;
    g.wcy=1;
    return;
  }
  int dstp=0; for (;dstp<7;dstp++) {
    if (g.stage[dstp]) continue;
    g.stage[dstp]=g.hand[p];
    g.hand[p]=0;
    break;
  }
}

static void activate_cursor() {
  if (g.wildcard_modal) {
    char ch='A'+g.wcy*9+g.wcx;
    if ((ch>='A')&&(ch<='Z')) {
      g.hand[g.wildcard_handp]=ch+0x20;
      play_tile(g.wildcard_handp);
    }
    g.wildcard_modal=0;
  } else {
    switch (g.cursor.y) {
      case 0: switch (g.cursor.x) {
          case 0: cast_spell(); break;
          case 1: request_new_hand(); break;
        } break;
      case 1: return_tile_to_hand(g.cursor.x); break;
      case 2: play_tile(g.cursor.x); break;
    }
  }
}

static void move_cursor(int dx,int dy) {
  if (g.wildcard_modal) {
    g.wcx+=dx;
    g.wcy+=dy;
    if (g.wcx<0) g.wcx=8; else if (g.wcx>8) g.wcx=0;
    if (g.wcy<0) g.wcy=2; else if (g.wcy>2) g.wcy=0;
  } else {
    g.cursor.x+=dx;
    g.cursor.y+=dy;
    if (g.cursor.y<0) g.cursor.y=2;
    else if (g.cursor.y>2) g.cursor.y=0;
    switch (g.cursor.y) {
      case 0: if (dy) g.cursor.x=0; else if (g.cursor.x<0) g.cursor.x=1; else if (g.cursor.x>1) g.cursor.x=0; break;
      case 1: case 2: if (g.cursor.x<0) g.cursor.x=6; else if (g.cursor.x>6) g.cursor.x=0; break;
    }
  }
}

void egg_client_update(double elapsed) {
  if ((g.cursor.animclock-=elapsed)<=0.0) {
    g.cursor.animclock+=0.200;
    if (++(g.cursor.animframe)>=4) g.cursor.animframe=0;
  }
  
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_LEFT)&&!(g.pvinput&EGG_BTN_LEFT)) move_cursor(-1,0);
    if ((input&EGG_BTN_RIGHT)&&!(g.pvinput&EGG_BTN_RIGHT)) move_cursor(1,0);
    if ((input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)) move_cursor(0,-1);
    if ((input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)) move_cursor(0,1);
    if ((input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) activate_cursor();
    g.pvinput=input;
  }
  
  foe_update(elapsed);
}

static void draw_integer(int texid,int16_t dstx,int16_t dsty,int v) {
  const int xstride=7;
  if (v==INT_MIN) v++;
  if (v<0) {
    v=-v;
    graf_draw_tile(&g.graf,texid,dstx,dsty,'-',0);
    dstx+=xstride;
  }
  if (v>=1000000000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+v/1000000000,0); dstx+=xstride; }
  if (v>= 100000000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/100000000)%10,0); dstx+=xstride; }
  if (v>=  10000000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/10000000)%10,0); dstx+=xstride; }
  if (v>=   1000000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/1000000)%10,0); dstx+=xstride; }
  if (v>=    100000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/100000)%10,0); dstx+=xstride; }
  if (v>=     10000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/10000)%10,0); dstx+=xstride; }
  if (v>=      1000) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/1000)%10,0); dstx+=xstride; }
  if (v>=       100) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/100)%10,0); dstx+=xstride; }
  if (v>=        10) { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+(v/10)%10,0); dstx+=xstride; }
                     { graf_draw_tile(&g.graf,texid,dstx,dsty,'0'+v%10,0); dstx+=xstride; }
}

void egg_client_render() {
  graf_reset(&g.graf);
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x406080ff);
  
  int texid_tiles=texcache_get_image(&g.texcache,RID_image_tiles);
  int i;
  int16_t dstx,dsty;
  
  // CPU charge meter.
  int16_t meterw=g.fbw-10;
  int progress=(g.foe.searchlen-2)*7+g.foe.searchp;
  progress=(progress*meterw)/30;
  if (progress<meterw) {
    graf_draw_rect(&g.graf,5,100,meterw,10,0x808080ff);
    graf_draw_rect(&g.graf,5,100,progress,10,0xff0000ff);
  } else {
    graf_draw_rect(&g.graf,5,100,meterw,10,0xff0000ff);
  }

  // CPU's last play.
  draw_integer(texid_tiles,5,120,g.foe.lastscore);
  for (i=0,dstx=10,dsty=140;i<7;i++,dstx+=18) {
    if (g.foe.lastplay[i]) graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,g.foe.lastplay[i],0);
  }
  
  // Hand rack.
  dsty=83;
  graf_draw_tile(&g.graf,texid_tiles,24,dsty,0x03,0);
  for (i=8,dstx=40;i-->0;dstx+=16) {
    graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,0x04,0);
  }
  graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,0x03,EGG_XFORM_XREV);
  
  // Control buttons.
  graf_draw_tile(&g.graf,texid_tiles,40,40,0x01,0);
  graf_draw_tile(&g.graf,texid_tiles,58,40,0x02,0);
  
  // Stage and hand tiles.
  for (i=0,dstx=40,dsty=60;i<7;i++,dstx+=18) {
    if (g.stage[i]) graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,g.stage[i],0);
    if (g.hand[i]) graf_draw_tile(&g.graf,texid_tiles,dstx,dsty+20,g.hand[i],0);
  }
  
  // Cursor.
  uint8_t cursorxform=0;
  switch (g.cursor.animframe) {
    case 0: break;
    case 1: cursorxform=EGG_XFORM_XREV|EGG_XFORM_SWAP; break;
    case 2: cursorxform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
    case 3: cursorxform=EGG_XFORM_YREV|EGG_XFORM_SWAP; break;
  }
  if (!g.wildcard_modal) {
    int16_t cursorx=40+g.cursor.x*18;
    int16_t cursory=40+g.cursor.y*20;
    graf_draw_tile(&g.graf,texid_tiles,cursorx,cursory,0x00,cursorxform);
  }
  
  // Score and most recent play.
  draw_integer(texid_tiles,200,40,g.score);
  draw_integer(texid_tiles,200,56,g.recent_score);
  
  // Wildcard modal. 9x3, with Cancel in the extra slot.
  if (g.wildcard_modal) {
    const int margin=3;
    const int tilesize=16;
    const int colc=9;
    const int rowc=3;
    int w=colc*tilesize+margin*2;
    int h=rowc*tilesize+margin*2;
    int x=(g.fbw>>1)-(w>>1);
    int y=(g.fbh>>1)-(h>>1);
    graf_draw_rect(&g.graf,x,y,w,h,0xc0c0c0c0);
    char ch='a';
    int yi=rowc; for (dsty=y+margin+(tilesize>>1);yi-->0;dsty+=tilesize) {
      int xi=colc; for (dstx=x+margin+(tilesize>>1);xi-->0;dstx+=tilesize,ch++) {
        if (ch>'z') ch=0x05;
        graf_draw_tile(&g.graf,texid_tiles,dstx,dsty,ch,0);
      }
    }
    graf_draw_tile(
      &g.graf,texid_tiles,
      x+margin+(tilesize>>1)+g.wcx*tilesize,
      y+margin+(tilesize>>1)+g.wcy*tilesize,
      0x00,cursorxform
    );
  }
  
  graf_flush(&g.graf);
}
