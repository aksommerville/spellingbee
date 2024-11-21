/* battle.h
 * Manages spelling battles between player and CPU, or between two players.
 * This must be wrapped in a "battle" modal.
 */
 
#ifndef BATTLE_H
#define BATTLE_H

#include "battler.h"
#include "letterbag.h"

#define BATTLE_STAGE_WELCOME  1
#define BATTLE_STAGE_GATHER   2 /* Interactive, and foe runs. */
#define BATTLE_STAGE_ATTACK1  3 /* Noninteractive, animate p1's play. In 2-player, it's whoever submitted first, not necessarily p1. */
#define BATTLE_STAGE_ATTACK2  4 /* '' p2 */
#define BATTLE_STAGE_P1WIN    5 /* Reporting victory. */
#define BATTLE_STAGE_P2WIN    6 /* In 1-player, reporting defeat. */
#define BATTLE_STAGE_TERM     7 /* If we enter this stage, we'll report termination at update. */

/* Wildcard modal must have at least 26 cells, and no wider than (COLC>>1) ie 10.
 * I think the squarer the better, to minimize travel.
 */
#define WCMODAL_COLC 6
#define WCMODAL_ROWC 5

#define BATTLE_WELCOME_TIME 1.000
#define BATTLE_ATTACK_TIME 3.000
#define BATTLE_FOLD_TIME 1.500 /* ATTACK stages run shorter if the attacker folds, there's not much to see. */
#define BATTLE_DAMAGE_TIME 1.000

#define BATTLE_HP_TICK_TIME 0.050 /* Seconds per HP unit. */

struct battle {
  int rid; // For diagnostics, and serves as a loaded flag. Never zero if load succeeds.
  int flagid; // Owner may set directly. If nonzero and the player wins, we set this flag.
  int songid;
  
  int stage; // BATTLE_STAGE_*
  double stageclock; // Counts down. <=0 for indefinite.
  struct battler p1,p2;
  struct battler *first,*second; // (null,p1,p2). Present during the ATTACK stages.
  int texid_msg,w_msg,h_msg;
  struct letterbag letterbag;
  int novowels; // If set, we'll remove all vowels from the letterbag at each shuffle.
  int dark;
  
  double cursorclock;
  int cursorframe;
  double hpclock;
  double bonus3x; // Counts down while we report an awarded 3xword item.
  int last_arrived;
  
  uint32_t *log; // RGBA
  int logw,logh;
  int log_texid;
  int logdirty;
};

void battle_cleanup(struct battle *battle);
void battle_init(struct battle *battle);

/* Aside from this call, the global game state is read-only to battle.
 * Call this before cleanup to update globals from the battle's final state.
 */
void battle_commit_to_globals(struct battle *battle);

/* (src) is a "battle" resource.
 * Provide (rid) just for logging context.
 */
int battle_load(struct battle *battle,const char *src,int srcc,int rid);

void battle_set_dark(struct battle *battle);

/* Update returns zero if finished or >0 if still running.
 * Render uses (g.graf) and overwrites the entire framebuffer.
 */
int battle_update(struct battle *battle,double elapsed);
void battle_render(struct battle *battle);

/* User inputs.
 */
void battle_move(struct battle *battle,int playerid,int dx,int dy);
void battle_activate(struct battle *battle,int playerid); // SOUTH
void battle_cancel(struct battle *battle,int playerid); // WEST

/* Private.
 */
void battle_begin_damage(struct battle *battle,struct battler *victim,int force);
void battle_log(struct battle *battle,const char *src,int srcc,uint32_t rgba);
void battle_logf(struct battle *battle,uint32_t rgba,const char *fmt,...); // Only '%s' and '%d'
int battle_decode(struct battle *battle,const char *src,int srcc,int rid);

#endif
