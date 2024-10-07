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

/* (dst) must point to 7 chars.
 * Remove up to 7 tiles from the set and write them at (dst).
 * If there are fewer than 7 remaining, write trailing zeroes to (dst).
 * Returns the count drawn, zero if the bag is empty.
 */
int letterbag_draw(char *dst,struct letterbag *bag);

/* Same idea as letterbag_draw() but only fill in existing zeroes in (dst).
 */
int letterbag_draw_partial(char *dst,struct letterbag *bag);

#endif
