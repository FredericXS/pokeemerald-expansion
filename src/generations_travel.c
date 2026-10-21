#include "global.h"
#include "generations.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "event_data.h"
#include "malloc.h"
#include "mail.h"
#include "pokedex.h"
#include "overworld.h"
#include "constants/heal_locations.h"

#if TESTING
// Exercise the same rollback path as a failed write/readback, without shipping
// a fault-injection interface in playable ROMs.
EWRAM_DATA s32 gGenerationsTransferFailAfter = 0;
#endif

struct TransferSlot
{
    u8 box;
    u8 slot;
    struct BoxPokemon before;
};

struct JohtoTransfer
{
    struct Pokemon party[PARTY_SIZE];
    struct Pokemon starter;
    struct TransferSlot slots[PARTY_SIZE];
};

static void RestoreBadgeFlags(enum Region region)
{
    u32 state = Generations_GetRegionState(region);
    // Set active region first: FlagSet routes badge writes to that region.
    VarSet(VAR_GENERATIONS_REGION, region);
    for (u32 badge = 0; badge < 8; badge++)
    {
        FlagClear(FLAG_BADGE01_GET + badge);
        if (state & (1u << badge))
            FlagSet(FLAG_BADGE01_GET + badge);
    }
}

bool32 Generations_CanUsePC(void)
{
    // Conservative prototype policy: no PC access at all in early Johto.
    // This needs no per-mon provenance or changes to the persisted layout.
    return !Generations_IsActive()
        || Generations_GetCurrentRegion() != REGION_JOHTO
        || Generations_CanAccessPreviousRegionPC(REGION_JOHTO);
}

u32 Generations_EnterJohto(enum Species starter)
{
    u32 state = Generations_GetRegionState(REGION_JOHTO);
    u32 count = gPartiesCount[B_TRAINER_PLAYER];
    if (Generations_GetCurrentRegion() != REGION_KANTO || !(state & REGION_PROGRESS_UNLOCKED))
        return GENERATIONS_TRAVEL_LOCKED;

    if (state & REGION_PROGRESS_ENTERED)
    {
        // Return visits never deposit or award a second starter.
        if (!(state & REGION_PROGRESS_STARTER) || count == 0)
            return GENERATIONS_TRAVEL_LOCKED;
        RestoreBadgeFlags(REGION_JOHTO);
        SetLastHealLocationWarp(HEAL_LOCATION_NEW_BARK_TOWN);
        return GENERATIONS_TRAVEL_OK;
    }

    if (starter != SPECIES_CHIKORITA && starter != SPECIES_CYNDAQUIL && starter != SPECIES_TOTODILE)
        return GENERATIONS_TRAVEL_INVALID_STARTER;
    if (count == 0 || count > PARTY_SIZE)
        return GENERATIONS_TRAVEL_UNSAFE_PARTY;

    // Active fused partners have engine-owned storage references. Require
    // unfusing first instead of relocating only half of a fusion.
    for (u32 i = 0; i < MAX_FUSION_STORAGE; i++)
        if (gPokemonStoragePtr->fusions[i].box.hasSpecies)
            return GENERATIONS_TRAVEL_UNSAFE_PARTY;

    struct JohtoTransfer *transfer = AllocZeroed(sizeof(*transfer));
    if (transfer == NULL)
        return GENERATIONS_TRAVEL_FAILED;
    memcpy(transfer->party, gParties[B_TRAINER_PLAYER], sizeof(transfer->party));

    // Validate copies: checksum detection must not mutate a rejected party.
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &transfer->party[i];
        enum Species species = GetMonData(mon, MON_DATA_SPECIES);
        if ((i < count && (species == SPECIES_NONE || species >= NUM_SPECIES || mon->box.isBadEgg
             || GetMonData(mon, MON_DATA_MAIL) != MAIL_NONE || ItemIsMail(GetMonData(mon, MON_DATA_HELD_ITEM))))
         || (i >= count && (species != SPECIES_NONE || mon->box.isBadEgg)))
        {
            Free(transfer);
            return GENERATIONS_TRAVEL_UNSAFE_PARTY;
        }
    }

    // Reserve enough slots for the WHOLE party, across box boundaries.
    u32 reserved = 0;
    for (u32 box = 0; box < TOTAL_BOXES_COUNT && reserved < count; box++)
    {
        for (u32 slot = 0; slot < IN_BOX_COUNT && reserved < count; slot++)
        {
            struct BoxPokemon *dest = GetBoxedMonPtr(box, slot);
            // A damaged occupancy flag must not turn an existing mon into
            // a free slot. Validate a copy so even bad checksums stay intact.
            struct BoxPokemon candidate = *dest;
            if (!dest->hasSpecies && !dest->isBadEgg && !dest->isEgg
             && GetBoxMonData(&candidate, MON_DATA_SPECIES) == SPECIES_NONE && !candidate.isBadEgg)
            {
                transfer->slots[reserved].box = box;
                transfer->slots[reserved].slot = slot;
                transfer->slots[reserved].before = *dest;
                reserved++;
            }
        }
    }
    if (reserved != count)
    {
        Free(transfer);
        return GENERATIONS_TRAVEL_PC_FULL;
    }

    CreateRandomMonWithIVs(&transfer->starter, starter, 5, USE_RANDOM_IVS);
    u32 location = MAPSEC_NEW_BARK_TOWN;
    SetMonData(&transfer->starter, MON_DATA_MET_LOCATION, &location);

    // No yielding, scripts or flash writes inside this transaction. PC copies
    // retain held items, egg data, PP, experience, IVs/EVs, HP loss and status.
    // Unlike CopyMonToPC, this does not heal or restore PP.
    u32 written = 0;
    for (u32 i = 0; i < count; i++)
    {
        struct TransferSlot *slot = &transfer->slots[i];
        struct BoxPokemon *dest = GetBoxedMonPtr(slot->box, slot->slot);
        *dest = transfer->party[i].box;
        written++;
        bool32 failed = memcmp(dest, &transfer->party[i].box, sizeof(*dest)) != 0;
#if TESTING
        failed |= gGenerationsTransferFailAfter == written;
#endif
        if (failed)
        {
            for (u32 j = 0; j < written; j++)
            {
                struct TransferSlot *restore = &transfer->slots[j];
                *GetBoxedMonPtr(restore->box, restore->slot) = restore->before;
            }
            Free(transfer);
            return GENERATIONS_TRAVEL_FAILED;
        }
    }

    // All fallible work finished. Publish team/progression together before
    // returning to the script that performs the fixed destination warp.
    // ZeroMonData sets HP before maxHP. Clear the old stats first so emptied
    // slots cannot inherit an old maxHP as boxed HP loss on save/reload.
    memset(gParties[B_TRAINER_PLAYER], 0, sizeof(transfer->party));
    ZeroPlayerPartyMons();
    gParties[B_TRAINER_PLAYER][0] = transfer->starter;
    gPartiesCount[B_TRAINER_PLAYER] = 1;
    VarSet(VAR_GENERATIONS_KANTO + 1, state | REGION_PROGRESS_ENTERED | REGION_PROGRESS_STARTER);
    RestoreBadgeFlags(REGION_JOHTO);
    SetLastHealLocationWarp(HEAL_LOCATION_NEW_BARK_TOWN);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(starter), FLAG_SET_SEEN);
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(starter), FLAG_SET_CAUGHT);
    Free(transfer);
    return GENERATIONS_TRAVEL_OK;
}

