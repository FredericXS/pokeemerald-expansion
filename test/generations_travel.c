#include "global.h"
#include "generations.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "event_data.h"
#include "malloc.h"
#include "pokedex.h"
#include "overworld.h"
#include "load_save.h"
#include "party_menu.h"
#include "wild_encounter.h"
#include "tilesets.h"
#include "heal_location.h"
#include "constants/maps.h"
#include "constants/heal_locations.h"
#include "test/test.h"
#include "test/overworld_script.h"

static void PrepareTravel(u32 count)
{
    Generations_InitNewGame();
    ResetPokemonStorageSystem();
    ZeroPlayerPartyMons();
    gPartiesCount[B_TRAINER_PLAYER] = count;
    gGenerationsTransferFailAfter = -1;
    for (u32 i = 0; i < count; i++)
        CreateRandomMonWithIVs(&gParties[B_TRAINER_PLAYER][i], SPECIES_BULBASAUR + i, 25 + i, 20 + i);
    for (u32 i = 0; i < 8; i++)
        Generations_GrantBadge(REGION_KANTO, i);
    Generations_RecordLeagueVictory(REGION_KANTO);
    Generations_RecordChampionVictory(REGION_KANTO);
}

struct TravelSnapshot
{
    struct SaveBlock1 save;
    struct PokemonStorage storage;
    struct Pokemon party[PARTY_SIZE];
    u8 count;
};

static struct TravelSnapshot *Snapshot(void)
{
    struct TravelSnapshot *s = Alloc(sizeof(*s));
    if (s != NULL)
    {
        s->save = *gSaveBlock1Ptr;
        s->storage = *gPokemonStoragePtr;
        memcpy(s->party, gParties[B_TRAINER_PLAYER], sizeof(s->party));
        s->count = gPartiesCount[B_TRAINER_PLAYER];
    }
    return s;
}

static void ExpectUnchanged(struct TravelSnapshot *s)
{
    EXPECT_EQ(memcmp(&s->save, gSaveBlock1Ptr, sizeof(s->save)), 0);
    EXPECT_EQ(memcmp(&s->storage, gPokemonStoragePtr, sizeof(s->storage)), 0);
    EXPECT_EQ(memcmp(s->party, gParties[B_TRAINER_PLAYER], sizeof(s->party)), 0);
    EXPECT_EQ(s->count, gPartiesCount[B_TRAINER_PLAYER]);
    Free(s);
}

static void FillPC(void)
{
    struct Pokemon filler;
    CreateRandomMonWithIVs(&filler, SPECIES_MAGIKARP, 5, 0);
    for (u32 box = 0; box < TOTAL_BOXES_COUNT; box++)
        for (u32 slot = 0; slot < IN_BOX_COUNT; slot++)
            *GetBoxedMonPtr(box, slot) = filler.box;
}

TEST("Generations travel: full or insufficient PC space aborts without any mutation")
{
    u32 freeSlots;
    PARAMETRIZE { freeSlots = 0; }
    PARAMETRIZE { freeSlots = 5; }
    PrepareTravel(6);
    FillPC();
    // Inconsistent occupancy metadata must never permit an overwrite.
    gPokemonStoragePtr->boxes[0][0].hasSpecies = FALSE;
    for (u32 i = 0; i < freeSlots; i++)
        ZeroBoxMonAt(TOTAL_BOXES_COUNT - 1, IN_BOX_COUNT - 1 - i);
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    EXPECT_EQ(Generations_EnterJohto(SPECIES_CHIKORITA), GENERATIONS_TRAVEL_PC_FULL);
    ExpectUnchanged(s);
}

