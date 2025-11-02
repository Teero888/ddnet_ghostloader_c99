#ifndef LIB_GHOST_H
#define LIB_GHOST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ghost_skin_t {
  int skin[6];
  int use_custom_color;
  int color_body;
  int color_feet;
  char skin_name[24];
} ghost_skin_t;

typedef struct ghost_character_t {
  int x;
  int y;
  int vel_x;
  int vel_y;
  int angle;
  int direction;
  int weapon;
  int hook_state;
  int hook_x;
  int hook_y;
  int attack_tick;
  int tick;
} ghost_character_t;

typedef struct ghost_path_t {
  int chunk_size;
  int num_items;
  ghost_character_t **chunks;
} ghost_path_t;

typedef struct ghost_t {
  ghost_skin_t skin;
  ghost_path_t path;
  int start_tick;
  char player[16];
  int playback_pos;
  char map[64];
  int time;
} ghost_t;

ghost_character_t *ghost_path_get(ghost_path_t *path, int index);
int load_ghost(ghost_t *ghost, const char *filename);
int save_ghost(ghost_t *ghost, const char *filename);
void free_ghost(ghost_t *ghost);

#ifdef __cplusplus
}
#endif

#endif // LIB_GHOST_H