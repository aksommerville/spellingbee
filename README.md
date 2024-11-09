# Spelling Bee

Requires [Egg](https://github.com/aksommerville/egg) to build.

Let's make something like Dragon Warrior, except every encounter is a Scrabble contest.

Copied the 2023 NASPA word list and eliminated all words >7 letters; they won't be reachable.
Also eliminated a few profanities, it's kind of shocking to hear the CPU players use them.
Gives us about 57k words in a 400 kB resource.

## TODO

- [ ] Encounters
- - [ ] Restructure as a modal, there's no reason for encounter to be a special thing.
- - [ ] Animate rejected words.
- - [ ] Animate length bonuses.
- - [ ] Different graphics for folding.
- - [ ] Thinking and attacking faces.
- - [ ] Tunable difficulty. Not a per-foe thing, but global preferences from the player.
- - - [ ] (E) Maximum foe word length. <-- We need this for sure, and it should be a per-foe thing. Keep the early ones easy.
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
- - [ ] Special encounters, monsters that have their own particular rules.
- - - [ ] Jackrabbit: You get only so much time to play, then he preempts you.
- - - [ ] Trickster: All letter scores are zero, you can only hurt him with length bonuses.
- - - [ ] Evil Twin: She doesn't draw from the bag. At each draw, she gets a copy of your hand.
- - - [ ] The Queen: Fight like normal, but only the letter Q can finish her.
- [ ] Outer world.
- - [ ] editor: Map commands
- - [ ] editor: sprite
- - [ ] NPCs.
- - [ ] Dialogue.
- [ ] Sound effects.
- [ ] Music.
- [ ] Battle mode (2p local)
- [ ] Kill yourself with a long word, and HP doesn't count down to zero.
