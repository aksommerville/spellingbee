# Spelling Bee

Requires [Egg](https://github.com/aksommerville/egg) to build.

Let's make something like Dragon Warrior, except every encounter is a Scrabble contest.

Copied the 2023 NASPA word list and eliminated all words >7 letters; they won't be reachable.
Also eliminated a few profanities, it's kind of shocking to hear the CPU players use them.
Gives us about 57k words in a 400 kB resource.

## TODO

- [ ] Encounters
- - [ ] 2-player might need some incentive to play first.
- - - How about a visible clock that starts ticking when the first player commits, and if it expires, second player auto-folds?
- - [ ] Tunable difficulty. Not a per-foe thing, but global preferences from the player.
- - - [ ] (H) Disable foe clock, he always plays his best possible word. ie no advantage to playing fast.
- - - [ ] (E) Auto spell check when staging.
- - - - Alternative: Prevent committing invalid words.
- - - [ ] (E) Show potential score when staging.
- - - [ ] (E) Suggest words from hand.
- - - [ ] (E) Show foe's upcoming score as he searches.
- - - [ ] (E) What if we made a separate "pidgin" dictionary, a subset of the main dictionary with only well-known words?
- - - - The hard part is the filtering, there's 57000 words here. Time yourself on the first 1000, up to TAD. 2008..2028. ...eliminated about 400/1000.
- - - - Figure about 20 hours of deleting words to finish that... is it worth it? ...yes
- - - - 2024-11-09: I'm thru the 5-letter B words now. If we do just one letter a day, there's 76 left. Big but doable.
- - - - [ ] Also, once that's done, eliminate anagrams. No sense having both BANE and BEAN, so pick the prettier.
- [ ] Outer world.
- - [ ] editor: Map commands
- - [ ] editor: sprite
- - [ ] NPCs.
- - [ ] Dialogue.
- - [ ] Shops.
- [x] New item: eraser, to blank one tile.
- [ ] Consider eliminating the letter multiplier items, not much point to them.
- [ ] Sound effects.
- [ ] Music.
- [ ] Battle mode (2p local). Battle is written for it, just need a way in.
- [ ] Determine the most effective possible play. (in theory it's 207, for a word like ZWXJKYY but I doubt that exists)
- - Also odds of being dealt each length of word.
- [ ] Global flags, eg for one-time monsters.
- [ ] Saved game.
- [ ] Detailed logging for beta test.
- [ ] Hello splash with long cutscene, like Season of Penance.
- [ ] Remove LFs from the dictionary during compile, and insert a 12-byte header, count of words of each length. That saves 57 kB in the final build.
