# Spelling Bee Saved Games

Stored under Egg key "save".

Content is plain text, never more than 256 bytes.

```
  7  hp 1..100
 15  xp 0..32767
 15  gold 0..32767
 42  inventory 0..99 * 6 (might grow later)
256  flags 32 bytes
```

Devising a deliberately obscure format to discourage cheating.

```
   4 checksum
   1 hp
   2 xp
   2 gold
   1 inv count (including item zero, should be 6)
 ... inventory, 1 byte each
   1 flags byte count (should be <=32; omit trailing zeroes)
 ... flags
 ... zero pad to a multiple of 3
```
Expect 51 bytes binary.
After the binary is assembled and checksum computed, XOR each byte against the previous (post-XOR) byte.
Filter, and the leading checksum, make it likely that a small change to any content will yield wildly different output.

After that filter, encode base64ish against this alphabet:
```
0x23..0x5b => 0..56
0x5d..0x63 => 57..63
# $ % & ' ( ) * + , - . / 0 1 2 3 4 5 6 7 8 9 : ; < = > ? @ A B C D E F G H I J K L M N O P Q R S T U V W X Y Z [
] ^ _ ` a b c
```
This alphabet is as contiguous as possible without containing quote or backslash -- encoded text is JSON-safe.
Encoded text length is always a multiple of 4, and decoded binary a multiple of 3.

Checksum:
 - Start with a nonce: 0xc396a5e7
 - For each byte starting at 4 (including padding):
 - - Rotate buffer left 7 bits.
 - - XOR buffer against input.
 - Encode big-endianly.
 
Validation:
 - Length >= 12, and a multiple of 4
 - Base64
 - Checksum
 - (hp) in 1..100
 - (xp) in 0..32767
 - (gold) in 0..32767
 - Inventory count overflow
 - Inventory zero must be zero
 - All other inventory counts in 0..99
 - Flag length overflow
 - Flag zero must be zero
 - Flag one must be one
 
```
Defaults: {"save":"-*.Hb<Y@J<Y@I]Q>I]Q>I]M="} XXX INVALID: flags missing
HP 1, G 40, XP 10, 1 3XLETTER: #(Q/F[^1D+5OMMMMM^QNM^MM
98, 7, 1, no items: I/,X=#K-%SO/%CK-%CK-%CO,
85, 7, 11, 1 3XWORD: +(QOO1:H_Q_L_bcR_bcQ_QcP : XXX The 3XWORD item loaded as 2XLETTER, and after loading that, no items.
```
