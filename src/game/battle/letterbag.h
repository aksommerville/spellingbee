/* letterbag.h
 * The set of unselected tiles.
 */
 
#ifndef LETTERBAG_H
#define LETTERBAG_H

#define LETTERBAG_SIZE 100

struct letterbag {
  char v[LETTERBAG_SIZE];
  int c;
};

/* Completely overwrite (bag) with a fresh shuffle of the full set.
 */
void letterbag_reset(struct letterbag *bag);

/* Remove all AEIOU. Y remains.
 */
void letterbag_remove_vowels(struct letterbag *bag);

/* (dst) must point to 7 chars.
 * Remove up to 7 tiles from the set and write them at (dst).
 * If there are fewer than 7 remaining, write trailing zeroes to (dst).
 * Returns the count in (*dst) after filling, zero if both hand and bag are empty.
 * Uppercase letters, or '@' for wildcard.
 */
int letterbag_draw(char *dst,struct letterbag *bag);

/* Same idea as letterbag_draw() but only fill in existing zeroes in (dst).
 */
int letterbag_draw_partial(char *dst,struct letterbag *bag);

#endif
