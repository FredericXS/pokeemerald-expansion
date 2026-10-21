#include "global.h"
#include "generations.h"
#include "event_data.h"
#include "pokemon.h"
#include "battle.h"
#include "starter_choose.h"
#include "pokedex.h"
#include "malloc.h"
#include "test/test.h"
#include "test/overworld_script.h"

static void AwardAllBadges(enum Region region)
{
    for (u32 badge = 0; badge < 8; badge++)
        Generations_GrantBadge(region, badge);
}

TEST("Generations: fresh progression starts with only Kanto unlocked")
{
    Generations_InitNewGame();
    EXPECT_EQ(Generations_GetCurrentRegion(), REGION_KANTO);
    EXPECT(Generations_GetRegionState(REGION_KANTO) & REGION_PROGRESS_ENTERED);
    EXPECT_EQ(VarGet(VAR_GENERATIONS_SCHEMA), GENERATIONS_SCHEMA);
    EXPECT(Generations_IsRegionUnlocked(REGION_KANTO));
    EXPECT(!Generations_IsRegionUnlocked(REGION_JOHTO));
    EXPECT(!Generations_IsRegionUnlocked(REGION_HOENN));
    EXPECT(!Generations_IsRegionUnlocked(REGION_PALDEA));
}

TEST("Generations: Oak gives each Kanto starter through the script engine")
{
    enum Species species;
    PARAMETRIZE { species = SPECIES_BULBASAUR; }
    PARAMETRIZE { species = SPECIES_CHARMANDER; }
    PARAMETRIZE { species = SPECIES_SQUIRTLE; }
    Generations_InitNewGame();
    ZeroPlayerPartyMons();
    gPartiesCount[B_TRAINER_PLAYER] = 0;
    gSpecialVar_0x8004 = species;
    RUN_OVERWORLD_SCRIPT(
        givemon VAR_0x8004, 5;
        special Generations_RecordStarter;
    );
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), species);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), 5);
    EXPECT(Generations_GetRegionState(REGION_KANTO) & REGION_PROGRESS_STARTER);
    // Recording progress repeatedly must not give another Pokémon.
    Generations_RecordStarter();
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);
}

TEST("Generations: starter IDs match Oak script ordering")
{
    ASSUME(IS_FRLG);
    EXPECT_EQ(GetStarterPokemon(0), SPECIES_BULBASAUR);
    EXPECT_EQ(GetStarterPokemon(1), SPECIES_SQUIRTLE);
    EXPECT_EQ(GetStarterPokemon(2), SPECIES_CHARMANDER);
    EXPECT_EQ(KANTO_DEX_MEW, 151);
}

TEST("Generations: badges persist once and raise regional cap")
{
    Generations_InitNewGame();
    EXPECT_EQ(Generations_GetLevelCap(REGION_KANTO), 15);
    FlagSet(FLAG_BADGE01_GET);
    FlagSet(FLAG_BADGE01_GET);
    EXPECT_EQ(Generations_GetLevelCap(REGION_KANTO), 20);
    EXPECT_EQ(Generations_GetRegionState(REGION_KANTO) & REGION_PROGRESS_BADGES, 1);
    EXPECT(!Generations_GrantBadge(REGION_KANTO, 8));
    EXPECT(!Generations_GrantBadge(REGION_JOHTO, 0));
}

TEST("Generations: League and Champion gate the next region in order")
{
    Generations_InitNewGame();
    EXPECT(!Generations_RecordLeagueVictory(REGION_KANTO));
    EXPECT(!Generations_RecordChampionVictory(REGION_KANTO));
    AwardAllBadges(REGION_KANTO);
    EXPECT(!Generations_RecordChampionVictory(REGION_KANTO));
    EXPECT(Generations_RecordLeagueVictory(REGION_KANTO));
    EXPECT(!Generations_IsRegionUnlocked(REGION_JOHTO));
    EXPECT(Generations_RecordChampionVictory(REGION_KANTO));
    EXPECT(Generations_IsRegionUnlocked(REGION_JOHTO));
    EXPECT(!(Generations_GetRegionState(REGION_JOHTO) & REGION_PROGRESS_ENTERED));
    EXPECT(!Generations_IsRegionUnlocked(REGION_HOENN));
    EXPECT_EQ(Generations_GetLevelCap(REGION_KANTO), MAX_LEVEL);
    EXPECT(Generations_RecordChampionVictory(REGION_KANTO));
    EXPECT_EQ(Generations_GetCurrentRegion(), REGION_KANTO);
}

