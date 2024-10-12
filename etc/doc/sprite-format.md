# Spelling Bee Sprite Format

This is almost exactly the same as our map format, and that's intentional, they share some plumbing.

See "map-format.md" for encoding semantics. Same thing for sprites, except:
- Signature is "\0SPR".
- No leading map image or dimensions.
- Different set of commands (but same length rules).
- Undecorated sprite type names are accepted as 2-byte argument tokens. (relevant to the `type` command).

## Commands

| Opcode | Keyword         | Description |
|--------|-----------------|-------------|
|   0x00 | EOF             | Optional. |
|   0x20 | image           | u16:imageid |
|   0x21 | type            | u16:sprtid |
|   0x22 | tile            | u8:tileid u8:xform |
|   0x40 | groups          | u32:grpmask |
