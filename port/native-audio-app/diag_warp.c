/* Diagnostic replays only (make WARP_TO=<mapHeaderId>,<x>,<z>[,<dir>]): once the field has run for
 * 90 frames with no task active, queue the game's own map-change task to that map/tile, so any
 * map can be tested without walking there. Hooked on FieldTask_Run, which field_system.c calls
 * every field frame from another object (so the link-time --wrap binds). Never part of a
 * release link; scripts/platinum.sh links without it. Map ids: the MAP_HEADER_* enum in the
 * decompilation (Oreburgh City = 45, Oreburgh Mine B1F = 198).
 * With UNDERGROUND=1 as well, once the warp is done and the field is idle for another 60 frames, start the
 * Underground entry exactly as the Explorer Kit does (UseExplorerKitInField), so the scenario does not depend on
 * where the kit sits in the save's bag. The comms/save prompts still need A presses from the input script.
 * With GIVE_ITEM=<itemId>:<count> as well, the item is added to the bag when the warp starts (e.g. 93:1, a Heart
 * Scale for the Pastoria move relearner). TEACH_MOVE=<partySlot>:<moveId> puts a move in that Pokemon's first move
 * slot and ALL_BADGES=1 grants the eight badges, e.g. to test Fly (move 19) from the party menu. */
#include <nitro.h>
#include <stdlib.h>
#include <stdio.h>
#include "field_task.h"
#if defined(GIVE_ITEM) || defined(TEACH_MOVE) || defined(ALL_BADGES)
#include "bag.h"
#include "field/field_system.h"
#include "party.h"
#include "pokemon.h"
#include "save_player.h"
#include "savedata.h"
#include "trainer_info.h"
#endif
#ifdef WARP_UNDERGROUND
#include "field/field_system.h"
#include "map_object.h"
#endif

extern void PSPNativeMemLog(const char *fmt, ...);
extern BOOL __real_FieldTask_Run(FieldSystem *fieldSystem);
extern void FieldTask_StartMapChangeFull(FieldTask *task, int mapHeaderID, int warpId, int x, int z, int dir);
#ifdef WARP_UNDERGROUND
/* Declared here rather than via field_map_change.h, whose FieldTask_StartMapChangeFull takes an enum. */
extern struct MapChangeUndergroundContext *MapChangeUndergroundContext_New(FieldSystem *fieldSystem);
extern BOOL FieldTask_MapChangeToUnderground(FieldTask *task);
#endif

/* Tick 1 queues the map change as a child task and yields; tick 2 (after the change finished)
 * ends the task. Same shape as the script engine's Warp command. */
static BOOL DiagWarpTask(FieldTask *task)
{
    static int state;
    if (state == 0) {
        int map, x, z, dir = 1; const char *s = WARP_TO;
        map = (int)strtol(s, (char **)&s, 10); if (*s == ',') s++;
        x = (int)strtol(s, (char **)&s, 10);   if (*s == ',') s++;
        z = (int)strtol(s, (char **)&s, 10);   if (*s == ',') { s++; dir = (int)strtol(s, (char **)&s, 10); }
        PSPNativeMemLog("[DIAG] warp to map %d (%d,%d) dir %d", map, x, z, dir);
        printf("[DIAG] warp to map %d (%d,%d) dir %d\n", map, x, z, dir);
#ifdef GIVE_ITEM
        {
            const char *g = GIVE_ITEM;
            int item = (int)strtol(g, (char **)&g, 10), count = 1;
            if (*g == ':') { g++; count = (int)strtol(g, (char **)&g, 10); }
            BOOL added = Bag_TryAddItem(SaveData_GetBag(FieldTask_GetFieldSystem(task)->saveData), item, count, HEAP_ID_FIELD2);
            printf("[DIAG] give item %d x%d: %s\n", item, count, added ? "added" : "bag full");
        }
#endif
#ifdef TEACH_MOVE
        {
            const char *t = TEACH_MOVE;
            int slot = (int)strtol(t, (char **)&t, 10), move = 0;
            if (*t == ':') { t++; move = (int)strtol(t, (char **)&t, 10); }
            Pokemon_ResetMoveSlot(Party_GetPokemonBySlotIndex(SaveData_GetParty(FieldTask_GetFieldSystem(task)->saveData), slot), move, 0);
            printf("[DIAG] teach move %d to party slot %d\n", move, slot);
        }
#endif
#ifdef ALL_BADGES
        for (int badge = 0; badge < 8; badge++) {
            TrainerInfo_SetBadge(SaveData_GetTrainerInfo(FieldTask_GetFieldSystem(task)->saveData), badge);
        }
        printf("[DIAG] all badges\n");
#endif
        state = 1;
        FieldTask_StartMapChangeFull(task, map, -1, x, z, dir);
        return FALSE;
    }
    printf("[DIAG] warp done\n");
    return TRUE;
}

BOOL __wrap_FieldTask_Run(FieldSystem *fieldSystem)
{
    static int frames, done;
    if (!done && !FieldSystem_IsRunningTask(fieldSystem) && ++frames == 90) {
        done = 1;
        FieldSystem_CreateTask(fieldSystem, DiagWarpTask, NULL);
    }
#ifdef WARP_UNDERGROUND
    static int idle, entered;
    if (done && !entered && !FieldSystem_IsRunningTask(fieldSystem) && ++idle == 60) {
        entered = 1;
        printf("[DIAG] entering the Underground\n");
        struct MapChangeUndergroundContext *ctx = MapChangeUndergroundContext_New(fieldSystem);
        MapObjectMan_PauseAllMovement(fieldSystem->mapObjMan);
        FieldSystem_CreateTask(fieldSystem, FieldTask_MapChangeToUnderground, ctx);
    }
#endif
    return __real_FieldTask_Run(fieldSystem);
}
