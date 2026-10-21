#ifndef GUARD_GENERATIONS_H
#define GUARD_GENERATIONS_H

#include "global.h"
#include "constants/regions.h"
#include "battle_gimmick.h"

// SaveBlock1 event variables, reserved exclusively for this project.
#define VAR_GENERATIONS_SCHEMA       VAR_UNUSED_0x40DB
#define VAR_GENERATIONS_MECHANICS    VAR_UNUSED_0x40DC
#define VAR_GENERATIONS_REGION       VAR_UNUSED_0x40E5
#define VAR_GENERATIONS_KANTO        VAR_UNUSED_0x40F7
#define GENERATIONS_SCHEMA           0x4701
#define GENERATIONS_CAMPAIGN_COUNT   9

#define REGION_PROGRESS_BADGES      0x00FF
#define REGION_PROGRESS_LEAGUE      (1 << 8)
#define REGION_PROGRESS_CHAMPION    (1 << 9)
#define REGION_PROGRESS_UNLOCKED    (1 << 10)
#define REGION_PROGRESS_ENTERED     (1 << 11)
#define REGION_PROGRESS_LEGACY_PC    (1 << 12)
#define REGION_PROGRESS_STARTER     (1 << 13)

struct Pokemon;
struct BattlePokemon;

void Generations_InitNewGame(void);
bool32 Generations_IsActive(void);
enum Region Generations_GetCurrentRegion(void);
u32 Generations_GetRegionState(enum Region region);
bool32 Generations_IsRegionUnlocked(enum Region region);
bool32 Generations_GrantBadge(enum Region region, u32 badge);
void Generations_OnFlagSet(u16 flag);
bool32 Generations_RecordLeagueVictory(enum Region region);
bool32 Generations_RecordChampionVictory(enum Region region);
void Generations_RecordStarter(void); // Script special; verifies the party.
u32 Generations_GetLevelCap(enum Region region);
u32 Generations_GetDexLimit(enum Region region);
bool32 Generations_IsWildSpeciesAllowed(enum Region region, enum Species species);
bool32 Generations_IsGiftSpeciesAllowed(enum Region region, enum Species species, bool32 specialEvent);
bool32 Generations_IsPartySpeciesAllowed(enum Species species);
bool32 Generations_CanAccessPreviousRegionPC(enum Region region);
bool32 Generations_UnlockMechanic(enum Gimmick gimmick);
bool32 Generations_IsMechanicUnlocked(enum Gimmick gimmick);

// Isolated demonstration only. Not wired into battle controllers: see STATUS.md.
void Generations_ProjectBattleMon(struct Pokemon *mon, struct BattlePokemon *battleMon, enum Region region);

#endif
