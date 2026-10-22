# Kanto Foundation Prototype

The next milestone is now available: [Johto progression test ROM](JOHTO_TEST_ROM.md).
It adds transport, atomic first-entry party deposit, starter reservations,
HGSS-based New Bark and Route 29 maps, distinct interiors, healing and
test-only progress shortcuts. Existing Johto saves migrate to safe map positions.
The foundation validation record below describes the original Kanto baseline.

## Scope

The prototype reuses this fork's FRLG maps, Oak introduction, starter scene,
rival battle and native Kanto overworld inside pokeemerald-expansion. It
is not a claim that a complete, audited Kanto campaign or nine-region game
has been delivered. New games begin in the player's bedroom in Pallet
Town. Walk downstairs, leave the house and approach the northern exit;
Oak takes the player to the lab. Choose one of the three Poké Balls and
confirm to receive a level-5 Bulbasaur, Squirtle or Charmander.

## Implemented

- `make` builds `pokegenerations.gba` using the existing FIRERED mode. The
  original engine, Gen I–IX species and modern mechanics are retained.
- Kanto starter IDs now agree with the existing Oak scripts (Bulbasaur,
  Squirtle, Charmander), including downstream `GetStarterPokemon` lookups.
- New-game initialization reserves 12 existing event variables (24 bytes):
  schema at 0x40DB, mechanic bits at 0x40DC, active campaign region at
  0x40E5, and nine region words at 0x40F7–0x40FF in campaign order. Each
  word contains eight badge bits, League, Champion, unlocked, entered,
  access-to-prior-PC and starter-received bits. No fields were appended.
- Only Kanto starts unlocked/entered. Badge flag writes record distinct
  regional badges. The Kanto Hall of Fame records League/Champion and
  unlocks Johto's progression bit after eight badges. It does **not**
  create transport or permit travel to a nonexistent destination.
- Separate policy APIs for dex extent, normal wild species, explicit
  special-event gifts, party eligibility and previous-region PC access.
  Normal Kanto wild eligibility excludes later species and regional forms.
  A gift exception does not change the wild policy; later species are not
  intrinsically banned from the party. These policy APIs are foundations,
  not a replacement for auditing every encounter/gift/evolution script.
- Kanto's native regional dex order excludes cross-generation additions;
  national seen/caught storage and all species remain available. The
  engine's existing regional-count constants include a sentinel; MEW is
  regional index 151.
- Mechanic story-unlock API checks Kalos/Alola/Galar/Paldea respectively.
  Once set, bits survive returning to earlier regions. The central battle
  eligibility check additionally requires that unlock for the player's
  side in a campaign save. Normal item, charge and battle restrictions
  still apply. NPC eligibility remains unchanged. Ultra Burst shares the
  Alola unlock region. Dynamax includes Gigantamax via the existing engine.
- Per-region, per-badge cap table (15/20/25/30/35/40/45/50/60; Champion:
  100). Prior-PC threshold is four badges outside Kanto, configurable in
  the same table. These values are provisional.
- An isolated `Generations_ProjectBattleMon` demonstration calculates
  capped stats on a disposable Pokémon copy using the engine's existing
  formulas. Real data is left intact. It is **not connected to live battle
  controllers**, so ordinary prototype battles currently use real levels.
- CI on master/upcoming pushes and PRs builds the prototype, uploads
  `pokegenerations.gba` as `pokemon-generations-kanto`, and retains the
  existing variant/release/test checks. CI runs only after pushing to
  GitHub; local configuration does not prove a hosted run succeeded.

## Battle-cap integration boundary

A level-75 Charizard can be projected to level-35 battle data with its
real level, experience, IVs, EVs, nature, moves and original boxed data
unchanged. Max HP and offensive/defensive stats come from the normal stat
calculator. Current HP preserves its fraction with floor rounding, at
least 1 HP for a living mon, and 0 HP for a fainted mon. Removing the cap
returns the original projection. No temporary level is written to the
party or saved. The calculator's level-up HP scratch value is restored.

