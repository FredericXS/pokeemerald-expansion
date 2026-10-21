# Johto progression test ROM

Based on commit `3936a86c6d038924f4f79768ac495ab075534e0f`.
This milestone adds a small playable Johto expedition, using existing FRLG
art and geometry. New Bark and Route 29 are prototype layouts, not a
recreation of their Gold/Silver maps or a complete Johto campaign.

## Build and play

```sh
source /workspace/.cloud-setup/activate.sh
cd /workspace/pokeemerald-expansion
make -j5 -O all BUILD_NAME=generations-progression GENERATIONS_TEST_ROM=1
```

Open `pokegenerations-progression.gba` in mGBA. Use a new game or a backup
of a save made by this project's schema `0x4701`; never use an original
Emerald/FRLG save. The file name keeps test saves separate from the regular
`pokegenerations.gba` build. No base ROM is required.

1. The scientist in the Pallet bedroom offers the Kanto progression
   shortcut: eight badges, League and Champion. Confirm explicitly; it
   changes saved progress. It leaves Oak's introductory quest intact:
   obtain a Kanto starter normally before departing.
2. Talk to the sailor near Pallet's central sign. Confirm the expedition,
   then choose Chikorita, Cyndaquil or Totodile through the reservation
   questions. Declining all three cancels without moving anything.
3. On first entry, the entire Kanto party is deposited and the reserved
   level-5 Johto starter becomes the only party member. Arrival is inside
   Elm's lab. Lab ball displays explain the reservations; they do not
   award additional starters. Elm supplies five Poké Balls, repeatably.
4. Leave the lab and walk north to the Route 29 test area. Its grass has
   level 2–4 Sentret, Hoothoot, Pidgey and Rattata. The western house in
   New Bark provides healing, a PC and the local blackout/Teleport return
   point. Other Johto routes and gyms are outside this milestone.
5. The scientist in New Bark offers the four-Johto-badge shortcut. This
   opens the PC and the sailor's return passage. Kanto and Johto badge
   histories are separate and restored on travel. Fly is disabled in Johto
   until a regional Fly map exists; Teleport and blackouts stay local.
   Return visits keep the
   current party and never award/deposit another starter; the reservation
   answer on a revisit is ignored.

Without `GENERATIONS_TEST_ROM=1`, the scientists cannot alter progression.
Use a distinct `BUILD_NAME` for each flag configuration; Make does not
track changes to command-line compiler flags as dependencies.

## Transfer contract

First entry requires an unlocked Johto and a nonempty, valid contiguous
party. It preflights and reserves one free PC slot per party member across
all 14 boxes. A full PC, or even one fewer slot than required, aborts
without changing the party, storage, regional progress, badge flags or
respawn. Free enough slots in Kanto and retry. Existing boxed Pokémon are
never moved or overwritten.

Party copies and a new starter are staged in heap memory. All destination
writes are read back before removing the original party. A failed readback
restores every written slot, including its original empty-slot bytes.
Exact boxed data is retained: personality, OT, experience, IVs, EVs,
nature, moves, PP, held items, eggs, HP loss and status. The transfer does
not use the engine's healing `CopyMonToPC` helper. Party-only derived stats
are reconstructed normally when withdrawing. Attached mail, held mail,
active fused partners and damaged Pokémon block entry; remove mail or
unfuse first. Eggs are supported.

The transaction is synchronous: it starts no tasks, yields to no scripts,
and writes no flash. Party, progression and the local respawn are committed
before control returns to the fixed-destination warp script. Saving later
uses the engine's existing alternating flash slots. There is no schema or
storage-layout change. Automated reload coverage exercises save blocks;
it is not a power-interruption test of the flash hardware.

Until four Johto badges, this prototype locks the entire PC and return
passage. This is deliberately conservative: it does not implement
per-Pokémon regional provenance or a separate Johto-only PC. No global
battle-level cap or complete regional acquisition audit is added here.

## Validation

```sh
make -j5 -O check BUILD_NAME=generations-progression GENERATIONS_TEST_ROM=1 TESTS="Generations"
make -j5 -O check BUILD_NAME=generations-progression GENERATIONS_TEST_ROM=1 TESTS="test/save.c"
make -j5 -O check BUILD_NAME=generations-progression GENERATIONS_TEST_ROM=1 TESTS="Spikes"
```

The focused runs pass 23 regional tests, 4 save-layout tests and 4 Spikes
tests. Regional cases include all three starters through the real script
interpreter; PC full and insufficient capacity; a six-member transfer
across boxes; exact boxed data and unrelated slot preservation; eggs,
items, PP, HP and status; rollback after writes 1, 3 and 6; rejected mail,
fusions, bad checksums, empty teams, invalid starters, locked progress and
unknown schemas; duplicate calls, return and revisit; regional badge
restoration; saved-party/save-block reload; and linked destination maps.
The runner reports parametrized scenarios under their parent test count.

The test ROM occupies 26,970,532 bytes of ROM (80.38%) and is padded to
32 MiB. EWRAM remains 226,704 bytes and IWRAM 28,648 bytes. Transfer staging
uses the existing heap, without enlarging any saved structure.

The broad suite run completed with 5,439 passed, 18 known failing, 9 expected
failing and 421 TODO (5,887 total), exit status 0 and no unexpected failures.
It preceded the final Fly/Box Link/encounter-table/vacancy hardening changes;
the focused 31-test runs above were repeated against the final sources.
This distinction avoids treating the broad run as final coverage of those
last changes.

A button-driven mGBA 0.10.5 smoke test started a new game, used the bedroom
shortcut, obtained Bulbasaur through Oak's scene and completed the rival
battle. A synthetic full-PC fixture was then applied in emulator RAM:
the real sailor dialogue refused entry, with all 600 party bytes and
34,144 storage bytes unchanged and the player still in Pallet. Restoring
that fixture's free slots allowed travel into Elm's lab; the original
80-byte boxed Pokémon matched exactly. Walking left the lab, entered New
Bark and crossed into Route 29.

After correcting the encounter-table build guard, a scripted fixture using
the real give-mon, shortcut and transfer specials prepared a fresh Johto
save. The in-game Save command wrote a 131,072-byte flash image. A new
emulator process loaded that save with the final ROM and Continue returned
to Route 29: all 600 party bytes and the transferred 80-byte boxed mon
matched, schema was `0x4701`, and region was Johto. Walking in the grass
started a level-3 Sentret encounter. These checks are distinct from the
unit save-block reload and do not simulate power loss during flash writes.

Final ROM SHA-256:
`f92da43aa6192b503499dff44d5803cf1078011267c9baed643f286876822789`.
