#include "bee.h"
#include "battle/dict.h"
#include "flag_names.h"
#include "hack.h"

struct globals g={0};

/* Quit.
 */

void egg_client_quit(int status) {
  save_game();
}

/* Init.
 */

int egg_client_init() {
  egg_texture_get_status(&g.fbw,&g.fbh,1);
  
  g.texcache.graf=&g.graf;
  
  if ((g.romc=egg_get_rom(0,0))<=0) return -1;
  if (!(g.rom=malloc(g.romc))) return -1;
  if (egg_get_rom(g.rom,g.romc)!=g.romc) return -1;
  strings_set_rom(g.rom,g.romc);
  
  if (!(g.font=font_new())) return -1;
  if (font_add_image_resource(g.font,0x0020,RID_image_font9_0020)<0) return -1;
  
  srand_auto();
  
  if (!modal_spawn(&modal_type_hello)) return -1;
  
  if (ghack) {
    int err=ghack->init();
    if (err<0) return err;
  }
  
  return 0;
}

/* Update.
 */

void egg_client_update(double elapsed) {

  if (ghack) {
    int input=egg_input_get_one(0);
    if (input!=g.pvinput) {
      if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) egg_terminate(0);
      g.pvinput=input;
    }
    ghack->update(elapsed,input);
    return;
  }
  
  g.stats.playtime+=elapsed;
  
  int input=egg_input_get_one(0);//TODO 2-player mode
  if (input!=g.pvinput) {
    if ((input&EGG_BTN_AUX3)&&!(g.pvinput&EGG_BTN_AUX3)) egg_terminate(0);
    if (g.modalc>0) {
      modal_input(g.modalv[g.modalc-1],input,g.pvinput);
    } else {
      if ((input&EGG_BTN_AUX1)&&!(g.pvinput&EGG_BTN_AUX1)) modal_spawn(&modal_type_pause);
      if ((input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) world_activate(&g.world);
      if ((input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST)) world_cancel(&g.world);
    }
    g.pvinput=input;
  }
  
  if (g.modalc>0) {
    modal_update(g.modalv[g.modalc-1],elapsed);
  } else {
    world_update(&g.world,elapsed);
  }
  
  if (!g.modalc&&(g.stats.hp<=0)) {
    save_game();
    modal_spawn(&modal_type_hello);
  }
}

/* Render.
 */

void egg_client_render() {
  graf_reset(&g.graf);
  
  if (ghack) {
    ghack->render();
    graf_flush(&g.graf);
    return;
  }
  
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

/* Save game, if there's one running.
 */
 
void save_game() {
  if (!g.world.mapid) return; // Looks like no game running -- don't save. eg "Quit" immediately from hello modal.
  char save[256];
  int savec=saved_game_encode(save,sizeof(save),&g.stats);
  if ((savec>=0)&&(savec<=sizeof(save))) {
    if (egg_store_set("save",4,save,savec)>=0) {
      //fprintf(stderr,"Saved game.\n");
    }
  }
}

/* Flag bits.
 */
 
int flag_get(int flagid) {
  if (flagid<0) return 0;
  int major=flagid>>3;
  if (major>=sizeof(g.stats.flags)) return 0;
  uint8_t mask=1<<(flagid&7);
  return g.stats.flags[major]&mask;
}

int flag_set(int flagid,int v) {
  if (!flag_set_nofx(flagid,v)) return 0;
  world_recheck_flags(&g.world);
  return 1;
}

int flag_set_nofx(int flagid,int v) {
  if (flagid<0) return 0;
  int major=flagid>>3;
  if (major>=sizeof(g.stats.flags)) return 0;
  uint8_t mask=1<<(flagid&7);
  if ((flagid==FLAG_zero)||(flagid==FLAG_one)) return 0; // These are not allowed to change.
  if (v) {
    if (g.stats.flags[major]&mask) return 0;
    g.stats.flags[major]|=mask;
  } else {
    if (!(g.stats.flags[major]&mask)) return 0;
    g.stats.flags[major]&=~mask;
  }
  return 1;
}

int something_being_carried() {
  #define _(tag) if (flag_get(FLAG_##tag)) return FLAG_##tag;
  _(watercan)
  _(flower)
  #undef _
  return 0;
}