TEST("Generations travel: six deposits cross boxes and retain exact boxed data")
{
    PrepareTravel(6);
    FillPC();
    struct Pokemon *party = gParties[B_TRAINER_PLAYER];
    u32 item = ITEM_LEFTOVERS, hp = 1, status = STATUS1_POISON, pp = 1, egg = TRUE;
    SetMonData(&party[0], MON_DATA_HELD_ITEM, &item);
    SetMonData(&party[0], MON_DATA_HP, &hp);
    SetMonData(&party[0], MON_DATA_STATUS, &status);
    SetMonData(&party[0], MON_DATA_PP1, &pp);
    hp = 0;
    SetMonData(&party[1], MON_DATA_HP, &hp);
    SetMonData(&party[5], MON_DATA_IS_EGG, &egg);
    for (u32 i = 0; i < 6; i++)
        ZeroBoxMonAt(i * 2, IN_BOX_COUNT - 1);
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    EXPECT_EQ(Generations_EnterJohto(SPECIES_CYNDAQUIL), GENERATIONS_TRAVEL_OK);
    for (u32 i = 0; i < 6; i++)
    {
        struct BoxPokemon *boxed = GetBoxedMonPtr(i * 2, IN_BOX_COUNT - 1);
        EXPECT_EQ(memcmp(boxed, &s->party[i].box, sizeof(*boxed)), 0);
        // Update the expected destination only: all other 414 slots stay exact.
        s->storage.boxes[i * 2][IN_BOX_COUNT - 1] = s->party[i].box;
        struct Pokemon restored;
        BoxMonToMon(boxed, &restored);
        EXPECT_EQ(GetMonData(&restored, MON_DATA_EXP), GetMonData(&s->party[i], MON_DATA_EXP));
        EXPECT_EQ(restored.hp, s->party[i].hp);
        EXPECT_EQ(restored.status, s->party[i].status);
    }
    EXPECT_EQ(memcmp(&s->storage, gPokemonStoragePtr, sizeof(s->storage)), 0);
    EXPECT_EQ(gPartiesCount[B_TRAINER_PLAYER], 1);
    EXPECT_EQ(GetMonData(&party[0], MON_DATA_SPECIES), SPECIES_CYNDAQUIL);
    EXPECT_EQ(GetMonData(&party[0], MON_DATA_LEVEL), 5);
    for (u32 i = 1; i < PARTY_SIZE; i++)
        EXPECT_EQ(GetMonData(&party[i], MON_DATA_SPECIES), SPECIES_NONE);
    EXPECT_EQ(Generations_GetCurrentRegion(), REGION_JOHTO);
    EXPECT(Generations_GetRegionState(REGION_JOHTO) & REGION_PROGRESS_STARTER);
    EXPECT_EQ(Generations_GetRegionState(REGION_JOHTO) & REGION_PROGRESS_BADGES, 0);
    for (u32 i = 0; i < 8; i++)
        EXPECT(!FlagGet(FLAG_BADGE01_GET + i));
    Free(s);
}

TEST("Generations travel: every starter is awarded once through the transport script")
{
    enum Species starter;
    PARAMETRIZE { starter = SPECIES_CHIKORITA; }
    PARAMETRIZE { starter = SPECIES_CYNDAQUIL; }
    PARAMETRIZE { starter = SPECIES_TOTODILE; }
    PrepareTravel(1);
    gSpecialVar_0x8004 = starter;
    RUN_OVERWORLD_SCRIPT(call Generations_EventScript_CommitJohto;);
    EXPECT_EQ(gSpecialVar_Result, GENERATIONS_TRAVEL_OK);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), starter);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MET_LOCATION), MAPSEC_NEW_BARK_TOWN);
    EXPECT(GetSetPokedexFlag(SpeciesToNationalPokedexNum(starter), FLAG_GET_CAUGHT));
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    EXPECT_EQ(Generations_EnterJohto(starter), GENERATIONS_TRAVEL_LOCKED);
    ExpectUnchanged(s);
}

TEST("Generations travel: write failure rolls back every reserved PC slot")
{
    s32 failAfter;
    PARAMETRIZE { failAfter = 1; }
    PARAMETRIZE { failAfter = 3; }
    PARAMETRIZE { failAfter = 6; }
    PrepareTravel(6);
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    gGenerationsTransferFailAfter = failAfter;
    EXPECT_EQ(Generations_EnterJohto(SPECIES_TOTODILE), GENERATIONS_TRAVEL_FAILED);
    gGenerationsTransferFailAfter = -1;
    ExpectUnchanged(s);
}

