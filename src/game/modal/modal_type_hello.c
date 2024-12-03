/* modal_type_hello.c
 * The main menu, first thing a user sees.
 */
 
#include "game/bee.h"
#include "game/flag_names.h"

//TODO I was picturing Goody rolls in a chalkboard periodically and writes game-related tips on it.
// Too much effort for now, but maybe consider that in the future.

#define HELLO_OPTION_CONTINUE 0
#define HELLO_OPTION_NEW 1
#define HELLO_OPTION_BATTLE 2
#define HELLO_OPTION_SETTINGS 3
#define HELLO_OPTION_QUIT 4
#define HELLO_OPTION_CREDITS 5
#define HELLO_OPTION_LIMIT 6

/* Timing for Dot, periodically levitating tiles to spell "SPELLING BEE".
 */
#define DOT_START_TIME  2.0
#define DOT_RAISE_TIME  3.0
#define DOT_DROP_TIME  16.0
#define DOT_RECUP_TIME 17.0
#define DOT_PERIOD     23.0
#define DOT_FRAME_TIME 0.250

/* Cards with little blurbs of text that slide in periodically.
 */

// Relative to period start.
#define CARD_MESSAGE_START_TIME  3.0
#define CARD_MESSAGE_UP_TIME     4.0
#define CARD_MESSAGE_DOWN_TIME   9.0
#define CARD_MESSAGE_PERIOD     10.0

/* You get 4 lines, each with up to 19 characters.
 */
static const struct card_message {
  const char *msg;
  int only_if_save_exists;
} card_messagev[]={
  {"",1}, // Placeholder for saved game status.
  {"Beware!\n\"New Game\" will\nerase your save.",1},
  {"By AK Sommerville\nDecember 2024",0},
  {"For more games:\naksommerville.com",0},
};

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
  char savebin[256];
  int savebinc;
  struct saved_game save;
  char savedesc[256];
  int savedescc;
  int selp;
  int restore_music;
  
  int texid_typewriter;
  int16_t twcolw,twrowh;
  int card_messagep;
  double card_message_clock;
  int cardw,cardh;
  
  double dot_clock;
  double dot_anim_clock;
  int dot_anim_frame;
  struct floating_letter {
    char letter;
    double fx,fy; // Target position when levitated (top-left corner).
    double sx,sy; // Position when stacked (again, top-left corner, and there's dead space on the tile's top).
  } floating_letterv[11];
};

#define MODAL ((struct modal_hello*)modal)

/* Cleanup.
 */
 
static void _hello_del(struct modal *modal) {
  while (MODAL->optionc-->0) {
    egg_texture_del(MODAL->optionv[MODAL->optionc].texid);
  }
  egg_texture_del(MODAL->texid_typewriter);
}

/* Add option.
 * Position is initially zero, you must set later.
 */
 
static int hello_add_option(struct modal *modal,int index,int enable) {
  if (MODAL->optionc>=HELLO_OPTION_LIMIT) return -1;
  struct hello_option *option=MODAL->optionv+MODAL->optionc++;
  option->enable=enable?1:0;
  option->index=index;
  if ((option->texid=font_texres_oneline(g.font,RID_strings_hello,index,g.fbw,enable?0xffffffff:0x40c060ff))<1) return -1;
  egg_texture_get_status(&option->w,&option->h,option->texid);
  return 0;
}

/* Describe saved game for display as a card message.
 */
 
