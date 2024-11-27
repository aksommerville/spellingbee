# Spelling Bee

Requires [Egg](https://github.com/aksommerville/egg) to build.

Let's make something like Dragon Warrior, except every encounter is a Scrabble contest.

Copied the 2023 NASPA word list and eliminated all words >7 letters; they won't be reachable.
Also eliminated a few profanities, it's kind of shocking to hear the CPU players use them.
And "PIZZAZZ", the only word in the dictionary which was not actually reachable.
Gives us about 57k words in a 400 kB resource.

## TODO

- [ ] Music.
- - [ ] 2-player mode.
- - [ ] Restore music on returning from 2-player mode (hello modal stays open throughout)
- [ ] Sound effects.
- [ ] Detailed logging for beta test.
- [ ] Victory splash, also a long cutscene. And report stats!
- [ ] Currently AUX1 aborts any battle. Can't have that in real life. But we do need a way out of 2-player mode. Keep it until the last minute; it's helpful to be able to escape.
- [ ] Beehive in battle, I'd like the bees animated, orbiting the hive. Can we generalize that, and also use it for like stars around the head when struck?
- [ ] Feedback when bumping a wall, both visual and auditory.
- [ ] Side quests with gold as the reward.
- - [ ] Kill all the Lurking Terrors in the cellar.
- - [ ] Advanced Endurance Gauntlet: Kill like 20 skeletons, but one miss and they kill you.
- - [x] Graverobbing! Put an archaeology professor in the lab's first room. "There's treasure in the grave of JOHN SMITH (1912-1975). If only I knew how to find that grave..."
- - - Assign name and dates to each grave. Sort them alphabetically on the horizontal and chronologically on the vertical.
- - - Professor selects a grave at random when you talk to him.
- - - Add some "oh duh" hints in the cemetery. NPCs near the limits saying like "Everyone over here has a Z name, weird."
- - - Currently 133 graves. 55 accessible without tunneling.
- [ ] More helpful NPCs, esp near the start of each zone.
- - [ ] Goody, right at the beginning. Reports stats, and maybe contextual advice for the outstanding books?
- - [ ] Garden: Explain paths and bug spray.
- [ ] Proper graphics.
- - [ ] Battle avatars
- - [ ] Meals
- - [ ] Map decoration
- [ ] Refine maps.
- - [ ] Arrange cellar such that you have to walk through the dark at least a little.
- - [ ] Add a path in the garden, and lots of brambles.
- - [ ] Trick switches in the lab that you *don't* want to toggle.
- [ ] Once everything is laid out, review all in-game text and confirm we're not using any 7-or-shorter-letter words that aren't in the dictionary. (proper nouns, onomotapaeiae, etc)
- [ ] Egg: Web: Sometimes there's a haywire note as songs change.
- - 2024-11-26 repairs in egg seem to have mitigated it, but i'm still hearing something off now and then

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

Most monsters do not use the same dictionary as the player, they use a simpler "pidgin" dictionary.
Only Scholar and Disemvoweller use the full dictionary.

Pidgin dictionary has 12657 words, compared to 57067 for the full. Best plays per length:
```
2: 11 for 'QI'
3: 16 for 'ZEK'
4: 22 for 'QUIZ'
5: 28 for 'JAZZY'
6: 37 for 'QUEAZY'
```

## Additional (non-dictionary) notes for the player

The Endurance Gauntlet is 317 steps long.
In each block of 128 steps there will be exactly 10 skeletons.
So on average, you'll encounter 25 skeletons. No fewer than 20, and no more than 30 as long as you don't backtrack.

All 6 books are fictitious (as in, they aren't real books, not that they're books of fiction).
"To Serve Man" I stole from the Twilight Zone, the other 5 are original.

Monsters with a charge meter (most of them), they will always fold if the meter hasn't started charging.
If there's no charge meter, the monster always plays its best possible word.

Rabbit plays preemptively: If its charge meter fills up, the player automatically folds.

Scoring:
 - Distribution and point values of letters are exactly as in Scrabble.
 - 5-letter words get a 5-point bonus, 6-letter 10 points, and 7-letter 50 points.
 - Double and triple word modifiers are applied before the length bonus.
 - Eyeball: Any word containing I gets zero points. Any word containing U (but not I) gets a 10-point bonus after everything else.
 - Eyeball's U bonus only applies once if the word has multiple U.
 - Coyote discards points and only the length bonus counts. This applies in both directions.
 - Sixclops disqualifies any word whose length is not 6. If the length is 6, it scores as usual.
 - Queen: If a word would kill her but does not contain Q, its value is reduced to leave her with exactly 1 HP.
 - If a word is not in the dictionary, it damages you instead of the monster, bonuses and all.
 - No penalty for folding, except that the monster gets a free play.