TEST("Generations: unknown schema and non-campaign regions fail closed")
{
    Generations_InitNewGame();
    EXPECT(!Generations_IsRegionUnlocked(REGION_HISUI));
    EXPECT(!Generations_GrantBadge(REGION_NONE, 0));
    EXPECT_EQ(Generations_GetRegionState(REGIONS_COUNT), 0);
    VarSet(VAR_GENERATIONS_SCHEMA, 0xFFFF);
    EXPECT(!Generations_IsActive());
    EXPECT_EQ(Generations_GetCurrentRegion(), REGION_NONE);
    EXPECT_EQ(Generations_GetLevelCap(REGION_KANTO), 0);
    EXPECT(!Generations_IsRegionUnlocked(REGION_KANTO));
    EXPECT(!Generations_UnlockMechanic(GIMMICK_MEGA));
    EXPECT_EQ(VarGet(VAR_GENERATIONS_SCHEMA), 0xFFFF);
}

TEST("Generations: progression survives a SaveBlock1 memory round trip")
{
    Generations_InitNewGame();
    AwardAllBadges(REGION_KANTO);
    Generations_RecordLeagueVictory(REGION_KANTO);
    Generations_RecordChampionVictory(REGION_KANTO);
    u32 state = Generations_GetRegionState(REGION_KANTO);
    struct SaveBlock1 *saved = Alloc(sizeof(*saved));
    EXPECT(saved != NULL);
    *saved = *gSaveBlock1Ptr;
    memset(gSaveBlock1Ptr, 0, sizeof(*gSaveBlock1Ptr));
    EXPECT(!Generations_IsActive());
    *gSaveBlock1Ptr = *saved;
    Free(saved);
    EXPECT_EQ(Generations_GetRegionState(REGION_KANTO), state);
    EXPECT(Generations_IsRegionUnlocked(REGION_JOHTO));
    EXPECT_EQ(Generations_GetCurrentRegion(), REGION_KANTO);
}

TEST("Generations: encounter gift and party policies are independent")
{
    Generations_InitNewGame();
    EXPECT_EQ(Generations_GetDexLimit(REGION_KANTO), 151);
    EXPECT(Generations_IsWildSpeciesAllowed(REGION_KANTO, SPECIES_MEW));
    EXPECT(!Generations_IsWildSpeciesAllowed(REGION_KANTO, SPECIES_CHIKORITA));
    EXPECT(!Generations_IsWildSpeciesAllowed(REGION_KANTO, SPECIES_RATTATA_ALOLA));
    EXPECT(!Generations_IsWildSpeciesAllowed(REGION_KANTO, SPECIES_NONE));
    EXPECT(!Generations_IsWildSpeciesAllowed(REGION_KANTO, NUM_SPECIES));
    EXPECT(!Generations_IsGiftSpeciesAllowed(REGION_KANTO, SPECIES_CHIKORITA, FALSE));
    EXPECT(Generations_IsGiftSpeciesAllowed(REGION_KANTO, SPECIES_CHIKORITA, TRUE));
    EXPECT(!Generations_IsGiftSpeciesAllowed(REGION_JOHTO, SPECIES_CHIKORITA, TRUE));
    EXPECT(Generations_IsPartySpeciesAllowed(SPECIES_CHIKORITA));
    EXPECT(Generations_IsPartySpeciesAllowed(SPECIES_PECHARUNT));
}

TEST("Generations: mechanic unlocks require their story region and survive returning")
{
    Generations_InitNewGame();
    EXPECT(!Generations_IsMechanicUnlocked(GIMMICK_MEGA));
    EXPECT(!Generations_IsMechanicUnlocked(GIMMICK_Z_MOVE));
    EXPECT(!Generations_IsMechanicUnlocked(GIMMICK_DYNAMAX));
    EXPECT(!Generations_IsMechanicUnlocked(GIMMICK_TERA));
    EXPECT(!Generations_UnlockMechanic(GIMMICK_MEGA));
    // Fixture only: no production travel API exists before safe transfer does.
    VarSet(VAR_GENERATIONS_KANTO + 5, REGION_PROGRESS_UNLOCKED | REGION_PROGRESS_ENTERED);
    VarSet(VAR_GENERATIONS_REGION, REGION_KALOS);
    EXPECT(Generations_UnlockMechanic(GIMMICK_MEGA));
    EXPECT(!Generations_UnlockMechanic(GIMMICK_TERA));
    VarSet(VAR_GENERATIONS_REGION, REGION_KANTO);
    EXPECT(Generations_IsMechanicUnlocked(GIMMICK_MEGA));
    EXPECT(!Generations_IsMechanicUnlocked(GIMMICKS_COUNT));
}

TEST("Generations: Johto cap and prior PC unlock depend on distinct badges")
{
    Generations_InitNewGame();
    VarSet(VAR_GENERATIONS_KANTO + 1, REGION_PROGRESS_UNLOCKED | REGION_PROGRESS_ENTERED);
    EXPECT_EQ(Generations_GetLevelCap(REGION_JOHTO), 15);
    EXPECT(!Generations_CanAccessPreviousRegionPC(REGION_JOHTO));
    for (u32 badge = 0; badge < 4; badge++)
        Generations_GrantBadge(REGION_JOHTO, badge);
    EXPECT_EQ(Generations_GetLevelCap(REGION_JOHTO), 35);
    EXPECT(Generations_CanAccessPreviousRegionPC(REGION_JOHTO));
    EXPECT_EQ(Generations_GetLevelCap(REGION_KANTO), 15);
}

