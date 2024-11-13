#include "game/bee.h"
#include "letterbag.h"

/* Scrabble distribution per Wikipedia.
   And my teacup agrees.
   2 ?
   9 A
   2 B
   2 C
   4 D
  12 E
   2 F
   3 G
   2 H
   9 I
   1 J
   1 K
   4 L
   2 M
   6 N
   8 O
   2 P
   1 Q
   6 R
   4 S
   6 T
   4 U
   2 V
   2 W
   1 X
   2 Y
   1 Z
 ---
 100
 
*/

const char letterbag_sorted[LETTERBAG_SIZE]=
  "@@AAAAAAAA"
  "ABBCCDDDDE"
  "EEEEEEEEEE"
  "EFFGGGHHII"
  "IIIIIIIJKL"
  "LLLMMNNNNN"
  "NOOOOOOOOP"
  "PQRRRRRRSS"
  "SSTTTTTTUU"
  "UUVVWWXYYZ"
;

/* Reset.
 */
 
void letterbag_reset(struct letterbag *bag) {
  memcpy(bag->v,letterbag_sorted,LETTERBAG_SIZE);
  bag->c=LETTERBAG_SIZE;
  // Swap each slot with some other chosen at random, possibly itself.
  char *p=bag->v;
  int i=LETTERBAG_SIZE;
  for (;i-->0;p++) {
    char tmp=*p;
    int op=rand()%LETTERBAG_SIZE;
    *p=bag->v[op];
    bag->v[op]=tmp;
  }
}

/* Remove vowels.
 */
 
void letterbag_remove_vowels(struct letterbag *bag) {
  int i=bag->c;
  while (i-->0) {
    if ((bag->v[i]=='A')||(bag->v[i]=='E')||(bag->v[i]=='I')||(bag->v[i]=='O')||(bag->v[i]=='U')) {
      bag->c--;
      memmove(bag->v+i,bag->v+i+1,bag->c-i);
    }
  }
}

/* Draw.
 */

int letterbag_draw(char *dst,struct letterbag *bag) {
  memset(dst,0,7);
  return letterbag_draw_partial(dst,bag);
}

int letterbag_draw_partial(char *dst,struct letterbag *bag) {
  int result=0,i=0;
  for (;i<7;i++,dst++) {
    if (*dst) {
      result++;
      continue;
    }
    if (bag->c>0) {
      *dst=bag->v[--(bag->c)];
      result++;
    }
  }
  return result;
}