TEST("Generations travel: mail fusion damage invalid choices and locked progress fail closed")
{
    u32 scenario;
    PARAMETRIZE { scenario = 0; } // Mail index.
    PARAMETRIZE { scenario = 1; } // Held mail even with no index.
    PARAMETRIZE { scenario = 2; } // Active fused partner.
    PARAMETRIZE { scenario = 3; } // Invalid checksum.
    PARAMETRIZE { scenario = 4; } // Invalid starter.
    PARAMETRIZE { scenario = 5; } // Johto still locked.
    PARAMETRIZE { scenario = 6; } // Unknown schema.
    PARAMETRIZE { scenario = 7; } // Empty party.
    PrepareTravel(1);
    u32 value = 0;
    enum Species starter = SPECIES_CHIKORITA;
    u32 expected = GENERATIONS_TRAVEL_UNSAFE_PARTY;
    switch (scenario)
    {
    case 0: SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_MAIL, &value); break;
    case 1: value = ITEM_ORANGE_MAIL; SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_HELD_ITEM, &value); break;
    case 2: gPokemonStoragePtr->fusions[0] = gParties[B_TRAINER_PLAYER][0]; break;
    case 3: gParties[B_TRAINER_PLAYER][0].box.checksum ^= 1; break;
    case 4: starter = SPECIES_BULBASAUR; expected = GENERATIONS_TRAVEL_INVALID_STARTER; break;
    case 5: VarSet(VAR_GENERATIONS_KANTO + 1, 0); expected = GENERATIONS_TRAVEL_LOCKED; break;
    case 6: VarSet(VAR_GENERATIONS_SCHEMA, 0xFFFF); expected = GENERATIONS_TRAVEL_LOCKED; break;
    case 7: ZeroPlayerPartyMons(); gPartiesCount[B_TRAINER_PLAYER] = 0; break;
    }
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    EXPECT_EQ(Generations_EnterJohto(starter), expected);
    ExpectUnchanged(s);
}

TEST("Generations travel: return and revisits retain party and restore regional badges")
{
    PrepareTravel(1);
    EXPECT_EQ(Generations_EnterJohto(SPECIES_TOTODILE), GENERATIONS_TRAVEL_OK);
    EXPECT(!Generations_CanUsePC());
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    EXPECT(!Generations_ReturnToKanto());
    ExpectUnchanged(s);
    for (u32 i = 0; i < 4; i++)
        FlagSet(FLAG_BADGE01_GET + i);
    EXPECT(Generations_CanUsePC());
    EXPECT(!SetUpFieldMove_Fly());
    s = Snapshot();
    EXPECT(s != NULL);
    EXPECT(Generations_ReturnToKanto());
    EXPECT_EQ(Generations_GetCurrentRegion(), REGION_KANTO);
    for (u32 i = 0; i < 8; i++)
        EXPECT(FlagGet(FLAG_BADGE01_GET + i));
    FillPC(); // A revisit has no deposits, so a full PC is harmless.
    struct PokemonStorage *pc = Alloc(sizeof(*pc));
    EXPECT(pc != NULL);
    *pc = *gPokemonStoragePtr;
    EXPECT_EQ(Generations_EnterJohto(SPECIES_CHIKORITA), GENERATIONS_TRAVEL_OK);
    EXPECT_EQ(memcmp(pc, gPokemonStoragePtr, sizeof(*pc)), 0);
    EXPECT_EQ(memcmp(s->party, gParties[B_TRAINER_PLAYER], sizeof(s->party)), 0);
    EXPECT_EQ(Generations_GetRegionState(REGION_KANTO) & REGION_PROGRESS_BADGES, 0xFF);
    EXPECT_EQ(Generations_GetRegionState(REGION_JOHTO) & REGION_PROGRESS_BADGES, 0xF);
    for (u32 i = 0; i < 8; i++)
        EXPECT_EQ(FlagGet(FLAG_BADGE01_GET + i), i < 4);
    Free(pc);
    Free(s);
}

TEST("Generations travel: transferred storage party and progression survive save-block reload")
{
    PrepareTravel(6);
    EXPECT_EQ(Generations_EnterJohto(SPECIES_CHIKORITA), GENERATIONS_TRAVEL_OK);
    SavePlayerParty();
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    memset(gSaveBlock1Ptr, 0, sizeof(*gSaveBlock1Ptr));
    memset(gPokemonStoragePtr, 0, sizeof(*gPokemonStoragePtr));
    ZeroPlayerPartyMons();
    gPartiesCount[B_TRAINER_PLAYER] = 0;
    *gSaveBlock1Ptr = s->save;
    *gPokemonStoragePtr = s->storage;
    LoadPlayerParty();
    EXPECT_EQ(Generations_GetCurrentRegion(), REGION_JOHTO);
    EXPECT(Generations_GetRegionState(REGION_JOHTO) & REGION_PROGRESS_STARTER);
    EXPECT_EQ(gSaveBlock1Ptr->lastHealLocation.mapGroup, MAP_GROUP(MAP_NEW_BARK_TOWN));
    EXPECT_EQ(gSaveBlock1Ptr->lastHealLocation.mapNum, MAP_NUM(MAP_NEW_BARK_TOWN));
    ExpectUnchanged(s);
}

