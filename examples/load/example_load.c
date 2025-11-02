#include <ddnet_ghost/ghost.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    printf("No ghost file specified\n");
    return 1;
  }
  const char *filename = argv[1];

  printf("Attempting to load ghost file: %s\n", filename);

  ghost_t *ghost = ghost_load(filename);
  if (!ghost) {
    printf("Ghost file could not be loaded\n");
    return 1;
  }

  printf("Ghost loaded successfully!\n\n");

  printf("--- Ghost Info ---\n");
  printf("Player:     %s\n", ghost->player);
  printf("Map:        %s\n", ghost->map);
  printf("Time:       %.3fs\n", ghost->time / 1000.0f);
  printf("Start Tick: %d\n", ghost->start_tick);
  printf("Num Snaps:  %d\n", ghost->path.num_items);
  printf("\n");

  printf("--- Skin Info ---\n");
  printf("Skin Name:  %s\n", ghost->skin.skin_name);
  printf("Custom Color: %s\n", ghost->skin.use_custom_color ? "Yes" : "No");
  printf("Color Body: %d\n", ghost->skin.color_body);
  printf("Color Feet: %d\n", ghost->skin.color_feet);
  printf("\n");

  printf("--- Path Sample (every 50th index) ---\n");
  for (int i = 0; i < ghost->path.num_items; i += 50) {
    ghost_character_t *snap = ghost_get_snap(&ghost->path, i);
    if (!snap)
      continue;
    printf("Snap %d (Tick %d):\n", i, snap->tick);
    printf("\tPos:   (%d, %d)\n", snap->x, snap->y);
    printf("\tVel:   (%d, %d)\n", snap->vel_x, snap->vel_y);
    printf("\tAngle: %d\n", snap->angle);
  }

  ghost_free(ghost);
  return 0;
}
