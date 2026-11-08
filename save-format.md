# THESIMS.SAV — save format (The Sims Bustin' Out, N-Gage, game id `6RAK`)

Reverse-engineered from a genuine save dump. The format is simple and fully
understood for the purpose of renaming players.

## File

- Path on device: `C:\SYSTEM\APPS\6RAK\THESIMS.SAV` (private app folder)
- Size: **8192 bytes** (fixed)
- Byte order: little-endian
- The file is a sequence of "records". Every record has a 16-bit checksum as
  its final word, and the sum of all 16-bit words in a record is **0 mod 65536**.

## Top-level layout

| Offset | Size | Meaning |
|---|---:|---|
| `0x0000` | 16 | file header record |
| `0x0010` | 2040 | save slot 0 |
| `0x0808` | 2040 | save slot 1 |
| `0x1000` | 2040 | save slot 2 |
| `0x17F8` | 2040 | save slot 3 |
| `0x1FF0` | 16 | empty record (all zeros, sum 0) |

Slot stride is `0x7F8` (2040 bytes).

## File header record (16 bytes)

```
41 53 49 45 30 30 30 30 24 64 00 01 02 00 f0 a1
A  S  I  E  0  0  0  0
|------------------|  |------------------|
 magic "ASIE0000"      header payload + checksum
```

- Bytes `0x00..0x0B` are constant (`ASIE0000` + `24 64 00 01`).
  `0x0C..0x0D` = `02 00`; `0x0E..0x0F` = `F0 A1`.
- Last word (`0xA1F0`) is the header's checksum: the 16-bit word sum of the
  whole 16-byte record is 0.
- This record was never modified by the renaming tool.

## Save slot (2040 bytes)

| Slot offset | Size | Meaning |
|---|---:|---|
| `+0x000` | 11 | slot metadata (id / packed date-time) — not decoded |
| `+0x00B` | 3 | **money**, 24-bit little-endian |
| `+0x00E` | 16 | **player name**, ASCII, zero-padded |
| `+0x01E` | 2008 | game state (serialized Sim/world data) |
| `+0x7F6` | 2 | **slot checksum** (last little-endian `uint16`) |

The name is a plain ASCII C-string inside a fixed 16-byte field. The game
allows short names (the tool enforces 6 printable ASCII characters, which is
what the game UI accepts).

Example slot 0 name bytes at file offset `0x001E`:

```
50 6C 61 79 65 72 00 00 00 00 00 00 00 00 00 00   "Player" + zeros
```

## Money

- Location: `slot+0x0B`, an unsigned **24-bit little-endian** integer
  (range `0..0xFFFFFF` = 16,777,215). The game UI caps it at `999999`.
- Example (genuine save, slot 0):
  `30 9B 01` -> `0x019B30` = **105264**.

| Slot | money bytes | value |
|---|---|---|
| 0 | `30 9B 01` | 105264 |
| 1 | `86 16 00` | 5766 |
| 2 | `30 9B 01` | 105264 |
| 3 | `C6 9B 01` | 105414 |

The value 105264 occurs nowhere else in the file, so this field is the only
copy of the household funds. Changing it requires recalculating the slot
checksum (below).

## Checksum

For each slot, with `w[0..1019]` = the 1020 little-endian 16-bit words of the
2040-byte slot (`w[1019]` is the slot's last word):

```
sum(w[0..1019]) == 0   (mod 65536)
```

Therefore, after editing the name (or any bytes):

```
w[1019] = (0 - sum(w[0..1018])) & 0xFFFF
```

This is the same rule the 16-byte header and the trailing empty record follow.

### Why a raw name edit is rejected

The game validates the checksum per slot. Editing a slot's name changes that
slot's 16-bit word sum but leaves the stored last word untouched, making the
sum non-zero. The game then treats that slot as invalid
(shown as empty), while the untouched slots still load. This matches the
observed behaviour exactly.

## Metadata (not needed for renaming)

The 11 bytes at `+0x00..+0x0A` (before money) contain a small record id /
packed date-time and flags. Example bytes from the genuine save (slot starts at
`0x0010`, `0x0808`, `0x1000`, `0x17F8`):

```
slot0: 26 00 00 00 00 1a 00 0d 1c 06 00 30 9b 01
slot1: 18 00 00 00 00 14 00 0b 28 2d 00 86 16 00
slot2: 26 00 00 00 00 1a 00 0d 19 06 00 30 9b 01
slot3: 01 00 00 00 00 19 00 10 1a 33 00 c6 9b 01
```

These were left intact by the tool.

## Tool: `simsrename`

Source: `simsrename/main.cpp`, `simsrename/CMakeLists.txt`.

Build (Linux/macOS):

```sh
g++ -O2 -std=c++17 -o simsrename main.cpp
# or
cmake -B build && cmake --build build
```

Build (Windows): open the folder in VS / run `cmake -B build` then build, or
cross-compile a static exe with MinGW:

```sh
x86_64-w64-mingw32-g++ -O2 -std=c++17 -static -o simsrename.exe main.cpp
```

The Windows build was run under Wine and produced a byte-identical result.

Usage:

```sh
simsrename <THESIMS.SAV> <name> [slot|all]
simsrename <THESIMS.SAV> --money <slot|all> <amount>
```

- `name`: 1-6 printable ASCII characters (Latin).
- `--money`: set funds, `0`-`999999`.
- `slot`: `0`-`3` to patch one slot, `all` (default for name) for all four.
- A backup `<THESIMS.SAV>.bak` is created once (if not already present).
- All other bytes are preserved; checksums are recalculated.

Verification: running the tool on the genuine save with `Player` produces a
file byte-identical (md5 `c2847d81caa7881a7f216688ad0443d3`) to the
independently patched save that was confirmed working in-game.

Money verification: `simsrename THESIMS_PLAYER.SAV --money 0 999999` changes
only 5 bytes (slot 0 money `3F 42 0F`, plus its 2 checksum bytes) and keeps all
slot sums at 0. Output saved as `6RAK/THESIMS_MONEY.SAV` for in-game testing.

## History / notes

- `6RAK_2.APP` (230 KB) is the S60 front-end (N-Gage Arena login, drawing,
  shared memory `I3D_SHARED_MEMORY_SIMS`); it contains no save-format constants
  or the save filename.
- `6RAK.NAX` (4 MB) imports Symbian `EZLIB` and holds compressed/obfuscated
  game content. Its payload is not a plain zlib stream. Not needed for this
  task; left unresolved.
- `THESIMS.DAT` is a `DTRZ` archive with 497 named entries (`thesims.dzs`,
  S3M music, assets). Not needed for this task.
- The save format was derived entirely from the save file itself (structure,
  checksum invariant across all four slots, and in-game confirmation).
