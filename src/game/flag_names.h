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
#define FLAG_bodyguard1 12
#define FLAG_bodyguard2 13
#define FLAG_bodyguard3 14
#define FLAG_bodyguard4 15
#define FLAG_bodyguard5 16
#define FLAG_bodyguard6 17
#define FLAG_bodyguard7 18
#define FLAG_bodyguard8 19
#define FLAG_bodyguard9 20
#define FLAG_bodyguard10 21
#define FLAG_watercan 22 /* Highly volatile: Is she carrying the watercan right now? */
#define FLAG_watercan1 23
#define FLAG_watercan2 24
#define FLAG_watercan3 25
#define FLAG_bridge1 26
#define FLAG_bridge2 27
#define FLAG_bridge3 28
#define FLAG_lablock1 29
#define FLAG_lablock2 30
#define FLAG_lablock3 31
#define FLAG_lablock4 32
#define FLAG_lablock5 33
#define FLAG_lablock6 34
#define FLAG_lablock7 35
#define FLAG_graverob1 36
#define FLAG_graverob2 37
#define FLAG_graverob3 38
#define FLAG_graverob4 39
#define FLAG_graverob5 40
#define FLAG_watercan4 41
#define FLAG_bridge4 42
#define FLAG_flower 43
#define FLAG_flower_done 44
#define FLAG_goody_hello 45

#endif