TEST("Generations travel: shortcut is gated by the test ROM build flag")
{
    PrepareTravel(1);
    Generations_InitNewGame();
    RUN_OVERWORLD_SCRIPT(special Generations_TestProgress;);
#if GENERATIONS_TEST_ROM
    EXPECT(gSpecialVar_Result);
    EXPECT(Generations_IsRegionUnlocked(REGION_JOHTO));
    EXPECT_EQ(Generations_EnterJohto(SPECIES_CYNDAQUIL), GENERATIONS_TRAVEL_OK);
    RUN_OVERWORLD_SCRIPT(special Generations_TestProgress;);
    EXPECT(gSpecialVar_Result);
    EXPECT_EQ(Generations_GetRegionState(REGION_JOHTO) & REGION_PROGRESS_BADGES, 0xF);
    EXPECT(Generations_CanUsePC());
#else
    EXPECT(!gSpecialVar_Result);
    EXPECT(!Generations_IsRegionUnlocked(REGION_JOHTO));
#endif
}

TEST("Generations travel: Johto maps are linked with reciprocal routes and safe interiors")
{
    const struct MapHeader *town = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_NEW_BARK_TOWN), MAP_NUM(MAP_NEW_BARK_TOWN));
    const struct MapHeader *route = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_JOHTO_ROUTE29), MAP_NUM(MAP_JOHTO_ROUTE29));
    const struct MapHeader *lab = Overworld_GetMapHeaderByGroupAndId(MAP_GROUP(MAP_NEW_BARK_TOWN_ELMS_LAB), MAP_NUM(MAP_NEW_BARK_TOWN_ELMS_LAB));
    EXPECT_EQ(town->regionMapSectionId, MAPSEC_NEW_BARK_TOWN);
    EXPECT_EQ(route->regionMapSectionId, MAPSEC_JOHTO_ROUTE_29);
    EXPECT_EQ(town->mapLayout->width, 32);
    EXPECT_EQ(town->mapLayout->height, 32);
    EXPECT_EQ(route->mapLayout->width, 96);
    EXPECT_EQ(route->mapLayout->height, 32);
    EXPECT(town->mapLayout->secondaryTileset == &gTileset_Johto);
    EXPECT(route->mapLayout->secondaryTileset == &gTileset_Johto);
    EXPECT_EQ(town->events->warpCount, 5);
    EXPECT_EQ(town->events->warps[0].x, 23);
    EXPECT_EQ(town->events->warps[0].y, 12);
    EXPECT_EQ(town->events->warps[1].x, 12);
    EXPECT_EQ(town->events->warps[1].y, 9);
    EXPECT_EQ(town->events->warps[2].x, 7);
    EXPECT_EQ(town->events->warps[2].y, 21);
    EXPECT_EQ(town->events->warps[3].x, 18);
    EXPECT_EQ(town->events->warps[3].y, 23);
    EXPECT_EQ(town->events->warps[4].x, 16);
    EXPECT_EQ(town->events->warps[4].y, 8);
    EXPECT_EQ(town->connections->count, 1);
    EXPECT_EQ(route->connections->count, 1);
    EXPECT_EQ(town->connections->connections[0].direction, CONNECTION_WEST);
    EXPECT_EQ(route->connections->connections[0].direction, CONNECTION_EAST);
    EXPECT_EQ(town->connections->connections[0].offset, 0);
    EXPECT_EQ(route->connections->connections[0].offset, 0);
    EXPECT_EQ(town->connections->connections[0].mapGroup, MAP_GROUP(MAP_JOHTO_ROUTE29));
    EXPECT_EQ(route->connections->connections[0].mapNum, MAP_NUM(MAP_NEW_BARK_TOWN));
    EXPECT_EQ(lab->events->warps[0].mapGroup, MAP_GROUP(MAP_NEW_BARK_TOWN));
    EXPECT_EQ(lab->events->warps[0].mapNum, MAP_NUM(MAP_NEW_BARK_TOWN));
}

TEST("Generations travel: Route 29 has live Johto encounters in the selected game build")
{
    struct WarpData old = gSaveBlock1Ptr->location;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_JOHTO_ROUTE29);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_JOHTO_ROUTE29);
    u32 id = GetCurrentMapWildMonHeaderId();
    EXPECT_NE(id, HEADER_NONE);
    enum TimeOfDay time = GetTimeOfDayForEncounters(id, WILD_AREA_LAND);
    const struct WildPokemonInfo *info = gWildMonHeaders[id].encounterTypes[time].landMonsInfo;
    EXPECT(info != NULL);
    EXPECT(info->encounterRate > 0);
    bool32 sentret = FALSE, hoothoot = FALSE;
    for (u32 i = 0; i < 12; i++)
    {
        const struct WildPokemon *mon = &info->wildPokemon[i];
        EXPECT_EQ(mon->minLevel, 2);
        EXPECT_EQ(mon->maxLevel, 4);
        sentret |= mon->species == SPECIES_SENTRET;
        hoothoot |= mon->species == SPECIES_HOOTHOOT;
    }
    EXPECT(sentret && hoothoot);
    gSaveBlock1Ptr->location = old;
}