bool32 Generations_ReturnToKanto(void)
{
    // Prevent retrieving the old party in Kanto before Johto's PC threshold.
    if (Generations_GetCurrentRegion() != REGION_JOHTO
     || !Generations_CanAccessPreviousRegionPC(REGION_JOHTO)
     || gPartiesCount[B_TRAINER_PLAYER] == 0)
        return FALSE;
    RestoreBadgeFlags(REGION_KANTO);
    SetLastHealLocationWarp(HEAL_LOCATION_PALLET_TOWN);
    return TRUE;
}

void Generations_TravelToJohto(void)
{
    gSpecialVar_Result = Generations_EnterJohto(gSpecialVar_0x8004);
}

void Generations_TravelToKanto(void)
{
    gSpecialVar_Result = Generations_ReturnToKanto();
}

void Generations_CheckPC(void)
{
    gSpecialVar_Result = Generations_CanUsePC();
}

void Generations_TestProgress(void)
{
    gSpecialVar_Result = FALSE;
#if GENERATIONS_TEST_ROM
    if (!Generations_IsActive())
        return;
    enum Region region = Generations_GetCurrentRegion();
    if (region == REGION_KANTO)
    {
        // Only progression metadata; do not skip or modify Oak's quest scenes.
        for (u32 badge = 0; badge < 8; badge++)
            Generations_GrantBadge(region, badge);
        Generations_RecordLeagueVictory(region);
        Generations_RecordChampionVictory(region);
    }
    else if (region == REGION_JOHTO)
    {
        for (u32 badge = 0; badge < 4; badge++)
            Generations_GrantBadge(region, badge);
    }
    else
        return;
    RestoreBadgeFlags(region);
    gSpecialVar_Result = TRUE;
#endif
}
