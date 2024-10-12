#include "bee.h"

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
  
  /*TODO Proper game model init.
  g.hp=100;
  g.inventory[ITEM_2XLETTER]=2;
  g.inventory[ITEM_3XLETTER]=3;
  g.inventory[ITEM_2XWORD]=4;
  g.inventory[ITEM_3XWORD]=10;
  encounter_begin(&g.encounter);
  /**/
  if (world_init(&g.world)<0) return -1;
  
  return 0;
}

/* Update.
 */

void egg_client_update(double elapsed) {
  
  int input=egg_input_get_one(0);
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) egg_terminate(0);
    if (g.encounter.active) {
      if ((input&EGG_BTN_LEFT)&&!(g.pvinput&EGG_BTN_LEFT)) encounter_move(&g.encounter,-1,0);
      if ((input&EGG_BTN_RIGHT)&&!(g.pvinput&EGG_BTN_RIGHT)) encounter_move(&g.encounter,1,0);
      if ((input&EGG_BTN_UP)&&!(g.pvinput&EGG_BTN_UP)) encounter_move(&g.encounter,0,-1);
      if ((input&EGG_BTN_DOWN)&&!(g.pvinput&EGG_BTN_DOWN)) encounter_move(&g.encounter,0,1);
      if ((input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) encounter_activate(&g.encounter);
      if ((input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST)) encounter_cancel(&g.encounter);
    } else {
      if ((input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) world_activate(&g.world);
      if ((input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST)) world_cancel(&g.world);
    }
    g.pvinput=input;
  }
  
  if (g.encounter.active) {
    encounter_update(&g.encounter,elapsed);
  } else {
    /*XXX*
    if (g.hp<=0) {
      fprintf(stderr,"RESET HP\n");
      g.hp=100;
    }
    encounter_begin(&g.encounter);
    /**/
    world_update(&g.world,elapsed);
  }
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  if (g.encounter.active) {
    encounter_render(&g.encounter);
  } else {
    //TODO Whatever happens when we're not in an encounter...
    //graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x808080ff);
    world_render(&g.world);
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
