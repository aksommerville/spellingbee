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

- [ ] Multiplier stamps.
- [ ] Animate rejected words.
- [ ] Animate length bonuses.
- [ ] Fancy up the encounter graphics.
- [x] The dictionary has some profanity. Either remove them, or ensure that the CPU won't play them.
- - Removed a B, S, and F word. C, N, and K words weren't there already. And others? I think hell and damn are ok.
- - And there's "asshole", but I like that because it's a hilarious thing to say to the foe as you dispatch him.
- [x] There are way more 7-letter candidates than 2-letter. Can we curve the foe's search clock somehow to make him spend more time at the low end?
- - I want fast players to be able to hold most foes to 2 or 3 letters per word, but as is that is basically impossible.
- [ ] Outer world.
- [ ] Sound effects.
- [ ] Music.