TEST("Generations travel: old Johto map saves relocate once without touching Pokemon")
{
    u32 map;
    s16 x, y;
    PARAMETRIZE { map = MAP_NEW_BARK_TOWN; x = 14; y = 14; }
    PARAMETRIZE { map = MAP_JOHTO_ROUTE29; x = 93; y = 14; }
    PARAMETRIZE { map = MAP_NEW_BARK_TOWN_ELMS_LAB; x = 6; y = 10; }
    PARAMETRIZE { map = MAP_NEW_BARK_TOWN_HEALING_HOUSE; x = 6; y = 6; }
    PrepareTravel(6);
    EXPECT_EQ(Generations_EnterJohto(SPECIES_TOTODILE), GENERATIONS_TRAVEL_OK);
    u32 oldState = Generations_GetRegionState(REGION_JOHTO) & ~REGION_PROGRESS_HGSS_MAPS;
    VarSet(VAR_GENERATIONS_KANTO + 1, oldState);
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(map);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(map);
    gSaveBlock1Ptr->pos.x = 12;
    gSaveBlock1Ptr->pos.y = 35; // Old vertical Route 1 geometry.
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    EXPECT(Generations_RelocateLegacyJohtoSave());
    EXPECT_EQ(gSaveBlock1Ptr->pos.x, x);
    EXPECT_EQ(gSaveBlock1Ptr->pos.y, y);
    EXPECT_EQ(gSaveBlock1Ptr->location.mapGroup, MAP_GROUP(map));
    EXPECT_EQ(gSaveBlock1Ptr->location.mapNum, MAP_NUM(map));
    EXPECT_EQ(Generations_GetRegionState(REGION_JOHTO), oldState | REGION_PROGRESS_HGSS_MAPS);
    EXPECT_EQ(memcmp(s->party, gParties[B_TRAINER_PLAYER], sizeof(s->party)), 0);
    EXPECT_EQ(memcmp(&s->storage, gPokemonStoragePtr, sizeof(s->storage)), 0);
    Free(s);
    s = Snapshot();
    EXPECT(s != NULL);
    EXPECT(!Generations_RelocateLegacyJohtoSave());
    ExpectUnchanged(s);
}

TEST("Generations travel: relocation never migrates Kanto or an unknown save schema")
{
    bool32 corrupt;
    PARAMETRIZE { corrupt = FALSE; }
    PARAMETRIZE { corrupt = TRUE; }
    PrepareTravel(1);
    if (corrupt)
    {
        EXPECT_EQ(Generations_EnterJohto(SPECIES_TOTODILE), GENERATIONS_TRAVEL_OK);
        VarSet(VAR_GENERATIONS_SCHEMA, 0xFFFF);
    }
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_JOHTO_ROUTE29);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_JOHTO_ROUTE29);
    struct TravelSnapshot *s = Snapshot();
    EXPECT(s != NULL);
    EXPECT(!Generations_RelocateLegacyJohtoSave());
    ExpectUnchanged(s);
}

TEST("Generations travel: Johto blackout returns to the local house and healer")
{
    PrepareTravel(1);
    EXPECT_EQ(Generations_EnterJohto(SPECIES_CHIKORITA), GENERATIONS_TRAVEL_OK);
    struct WarpData warp;
    SetWhiteoutRespawnWarpAndHealerNPC(&warp);
    EXPECT_EQ(warp.mapGroup, MAP_GROUP(MAP_NEW_BARK_TOWN_HEALING_HOUSE));
    EXPECT_EQ(warp.mapNum, MAP_NUM(MAP_NEW_BARK_TOWN_HEALING_HOUSE));
    EXPECT_EQ(warp.x, 5);
    EXPECT_EQ(warp.y, 5);
    const struct HealLocation *heal = GetHealLocation(HEAL_LOCATION_NEW_BARK_TOWN);
    EXPECT_EQ(heal->x, 23);
    EXPECT_EQ(heal->y, 13);
}
