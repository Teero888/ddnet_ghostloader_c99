#include "ghost_lib.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  printf("Creating a new ghost...\n");

  ghost_t *ghost = ghost_create();
  if (!ghost) {
    printf("Failed to create ghost.\n");
    return 1;
  }

  ghost_set_meta(ghost, "MyPlayer", "MyMap", 12345);
  ghost_set_skin(ghost, "default", 0, 0, 0);

  ghost_character_t snap;
  memset(&snap, 0, sizeof(snap));

  printf("Generating 10 dummy snapshots...\n");
  for (int i = 0; i < 10; i++) {
    snap.tick = 1000 + i;
    snap.x = i * 10 * 32;
    snap.y = i * 5 * 32;
    snap.vel_x = i * 20;
    snap.angle = i * 90;
    ghost_add_snap(ghost, &snap);
  }

  const char *filename = "my_new_ghost.gho";
  printf("Saving ghost to '%s'...\n", filename);
  if (ghost_save(ghost, filename) != 0) {
    printf("Failed to save ghost.\n");
    ghost_free(ghost);
    return 1;
  }

  printf("Ghost saved successfully.\n\n");
  ghost_free(ghost);

  printf("--- Verification ---\n");
  printf("Reloading '%s' to verify...\n", filename);
  ghost_t *reloaded_ghost = ghost_load(filename);
  if (!reloaded_ghost) {
    printf("Failed to reload ghost!\n");
    return 1;
  }

  printf("Reloaded successfully.\n");
  printf("Player: %s (Expected: MyPlayer)\n", reloaded_ghost->player);
  printf("Map: %s (Expected: MyMap)\n", reloaded_ghost->map);
  printf("Time: %d (Expected: 12345)\n", reloaded_ghost->time);
  printf("Ticks: %d (Expected: 10)\n", reloaded_ghost->path.num_items);
  printf("Start Tick: %d (Expected: 1000)\n", reloaded_ghost->start_tick);
  printf("Skin: %s (Expected: default)\n", reloaded_ghost->skin.skin_name);

  ghost_character_t *last_snap = ghost_get_snap(&reloaded_ghost->path, 9);
  printf("Last snap X: %d (Expected: %d)\n", last_snap->x, 9 * 10 * 32);

  ghost_free(reloaded_ghost);
  return 0;
}
