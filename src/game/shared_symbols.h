/* Symbols visible to both C and Egg tools.
 */
 
#ifndef SHARED_SYMBOLS_H
#define SHARED_SYMBOLS_H

#define EGGDEV_importUtil "stdlib,res,graf,font,text"
#define EGGDEV_ignoreData "*.pre"

#define NS_sys_tilesize 16

#define NS_tilesheet_physics 1
#define NS_tilesheet_neighbors 0
#define NS_tilesheet_family 0
#define NS_tilesheet_weight 0

#define CMD_map_song               0x20 /* u16:songid */
#define CMD_map_image              0x21 /* u16:imageid */
#define CMD_map_hero               0x22 /* u16:pos */
#define CMD_map_battlebg           0x23 /* u16:row, image "battlebg" */
#define CMD_map_battle             0x40 /* u16:battleid u16:weight */
#define CMD_map_flagtile           0x41 /* u16:pos u8:flagid u8:reserved */
#define CMD_map_pickup             0x42 /* u16:pos u8:flagid u8:extraflag */
#define CMD_map_flageffect         0x43 /* u16:pos u8:flagid_effect u8:flagid_cause */
#define CMD_map_toggle             0x44 /* u16:pos u8:flagid u8:reserved */
#define CMD_map_door               0x60 /* u16:pos u16:mapid u16:dstpos u16:reserved */
#define CMD_map_sprite             0x61 /* u16:pos u16:spriteid u32:params */
#define CMD_map_message            0x62 /* u16:pos u16:stringid u16:index u8:action u8:qualifier */
#define CMD_map_lights             0x63 /* u16:pos u32:zone u8:flagid u8:reserved */
#define CMD_map_grave              0xe0 /* u16:pos ...text */

#define CMD_sprite_image         0x20 /* u16:imageid */
#define CMD_sprite_type          0x21 /* u16:spritetype */
#define CMD_sprite_tile          0x22 /* u8:tileid u8:xform */
#define CMD_sprite_reserved      0x2f /* u16: type-specific whatever */
#define CMD_sprite_groups        0x40 /* u32:bits */

#define NS_spritetype_dummy             0
#define NS_spritetype_hero              1
#define NS_spritetype_foe               2
#define NS_spritetype_kitchen           3
#define NS_spritetype_merchant          4
#define NS_spritetype_customer          5
#define NS_spritetype_karate            6
#define NS_spritetype_dialogue          7
#define NS_spritetype_archaeologist     8
#define NS_spritetype_lovers            9
#define NS_spritetype_goody            10
#define NS_spritetype_englishprof      11
#define NS_spritetype_blackbelt        12
#define NS_spritetype_mrclean          13

#define NS_flag_zero 0 /* Dummy, value is always zero. */
#define NS_flag_one 1 /* Dummy, value is always one. */
#define NS_flag_book1 2 /* cemetery */
#define NS_flag_book2 3 /* lab */
#define NS_flag_book3 4 /* gym */
#define NS_flag_book4 5 /* garden */
#define NS_flag_book5 6 /* cellar */
#define NS_flag_book6 7 /* queen */
#define NS_flag_lights1 8
#define NS_flag_lights2 9
#define NS_flag_lights3 10
#define NS_flag_lights4 11
#define NS_flag_bodyguard1 12
#define NS_flag_bodyguard2 13
#define NS_flag_bodyguard3 14
#define NS_flag_bodyguard4 15
#define NS_flag_bodyguard5 16
#define NS_flag_bodyguard6 17
#define NS_flag_bodyguard7 18
#define NS_flag_bodyguard8 19
#define NS_flag_bodyguard9 20
#define NS_flag_bodyguard10 21
#define NS_flag_watercan 22 /* Highly volatile: Is she carrying the watercan right now? */
#define NS_flag_watercan1 23
#define NS_flag_watercan2 24
#define NS_flag_watercan3 25
#define NS_flag_bridge1 26
#define NS_flag_bridge2 27
#define NS_flag_bridge3 28
#define NS_flag_lablock1 29
#define NS_flag_lablock2 30
#define NS_flag_lablock3 31
#define NS_flag_lablock4 32
#define NS_flag_lablock5 33
#define NS_flag_lablock6 34
#define NS_flag_lablock7 35
#define NS_flag_graverob1 36
#define NS_flag_graverob2 37
#define NS_flag_graverob3 38
#define NS_flag_graverob4 39
#define NS_flag_graverob5 40
#define NS_flag_watercan4 41
#define NS_flag_bridge4 42
#define NS_flag_flower 43
#define NS_flag_flower_done 44
#define NS_flag_goody_hello 45
#define NS_flag_englishprof 46 /* Has she collected the 60-point-word reward? */
#define NS_flag_lablock8 47 /* The lock that you want *unset* */
#define NS_flag_blackbelt 48
#define NS_flag_mrclean 49

#endif
