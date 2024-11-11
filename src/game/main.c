#include "bee.h"
#include "battle/dict.h"

struct globals g={0};

/* Quit.
 */

void egg_client_quit(int status) {
}

/* Init.
 */

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
  
  if (world_init(&g.world)<0) return -1;
  
  /*XXX Start with some items. */
  g.inventory[ITEM_2XLETTER]=3;
  g.inventory[ITEM_3XLETTER]=3;
  g.inventory[ITEM_2XWORD]=3;
  g.inventory[ITEM_3XWORD]=3;
  g.inventory[ITEM_ERASER]=3;
  /**/
  
  /*XXX Quick cudgel to check a word's score one-off. *
  const char word[]="GUMDROP";
  struct rating_detail detail={
    .modifier=ITEM_NOOP,
    .forbidden="",
    .super_effective="",
    .lenonly=0,
  };
  int score=dict_rate_word(&detail,RID_dict_nwl2023,word,sizeof(word)-1);
  fprintf(stderr,"'%s' (%d,%s,%s,%d) => %d\n",word,detail.modifier,detail.forbidden,detail.super_effective,detail.lenonly,score);
  return -1;
  /**/
  
  return 0;
}

/* Update.
 */

void egg_client_update(double elapsed) {
  
  int input=egg_input_get_one(0);//TODO 2-player mode
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) egg_terminate(0);
    if (g.modalc>0) {
      modal_input(g.modalv[g.modalc-1],input,g.pvinput);
    } else {
      if ((input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) world_activate(&g.world);
      if ((input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST)) world_cancel(&g.world);
    }
    g.pvinput=input;
  }
  
  if (g.modalc>0) {
    modal_update(g.modalv[g.modalc-1],elapsed);
  } else if (g.hp<=0) {
    fprintf(stderr,"Terminating due to dead. XP=%d\n",g.xp);
    egg_terminate(0);
  } else {
    world_update(&g.world,elapsed);
  }
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  
  int opaquep=-1;
  int i=g.modalc;
  while (i-->0) if (g.modalv[i]->opaque) { opaquep=i; break; }
  
  // Draw world, if there's no opaque modal.
  if (opaquep<0) {
    world_render(&g.world);
  }
  
  // Draw modals, starting with the first opaque one.
  if (opaquep<0) opaquep=0;
  for (;opaquep<g.modalc;opaquep++) {
    modal_render(g.modalv[opaquep]);
  }
  
  graf_flush(&g.graf);
}

/* Get resource.
 */
 
int rom_get_res(void *dstpp,int tid,int rid) {
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return 0;
  struct rom_res *res;
  while (res=rom_reader_next(&reader)) {
    if (res->tid>tid) return 0;
    if (res->tid<tid) continue;
    if (res->rid>rid) return 0;
    if (res->rid<rid) continue;
    *(const void**)dstpp=res->v;
    return res->c;
  }
  return 0;
}

/* Iterate map or sprite commands.
 */
 
int cmd_reader_next(const uint8_t **argv,uint8_t *opcode,struct cmd_reader *reader) {
  if (reader->p>=reader->c) return -1;
  *opcode=reader->v[reader->p++];
  if (!*opcode) { reader->p=reader->c; return -1; }
  int argc=0;
  switch ((*opcode)&0xe0) {
    case 0x00: break;
    case 0x20: argc=2; break;
    case 0x40: argc=4; break;
    case 0x60: argc=8; break;
    case 0x80: argc=12; break;
    case 0xa0: argc=16; break;
    case 0xc0: if (reader->p>=reader->c) { reader->p=reader->c; return -1; } argc=reader->v[reader->p++]; break;
    case 0xe0: reader->p=reader->c; return -1;
  }
  if (reader->p>reader->c-argc) { reader->p=reader->c; return -1; }
  *argv=reader->v+reader->p;
  reader->p+=argc;
  return argc;
}
