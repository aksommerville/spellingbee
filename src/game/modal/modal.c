#include "game/bee.h"

/* Delete.
 */
 
void modal_del(struct modal *modal) {
  if (!modal) return;
  if (modal->type->del) modal->type->del(modal);
  free(modal);
}

/* New.
 */
 
struct modal *modal_new(const struct modal_type *type) {
  if (!type) return 0;
  struct modal *modal=calloc(1,type->objlen);
  if (!modal) return 0;
  modal->type=type;
  if (type->init&&(type->init(modal)<0)) {
    modal_del(modal);
    return 0;
  }
  return modal;
}

/* Pop from stack.
 */

void modal_pop(struct modal *modal) {
  if (modal) {
    int i=g.modalc;
    while (i-->0) {
      if (g.modalv[i]==modal) {
        g.modalc--;
        memmove(g.modalv+i,g.modalv+i+1,sizeof(void*)*(g.modalc-i));
        modal_del(modal);
        return;
      }
    }
  } else if (g.modalc>0) {
    g.modalc--;
    modal_del(g.modalv[g.modalc]);
  }
}

/* Spawn.
 */
 
struct modal *modal_spawn(const struct modal_type *type) {
  if (g.modalc>=MODAL_LIMIT) return 0;
  struct modal *modal=modal_new(type);
  if (!modal) return 0;
  g.modalv[g.modalc++]=modal;
  return modal;
}
