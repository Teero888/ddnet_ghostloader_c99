#include <ddnet_ghost/ghost.h>
#include <stdio.h>
#include <string.h>

int compare_characters(ghost_character_t *char1, ghost_character_t *char2,
                       int i) {
  if (char1->x != char2->x) {
    printf("Snap %d: x mismatch (%d != %d)\n", i, char1->x, char2->x);
    return 1;
  }
  if (char1->y != char2->y) {
    printf("Snap %d: y mismatch (%d != %d)\n", i, char1->y, char2->y);
    return 1;
  }
  if (char1->vel_x != char2->vel_x) {
    printf("Snap %d: vel_x mismatch (%d != %d)\n", i, char1->vel_x,
           char2->vel_x);
    return 1;
  }
  if (char1->vel_y != char2->vel_y) {
    printf("Snap %d: vel_y mismatch (%d != %d)\n", i, char1->vel_y,
           char2->vel_y);
    return 1;
  }
  if (char1->angle != char2->angle) {
    printf("Snap %d: angle mismatch (%d != %d)\n", i, char1->angle,
           char2->angle);
    return 1;
  }
  if (char1->direction != char2->direction) {
    printf("Snap %d: direction mismatch (%d != %d)\n", i, char1->direction,
           char2->direction);
    return 1;
  }
  if (char1->weapon != char2->weapon) {
    printf("Snap %d: weapon mismatch (%d != %d)\n", i, char1->weapon,
           char2->weapon);
    return 1;
  }
  if (char1->hook_state != char2->hook_state) {
    printf("Snap %d: hook_state mismatch (%d != %d)\n", i, char1->hook_state,
           char2->hook_state);
    return 1;
  }
  if (char1->hook_x != char2->hook_x) {
    printf("Snap %d: hook_x mismatch (%d != %d)\n", i, char1->hook_x,
           char2->hook_x);
    return 1;
  }
  if (char1->hook_y != char2->hook_y) {
    printf("Snap %d: hook_y mismatch (%d != %d)\n", i, char1->hook_y,
           char2->hook_y);
    return 1;
  }
  if (char1->attack_tick != char2->attack_tick) {
    printf("Snap %d: attack_tick mismatch (%d != %d)\n", i, char1->attack_tick,
           char2->attack_tick);
    return 1;
  }
  if (char1->tick != char2->tick) {
    printf("Snap %d: tick mismatch (%d != %d)\n", i, char1->tick, char2->tick);
    return 1;
  }
  return 0;
}

int main(void) {

  ghost_t *ghost = ghost_load("run_dead_silence.gho");
  if (!ghost) {
    printf("Ghost file could not be loaded\n");
    return 1;
  }

  printf("Loaded original ghost '%s'\n"
         "\tPlayer: %s\n"
         "\tMap: %s\n"
         "\tSkin: %s\n"
         "\tTime: %.3f\n"
         "\tTicks: %d\n",
         "run_dead_silence.gho", ghost->player, ghost->map,
         ghost->skin.skin_name, ghost->time / 1000.f, ghost->path.num_items);

  if (ghost_save(ghost, "written_ghost.gho")) {
    printf("Ghost file could not be written back\n");
    ghost_free(ghost);
    return 1;
  }
  printf("Ghost file written back successfully to 'written_ghost.gho'\n");

  ghost_t *ghost2 = ghost_load("written_ghost.gho");
  if (!ghost2) {
    printf("Written ghost file could not be loaded\n");
    ghost_free(ghost);
    return 1;
  }

  printf("Loaded written ghost for verification...\n");

  int mismatches = 0;

  if (strcmp(ghost->player, ghost2->player) != 0) {
    printf("MISMATCH: player ('%s' != '%s')\n", ghost->player, ghost2->player);
    mismatches++;
  }
  if (strcmp(ghost->map, ghost2->map) != 0) {
    printf("MISMATCH: map ('%s' != '%s')\n", ghost->map, ghost2->map);
    mismatches++;
  }
  if (ghost->time != ghost2->time) {
    printf("MISMATCH: time (%d != %d)\n", ghost->time, ghost2->time);
    mismatches++;
  }
  if (ghost->start_tick != ghost2->start_tick) {
    printf("MISMATCH: start_tick (%d != %d)\n", ghost->start_tick,
           ghost2->start_tick);
    mismatches++;
  }

  if (memcmp(&ghost->skin, &ghost2->skin, sizeof(ghost_skin_t)) != 0) {
    printf("MISMATCH: skin data does not match\n");
    if (strcmp(ghost->skin.skin_name, ghost2->skin.skin_name) != 0) {
      printf("\tSkin name: '%s' != '%s'\n", ghost->skin.skin_name,
             ghost2->skin.skin_name);
    }
    mismatches++;
  }

  if (ghost->path.num_items != ghost2->path.num_items) {
    printf("MISMATCH: path.num_items (%d != %d)\n", ghost->path.num_items,
           ghost2->path.num_items);
    mismatches++;
  } else {
    printf("Comparing %d snapshots...\n", ghost->path.num_items);
    for (int i = 0; i < ghost->path.num_items; i++) {
      ghost_character_t *char1 = ghost_get_snap(&ghost->path, i);
      ghost_character_t *char2 = ghost_get_snap(&ghost2->path, i);
      if (compare_characters(char1, char2, i) != 0) {
        mismatches++;
        printf("...Stopping comparison after first snapshot mismatch.\n");
        break;
      }
    }
  }

  printf("----------------------------------------\n");
  if (mismatches == 0) {
    printf("SUCCESS: Written ghost is identical to the original.\n");
  } else {
    printf("FAILURE: Found %d mismatch(es) between original and written "
           "ghost->\n",
           mismatches);
  }
  printf("----------------------------------------\n");

  ghost_free(ghost);
  ghost_free(ghost2);

  return mismatches;
}
