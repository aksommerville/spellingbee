# Spelling Bee

Requires [Egg](https://github.com/aksommerville/egg) to build.

Let's make something like Dragon Warrior, except every encounter is a Scrabble contest.

Copied the 2023 NASPA word list and eliminated all words >7 letters; they won't be reachable.
Eliminated some profanity: The B, S, and F words. C, N, and K words weren't there already.
And "PIZZAZZ", the only word in the dictionary which was not actually reachable.
Gives us about 57k words in a 400 kB resource.

## TODO

- [x] Detailed logging for beta test.
- [ ] Disable TRACE logging before the public release.
- [ ] Return to menu from 2-player battle.
- [ ] Side quests with gold as the reward.
- - [ ] Advanced Endurance Gauntlet: Kill like 20 skeletons, but one miss and they kill you.
- - [ ] Is it difficult to turn all the cellar lights back off? ...not really but there's a little bit of zing to it. Worth a small prize I guess.
- [ ] Once everything is laid out, review all in-game text and confirm we're not using any 7-or-shorter-letter words that aren't in the dictionary. (proper nouns, onomotapaeiae, etc)
- [ ] Egg: Web: Sometimes there's a haywire note as songs change.
- - 2024-11-26 repairs in egg seem to have mitigated it, but i'm still hearing something off now and then.
- - I think there's something wrong in native too. Hardly ever happens and not very noticeable when it does, but there can be a kathunk when a song begins.
- - ...the native bit might be in my head.
- [ ] Modifying tool doesn't force a rebuild of custom resources.
- [ ] 2p: Choose an avatar and background.
- [ ] Editor seems to think that '#' starts a line comment in strings. Pretty sure it's not supposed to.
- [ ] Egg song editor: Delay on EGS note event (EGS file) gets mangled. Trying to set the second note of sound/2-ui_dismiss to 80 ms, it changes to 1153.
- [ ] Redraw rabbit sprite, or put him on a darker background. Poor contrast.

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

There's a ceiling of 147 points for a single play (MUZJIKS with 3x word against the Eyeball, but that actually isn't possible since there's an "I").
I haven't determined what the best legal play is, but definitely at least 137.

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
