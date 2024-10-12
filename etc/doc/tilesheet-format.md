# Spelling Bee Tilesheet Format

`tilesheet` resources are parallel to `image` resources.
Contains extra data associated with the 256 tiles of the image.

There's a text format for source and editor, and a binary format to ship.
Text format contains some data that gets stripped during compile, relevant only to the editor.

## Text Format

The first non-empty line is a table name, or integer 1..255.
Followed by 16 lines of 32 hex digits each.
Tables must not be repeated, and must each contain all 256 cells.

## Binary Format

Starts with 4-byte signature: "\0TLS"

Followed by any number of tables:
```
u8 tableid
u8 tileid0
u8 count-1
... data, 1 byte per tile
```

Each table exists only once, but may be named multiple times.
Order matters: Every table is initially straight zeroes, and each table record overwrites, in order.
(Though if that comes up, you've formatted it suboptimally).
No need to sort the records otherwise (eg `tableid` can be out of order).

## Tables

Any table listed here without a `tableid` is dropped at compile, and only available to the editor.
The compiler quietly drops tables with unknown names.

| ID  | Name      | Description |
|-----|-----------|-------------|
|   0 |           | EOF, not a table. The rest of the record header is omitted. |
|   1 | physics   | Enum, see below. |
|   2 | overlay   | Tile ID to render at (y-1) to this one, above sprites. |
|   3 | animate   | Nonzero to statically animate with N tiles immediately following this one. |
|     | neighbors | 8 bits describing which of my neighbors must belong to the same family. 0x80..0x01=(NW,N,NE,W,E,SW,S,E) |
|     | family    | Arbitrary enum for neighbor matching. 0=None |
|     | weight    | Where multiple tiles match, how likely should this be? (0..254,255)=(likely,unlikely,appt only) |

## physics enum

| value | meaning |
|-------|---------|
|     0 | vacant  |
|     1 | solid   |
|     2 | water   |
|     3 | hole    |
