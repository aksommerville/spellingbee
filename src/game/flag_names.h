/* flag_names.h
 * Authoritative list of game state flags.
 * See FLAGS_SIZE in bee.h; we can go up to 2**FLAGS_SIZE-1. (references will be stored 8-bit)
 * !!! Be mindful of formatting. This file is read by the editor and resource compiler too. !!!
 */
 
#ifndef _FLAG_NAMES_H
#define _FLAG_NAMES_H

#define FLAG_zero 0 /* Dummy, value is always zero. */
#define FLAG_one 1 /* Dummy, value is always one. */
#define FLAG_book1 2
#define FLAG_book2 3
#define FLAG_book3 4
#define FLAG_book4 5
#define FLAG_book5 6
#define FLAG_book6 7
#define FLAG_lights1 8
#define FLAG_lights2 9
#define FLAG_lights3 10
#define FLAG_lights4 11

#endif
