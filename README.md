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
- - - [ ] (E) Auto spell check when staging.
- - - - Alternative: Prevent committing invalid words.
- - - [ ] (E) Show potential score when staging.
- - - [ ] (E) Show foe's upcoming score as he searches.
- [ ] Sound effects.
- [ ] Music.
- - [ ] 2-player mode.
- [ ] Detailed logging for beta test.
- [ ] Hello splash with long cutscene, like Season of Penance.
- - [ ] Consider revisiting this song and longing it up a little. I want to be able to run it ad nauseum in a kiosk.
- [ ] Victory splash, also a long cutscene.
- [ ] Currently AUX1 aborts any battle. Can't have that in real life. But we do need a way out of 2-player mode. Keep it until the last minute; it's helpful to be able to escape.
- [ ] Beehive in battle, I'd like the bees animated, orbiting the hive. Can we generalize that, and also use it for like stars around the head when struck?
- [x] Animate new tiles entering player's hand.
- [x] During the attack stages, show the word centered and still. It's super hard to read in motion. Or maybe pause at the center of the arc?
- [ ] Feedback when bumping a wall, both visual and auditory.
- [ ] Can we cheat the hero backward when returning from battle as a hint, "you were walking this way"?
- [ ] More helpful NPCs, esp near the start of each zone.
- [ ] Proper graphics.
- [ ] Many more random battles.
- - [ ] Queen's Guard
- - [ ] Ghost
- - [ ] Robin
- - [ ] Ladybug
- [ ] Refine maps.
- - [x] Lab: Put all the locks at the beginning, make it less linear.
- - [x] Remove exit locks? They're kind of pointless now that we warp home on recovering the book.
- - [ ] Arrange cellar such that you have to walk through the dark at least a little.
- [ ] Once everything is laid out, review all in-game text and confirm we're not using any 7-or-shorter-letter words that aren't in the dictionary. (proper nouns, onomotapaeiae, etc)
- [x] Rephrase the 'battle' command so we state exactly how many bag slots. And bump it up to 128.
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

The only letters that don't appear in a 2-letter word are C and V.
There are 2 each of C and V, so if you have a blank and a full hand you can definitely play a 2-letter word.
(and therefore if you have a blank and a full hand, you definitely have a word).

There are full hands that can not produce a word. I haven't figured out yet how to enumerate them.

Pidgin dictionary has 12657 words, compared to 57067 for the full. Best plays per length:
```
2: 11 for 'QI'
3: 16 for 'ZEK'
4: 22 for 'QUIZ'
5: 28 for 'JAZZY'
6: 37 for 'QUEAZY'
```
