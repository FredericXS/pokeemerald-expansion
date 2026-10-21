# Pokémon Generations: Kanto Foundation Prototype

## Inspection and unmodified baseline

This is FredericXS/pokeemerald-expansion at `f4dc700a130330fd2f9325a17bbb1e2d91dbf091`.
README.md identifies the base as RH Hideout expansion **1.17.1**. This fork
also incorporates FireRed/LeafGreen into the same expansion engine. It is
not a clean upstream Emerald checkout. There are 939 map definitions.

The unmodified `make -j5 -O all` succeeds with Debian ARM GCC 14.2.1 and
binutils 2.44. The baseline linker reports ROM 26,752,224 bytes, EWRAM
226,420 bytes and IWRAM 28,392 bytes. gbafix pads the output to 33,554,432
bytes (32 MiB); the padded file length is not occupied ROM space. The
unmodified Spikes subset passes 4/4 tests in the bundled headless mGBA.
There is no input base ROM requirement in this build workflow.

## Existing systems (before modifications)

| System | Implementation and implications |
| --- | --- |
| Species | `src/pokemon.c`, `src/data/pokemon/species_info/`, `include/config/species_enabled.h`: Gen I–IX and forms already compiled in. Do not remove families to restrict encounters. |
| Storage | `src/pokemon_storage_system.c`, `include/pokemon_storage_system.h`: 14 boxes × 30 = 420 stored Pokémon, plus six party slots. |
| Dex | `src/pokedex.c`, `src/pokedex_plus_hgss.c`, `include/constants/pokedex.h`, `src/data/pokemon/pokedex_orders.h`: national seen/caught bits, Hoenn and Kanto orders already exist. Cross-generation evolutions are optionally inserted into regional orders. |
| Maps | `data/maps`, `data/layouts`, `data/tilesets`, `map_data_rules.mk`: JSON headers/events and binary layouts processed by mapjson. Existing PalletTown_Frlg and Oak lab use FRLG tilesets and scripts. These are verified assets, not newly fabricated placeholders. Other Kanto content exists but has not been audited as a complete campaign. |
| Trainers | `src/battle_setup.c`, `src/data/trainers*.h`, `data/trainers*.json`, `trainer_rules.mk`: existing trainer processing, battle engine and battle tests. |
| Badges/events | `include/constants/flags.h`, `include/constants/vars.h`, FRLG counterparts, `src/event_data.c`: persisted flags and 256 u16 event variables in SaveBlock1. Both map sets share the actual storage. Unused names in the FRLG header are not a second independent array. |
| Saves | `src/save.c`, `include/save.h`, `include/global.h`: two alternating 14-sector slots, checksums and rotating sectors in 128 KiB flash. SaveBlock3 uses sector slack. Existing size assertions and `test/save.c` detect structural changes. |
| Level/experience | `src/caps.c`, `include/config/caps.h`: optional hard/soft experience caps. `CalculateMonStatsCont` in pokemon.c derives real level from experience and writes party stats. These are not temporary battle-level caps. |
| Battle projection | `PokemonToBattleMon` copies party data to BattlePokemon. Battle controllers also exchange raw party fields, update HP and experience and reapply transformed stats. Changing only this converter cannot implement a safe universal cap. |
| Modern mechanics | `src/battle_gimmick.c`, `CanMegaEvolve` in `src/battle_util.c`, `src/battle_z_move.c`, `src/battle_dynamax.c`, `src/battle_terastal.c`, `src/data/gimmicks.h`: Mega/Ultra, Z-Moves, Dynamax/Gigantamax and Tera already exist. Item, trainer and battle eligibility remain engine responsibilities. |
| Startup | `src/new_game.c`, `src/overworld.c`, `src/main_menu.c`, `src/oak_speech.c`: FRLG mode already selects Oak's introduction and Pallet bedroom instead of the truck. Existing Oak starter events give Bulbasaur, Squirtle or Charmander at level 5. |
| Build/CI | GNU Make, host C/C++, ARM GCC/binutils/newlib, libpng, Python. `make tools`, `make all`, `make check TESTS="Spikes"`. Existing `.github/workflows/build.yml` compiles variants and tests but does not upload a ROM. |

## Foundation design

Reuse the existing enum Region, including REGION_NONE and REGION_HISUI;
never renumber it. Campaign order is an explicit nine-element table.
Persist project state in reserved, previously unused event-variable slots,
without moving any save fields or changing Pokémon structures. Unknown
project save schema must fail closed; do not silently initialize loaded
legacy saves. New-game initialization is the only automatic initialization.

The first playable target uses the fork's existing FRLG mode within the
expansion engine. No separate engine is introduced. Later regions, party
transfer and effective battle-level caps are separate integration steps.
No travel API should move the player or remove Pokémon before the transfer
transaction and target-map availability have been verified.

Keep national registration, encounter eligibility, gift exceptions, party
eligibility and PC access as separate concepts. Registering a Pokémon must
never clear earlier seen/caught bits. Unlock checks supplement, rather than
replace, the engine's normal gimmick eligibility.

See `docs/generations/STATUS.md` for implemented/tested scope, save budget,
cap demonstration limits and the next milestone. Neither the nine-region
campaign nor support for storing 1,000 Pokémon is delivered by this prototype.
