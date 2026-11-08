# simsrename

A small command-line editor for **The Sims Bustin' Out** (Nokia N-Gage, game id
`6RAK`) save files (`THESIMS.SAV`).

It can:

- change the player name (1-6 printable ASCII characters) in one or all four
  save slots;
- set household money (0-999999);
- automatically recalculate the per-slot checksum so the game accepts the save.

## Build

Requires a C++17 compiler.

Linux:

```sh
g++ -O2 -std=c++17 -o simsrename main.cpp
```

or with CMake:

```sh
cmake -B build && cmake --build build
```

Windows (MinGW):

```sh
x86_64-w64-mingw32-g++ -O2 -std=c++17 -static -o simsrename.exe main.cpp
```

MSVC: open the folder or use the CMake project.

## Usage

```sh
simsrename <THESIMS.SAV> <name> [slot|all]
simsrename <THESIMS.SAV> --money <slot|all> <amount>
```

- `slot`: `0`-`3`, or `all` (default when renaming).
- A `<THESIMS.SAV>.bak` backup is created once, before the first modification.
- The save file is edited in place.

Examples:

```sh
simsrename THESIMS.SAV Player            # rename in all four slots
simsrename THESIMS.SAV Test 2            # rename in slot 2 only
simsrename THESIMS.SAV --money 0 999999  # set slot 0 money
```

## Save format

See [save-format.md](save-format.md) for the full layout and checksum details.

## Notes

- Always keep a backup of your save.
- Tested with a genuine save dump; the name change was confirmed in-game.

## License

MIT — see [LICENSE](LICENSE).
