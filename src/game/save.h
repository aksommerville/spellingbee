/* save.h
 * Manages the saved game.
 */
 
#ifndef SAVE_H
#define SAVE_H

#define ITEM_NOOP       0 /* Don't use zero. */
#define ITEM_BUGSPRAY   1
#define ITEM_UNFAIRIE   2
#define ITEM_2XWORD     3
#define ITEM_3XWORD     4
#define ITEM_ERASER     5
#define ITEM_COUNT      6

// Keep this exactly 32: It's convenient to have space for exactly 256 flags.
#define FLAGS_SIZE 32

struct saved_game {
  int hp; // 1..100
  int xp; // 0..32767
  int gold; // 0..32767
  uint8_t inventory[ITEM_COUNT]; // 0..99 each
  uint8_t flags[FLAGS_SIZE]; // Bits indexed by FLAG_*, little-endianly.
  int gravep; // Which grave in the cemetery is currently marked for treasure? Zero if none, or index+1.
  double playtime; // Total play time, including time in modals.
  int battlec; // Total count of battles fought.
  int wordc; // Total count of words played in battle, including invalid words but not folds.
  int scoretotal; // Sum of all positive word scores.
  int bestscore; // Points earned by (bestword), may include contextual bonuses.
  char bestword[7]; // Best single play you've made. Padded with zeroes.
  int stepc; // How many steps taken in outer world.
  int flower_stepc; // Counts up. Relevant if FLAG_flower.
  int bugspray;
};

/* For the most part the defaults are zero, but use this instead of assuming anything.
 * (eg hp must have a nonzero default).
 */
void saved_game_default(struct saved_game *game);

/* Keep valid content as is, and clamp anything out of bounds.
 */
void saved_game_force_valid(struct saved_game *game);

/* Null if valid, or a canned developer message if not.
 */
const char *saved_game_validate(const struct saved_game *game);

/* All encoding concerns are managed here.
 * Decoding always blindly wipes (game), and always leaves it valid on exit, even on failures.
 * Encoding never fails. If anything is out of range, we clamp it.
 * Encoded data is G0 with no space, quote, or backslash (ie it can be dropped verbatim in a JSON string token).
 * Longest possible encoded game is currently exactly 100 bytes.
 */
int saved_game_decode(struct saved_game *game,const void *src,int srcc);
int saved_game_encode(void *dst,int dsta,const struct saved_game *game);

#endif
