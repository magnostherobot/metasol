# `centre` - A General Solitaire Solver that Respects Hidden Information

`centre` has been developed to showcase the abilities of `metasol`: an API for
wrapping thoughtful solvers (ones that look at face-down cards) to create
solvers that do not require any hidden information. The thoughtful solver used
by `centre` is _Solvitaire_, which can solve many different solitaire variants,
and as such `centre` is also highly flexible.

## Getting `centre` onto Your System

### Dependencies

`centre` contains all of its dependencies, however _Solvitaire_ requires the C++
Boost libraries installed on the system.

`centre`'s dependencies can be built by running `make init-submodules`.

### Building

Building should be as simple as running `make`.

## Usage

The most basic usage of `centre` is to run it like so:

```
centre rules/klondike
```

This will run a randomly-generated game of _Klondike_ solitaire. Options can be
passed after the rule file is specified - the list of possible options is
viewable using `centre --help`.

### Rule Files

The rule files accepted by `centre` are simple JSON files that accept the
following options:

|Option | Meaning|
|-------|--------|
|`tableau size` | The number of piles in the tableau.|
|`deck count` | The number of decks used.|
|`max rank` | The highest rank of card used in the game. The usual is 13 (up to kings).|
|`hole present` | Whether or not a hole is featured.|
|`hole build loops` | Whether or not aces can be placed on kings in the hole, and vice versa.|
|`foundations present` | Whether or not foundations are featured. The number of foundations, if enabled, is always equal to `4 * deck count`.|
|`foundations removable` | Once a card has been placed in the foundations, whether or not it can be moved again.|
|`foundations accept only complete piles` | Whether or not complete piles are the only way to move cards to the foundations.|
|`diagonal deal` | Whether or not a diagonal deal (i.e. the deal used in _Klondike_) is used when laying out the tableau.|
|`number of cells` | The number of cells used in the game.|
|`cells pre-filled` | The number of cells that start play containing a card.|
|`cards in stock` | The number of cards in the stock at the start of play.|
|`stock deal count` | The number of cards removed from the stock when dealing from it.|
|`stock redeal` | Whether or not the stock can be reused once it has been entirely dealt.|
|`cards in reserve`| The number of cards placed in the reserve.|
|`reserve stacked`| Whether or not the reserve is treated as a pile.|
|`cards in sequence`| The number of cards in the sequence.|
|`sequence fixed suit`| Whether or not a sequence uses a specific suit.|
|`accordion size`| The number of cards in the accordion.|
|`build policy`| The rule governing the movement of single cards across tableau piles.|
|`spaces policy`| The rule governing the movement of single cards into empty tableau piles.|
|`move built group`| The conditions for moving a group of cards.|
|`group build policy`| The rule governing the movement of a group of cards across tableau piles.|
|`foundations initialised`| Which foundations, if any, start play containing a card.|
|`stock deal type`| Where cards from the stock go.|
|`face up cards`| What cards are face-up when play begins.|
|`sequence direction`| The direction in which cards are placed on the sequence.|
|`sequence build policy`| The rule governing how cards are moved within the sequence.|
|`accordion moves`| An array of legal accordion moves.|
|`accordion policies`| An array of policies for the above moves.|

### Settings

Some settings can be passed to `centre` via the command line. Additionally,
settings can be specified in a JSON file.

|Name in File | Name on CLI | Meaning |
|-------------|----------------------|---------|
|`max concurrent threads` | | The maximum number of threads `centre` will have working at once.|
|`max concurrent games` | | The maximum number of games `centre` will try to run in parallel.|
|`solveseed`|`--solveseed`|The seed used by `centre` when making decisions.|
| |`--dealseed`| The seed used by `centre` when generating a game-state.|
|`run forever` | `--forever` | Runs `centre` indefinitely, on as many deals as possible.|
|`run timeout` | `--limittime` | The maximum time a _Solvitaire_ instance may run.|
|`run node limit` | `--limitnodes` | The maximum number of nodes a _Solvitaire_ instance may use.|
|`initial vote count`| | The number of voters `centre` usually uses.|
|`agree ratio` | | The percentage of votes that must agree for the move to be made.|
|`vote increase step` | | The number of voters to add in the case of a split vote.|
|`max vote count` | | The maximum number of voters `centre` will use.|
