/* Diagnostic replays only (make DOUBLE_SING=1): once the field has run for 90 frames with no task active, start the
 * double battle against Beth & Bob (trainer 533, a real double-battle trainer) and replace every team with Lv30
 * Clefairy that know only Sing, so both the player and the AI use Sing in a double battle. Hooked on FieldTask_Run
 * (like diag_warp.c, so not combinable with WARP_TO) and Trainer_Encounter (to swap the parties after the game has
 * built them). Never part of a release link. */
#include <nitro.h>
#include <stdio.h>
#include "field_task.h"
#include "field_battle_data_transfer.h"
#include "encounter.h"
#include "heap.h"
#include "party.h"
#include "pokemon.h"

#define DIAG_TRAINER 533   /* TRAINER_BELLE_AND_PA_BETH_AND_BOB */
#define DIAG_SPECIES 35    /* SPECIES_CLEFAIRY */
#define DIAG_MOVE 47       /* MOVE_SING */

extern BOOL __real_FieldTask_Run(FieldSystem *fieldSystem);
extern void __real_Trainer_Encounter(FieldBattleDTO *dto, const SaveData *saveData, enum HeapID heapID);

static void FillWithSingers(Party *party, int count, enum HeapID heapID)
{
    Pokemon *mon = Pokemon_New(heapID);
    Party_InitWithCapacity(party, MAX_PARTY_SIZE);
    for (int i = 0; i < count; i++) {
        Pokemon_InitWith(mon, DIAG_SPECIES, 30, 20, FALSE, 0, OTID_NOT_SHINY, 0);
        Pokemon_ResetMoveSlot(mon, DIAG_MOVE, 0);
        for (int slot = 1; slot < 4; slot++) {
            Pokemon_ResetMoveSlot(mon, 0, slot);
        }
        Party_AddPokemon(party, mon);
    }
    Heap_Free(mon);
}

void __wrap_Trainer_Encounter(FieldBattleDTO *dto, const SaveData *saveData, enum HeapID heapID)
{
    __real_Trainer_Encounter(dto, saveData, heapID);
    FillWithSingers(dto->parties[BATTLER_PLAYER_1], 2, heapID);
    FillWithSingers(dto->parties[BATTLER_ENEMY_1], 2, heapID);
    FillWithSingers(dto->parties[BATTLER_ENEMY_2], 2, heapID);
    printf("[DIAG] double battle vs trainer %d, battle type 0x%x, all Clefairy with Sing\n", DIAG_TRAINER, (unsigned)dto->battleType);
}

/* Tick 1 starts the encounter as a child task; tick 2 (after the battle) ends. */
static BOOL DiagDoubleTask(FieldTask *task)
{
    static int state;
    if (state++ == 0) {
        Encounter_NewVsTrainer(task, DIAG_TRAINER, DIAG_TRAINER, 0, HEAP_ID_FIELD2, NULL);
        return FALSE;
    }
    printf("[DIAG] double battle over\n");
    return TRUE;
}

BOOL __wrap_FieldTask_Run(FieldSystem *fieldSystem)
{
    static int frames, done;
    if (!done && !FieldSystem_IsRunningTask(fieldSystem) && ++frames == 90) {
        done = 1;
        FieldSystem_CreateTask(fieldSystem, DiagDoubleTask, NULL);
    }
    return __real_FieldTask_Run(fieldSystem);
}