static int hello_describe_save(char *dst,int dsta,const struct saved_game *game) {
  int dstc=0;
  #define LITERAL(str) { \
    if (dstc>dsta-sizeof(str)) return 0; \
    memcpy(dst+dstc,str,sizeof(str)-1); \
    dstc+=sizeof(str)-1; \
  }
  #define DECINT(_v,mindigitc) { \
    if (dstc>dsta-4) return 0; \
    int v=_v; \
    if ((v>=1000)||(mindigitc>=4)) dst[dstc++]='0'+((v/1000)%10); \
    if ((v>=100)||(mindigitc>=3)) dst[dstc++]='0'+((v/100)%10); \
    if ((v>=10)||(mindigitc>=2)) dst[dstc++]='0'+((v/10)%10); \
    dst[dstc++]='0'+v%10; \
  }
  #define F(flagid) (game->flags[(flagid)>>3]&(1<<((flagid)&7)))
  
  LITERAL("Progress: ")
  int numer=0,denom=0;
  if (F(FLAG_book1)) numer++; denom++;
  if (F(FLAG_book2)) numer++; denom++;
  if (F(FLAG_book3)) numer++; denom++;
  if (F(FLAG_book4)) numer++; denom++;
  if (F(FLAG_book5)) numer++; denom++;
  if (F(FLAG_book6)) numer++; denom++;
  if (F(FLAG_graverob5)) numer++; denom++;
  if (F(FLAG_flower_done)) numer++; denom++;
  if (F(FLAG_englishprof)) numer++; denom++;
  if (F(FLAG_blackbelt)) numer++; denom++;
  int pct=(numer*100)/denom;
  DECINT(pct,1)
  LITERAL("%\n")
  
  LITERAL("Books: ")
  int bookc=0;
  if (F(FLAG_book1)) bookc++;
  if (F(FLAG_book2)) bookc++;
  if (F(FLAG_book3)) bookc++;
  if (F(FLAG_book4)) bookc++;
  if (F(FLAG_book5)) bookc++;
  if (F(FLAG_book6)) bookc++;
  DECINT(bookc,1)
  LITERAL("/6\n")
  
  LITERAL("Time: ")
  int sec=(int)game->playtime;
  int min=sec/60; sec%=60;
  int hour=min/60; min%=60;
  DECINT(hour,1)
  LITERAL(":")
  DECINT(min,2)
  LITERAL(":")
  DECINT(sec,2)
  LITERAL("\n")
  
  LITERAL("Gold: ")
  DECINT(game->gold,1)
  LITERAL("\n")
  
  #undef LITERAL
  #undef DECINT
  #undef F
  return dstc;
}

/* Init.
 */
 
static int _hello_init(struct modal *modal) {
  modal->opaque=1;
  
  egg_texture_load_image(MODAL->texid_typewriter=egg_texture_new(),RID_image_typewriter);
  int w=0,h=0;
  egg_texture_get_status(&w,&h,MODAL->texid_typewriter);
  MODAL->twcolw=w/16;
  MODAL->twrowh=h/6;
  
  /* Prepare the set of levitatable tiles.
   */
  struct floating_letter *letter=MODAL->floating_letterv;
  int16_t fx=105,fy=25;
  int16_t sx=190,sy=69;
  const char *letters="SPELLINGBEE";
  int i=0;
  for (;i<11;i++,letter++,fx+=12,sy-=2) {
    if (i==8) fx+=12;
    letter->letter=letters[i];
    letter->fx=fx;
    letter->fy=fy-sin((i*M_PI)/10)*8.0;
    letter->sx=sx;
    letter->sy=sy;
  }
  
  /* Acquire the encoded saved game.
   */
  MODAL->savebinc=egg_store_get(MODAL->savebin,sizeof(MODAL->savebin),"save",4);
  if ((MODAL->savebinc<0)||(MODAL->savebinc>sizeof(MODAL->savebin))) MODAL->savebinc=0;
  saved_game_decode(&MODAL->save,MODAL->savebin,MODAL->savebinc);
  MODAL->savedescc=hello_describe_save(MODAL->savedesc,sizeof(MODAL->savedesc),&MODAL->save);
  
  /* Decide which options are available.
   * TODO Can we optionally disable QUIT via some Egg runtime setting? Thinking about kiosks.
   * TODO Enable SETTINGS when ready. Or remove it. I don't think there will be any tings to set.
   */
  hello_add_option(modal,HELLO_OPTION_CONTINUE,MODAL->savebinc);
  hello_add_option(modal,HELLO_OPTION_NEW,1);
  hello_add_option(modal,HELLO_OPTION_BATTLE,1);
  hello_add_option(modal,HELLO_OPTION_CREDITS,1);
  hello_add_option(modal,HELLO_OPTION_SETTINGS,0);
  hello_add_option(modal,HELLO_OPTION_QUIT,1);
  
  /* Position options.
   * Center vertically in the lower half of the screen.
   * Horizontally, maintain a flush left edge, near the screen's left.
   */
  int hsum=0;
  for (i=0;i<MODAL->optionc;i++) {
    struct hello_option *option=MODAL->optionv+i;
    hsum+=option->h;
  }
  int x=40;
  int y=((g.fbh*3)>>2)-(hsum>>1);
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
  
  egg_play_song(RID_song_open_arms,0,1);
  
  return 0;
}

/* User actions.
 */
 
