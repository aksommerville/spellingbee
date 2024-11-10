/* battler.h
 * Represents one contestant in a battle, either human or CPU.
 */
 
#ifndef BATTLER_H
#define BATTLER_H

#include "dict.h"

struct battle;

#define BATTLER_FACE_NEUTRAL 0
#define BATTLER_FACE_ATTACK 1
#define BATTLER_FACE_OOPS 2
#define BATTLER_FACE_HURT 3
#define BATTLER_FACE_DEAD 4
#define BATTLER_FACE_WIN 5

/* CPU players try to discover the entire set of possible words,
 * but they'll never store more than so many.
 * Once the list is full, new words evict the current least-valuable word.
 */
#define BATTLER_SEARCH_LIMIT 64

/* Seconds per word, to pay out the initial search for CPU players.
 * Roughly (desired total time / dictionary size).
 */
#define BATTLER_SEC_PER_SEARCH 0.00001

struct battler {
  int id; // 1,2. Owner sets and we never change.
  int human;
  char name[16]; // For display.
  int namec;
  int dictid;
  struct {
    int imageid; // All battler avatars face right naturally.
    int x,y,w,h; // Bounds of neutral, leftmost face.
    int face; // srcx=x+w*face
  } avatar;
  
  /* Tiles currently drawn and playable.
   * Uppercase letters, zero for none, or '@' for blank.
   */
  char hand[7];
  
  /* In-progress word assembly, for human battlers only.
   * Uppercase letters, lowercase letters, and trailing zeroes.
   */
  char stage[7];
  
  /* The word being played, established at the end of gather stage.
   */
  char attack[7];
  int attackc;
  int force;
  struct rating_detail detail;
  
  /* Inventory and HP are stored independently here.
   * In a typical 1-player game, they sync against global at the start and end of the battle.
   */
  int inventory[ITEM_COUNT];
  int hp;
  
  int ready; // Signals they've submitted during gather.
  int confirm_fold; // You have to click fold twice to actually do it.
  int selx,sely; // Input cursor. 3 rows of 7 columns.
  int modifier; // itemid
  int maxword; // CPU player won't search for words longer than this. Default 7.
  double wakeup; // (s) CPU player folds if player completes gather faster than this.
  double charge; // (s) Gather time before we play our most effective candidate. >wakeup
  // These constraints apply to the battler that plays the constrained word (ie Dot, not the foe):
  char forbidden[8]; // Must be terminated, so effective limit 7.
  char super_effective[8]; // ''
  char finisher; // The killing stroke must contain this letter, otherwise hp clamps to 1.
  int reqlen; // Only words of this exact length are effective.
  int damage; // Damage recently sustained, for display.
  double damageclock;
  int damage_done; // So we don't accidentally hurt twice per cycle.
  int disphp; // What to display right now; moves toward (hp) over time.
  
  /* Wildcard modal, for human players only.
   */
  int wcmodal;
  int wcx,wcy;
  
  /* Dictionary search, for CPU players only.
   */
  struct dict_bucket bucketv[6];
  int bucketc;
  struct candidate {
    int score;
    char word[7]; // Pad with zeroes if len<7. Uppercase only.
  } candidatev[BATTLER_SEARCH_LIMIT];
  int candidatec;
  double gatherclock; // Counts up during GATHER stage.
  double searchclock; // For paying out the initial search.
  int searchbucketp; // Which bucket searching.
  int searchwordp; // Which word in bucket. If >=bucketc, search is finished.
  char startv[7]; // Hand, in alphabetical order, with duplicates and wildcards removed.
  int startc;
  int startp;
};

void battler_init_human(struct battler *battler);
void battler_init_cpu(struct battler *battler);//XXX Probably need to be more detailed than that.

/* Battle must call these for both battlers at the start of the GATHER stage.
 * Battle initializes (hand) first.
 */
void battler_begin_round(struct battler *battler,struct battle *battle);

/* Battle calls for both battlers at each cycle of GATHER stage.
 */
void battler_update(struct battler *battler,double elapsed,struct battle *battle);

/* Notify that GATHER stage is ending.
 * CPU battlers will use this time to commit their current best play.
 */
void battler_commit(struct battler *battler,struct battle *battle);

/* Battle will only call this for humans, and only when in GATHER stage.
 */
void battler_move(struct battler *battler,int dx,int dy);
void battler_activate(struct battler *battler);
void battler_cancel(struct battler *battler);

#endif
