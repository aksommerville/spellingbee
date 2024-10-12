/* sprite.h
 * Abstract framework for sprites.
 * Sprites have a visible presence on screen, physical presence in the model world, and can update at each cycle.
 * Can instantiate programmatically or from a "sprite" resource.
 * Every sprite has a "type", ie some C code and metadata, which can be identified with an integer.
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprite_type;
struct sprite_group;

/* Registry of types.
 * To add a type, just append it to SPRITE_TYPE_FOR_EACH and create its struct sprite_type somewhere.
 * It will automatically get an id based on the order of SPRITE_TYPE_FOR_EACH.
 */
 
#define SPRITE_TYPE_FOR_EACH \
  _(dummy)
  
#define _(tag) extern const struct sprite_type sprite_type_##tag;
SPRITE_TYPE_FOR_EACH
#undef _

#endif