TEST("Generations: level 75 Charizard projects level 35 stats without changing stored data")
{
    Generations_InitNewGame();
    VarSet(VAR_GENERATIONS_KANTO + 1, REGION_PROGRESS_UNLOCKED | REGION_PROGRESS_ENTERED | 0xF);
    struct Pokemon mon, reference;
    struct BattlePokemon projected;
    CreateRandomMonWithIVs(&mon, SPECIES_CHARIZARD, 75, 27);
    u32 ev = 100, nature = NATURE_MODEST;
    SetMonData(&mon, MON_DATA_SPATK_EV, &ev);
    SetMonData(&mon, MON_DATA_HIDDEN_NATURE, &nature);
    SetMonMoveSlot(&mon, MOVE_FLAMETHROWER, 0);
    CalculateMonStats(&mon);
    struct Pokemon snapshot = mon;
    reference = mon;
    u32 exp = gExperienceTables[gSpeciesInfo[SPECIES_CHARIZARD].growthRate][35];
    SetMonData(&reference, MON_DATA_EXP, &exp);
    CalculateMonStats(&reference);
    Generations_ProjectBattleMon(&mon, &projected, REGION_JOHTO);
    EXPECT_EQ(projected.level, 35);
    EXPECT_EQ(projected.maxHP, reference.maxHP);
    EXPECT_EQ(projected.hp, reference.maxHP);
    EXPECT_EQ(projected.attack, reference.attack);
    EXPECT_EQ(projected.defense, reference.defense);
    EXPECT_EQ(projected.speed, reference.speed);
    EXPECT_EQ(projected.spAttack, reference.spAttack);
    EXPECT_EQ(projected.spDefense, reference.spDefense);
    EXPECT_EQ(projected.experience, GetMonData(&mon, MON_DATA_EXP));
    EXPECT_EQ(projected.moves[0], MOVE_FLAMETHROWER);
    EXPECT_EQ(memcmp(&snapshot, &mon, sizeof(mon)), 0);
    AwardAllBadges(REGION_JOHTO);
    Generations_RecordLeagueVictory(REGION_JOHTO);
    Generations_RecordChampionVictory(REGION_JOHTO);
    Generations_ProjectBattleMon(&mon, &projected, REGION_JOHTO);
    EXPECT_EQ(projected.level, 75);
    EXPECT_EQ(projected.maxHP, mon.maxHP);
    EXPECT_EQ(memcmp(&snapshot, &mon, sizeof(mon)), 0);
}

TEST("Generations: projected HP preserves fainting and injury ratios")
{
    u32 hp;
    PARAMETRIZE { hp = 0; }
    PARAMETRIZE { hp = 1; }
    PARAMETRIZE { hp = 50; }
    Generations_InitNewGame();
    struct Pokemon mon;
    struct BattlePokemon projected;
    CreateRandomMonWithIVs(&mon, SPECIES_BLASTOISE, 75, 31);
    SetMonData(&mon, MON_DATA_HP, &hp);
    struct Pokemon snapshot = mon;
    Generations_ProjectBattleMon(&mon, &projected, REGION_KANTO);
    EXPECT_EQ(projected.level, 15);
    EXPECT_EQ(projected.hp, hp == 0 ? 0 : max(1, hp * projected.maxHP / mon.maxHP));
    EXPECT_EQ(memcmp(&snapshot, &mon, sizeof(mon)), 0);
}

TEST("Generations: unlocking a region preserves national seen and caught records")
{
    Generations_InitNewGame();
    GetSetPokedexFlag(NATIONAL_DEX_BULBASAUR, FLAG_SET_SEEN);
    GetSetPokedexFlag(NATIONAL_DEX_BULBASAUR, FLAG_SET_CAUGHT);
    AwardAllBadges(REGION_KANTO);
    Generations_RecordLeagueVictory(REGION_KANTO);
    Generations_RecordChampionVictory(REGION_KANTO);
    EXPECT_EQ(Generations_GetDexLimit(REGION_JOHTO), 251);
    EXPECT(GetSetPokedexFlag(NATIONAL_DEX_BULBASAUR, FLAG_GET_SEEN));
    EXPECT(GetSetPokedexFlag(NATIONAL_DEX_BULBASAUR, FLAG_GET_CAUGHT));
    EXPECT(!GetSetPokedexFlag(NATIONAL_DEX_CHIKORITA, FLAG_GET_SEEN));
}
