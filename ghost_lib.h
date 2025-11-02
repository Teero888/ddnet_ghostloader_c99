#ifndef GHOST_LIB_H
#define GHOST_LIB_H

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

ghost_t *ghost_load(const char *filename);
ghost_t *ghost_create(void);
void ghost_free(ghost_t *ghost);
int ghost_save(const ghost_t *ghost, const char *filename);
void ghost_set_meta(ghost_t *ghost, const char *player, const char *map,
                    int time_ms);
void ghost_set_skin(ghost_t *ghost, const char *skin_name, int use_custom_color,
                    int color_body, int color_feet);
void ghost_add_snap(ghost_t *ghost, const ghost_character_t *snap);
ghost_character_t *ghost_get_snap(const ghost_path_t *path, int index);

#ifdef __cplusplus
}
#endif

#endif // GHOST_LIB_H