This is not sufficient for a universal live cap. Before enabling it,
introduce a single battle-local projection shared by initialization,
switch-in, controller buffers, HP/PP/status synchronization and AI. Test
healing and held-item percentage effects, mid-battle experience/level-ups,
evolution, form recalculation, Mega stats, Dynamax HP multipliers, Tera
forms, Shedinja, transformed/copied stats, faint/revive and save/continue.
Experience and evolution should continue using the real level. Decide
whether EXP uses the effective or real opponent level. Define inverse HP
mapping once to prevent cumulative healing/rounding exploits when
switching or crossing cap boundaries. Link/facility battles need explicit
rules. Do not apply the projection to opponents by accident.

Higher evolution stages, powerful moves and accumulated EVs remain strong
even at the same effective level; caps alone do not establish balanced
regional difficulty. Newly caught and imported party members should use
the same eventual player-side projection rule.

## Save and storage budget

The engine uses 32 × 4096-byte sectors (128 KiB flash), two 14-sector save
slots, and four sectors for Hall of Fame/Trainer Hill/recorded battle.
Each save sector has 3968 payload bytes and a 116-byte SaveBlock3 chunk.

| Structure | Existing size | Payload capacity | Unused bytes |
| --- | ---: | ---: | ---: |
| SaveBlock1, Emerald | 15,568 | 15,872 | 304 |
| SaveBlock1, existing FRLG mode | 15,756 | 15,872 | 116 |
| SaveBlock2 | 3,884 | 3,968 | 84 |
| SaveBlock3 (current config) | 4 | 1,624 | 1,620 |
| PokémonStorage | 34,144 | 35,712 | 1,568 |

The ASLR RAM wrappers add 128 bytes per block; those are not persisted
payload. BoxPokemon remains 80 bytes and party Pokemon 100 bytes. The
existing storage includes four fusion slots as well as boxes and names.
This milestone does not increase any structure size. The existing save
layout test now explicitly recognizes both preexisting mode layouts.

Reusing formerly unused variables preserves structural layout, not semantic
compatibility with arbitrary saves. Start a **new game** for this project;
there is no migration from vanilla Emerald, vanilla FRLG or older hacks.
Unknown schema queries fail closed and do not rewrite the save. Never use
new-game initialization as a load-time migration. Copies of user saves
must be backed up before future migrations.

At least 34 boxes (1,020 slots) would require 81,600 bytes for boxed mons
alone, exceeding the current 35,712-byte allocation by 45,888 bytes before
names, wallpapers and fusion slots. It also consumes more EWRAM than the
current build's free budget. A safe implementation needs a documented
flash layout, migration and rollback, PC indexing/UI review, possible
storage compression or external persistence design, and save interruption
and corruption tests. Increasing TOTAL_BOXES_COUNT alone is unsafe.

## Not yet implemented

- Johto beyond the New Bark/Route 29 test area, later regions and full campaigns.
- Per-Pokémon regional provenance and selective access to earlier-region PC mons.
  The expedition currently locks the whole PC and return travel until four
  Johto badges; see the milestone document for transfer and mail/fusion policy.
- Selective PC UI enforcement for origin restrictions and audits of daycare,
  trades and alternate acquisition paths beyond the closed expedition area.
- Cumulative regional dex UI for Johto–Paldea. Threshold policy is present;
  currently the native Kanto/national interfaces remain in use. Unlocking
  must never reset seen/caught records.
- Global enforcement of every acquisition policy (gifts, roamers,
  outbreaks, breeding, evolution, trades, special static encounters).
  The existing early Kanto encounter tables are reused; later content
  needs an explicit audit before it is considered a campaign.
- Story events/items for later battle mechanics; none start unlocked.
- Live effective-level caps and expanded storage, as described above.

## Build, tests and ROM access

On Debian/Ubuntu, install the dependencies listed in INSTALL.md, then:

```sh
make -j5 -O all
make -j5 -O check TESTS="Generations:"
make -j5 -O check TESTS="test/save.c"
make -j5 -O check TESTS="Spikes"
```

