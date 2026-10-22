# Johto progression test ROM

Based on commit `3936a86c6d038924f4f79768ac495ab075534e0f`.
This milestone adds a playable Johto expedition. New Bark (32×32) and
Route 29 (96×32) adapt HeartGold/SoulSilver terrain, door coordinates and
west/east connections to the GBA engine. The lab, player’s home, southwest
house, rival’s home, upstairs study and Route 46 gate have separate interiors.
Community HGSS building graphics and original wind turbines distinguish the
town from Pallet. This remains the first area, not a complete Johto campaign.

Sources, artwork credits and regeneration instructions are in
[Johto map credits](../../graphics/johto/CREDITS.md).

![HGSS-based New Bark map](../../graphics/johto/previews/NewBarkTown.png)

[View the complete Route 29 map](../../graphics/johto/previews/JohtoRoute29.png).

## Build and play

```sh
source /workspace/.cloud-setup/activate.sh
cd /workspace/pokeemerald-expansion
make -j5 -O all BUILD_NAME=generations-progression GENERATIONS_TEST_ROM=1
```

The [Johto test ROM workflow](https://github.com/FredericXS/pokeemerald-expansion/actions/workflows/test-rom.yml)
builds this ROM on pushes to `master`/`upcoming` and pull requests. It can
also be started manually with **Run workflow**. Open a successful run and
download **pokemon-generations-johto-test** under **Artifacts**. The ZIP
contains the playable `.gba` and its SHA-256 checksum, retained for 30 days.

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
4. Leave the lab and walk west to Route 29. Its grass has
   level 2–4 Sentret, Hoothoot, Pidgey and Rattata. The player’s home in
   northeast New Bark provides healing from Mom, a PC and the local
   blackout/Teleport return point. The western Cherrygrove exit and northern
   Route 46 passage are visibly closed; other routes and gyms are outside
   this milestone.
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

The focused runs pass 26 regional tests, 4 save-layout tests and 4 Spikes
tests. Regional cases include all three starters through the real script
interpreter; PC full and insufficient capacity; a six-member transfer
across boxes; exact boxed data and unrelated slot preservation; eggs,
items, PP, HP and status; rollback after writes 1, 3 and 6; rejected mail,
fusions, bad checksums, empty teams, invalid starters, locked progress and
unknown schemas; duplicate calls, return and revisit; regional badge
restoration; saved-party/save-block reload; linked HGSS destination maps, one-time relocation of legacy Johto saves,
and the new home’s respawn coordinates.
The runner reports parametrized scenarios under their parent test count.

The test ROM occupies 26,991,548 bytes of ROM (80.44%) and is padded to
32 MiB. EWRAM remains 226,704 bytes and IWRAM 28,648 bytes. Transfer staging
uses the existing heap, without enlarging any saved structure.

The broad suite run completed with 5,439 passed, 18 known failing, 9 expected
failing and 421 TODO (5,887 total), exit status 0 and no unexpected failures.
It preceded the final Fly/Box Link/encounter-table/vacancy hardening changes;
the original focused 31-test runs were repeated against that milestone.
For the HGSS map revision, all 34 focused tests pass.
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

The smoke tests above describe the previous map revision. With the HGSS
revision, mGBA also loaded that real flash save from the old vertical Route 29
and relocated the player to (93,14) on the new horizontal route. All 600
party bytes and the transferred 80-byte boxed Pokémon still matched. Walking
then crossed the west/east map connection into New Bark and entered Elm’s
new lab through its HGSS-positioned door, then reached and entered the
player’s northeast home through its separate entrance.

Old Johto saves are relocated once before loading map objects; map IDs,
party/storage layout and schema remain unchanged. Keep a backup when
updating. New saves retain their exact position on subsequent loads.

Map validation and optional regeneration:

```sh
python3 tools/generations_maps/validate.py
# Pillow is needed only for regeneration/checking graphics:
python3 tools/generations_maps/generate.py --check
python3 tools/generations_maps/generate.py --preview /tmp/johto-maps
```

The validator checks reciprocal connections, all five town doors and their
return warps, NPC reachability, both directions of the route’s ledges, the
gate approach, closed unfinished exits and GBA tile budgets. The build
Action runs this validator before compiling. Every artifact includes its
own SHA-256 checksum.

HGSS revision ROM SHA-256:
`7af076978a61f54ce11a108f17af71ad80fa52cee87b6226e881b7a9136cbe19`.
