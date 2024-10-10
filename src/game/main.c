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
  
  g.hp=100;
  encounter_begin(&g.encounter);
  
  return 0;
}

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
    }
    g.pvinput=input;
  }
  
  if (g.encounter.active) {
    encounter_update(&g.encounter,elapsed);
  } else {
    if (g.hp<=0) g.hp=100;
    encounter_begin(&g.encounter);
  }
}

void egg_client_render() {
  graf_reset(&g.graf);
  if (g.encounter.active) {
    encounter_render(&g.encounter);
  } else {
    //TODO Whatever happens when we're not in an encounter...
    graf_draw_rect(&g.graf,0,0,g.fbw,g.fbh,0x808080ff);
  }
  graf_flush(&g.graf);
}