static void hello_do_continue(struct modal *modal) {
  if (!MODAL->savebinc) return;
  egg_play_sound(RID_sound_ui_activate);
  if (world_init(&g.world,MODAL->savebin,MODAL->savebinc)<0) {
    fprintf(stderr,"world_init(continue) failed!\n");
    return;
  }
  modal_pop(modal);
}
 
static void hello_do_new(struct modal *modal) {
  egg_play_sound(RID_sound_ui_activate);
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
  // Set a flag so when our updates resume we know to restore the music.
  MODAL->restore_music=1;
}
 
static void hello_do_settings(struct modal *modal) {
  fprintf(stderr,"%s\n",__func__);
  //TODO Push settings modal, once there is such a thing.
}
 
static void hello_do_quit(struct modal *modal) {
  egg_terminate(0);
}

static void hello_do_credits(struct modal *modal) {
  modal_victory_begin_narrativeless();
  MODAL->restore_music=1;
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
    case HELLO_OPTION_CREDITS: hello_do_credits(modal); break;
  }
}

/* Move selection.
 */
 
static void hello_move(struct modal *modal,int d) {
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
 
static void _hello_input(struct modal *modal,int input,int pvinput) {
  if ((input&EGG_BTN_UP)&&!(pvinput&EGG_BTN_UP)) hello_move(modal,-1);
  if ((input&EGG_BTN_DOWN)&&!(pvinput&EGG_BTN_DOWN)) hello_move(modal,1);
  if ((input&EGG_BTN_SOUTH)&&!(pvinput&EGG_BTN_SOUTH)) { hello_activate(modal); return; }
}

/* Update.
 */
 
static void _hello_update(struct modal *modal,double elapsed) {

  // Restore music on return from 2-player modal.
  if (MODAL->restore_music) {
    MODAL->restore_music=0;
    egg_play_song(RID_song_open_arms,0,1);
  }

  // Card message.
  if ((MODAL->card_message_clock+=elapsed)>=CARD_MESSAGE_PERIOD) {
    MODAL->card_message_clock-=CARD_MESSAGE_PERIOD;
    MODAL->card_messagep++;
    const int c=sizeof(card_messagev)/sizeof(struct card_message);
    if (MODAL->card_messagep>=c) MODAL->card_messagep=0;
  }
  if (!MODAL->savebinc&&card_messagev[MODAL->card_messagep].only_if_save_exists) {
    MODAL->card_messagep++;
    const int c=sizeof(card_messagev)/sizeof(struct card_message);
    if (MODAL->card_messagep>=c) MODAL->card_messagep=0;
  }
  
  // Dot.
  if ((MODAL->dot_clock+=elapsed)>=DOT_PERIOD) {
    MODAL->dot_clock-=DOT_PERIOD;
  }
  if ((MODAL->dot_anim_clock-=elapsed)<0.0) {
    MODAL->dot_anim_clock+=DOT_FRAME_TIME;
    if (++(MODAL->dot_anim_frame)>=2) MODAL->dot_anim_frame=0;
  }
}

/* Render some text with our typewriter.
 * Egg's font doesn't do alpha, so we're implementing this on our own.
 * We can cheat in two ways Egg can't: It's stricly monospaced, and only G0.
 * (dstx,dsty) are the top-left corner of the first glyph.
 * We respect explicit LFs, but don't break lines. Do that manually.
 */
 
static void hello_typewrite(struct modal *modal,int16_t dstx,int16_t dsty,const char *src,int srcc) {
  if (!src) return;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  graf_set_tint(&g.graf,0x282830ff);
  int16_t dstx0=dstx;
  for (;srcc-->0;src++,dstx+=MODAL->twcolw) {
    uint8_t tileid=*src;
    if (tileid==0x0a) {
      dstx=dstx0-MODAL->twcolw;
      dsty+=MODAL->twrowh;
      continue;
    }
    if ((tileid<0x20)||(tileid>=0x80)) continue;
    int16_t srcx=(tileid&0x0f)*MODAL->twcolw;
    int16_t srcy=((tileid-0x20)>>4)*MODAL->twrowh;
    graf_draw_decal(&g.graf,MODAL->texid_typewriter,dstx,dsty,srcx,srcy,MODAL->twcolw,MODAL->twrowh,0);
  }
  graf_set_tint(&g.graf,0);
}

/* Generate dynamic card messages.
 * Always returns in 0..dsta.
 */
 
static int hello_generate_card_message(char *dst,int dsta,struct modal *modal,const struct card_message *msg) {

  // Save status.
  if (msg->only_if_save_exists) {
    if (MODAL->savedescc>dsta) return 0;
    memcpy(dst,MODAL->savedesc,MODAL->savedescc);
    return MODAL->savedescc;
  }
  
  // ?
  if (dsta>=3) {
    memcpy(dst,"???",3);
    return 3;
  }
  return 0;
}

/* Letters, possibly being levitated by Dot.
 */
 
static void hello_draw_letters(struct modal *modal,int texid) {
  double t;
  if (MODAL->dot_clock>=DOT_DROP_TIME) t=1.0-(MODAL->dot_clock-DOT_DROP_TIME)/(DOT_RECUP_TIME-DOT_DROP_TIME);
  else if (MODAL->dot_clock>=DOT_RAISE_TIME) t=1.0;
  else if (MODAL->dot_clock>=DOT_START_TIME) t=(MODAL->dot_clock-DOT_START_TIME)/(DOT_RAISE_TIME-DOT_START_TIME);
  else t=0.0;
  if (t<0.0) t=0.0; else if (t>1.0) t=1.0;
  double eet=1.0-t;
  double tx=t*t;
  double eetx=1.0-tx;
  const struct floating_letter *letter=MODAL->floating_letterv;
  int i=11;
  for (;i-->0;letter++) {
    int16_t dstx=(int16_t)(letter->fx*tx+letter->sx*eetx);
    int16_t dsty=(int16_t)(letter->fy*t+letter->sy*eet);
    if ((t>0.0)&&(MODAL->dot_anim_frame&1)) {
      if (i&1) dsty++;
      else dsty--;
    }
    int16_t srcx=(t<=0.0)?288:(1+(letter->letter-'A')*11);
    int16_t srcy=40;
    graf_draw_decal(&g.graf,texid,dstx,dsty,srcx,srcy,11,11,0);
  }
}

/* Render.
 */
 
static void _hello_render(struct modal *modal) {
  graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x206030ff);
  
  // Dot levitating letters.
  int texid_bits=texcache_get_image(&g.texcache,RID_image_hellobits);
  if ((MODAL->dot_clock<DOT_START_TIME)||(MODAL->dot_clock>DOT_DROP_TIME)) {
    graf_draw_decal(&g.graf,texid_bits,155,45,1,1,28,38,0);
  } else {
    graf_draw_decal(&g.graf,texid_bits,155,45,30+MODAL->dot_anim_frame*29,1,28,38,0);
  }
  hello_draw_letters(modal,texid_bits);
  
  // Card message in the lower right.
  if (MODAL->card_message_clock>=CARD_MESSAGE_START_TIME) {
    const struct card_message *msg=card_messagev+MODAL->card_messagep;
    int texid_card=texcache_get_image(&g.texcache,RID_image_hello_card);
    if (!MODAL->cardw) {
      egg_texture_get_status(&MODAL->cardw,&MODAL->cardh,texid_card);
    }
    int16_t cardx=g.fbw-MODAL->cardw-10;
    int16_t cardy;
    if (MODAL->card_message_clock>=CARD_MESSAGE_DOWN_TIME) {
      cardy=g.fbh-MODAL->cardh+MODAL->cardh*((MODAL->card_message_clock-CARD_MESSAGE_DOWN_TIME)/(CARD_MESSAGE_PERIOD-CARD_MESSAGE_DOWN_TIME));
    } else if (MODAL->card_message_clock>=CARD_MESSAGE_UP_TIME) {
      cardy=g.fbh-MODAL->cardh;
    } else {
      cardy=g.fbh-MODAL->cardh*((MODAL->card_message_clock-CARD_MESSAGE_START_TIME)/(CARD_MESSAGE_UP_TIME-CARD_MESSAGE_START_TIME));
    }
    graf_draw_decal(&g.graf,texid_card,cardx,cardy,0,0,MODAL->cardw,MODAL->cardh,0);
    if (msg->msg[0]) { // Static text from the schedule.
      hello_typewrite(modal,cardx+5,cardy+5,msg->msg,-1);
    } else { // Placeholders that we generate on the fly.
      char tmp[256];
      int tmpc=hello_generate_card_message(tmp,sizeof(tmp),modal,msg);
      hello_typewrite(modal,cardx+5,cardy+5,tmp,tmpc);
    }
  }
  
  // Options menu.
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
