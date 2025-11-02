#include "ghost_lib.h"
#include <stdio.h>
#include <string.h> // Needed for memcmp and strcmp

// Helper function to compare two character snapshots
int compare_characters(SGhostCharacter *pChar1, SGhostCharacter *pChar2,
                       int i) {
  if (pChar1->m_X != pChar2->m_X) {
    printf("Snap %d: m_X mismatch (%d != %d)\n", i, pChar1->m_X, pChar2->m_X);
    return 1;
  }
  if (pChar1->m_Y != pChar2->m_Y) {
    printf("Snap %d: m_Y mismatch (%d != %d)\n", i, pChar1->m_Y, pChar2->m_Y);
    return 1;
  }
  if (pChar1->m_VelX != pChar2->m_VelX) {
    printf("Snap %d: m_VelX mismatch (%d != %d)\n", i, pChar1->m_VelX,
           pChar2->m_VelX);
    return 1;
  }
  if (pChar1->m_VelY != pChar2->m_VelY) {
    printf("Snap %d: m_VelY mismatch (%d != %d)\n", i, pChar1->m_VelY,
           pChar2->m_VelY);
    return 1;
  }
  if (pChar1->m_Angle != pChar2->m_Angle) {
    printf("Snap %d: m_Angle mismatch (%d != %d)\n", i, pChar1->m_Angle,
           pChar2->m_Angle);
    return 1;
  }
  if (pChar1->m_Direction != pChar2->m_Direction) {
    printf("Snap %d: m_Direction mismatch (%d != %d)\n", i, pChar1->m_Direction,
           pChar2->m_Direction);
    return 1;
  }
  if (pChar1->m_Weapon != pChar2->m_Weapon) {
    printf("Snap %d: m_Weapon mismatch (%d != %d)\n", i, pChar1->m_Weapon,
           pChar2->m_Weapon);
    return 1;
  }
  if (pChar1->m_HookState != pChar2->m_HookState) {
    printf("Snap %d: m_HookState mismatch (%d != %d)\n", i, pChar1->m_HookState,
           pChar2->m_HookState);
    return 1;
  }
  if (pChar1->m_HookX != pChar2->m_HookX) {
    printf("Snap %d: m_HookX mismatch (%d != %d)\n", i, pChar1->m_HookX,
           pChar2->m_HookX);
    return 1;
  }
  if (pChar1->m_HookY != pChar2->m_HookY) {
    printf("Snap %d: m_HookY mismatch (%d != %d)\n", i, pChar1->m_HookY,
           pChar2->m_HookY);
    return 1;
  }
  if (pChar1->m_AttackTick != pChar2->m_AttackTick) {
    printf("Snap %d: m_AttackTick mismatch (%d != %d)\n", i,
           pChar1->m_AttackTick, pChar2->m_AttackTick);
    return 1;
  }
  if (pChar1->m_Tick != pChar2->m_Tick) {
    printf("Snap %d: m_Tick mismatch (%d != %d)\n", i, pChar1->m_Tick,
           pChar2->m_Tick);
    return 1;
  }
  return 0; // Match
}

int main(void) {

  // Load the original ghost
  SGhost Ghost;
  if (load_ghost(&Ghost, "run_dead_silence.gho")) {
    printf("Ghost file could not be loaded\n");
    return 1;
  }

  printf("Loaded original ghost '%s'\n"
         "\tPlayer: %s\n"
         "\tMap: %s\n"
         "\tSkin: %s\n"
         "\tTime: %.3f\n"
         "\tTicks: %d\n",
         "run_dead_silence.gho", Ghost.m_aPlayer, Ghost.m_aMap,
         Ghost.m_Skin.m_aSkinName, Ghost.m_Time / 1000.f,
         Ghost.m_Path.m_NumItems);

  // Write it right back out to verify write functionality
  if (save_ghost(&Ghost, "written_ghost.gho")) {
    printf("Ghost file could not be written back\n");
    free_ghost(&Ghost);
    return 1;
  }
  printf("Ghost file written back successfully to 'written_ghost.gho'\n");

  // Read back the written ghost to verify it works
  SGhost Ghost2 = {}; // Initialize to zero
  if (load_ghost(&Ghost2, "written_ghost.gho")) {
    printf("Written ghost file could not be loaded\n");
    free_ghost(&Ghost);
    return 1;
  }

  printf("Loaded written ghost for verification...\n");

  // --- Verification ---
  int Mismatches = 0;

  // Compare metadata
  if (strcmp(Ghost.m_aPlayer, Ghost2.m_aPlayer) != 0) {
    printf("MISMATCH: m_aPlayer ('%s' != '%s')\n", Ghost.m_aPlayer,
           Ghost2.m_aPlayer);
    Mismatches++;
  }
  if (strcmp(Ghost.m_aMap, Ghost2.m_aMap) != 0) {
    printf("MISMATCH: m_aMap ('%s' != '%s')\n", Ghost.m_aMap, Ghost2.m_aMap);
    Mismatches++;
  }
  if (Ghost.m_Time != Ghost2.m_Time) {
    printf("MISMATCH: m_Time (%d != %d)\n", Ghost.m_Time, Ghost2.m_Time);
    Mismatches++;
  }
  if (Ghost.m_StartTick != Ghost2.m_StartTick) {
    printf("MISMATCH: m_StartTick (%d != %d)\n", Ghost.m_StartTick,
           Ghost2.m_StartTick);
    Mismatches++;
  }

  // Compare skin (using memcmp for the whole struct)
  if (memcmp(&Ghost.m_Skin, &Ghost2.m_Skin, sizeof(SGhostSkin)) != 0) {
    printf("MISMATCH: m_Skin data does not match\n");
    // Also check string name just in case
    if (strcmp(Ghost.m_Skin.m_aSkinName, Ghost2.m_Skin.m_aSkinName) != 0) {
      printf("\tSkin name: '%s' != '%s'\n", Ghost.m_Skin.m_aSkinName,
             Ghost2.m_Skin.m_aSkinName);
    }
    Mismatches++;
  }

  // Compare path data
  if (Ghost.m_Path.m_NumItems != Ghost2.m_Path.m_NumItems) {
    printf("MISMATCH: m_Path.m_NumItems (%d != %d)\n", Ghost.m_Path.m_NumItems,
           Ghost2.m_Path.m_NumItems);
    Mismatches++;
  } else {
    // Only compare snaps if the count is the same
    printf("Comparing %d snapshots...\n", Ghost.m_Path.m_NumItems);
    for (int i = 0; i < Ghost.m_Path.m_NumItems; i++) {
      SGhostCharacter *pChar1 = ghost_path_get(&Ghost.m_Path, i);
      SGhostCharacter *pChar2 = ghost_path_get(&Ghost2.m_Path, i);
      if (compare_characters(pChar1, pChar2, i) != 0) {
        Mismatches++;
        printf("...Stopping comparison after first snapshot mismatch.\n");
        break; // Stop after the first error to avoid spam
      }
    }
  }

  // --- Final Result ---
  printf("----------------------------------------\n");
  if (Mismatches == 0) {
    printf("✅ SUCCESS: Written ghost is identical to the original.\n");
  } else {
    printf("❌ FAILURE: Found %d mismatch(es) between original and written "
           "ghost.\n",
           Mismatches);
  }
  printf("----------------------------------------\n");

  // Free both ghosts
  free_ghost(&Ghost);
  free_ghost(&Ghost2);

  return Mismatches; // Return 0 on success, non-zero on failure
}