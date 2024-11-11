# Spelling Bee Battle Resource

"battle" resources are the configuration for one full spelling encounter.

Battles can be triggered by bumping a "foe" sprite, random selection after a step, or by launching in 2-player mode.

Going to try using plain text as both source and delivery, with no compilation.
So it's plain text but strictly formatted. No comments, and extraneous whitespace is an error.

Each line is a key, a single space, and the value.
Integers must be decimal.

| Key             | Description |
|-----------------|-------------|
| name            | Foe's name, plain text. |
| player2         | No value. Indicates a 2-player battle instead of man-vs-cpu. |
| hp              | Integer, default 100 |
| maxword         | 2..7, default 7 |
| forbidden       | Uppercase letters that will have no effect. |
| super_effective | Uppercase letters that trigger a bonus. |
| finisher        | Single uppercase letter that must be present for a word to kill. (otherwise it stops at HP 1) |
| reqlen          | Words of any other length have no effect. |
| imageid         | Integer (NB no compiler: Can't use names) |
| imagerow        | Y position in image (div 64). X is always zero and dimensions always 48x64. |
| fulldict        | No value. Foe uses the same dictionary as hero, instead of Pidgin as usual. |
| wakeup          | ms, initial interval where the human can play for free. |
| charge          | ms, time before we'll play our most powerful word. |
| xp              | How much to award after winning. |
| gold            | '' |
| logcolor        | 6 hex digits |
