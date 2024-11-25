/* hack.h
 * Interface to some top-level hacks for testing animation, etc.
 * Nothing here should be reachable in production builds.
 */
 
#ifndef HACK_H
#define HACK_H

/* Name of one of the externs below, or leave undefined for normal play.
 */
//#define HACK_SELECTED hack_avatar_animation

extern const struct hack *ghack;

/* All hooks are required.
 * No context; use globals.
 * In render, (g.graf) is initialized and flushed for you, but nothing else will render.
 * Main manages AUX3 to quit, all other input is yours.
 */
struct hack {
  int (*init)();
  void (*update)(double elapsed,int input);
  void (*render)();
};

extern const struct hack hack_avatar_animation;

#endif