In the configured cloud machine, first run
`source /workspace/.cloud-setup/activate.sh`. The local toolchain is outside
the checkout. Do not reuse existing object directories after changing a
build's GAME_VERSION without cleaning them. The project default uses its
own `build/generations` and `build/generations-test` directories.

Open the root `pokegenerations.gba` in mGBA. On GitHub, open a completed CI
run, download **pokemon-generations-kanto** under Artifacts and extract the
ROM. No compiled ROM or base ROM is committed to Git. No original ROM is
required by this repository's compilation workflow. Follow the licenses
and asset distribution terms of the underlying project.

## Validation record

The unmodified baseline and prototype builds both completed. The prototype
linker reports **26,964,660 bytes of occupied ROM (80.36%)**, EWRAM 226,704
bytes and IWRAM 28,648 bytes. The output file is padded to **33,554,432
bytes (32 MiB)**. This fits the conventional GBA ROM window. A final rebuild was byte-identical
to the ROM used for the functional smoke/save/continue test (SHA-256
`83f0fcf4eafd5e9fd6bfe855711ff4f9270f1c3dfb97dac0ac76b79e1c9323cc`).

Focused checks passed: **13/13 regional foundation tests**, **4/4 existing
save layout tests** and **4/4 Spikes tests**. The documentation index validator
and `git diff --check` also pass. The full `make -j5 -O check` run completed with **5,429 passed**, **18
known failing**, **9 expected failing**, and **421 TODO**, for **5,877 total**.
There were no unexpected failures. Known failing and TODO cases are
preexisting runner classifications; they are not ordinary passes. The
full run included the initial 12 regional tests; the added Pokédex
preservation case is verified separately with the final focused run.

A functional smoke run using official Debian mGBA 0.10.5 loaded the final
ROM from reset, advanced Oak's introduction, entered the Pallet bedroom,
walked downstairs and outside, triggered Oak at the northern exit,
followed him into the lab, and selected/received Bulbasaur. The party count
was one and the Kanto starter/progression bits were set. The real in-game
Save command wrote a 131,072-byte flash image. A **new emulator process**
loaded that save and Continue returned to the lab. Schema 0x4701, current
region Kanto and progress word 0x3C00 survived; all **100 bytes** of the
starter Pokemon matched the pre-save party, including its real level 5.
This is an actual save/continue check, separate from the in-memory unit
serialization test. It does not validate future transfer or migration.

The other two starters were verified through the real script interpreter
in automated tests; the entire visual cutscene was manually driven only
for Bulbasaur. Eight early encounter tables (FireRed/LeafGreen Route 1,
Route 2, Route 22 and Viridian Forest) were audited and contain only
original Gen I species. No complete Kanto campaign playthrough was run.
A hosted GitHub Actions run has not been executed from this machine.

## Next milestone

Audit and validate the opening Kanto sequence and regional acquisition
paths, then build a small transport/transfer test area with adversarial
full-PC and save interruption tests. Implement the unified live battle
projection with transformation/healing/EXP regressions before adding a
full Johto campaign. Keep the 32 MiB linker limit; use measured compression,
shared graphics/tilesets and data deduplication before considering banking
or nonstandard emulator extensions.

## Files changed in this milestone

- Build and entry point: `Makefile`, `README.md`, `.github/workflows/build.yml`.
- Regional module and variable reservation: `include/generations.h`,
  `src/generations.c`, `include/constants/vars.h`.
- Engine integration: `src/new_game.c`, `src/event_data.c`,
  `src/post_battle_event_funcs.c`, `src/battle_gimmick.c`.
- Kanto starter integration: `src/starter_choose.c`, `data/specials.inc`,
  `data/maps/PalletTown_ProfessorOaksLab_Frlg/scripts.inc`.
- Regional dex configuration: `include/config/species_enabled.h`.
- Tests: `test/generations.c`, `test/save.c`.
- Documentation: `docs/ARCHITECTURE.md`, `docs/generations/STATUS.md`,
  `docs/SUMMARY.md`.
