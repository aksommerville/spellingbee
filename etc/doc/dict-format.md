# Spelling Bee Dictionary Format

Input is plain text, uppercase words separated by LF, sorted by length then alphabetical.

Binary format is the same, but with the LFs removed, and a 12-byte header first: 2 bytes each, count of words of each length.
