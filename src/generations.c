#include "global.h"
#include "generations.h"
#include "event_data.h"
#include "pokemon.h"
#include "battle.h"

struct CampaignRegion
{
    enum Region region;
    u16 dexLimit;
    u8 levelCaps[9];
    u8 legacyPCBadge;
};

// Provisional balance. Keep each region independently configurable.
#define DEFAULT_CAPS {15, 20, 25, 30, 35, 40, 45, 50, 60}
static const struct CampaignRegion sCampaign[] =
{
    {REGION_KANTO,  NATIONAL_DEX_MEW,       DEFAULT_CAPS, 0},
    {REGION_JOHTO,  NATIONAL_DEX_CELEBI,    DEFAULT_CAPS, 4},
    {REGION_HOENN,  NATIONAL_DEX_DEOXYS,    DEFAULT_CAPS, 4},
    {REGION_SINNOH, NATIONAL_DEX_ARCEUS,    DEFAULT_CAPS, 4},
    {REGION_UNOVA,  NATIONAL_DEX_GENESECT,  DEFAULT_CAPS, 4},
    {REGION_KALOS,  NATIONAL_DEX_VOLCANION, DEFAULT_CAPS, 4},
    {REGION_ALOLA,  NATIONAL_DEX_MELMETAL,  DEFAULT_CAPS, 4},
    {REGION_GALAR,  NATIONAL_DEX_ENAMORUS,  DEFAULT_CAPS, 4},
    {REGION_PALDEA, NATIONAL_DEX_PECHARUNT, DEFAULT_CAPS, 4},
};
STATIC_ASSERT(ARRAY_COUNT(sCampaign) == GENERATIONS_CAMPAIGN_COUNT, CampaignRegionCount);
STATIC_ASSERT(VAR_GENERATIONS_KANTO + GENERATIONS_CAMPAIGN_COUNT - 1 <= VARS_END, CampaignVarsFit);

static s32 GetCampaignIndex(enum Region region)
{
    for (u32 i = 0; i < ARRAY_COUNT(sCampaign); i++)
        if (sCampaign[i].region == region)
            return i;
    return -1;
}

static u32 CountBadges(u32 state)
{
    u32 count = 0;
    for (u32 i = 0; i < 8; i++)
        if (state & (1u << i))
            count++;
    return count;
}

static bool32 SetRegionState(enum Region region, u32 state)
{
    s32 index = GetCampaignIndex(region);
    if (!Generations_IsActive() || index < 0)
        return FALSE;
    return VarSet(VAR_GENERATIONS_KANTO + index, state);
}

void Generations_InitNewGame(void)
{
    VarSet(VAR_GENERATIONS_SCHEMA, GENERATIONS_SCHEMA);
    VarSet(VAR_GENERATIONS_MECHANICS, 0);
    VarSet(VAR_GENERATIONS_REGION, REGION_KANTO);
    for (u32 i = 0; i < ARRAY_COUNT(sCampaign); i++)
        VarSet(VAR_GENERATIONS_KANTO + i, 0);
    SetRegionState(REGION_KANTO, REGION_PROGRESS_UNLOCKED | REGION_PROGRESS_ENTERED | REGION_PROGRESS_LEGACY_PC);
}

bool32 Generations_IsActive(void)
{
    return VarGet(VAR_GENERATIONS_SCHEMA) == GENERATIONS_SCHEMA;
}

enum Region Generations_GetCurrentRegion(void)
{
    enum Region region = VarGet(VAR_GENERATIONS_REGION);
    return Generations_IsActive() && GetCampaignIndex(region) >= 0 ? region : REGION_NONE;
}

u32 Generations_GetRegionState(enum Region region)
{
    s32 index = GetCampaignIndex(region);
    if (!Generations_IsActive() || index < 0)
        return 0;
    return VarGet(VAR_GENERATIONS_KANTO + index);
}

bool32 Generations_IsRegionUnlocked(enum Region region)
{
    return (Generations_GetRegionState(region) & REGION_PROGRESS_UNLOCKED) != 0;
}

bool32 Generations_GrantBadge(enum Region region, u32 badge)
{
    u32 state = Generations_GetRegionState(region);
    s32 index = GetCampaignIndex(region);
    if (index < 0 || badge >= 8 || !(state & REGION_PROGRESS_ENTERED))
        return FALSE;
    state |= 1u << badge;
    if (CountBadges(state) >= sCampaign[index].legacyPCBadge)
        state |= REGION_PROGRESS_LEGACY_PC;
    return SetRegionState(region, state);
}

void Generations_OnFlagSet(u16 flag)
{
    // Existing map scripts still need the engine badge flags. Regional words
    // retain their history when future travel restores the active flag set.
    if (flag >= FLAG_BADGE01_GET && flag <= FLAG_BADGE08_GET)
        Generations_GrantBadge(Generations_GetCurrentRegion(), flag - FLAG_BADGE01_GET);
}

bool32 Generations_RecordLeagueVictory(enum Region region)
{
    u32 state = Generations_GetRegionState(region);
    if (!(state & REGION_PROGRESS_ENTERED) || CountBadges(state) != 8)
        return FALSE;
    return SetRegionState(region, state | REGION_PROGRESS_LEAGUE);
}

bool32 Generations_RecordChampionVictory(enum Region region)
{
    u32 state = Generations_GetRegionState(region);
    s32 index = GetCampaignIndex(region);
    if (index < 0 || !(state & REGION_PROGRESS_LEAGUE) || CountBadges(state) != 8)
        return FALSE;
    SetRegionState(region, state | REGION_PROGRESS_CHAMPION | REGION_PROGRESS_LEGACY_PC);
    if (index + 1 < ARRAY_COUNT(sCampaign))
    {
        enum Region next = sCampaign[index + 1].region;
        SetRegionState(next, Generations_GetRegionState(next) | REGION_PROGRESS_UNLOCKED);
    }
    // Unlocking does not create a destination or transfer/clear the party.
    return TRUE;
}

