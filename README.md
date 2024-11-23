# Spelling Bee

Requires [Egg](https://github.com/aksommerville/egg) to build.

Let's make something like Dragon Warrior, except every encounter is a Scrabble contest.

Copied the 2023 NASPA word list and eliminated all words >7 letters; they won't be reachable.
Also eliminated a few profanities, it's kind of shocking to hear the CPU players use them.
And "PIZZAZZ", the only word in the dictionary which was not actually reachable.
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
- [ ] Consider eliminating the letter multiplier items, not much point to them. ...replace with a warding-off-battles item?
- [ ] Sound effects.
- [ ] Music.
- [x] Determine the most effective possible play. (in theory it's 207, for a word like ZWXJKYY but I doubt that exists)
- [ ] Determine odds of being dealt each length of word.
- - This is a bit above my skill level at statistics! Interesting. Might need to devote some time to this later. But it's not critical.
- [ ] Detailed logging for beta test.
- [ ] Hello splash with long cutscene, like Season of Penance.
- - [ ] Consider revisiting this song and longing it up a little. I want to be able to run it ad nauseum in a kiosk.
- [ ] Victory splash, also a long cutscene.
- [ ] Currently AUX1 aborts any battle. Can't have that in real life. But we do need a way out of 2-player mode. Keep it until the last minute; it's helpful to be able to escape.
- [ ] Beehive in battle, I'd like the bees animated, orbiting the hive. Can we generalize that, and also use it for like stars around the head when struck?
- [ ] Proper graphics.
- [x] I think we need a tiny victory song at the end of each battle.
- [ ] Many more random battles.
- - [ ] Queen's Guard
- - [ ] Ghost
- - [ ] Robin
- - [ ] Ladybug
- [ ] Once everything is laid out, review all in-game text and confirm we're not using any 7-or-shorter-letter words that aren't in the dictionary. (proper nouns, onomotapaeiae, etc)
- [ ] Rephrase the 'battle' command so we state exactly how many bag slots.
- [ ] Egg: Background music needs a global trim, it's much too loud relative to sound effects and my channel levels are already low.

## Dictionary Notes

Best plays (including length bonus and assuming minimal wildcards), full dict:
```
2: 11 for 'QI' or 'ZA'
3: 19 for 'ZAX'
4: 22 for 'QUIZ'
5: 28 for 'JAZZY'
6: 38 for 'MUZJIK'
7: 79 for 'MUZJIKS'
```
Best Disemvoweller-legal play is "RHYTHMS" for 68 points.

Words without vowels, even Y, full dict:
```
       HM  7
       MM  6
       SH  5
      BRR  5
      CWM 10
      GRR  4
      HMM 10
      MMM  6
      NTH  6
      PHT  8
      PST  5
      PWN  8
      SHH  9
      TSK  7
      ZZZ 10
     BRRR  6
     CWMS 11
     HMMM 10
     PFFT 12
     PSST  6
     PWNS  9
     SHHH  9
     TSKS  8
    CRWTH 18
    GRRRL 11
    PHPHT 20
   CRWTHS 24
   GRRRLS 17
   TSKTSK 19
  TSKTSKS 60
```
Exactly two appear to be real words, both of Welsh origin: `cwm` (a riverless valley) and `crwth` (a musical instrument).
And then there's a bunch like "PFFT", my thoughts exactly.
