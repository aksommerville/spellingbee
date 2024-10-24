/* modal.h
 * Short-lived modal interactions are modelled here.
 * This does not include the outer world or spelling encounters.
 * Mostly it's dialogue with NPCs. Also a pause menu, if we do that?
 */
 
#ifndef MODAL_H
#define MODAL_H

struct modal;
struct modal_type;

struct modal {
  const struct modal_type *type;
  int opaque;
};

struct modal_type {
  const char *name;
  int objlen;
  void (*del)(struct modal *modal);
  int (*init)(struct modal *modal);
  void (*input)(struct modal *modal,int input,int pvinput);
  void (*update)(struct modal *modal,double elapsed);
  void (*render)(struct modal *modal);
};

/* Create or destroy modals directly.
 * I don't expect clients to call these.
 */
void modal_del(struct modal *modal);
struct modal *modal_new(const struct modal_type *type);

/* Remove a specific modal from the stack and delete it, or if (modal) null, remove and delete whatever's on top.
 * Spawn: Create a modal, push on the stack, and return a WEAK reference.
 */
void modal_pop(struct modal *modal);
struct modal *modal_spawn(const struct modal_type *type);

static inline void modal_input(struct modal *modal,int input,int pvinput) { if (modal&&modal->type->input) modal->type->input(modal,input,pvinput); }
static inline void modal_update(struct modal *modal,double elapsed) { if (modal&&modal->type->update) modal->type->update(modal,elapsed); }
static inline void modal_render(struct modal *modal) { if (modal&&modal->type->render) modal->type->render(modal); }

extern const struct modal_type modal_type_message;

void modal_message_begin_single(int rid,int index);

#endif
