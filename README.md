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
- - - Or you get a free item for playing first? ...we'd also need something to prevent the second player from lolligagging after.
- - - [ ] I need to try 2-player against a real human, or even better, observe two stangers playing. Sign up for the next COGG event.
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
- - - - I'm cutting pidgin off at 6 letters. Foes that can play 7-letter words should also use the full dictionary. And it cuts the effort almost in half.
- [ ] Consider eliminating the letter multiplier items, not much point to them.
- [ ] Sound effects.
- [ ] Music.
- [ ] Determine the most effective possible play. (in theory it's 207, for a word like ZWXJKYY but I doubt that exists)
- - Also odds of being dealt each length of word.
- [ ] Detailed logging for beta test.
- [ ] Hello splash with long cutscene, like Season of Penance.
- [ ] Remove LFs from the dictionary during compile, and insert a 12-byte header, count of words of each length. That saves 57 kB in the final build.
- [x] Consider artificial controls on random battles. eg a minimum safe step count after each battle
- - Bagging might be an easy way to smooth that out. Try it.
- [ ] Currently AUX1 aborts any battle. Can't have that in real life. But we do need a way out of 2-player mode.
- [ ] kitchen: Show a pretty thumbnail of each dish.
- [ ] Proper maps.
- - [ ] Cemetery
- - [ ] Laboratory
- - [ ] Gym
- - - [x] Tune the gauntlet better. I think we really do need artificial battle massaging.
- - - - First try (killed most immediately): 47 HP, 64 XP
- - - - After bagging: 39 HP, 58 XP. I had a weak start. There are too many monsters, but the principle seems good.
- - - - 70 HP, 62 XP. There's 500..550 steps in the Gauntlet.
- - - - Bumped frequency down, so now there' 5/64 instead of 7/64.
- - - - 82 HP, 43 XP
- - - - Allowing each skeleton to hit me once, I made it about 300 steps, approaching the SE corner. XP 25.
- - - [x] But no, the gauntlet is still tiresome. Some requirements:
- - - - Must take no more than 20 minutes to get thru.
- - - - Must reliably kill the hero if monsters get one attack every battle. "very likey", doesn't have to be "certain".
- - - - Most battles should take 2 turns, and better to err toward 1 than 3. HP about 10?
- - - - Truncated map and reduced HP to 13: 9 minutes, 89 HP, 25 XP, playing well.
- - - - Same, but make them charge instantly: I can't even get east of the wall.
- - - - Normal foe, but delaying my first strike until charge begins: Reached Rabbit with exactly one HP. Took about 10 min.
- - - - Again playing hard: 8 min, 72 HP, 26 XP. I'm sure I can reach the rabbit with 100 HP... have to prove it tho.
- - [ ] Garden
- - [ ] Cellar
- - [ ] Queen's Chambers
- [ ] Beehive in battle, I'd like the bees animated, orbiting the hive. Can we generalize that, and also use it for like stars around the head when struck?
- [ ] Proper graphics.
- [ ] We're failing at `make edit` due to resource compilation when a map is invalid. Need to work around that; we want to use make edit to make it valid again!
- - Same if the code fails to compile. That shouldn't prevent editor from running.
- [ ] I think we need a tiny victory song at the end of each battle.
- [ ] Many more random battles.
- - [ ] Queen's Guard
- - [ ] Ghost
- - [ ] Robin
- - [ ] Ladybug
- [x] Need a helper in the editor for setting kitchen sprite params.
