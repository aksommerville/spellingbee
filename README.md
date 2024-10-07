# Spelling Bee

Requires [Egg](https://github.com/aksommerville/egg) to build.

Shamelessly ripping off [Spell Spells](https://js13kgames.com/2024/games/spell-spells) by Curtis Robinson, from the 2024 js13k competition.
That's an absolutely brilliant game where you fight monsters by spelling words at them.
But alas a few tragic flaws:
- Some kind of audio memory leak, it craps out after playing through two or three times.
- Needs a built-in dictionary.
- Bonuses feel arbitrary.
- I don't like the idea of building your own tile set, it's confusing. Better to have the proper Scrabble distribution.
- I want an outer world too.
(and I don't mean to impugn Spell Spells with any of this criticism, he made that thing in under a month and it fits in less than 13 kB!)

Copied the 2023 NASPA word list and eliminated all words >7 letters; they won't be reachable.
Gives us about 57k words in a 400 kB resource.

## TODO

- [x] Acquire a Scrabble dictionary.
- [x] Validate spell-check algorithms.
- [ ] CPU player:
- - Build up his set of playable words incrementally over time.
- - Start with 2-letter words, look up all the ones starting with the first letter, then second letter.
- - Perform another batch of searches at fixed intervals while the human is playing, say 200 ms per interval.
- - If you play fast, the computer will not be able to play 7-letter words.
- - [x] What's the largest set of words of a given length starting with the same letter?
- - - 7 letters starting with S: 2928
- - - 6 and below are all under 2000, and mostly under 1000.