void Generations_RecordStarter(void)
{
    u32 count = gPartiesCount[B_TRAINER_PLAYER];
    if (count == 0 || count > PARTY_SIZE)
        return;
    // Oak appends the received starter; the test gift may already be first.
    enum Species species = GetMonData(&gParties[B_TRAINER_PLAYER][count - 1], MON_DATA_SPECIES);
    if (Generations_GetCurrentRegion() == REGION_KANTO
     && (species == SPECIES_BULBASAUR || species == SPECIES_CHARMANDER || species == SPECIES_SQUIRTLE))
        SetRegionState(REGION_KANTO, Generations_GetRegionState(REGION_KANTO) | REGION_PROGRESS_STARTER);
}

u32 Generations_GetLevelCap(enum Region region)
{
    s32 index = GetCampaignIndex(region);
    u32 state = Generations_GetRegionState(region);
    if (index < 0 || !(state & REGION_PROGRESS_ENTERED))
        return 0;
    if (state & REGION_PROGRESS_CHAMPION)
        return MAX_LEVEL;
    return sCampaign[index].levelCaps[CountBadges(state)];
}

u32 Generations_GetDexLimit(enum Region region)
{
    s32 index = GetCampaignIndex(region);
    return index >= 0 && Generations_IsRegionUnlocked(region) ? sCampaign[index].dexLimit : 0;
}

bool32 Generations_IsWildSpeciesAllowed(enum Region region, enum Species species)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return FALSE;
    u32 dex = SpeciesToNationalPokedexNum(species);
    // Regional and battle-only forms are not original Kanto encounters.
    if (region == REGION_KANTO && species != GET_BASE_SPECIES_ID(species))
        return FALSE;
    return dex != NATIONAL_DEX_NONE && dex <= Generations_GetDexLimit(region);
}

bool32 Generations_IsGiftSpeciesAllowed(enum Region region, enum Species species, bool32 specialEvent)
{
    if (!Generations_IsRegionUnlocked(region) || species == SPECIES_NONE || species >= NUM_SPECIES)
        return FALSE;
    return specialEvent || Generations_IsWildSpeciesAllowed(region, species);
}

bool32 Generations_IsPartySpeciesAllowed(enum Species species)
{
    // Generation gating is an acquisition policy, not a party ban.
    return species != SPECIES_NONE && species < NUM_SPECIES;
}

bool32 Generations_CanAccessPreviousRegionPC(enum Region region)
{
    return (Generations_GetRegionState(region) & REGION_PROGRESS_LEGACY_PC) != 0;
}

static enum Region GetMechanicRegion(enum Gimmick gimmick)
{
    switch (gimmick)
    {
    case GIMMICK_MEGA: return REGION_KALOS;
    case GIMMICK_ULTRA_BURST:
    case GIMMICK_Z_MOVE: return REGION_ALOLA;
    case GIMMICK_DYNAMAX: return REGION_GALAR;
    case GIMMICK_TERA: return REGION_PALDEA;
    default: return REGION_NONE;
    }
}

bool32 Generations_UnlockMechanic(enum Gimmick gimmick)
{
    enum Region region = GetMechanicRegion(gimmick);
    if (region == REGION_NONE || Generations_GetCurrentRegion() != region
     || !(Generations_GetRegionState(region) & REGION_PROGRESS_ENTERED))
        return FALSE;
    return VarSet(VAR_GENERATIONS_MECHANICS, VarGet(VAR_GENERATIONS_MECHANICS) | (1u << gimmick));
}

bool32 Generations_IsMechanicUnlocked(enum Gimmick gimmick)
{
    if (gimmick == GIMMICK_NONE)
        return TRUE;
    if (!Generations_IsActive() || GetMechanicRegion(gimmick) == REGION_NONE)
        return FALSE;
    return (VarGet(VAR_GENERATIONS_MECHANICS) & (1u << gimmick)) != 0;
}

void Generations_ProjectBattleMon(struct Pokemon *mon, struct BattlePokemon *battleMon, enum Region region)
{
    u32 cap = Generations_GetLevelCap(region);
    u32 level = GetMonData(mon, MON_DATA_LEVEL);
    PokemonToBattleMon(mon, battleMon);
    if (cap == 0 || level <= cap)
        return;

    // Calculate using the engine, on a disposable copy only. Never save this
    // projection or copy its HP/level/experience back to the party.
    struct Pokemon projection = *mon;
    enum Species species = GetMonData(mon, MON_DATA_SPECIES);
    u32 experience = gExperienceTables[gSpeciesInfo[species].growthRate][cap];
    SetMonData(&projection, MON_DATA_EXP, &experience);
    projection.hp = projection.maxHP = 0;
    s32 oldLevelUpHP = gBattleScripting.levelUpHP;
    CalculateMonStats(&projection);
    gBattleScripting.levelUpHP = oldLevelUpHP;
    PokemonToBattleMon(&projection, battleMon);
    battleMon->experience = GetMonData(mon, MON_DATA_EXP);
    u32 hp = GetMonData(mon, MON_DATA_HP);
    u32 maxHP = GetMonData(mon, MON_DATA_MAX_HP);
    battleMon->hp = hp == 0 || maxHP == 0 ? 0 : max(1, hp * battleMon->maxHP / maxHP);
}
