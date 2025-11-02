# DDNet Ghost C

This is a minimalistic C99-compatible ghost file library, inspired by the ghost file loading and saving code from [DDNet](https://github.com/ddnet/ddnet).

## Features

* Loads ghost files.
* Creates and saves new ghost files.
* Simple, heap-based API (`create`, `load`, `free`).
* Helper functions for setting metadata.
* Dynamic snapshot adding.
* The only dependency is libc.

## API Overview

```c
// Loads a ghost from a file. Returns NULL on failure.
ghost_t *ghost_load(const char *filename);

// Creates a new, empty ghost struct.
ghost_t *ghost_create(void);

// Frees all memory associated with a ghost.
void ghost_free(ghost_t *ghost);

// Saves a ghost to a file. Returns 0 on success.
int ghost_save(const ghost_t *ghost, const char *filename);

// Helper to set player name, map name, and finish time.
void ghost_set_meta(ghost_t *ghost, const char *player, const char *map, int time_ms);

// Helper to set skin info.
void ghost_set_skin(ghost_t *ghost, const char *skin_name,
                    int use_custom_color, int color_body, int color_feet);

// Adds a snapshot to the ghost's path. Handles memory allocation.
void ghost_add_snap(ghost_t *ghost, const ghost_character_t *snap);

// Gets a pointer to a specific snapshot from the path.
ghost_character_t *ghost_get_snap(const ghost_path_t *path, int index);
````

## Usage

To use the ghost library in your project, simply include `ghost_lib.h` and compile `ghost_lib.c` along with your project.

### Example: Load Ghost

(See `example_load.c`)

```c
#include "ghost_lib.h"
#include <stdio.h>

int main(void) {
  const char *filename = "run_dead_silence.gho";
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
  printf("Num Snaps:  %d\n", ghost->path.num_items);
  printf("\n");

  printf("--- Path Sample ---\n");
  ghost_character_t *snap = ghost_get_snap(&ghost->path, 0);
  if (snap) {
    printf("First snap (Tick %d):\n", snap->tick);
    printf("\tPos:   (%d, %d)\n", snap->x, snap->y);
  }

  ghost_free(ghost);
  return 0;
}
```

### Example: Create and Save Ghost

(See `example_save.c`)

```c
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

  // Set metadata
  ghost_set_meta(ghost, "MyPlayer", "MyMap", 12345);
  ghost_set_skin(ghost, "default", 0, 0, 0);

  // Add some dummy data
  ghost_character_t snap;
  memset(&snap, 0, sizeof(snap));

  printf("Generating 10 dummy snapshots...\n");
  for (int i = 0; i < 10; i++) {
    snap.tick = 1000 + i; // Set tick for auto start_tick detection
    snap.x = i * 100;
    ghost_add_snap(ghost, &snap);
  }

  // Save the file
  const char *filename = "my_new_ghost.gho";
  printf("Saving ghost to '%s'...\n", filename);
  if (ghost_save(ghost, filename) != 0) {
    printf("Failed to save ghost.\n");
    ghost_free(ghost);
    return 1;
  }

  printf("Ghost saved successfully.\n");
  ghost_free(ghost);
  return 0;
}
```