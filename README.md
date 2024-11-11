# Spelling Bee

Requires [Egg](https://github.com/aksommerville/egg) to build.

Let's make something like Dragon Warrior, except every encounter is a Scrabble contest.

Copied the 2023 NASPA word list and eliminated all words >7 letters; they won't be reachable.
Also eliminated a few profanities, it's kind of shocking to hear the CPU players use them.
Gives us about 57k words in a 400 kB resource.

## TODO

- [ ] Encounters
- - [x] Random battles configurable via map.
- - [x] Add to resource: Gold, XP, logcolor
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
- - [x] Special encounters, monsters that have their own particular rules.
- - - [x] Jackrabbit: You get only so much time to play, then he preempts you.
- - - [x] Trickster: All letter scores are zero, you can only hurt him with length bonuses.
- - - [x] Evil Twin: She doesn't draw from the bag. At each draw, she gets a copy of your hand.
- - [x] Log
- [ ] Outer world.
- - [ ] editor: Map commands
- - [ ] editor: sprite
- - [ ] NPCs.
- - [ ] Dialogue.
- [ ] Sound effects.
- [ ] Music.
- [ ] Battle mode (2p local). Battle is written for it, just need a way in.
- [ ] Determine the most effective possible play. (in theory it's 207, for a word like ZWXJKYY but I doubt that exists)
