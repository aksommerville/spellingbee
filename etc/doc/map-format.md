# Spelling Bee Map Format

What's in a map?
- Dimensions are rectangular, variable size up to 255x255.
- Each cell is one byte, referring to a tile from the sheet.
- Additional loose commands.
- No edge neighbors. Map-map relations are effected through doors.

There's a text format for storage and consumption by the editor,
and a binary format that ships with the finished game.

## Text Format

Begins with the map image.
This is a giant hex dump.
Lines must all be the same length.
Leading and trailing space are not permitted. So (n*2) hex digits plus an LF, same for each line.

Followed by one blank line.

Followed by commands: `KEYWORD [ARGS...]`
ARGS are generic:
- Hexadecimal integer with "0x" prefix and even length => Integer of provided length. (can be arbitrarily long)
- Plain integer => u8
- `(u16)N` => explicit-length integer. "(u8)", "(u16)", "(u24)", "(u32)"
- JSON strings evaluate then emit verbatim.
- `@X,Y` => Two bytes. This notation flags it as a Point Of Interest for the editor.
- `@X,Y,W,H` => Four bytes. ''
- `TYPE:NAME` => Resource ID in two bytes. Name or ID input.

KEYWORD may also be a plain integer in 0..255.
Each KEYWORD has an intrisic expected length, and it's an error if ARGS don't match that.
KEYWORDs whose opcode is in 0xc0..0xdf take a variable-length argument, 0..255 bytes. You don't provide the length explicitly.

## Binary Format

Begins with 4-byte signature: "\0MAP"

Followed by:
```
u8 width: 1..255
u8 height: 1..255
```

Width and height are unbiased and zero is illegal.
Followed by raw map data, LRTB, one byte per cell.

Followed by commands.
One byte opcode, then an argument whose length is knowable from the high 3 bits of opcode:
```
00000000 => EOF
000xxxxx => 0
001xxxxx => 2
010xxxxx => 4
011xxxxx => 8
100xxxxx => 12
101xxxxx => 16
110xxxxx => Next byte is remaining length.
111xxxxx => Reserved
```

## Commands

| Opcode | Keyword         | Description |
|--------|-----------------|-------------|
|   0x00 | EOF             | Optional. |
|   0x20 | song            | u16:songid |
|   0x21 | image           | u16:imageid |
|   0x22 | hero            | u8:x u8:y ; Start here if we didn't enter from a door. eg the first map |
|   0x60 | door            | u8:srcx u8:srcy u16:mapid u8:dstx u8:dsty u8:reserved1 u8:reserved2 |
|   0x61 | sprite          | u16:spriteid u8:x u8:y u32:params |
|   0x62 | message         | u8:srcx u8:srcy u16:stringid u16:index u8:action u8:qualifier |

`action` for 0x62 `message`:
- 0: Nothing extra.
- 1: Restore HP.
