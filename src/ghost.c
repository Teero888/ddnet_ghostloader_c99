#include <ddnet_ghost/ghost.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HUFFMAN_EOF_SYMBOL 256
#define HUFFMAN_MAX_SYMBOLS (HUFFMAN_EOF_SYMBOL + 1)
#define HUFFMAN_MAX_NODES (HUFFMAN_MAX_SYMBOLS * 2 - 1)
#define HUFFMAN_LUTBITS 10
#define HUFFMAN_LUTSIZE (1 << HUFFMAN_LUTBITS)
#define HUFFMAN_LUTMASK (HUFFMAN_LUTSIZE - 1)

typedef struct huffman_node_t {
  unsigned bits;
  unsigned num_bits;
  unsigned short leafs[2];
  unsigned char symbol;
} huffman_node_t;

typedef struct huffman_context_t {
  huffman_node_t nodes[HUFFMAN_MAX_NODES];
  huffman_node_t *decode_lut[HUFFMAN_LUTSIZE];
  huffman_node_t *start_node;
  int num_nodes;
} huffman_context_t;

static const unsigned huffman_freq_table[HUFFMAN_MAX_SYMBOLS] = {
    1 << 30, 4545, 2657, 431, 1950, 919,  444, 482, 2244, 617, 838, 542,  715,
    1814,    304,  240,  754, 212,  647,  186, 283, 131,  146, 166, 543,  164,
    167,     136,  179,  859, 363,  113,  157, 154, 204,  108, 137, 180,  202,
    176,     872,  404,  168, 134,  151,  111, 113, 109,  120, 126, 129,  100,
    41,      20,   16,   22,  18,   18,   17,  19,  16,   37,  13,  21,   362,
    166,     99,   78,   95,  88,   81,   70,  83,  284,  91,  187, 77,   68,
    52,      68,   59,   66,  61,   638,  71,  157, 50,   46,  69,  43,   11,
    24,      13,   19,   10,  12,   12,   20,  14,  9,    20,  20,  10,   10,
    15,      15,   12,   12,  7,    19,   15,  14,  13,   18,  35,  19,   17,
    14,      8,    5,    15,  17,   9,    15,  14,  18,   8,   10,  2173, 134,
    157,     68,   188,  60,  170,  60,   194, 62,  175,  71,  148, 67,   167,
    78,      211,  67,   156, 69,   1674, 90,  174, 53,   147, 89,  181,  51,
    174,     63,   163,  80,  167,  94,   128, 122, 223,  153, 218, 77,   200,
    110,     190,  73,   174, 69,   145,  66,  277, 143,  141, 60,  136,  53,
    180,     57,   142,  57,  158,  61,   166, 112, 152,  92,  26,  22,   21,
    28,      20,   26,   30,  21,   32,   27,  20,  17,   23,  21,  30,   22,
    22,      21,   27,   25,  17,   27,   23,  18,  39,   26,  15,  21,   12,
    18,      18,   27,   20,  18,   15,   19,  11,  17,   33,  12,  18,   15,
    19,      18,   16,   26,  17,   18,   9,   10,  25,   22,  22,  17,   20,
    16,      6,    16,   15,  20,   14,   18,  24,  335,  1517};

typedef struct construct_node_t {
  unsigned short node_id;
  int frequency;
} construct_node_t;

static int compare_nodes(const void *a, const void *b) {
  const construct_node_t *node_a = *(const construct_node_t **)a;
  const construct_node_t *node_b = *(const construct_node_t **)b;
  return node_b->frequency - node_a->frequency;
}

static void set_bits_recursive(huffman_node_t *nodes, huffman_node_t *node,
                               int bits, unsigned depth) {
  if (node->leafs[1] != 0xffff)
    set_bits_recursive(nodes, &nodes[node->leafs[1]], bits | (1 << depth),
                       depth + 1);
  if (node->leafs[0] != 0xffff)
    set_bits_recursive(nodes, &nodes[node->leafs[0]], bits, depth + 1);

  if (node->num_bits) {
    node->bits = bits;
    node->num_bits = depth;
  }
}

static void construct_tree(huffman_context_t *ctx,
                           const unsigned *frequencies) {
  construct_node_t nodes_left_storage[HUFFMAN_MAX_SYMBOLS];
  construct_node_t *nodes_left[HUFFMAN_MAX_SYMBOLS];
  int num_nodes_left = HUFFMAN_MAX_SYMBOLS;

  for (int i = 0; i < HUFFMAN_MAX_SYMBOLS; i++) {
    ctx->nodes[i].num_bits = 0xFFFFFFFF;
    ctx->nodes[i].symbol = i;
    ctx->nodes[i].leafs[0] = 0xffff;
    ctx->nodes[i].leafs[1] = 0xffff;

    nodes_left_storage[i].frequency =
        (i == HUFFMAN_EOF_SYMBOL) ? 1 : frequencies[i];
    nodes_left_storage[i].node_id = i;
    nodes_left[i] = &nodes_left_storage[i];
  }

  ctx->num_nodes = HUFFMAN_MAX_SYMBOLS;

  while (num_nodes_left > 1) {
    qsort(nodes_left, num_nodes_left, sizeof(construct_node_t *),
          compare_nodes);

    huffman_node_t *new_node = &ctx->nodes[ctx->num_nodes];
    new_node->num_bits = 0;
    new_node->leafs[0] = nodes_left[num_nodes_left - 1]->node_id;
    new_node->leafs[1] = nodes_left[num_nodes_left - 2]->node_id;

    nodes_left[num_nodes_left - 2]->node_id = ctx->num_nodes;
    nodes_left[num_nodes_left - 2]->frequency +=
        nodes_left[num_nodes_left - 1]->frequency;

    ctx->num_nodes++;
    num_nodes_left--;
  }

  ctx->start_node = &ctx->nodes[ctx->num_nodes - 1];
  set_bits_recursive(ctx->nodes, ctx->start_node, 0, 0);
}

static void huffman_init(huffman_context_t *ctx) {
  memset(ctx, 0, sizeof(*ctx));
  construct_tree(ctx, huffman_freq_table);

  for (int i = 0; i < HUFFMAN_LUTSIZE; i++) {
    unsigned bits = i;
    huffman_node_t *node = ctx->start_node;

    for (int k = 0; k < HUFFMAN_LUTBITS; k++) {
      node = &ctx->nodes[node->leafs[bits & 1]];
      bits >>= 1;

      if (node->num_bits) {
        ctx->decode_lut[i] = node;
        break;
      }
    }

    if (!ctx->decode_lut[i])
      ctx->decode_lut[i] = node;
  }
}

static int huffman_decompress(const huffman_context_t *ctx, const void *input,
                              int in_size, void *output, int out_size) {
  const unsigned char *src = (const unsigned char *)input;
  const unsigned char *src_end = src + in_size;
  unsigned char *dst = (unsigned char *)output;
  unsigned char *dst_end = dst + out_size;

  unsigned bits = 0;
  unsigned bitcount = 0;
  const huffman_node_t *eof = &ctx->nodes[HUFFMAN_EOF_SYMBOL];

  while (1) {
    const huffman_node_t *node = NULL;

    if (bitcount >= HUFFMAN_LUTBITS)
      node = ctx->decode_lut[bits & HUFFMAN_LUTMASK];

    while (bitcount < 24 && src < src_end) {
      bits |= (*src++) << bitcount;
      bitcount += 8;
    }

    if (!node)
      node = ctx->decode_lut[bits & HUFFMAN_LUTMASK];

    if (!node)
      return -1;

    if (node->num_bits) {
      bits >>= node->num_bits;
      bitcount -= node->num_bits;
    } else {
      bits >>= HUFFMAN_LUTBITS;
      bitcount -= HUFFMAN_LUTBITS;

      while (1) {
        node = &ctx->nodes[node->leafs[bits & 1]];
        bits >>= 1;
        bitcount--;

        if (node->num_bits)
          break;
        if (bitcount == 0)
          return -1;
      }
    }

    if (node == eof)
      break;
    if (dst >= dst_end)
      return -1;
    *dst++ = node->symbol;
  }

  return (int)(dst - (unsigned char *)output);
}

enum {
  MAX_ITEM_SIZE = 128,
  NUM_ITEMS_PER_CHUNK = 50,
  MAX_CHUNK_SIZE = MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK,
  MAX_NAME_LENGTH = 16,
  IO_MAX_PATH_LENGTH = 512,
  SHA256_DIGEST_LENGTH = 256 / 8,
  SHA256_MAXSTRSIZE = 2 * SHA256_DIGEST_LENGTH + 1,
};

struct sha256_digest_t {
  unsigned char data[SHA256_DIGEST_LENGTH];
} typedef sha256_digest_t;

struct ghost_info_t {
  char owner[MAX_NAME_LENGTH];
  char map[64];
  int num_ticks;
  int time;
} typedef ghost_info_t;

struct ghost_header_t {
  unsigned char marker[8];
  unsigned char version;
  char owner[MAX_NAME_LENGTH];
  char map[64];
  unsigned char zeroes[sizeof(int32_t)];
  unsigned char num_ticks[sizeof(int32_t)];
  unsigned char time[sizeof(int32_t)];
  sha256_digest_t map_sha256;
} typedef ghost_header_t;

struct ghost_item_t {
  uint32_t data[MAX_ITEM_SIZE];
  int type;
} typedef ghost_item_t;

enum {
  GHOSTDATA_TYPE_SKIN = 0,
  GHOSTDATA_TYPE_CHARACTER_NO_TICK,
  GHOSTDATA_TYPE_CHARACTER,
  GHOSTDATA_TYPE_START_TICK
};

struct ghost_loader_t {
  void *file;
  char filename[IO_MAX_PATH_LENGTH];

  ghost_header_t header;
  ghost_info_t info;

  unsigned char buffer[MAX_CHUNK_SIZE];
  unsigned char buffer_temp[MAX_CHUNK_SIZE];
  unsigned char *buffer_pos;
  unsigned char *buffer_end;

  int buffer_num_items;
  int buffer_cur_item;
  int buffer_prev_item;
  ghost_item_t last_item;
} typedef ghost_loader_t;

static const unsigned char header_marker[8] = {'T', 'W', 'G', 'H',
                                               'O', 'S', 'T', 0};
static const unsigned char current_version = 6;

static bool mem_has_null(const void *block, size_t size) {
  const unsigned char *bytes = (const unsigned char *)block;
  for (size_t i = 0; i < size; i++) {
    if (bytes[i] == 0) {
      return true;
    }
  }

  return false;
}

static unsigned bytes_be_to_uint(const unsigned char *bytes) {
  return ((bytes[0] & 0xffu) << 24u) | ((bytes[1] & 0xffu) << 16u) |
         ((bytes[2] & 0xffu) << 8u) | (bytes[3] & 0xffu);
}

static int get_ticks(const ghost_header_t *header) {
  return bytes_be_to_uint(header->num_ticks);
}

static int get_time(const ghost_header_t *header) {
  return bytes_be_to_uint(header->time);
}

static bool validate_header(const ghost_header_t *header,
                            const char *filename) {
  if (memcmp(header->marker, header_marker, sizeof(header_marker)) != 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': invalid header marker\n",
        filename);
    return false;
  }

  if (header->version < 4 || header->version > current_version) {
    fprintf(stderr,
            "ghost_loader: Failed to read ghost file '%s': ghost version '%d' "
            "is not supported\n",
            filename, header->version);
    return false;
  }

  if (!mem_has_null(header->owner, sizeof(header->owner))) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': owner name is invalid\n",
        filename);
    return false;
  }

  if (!mem_has_null(header->map, sizeof(header->map))) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': map name is invalid\n",
        filename);
    return false;
  }

  const int num_ticks = get_ticks(header);
  if (num_ticks <= 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': number of ticks '%d' "
        "is invalid\n",
        filename, num_ticks);
    return false;
  }

  const int time = get_time(header);
  if (time <= 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': time '%d' is invalid\n",
        filename, time);
    return false;
  }

  return true;
}

typedef void *io_handle_t;
static io_handle_t read_header(ghost_header_t *header, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr,
            "ghost_loader: Failed to open ghost file '%s' for reading\n",
            filename);
    return NULL;
  }

  if (fread(header, sizeof(*header), 1, file) != 1) {
    fprintf(stderr,
            "ghost_loader: Failed to read ghost file '%s': failed to read "
            "header\n",
            filename);
    fclose(file);
    return NULL;
  }

  if (!validate_header(header, filename)) {
    fclose(file);
    return NULL;
  }

  return file;
}

static int io_seek(io_handle_t io, int64_t offset) {
#if defined(CONF_FAMILY_WINDOWS)
  return _fseeki64((FILE *)io, offset, SEEK_CUR);
#else
  return fseek((FILE *)io, offset, SEEK_CUR);
#endif
}

static ghost_info_t to_ghost_info(ghost_header_t *header) {
  ghost_info_t result;
  strcpy(result.owner, header->owner);
  strcpy(result.map, header->map);
  result.num_ticks = get_ticks(header);
  result.time = get_time(header);
  return result;
}

static void reset_loader_buffer(ghost_loader_t *loader) {
  loader->buffer_pos = loader->buffer;
  loader->buffer_end = loader->buffer;
  loader->buffer_num_items = 0;
  loader->buffer_cur_item = 0;
  loader->buffer_prev_item = -1;
}

static void undiff_item(const uint32_t *past, const uint32_t *diff,
                        uint32_t *out, size_t size) {
  while (size) {
    *out = *past + *diff;
    out++;
    past++;
    diff++;
    size--;
  }
}

static const unsigned char *var_unpack(const unsigned char *src, int *in_out,
                                       int src_size) {
  if (src_size <= 0)
    return NULL;

  const int sign = (*src >> 6) & 1;
  *in_out = *src & 0x3F;
  src_size--;

  static const int var_unpack_masks[] = {0x7F, 0x7F, 0x7F, 0x0F};
  static const int var_unpack_shifts[] = {6, 6 + 7, 6 + 7 + 7, 6 + 7 + 7 + 7};

  for (unsigned i = 0; i < 4; i++) {
    if (!(*src & 0x80))
      break;
    if (src_size <= 0)
      return NULL;
    src_size--;
    src++;
    *in_out |= (*src & var_unpack_masks[i]) << var_unpack_shifts[i];
  }

  src++;
  *in_out ^= -sign;
  return src;
}

static long var_decompress(const void *src_void, int src_size, void *dst_void,
                           int dst_size) {
  if (dst_size % sizeof(int) != 0) {
    return -1;
    fprintf(stderr, "Variable int invalid bounds\n");
  }

  const unsigned char *src = (unsigned char *)src_void;
  const unsigned char *src_end = src + src_size;
  int *dst = (int *)dst_void;
  const int *dst_end = dst + dst_size / sizeof(int);
  while (src < src_end) {
    if (dst >= dst_end)
      return -1;
    src = var_unpack(src, dst, src_end - src);
    if (!src)
      return -1;
    dst++;
  }

  return (long)((unsigned char *)dst - (unsigned char *)dst_void);
}

static bool read_chunk(ghost_loader_t *loader, int *type) {
  if (loader->header.version != 4) {
    loader->last_item.type = -1;
  }

  reset_loader_buffer(loader);

  unsigned char chunk_header[4];
  if (fread(chunk_header, sizeof(chunk_header), 1, loader->file) != 1) {
    return false;
  }

  *type = chunk_header[0];
  int size = (chunk_header[2] << 8) | chunk_header[3];
  loader->buffer_num_items = chunk_header[1];

  if (size <= 0 || size > MAX_CHUNK_SIZE) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': invalid chunk header "
        "size\n",
        loader->filename);
    return false;
  }

  if (fread(loader->buffer, size, 1, loader->file) != 1) {
    fprintf(stderr,
            "ghost_loader: Failed to read ghost file '%s': error reading chunk "
            "data\n",
            loader->filename);
    return false;
  }

  huffman_context_t ctx;
  huffman_init(&ctx);

  size = huffman_decompress(&ctx, loader->buffer, size, loader->buffer_temp,
                            sizeof(loader->buffer_temp));
  if (size < 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': error during network "
        "decompression\n",
        loader->filename);
    return false;
  }

  size = var_decompress(loader->buffer_temp, size, loader->buffer,
                        sizeof(loader->buffer));
  if (size < 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': error during intpack "
        "decompression\n",
        loader->filename);
    return false;
  }

  loader->buffer_end = loader->buffer + size;
  return true;
}

static bool read_next_type(ghost_loader_t *loader, int *type) {
  if (!loader->file) {
    fprintf(stderr, "ghost_loader: File not open\n");
    *type = -1;
    return false;
  }

  if (loader->buffer_cur_item != loader->buffer_prev_item &&
      loader->buffer_cur_item < loader->buffer_num_items) {
    *type = loader->last_item.type;
  } else if (!read_chunk(loader, type)) {
    *type = -1;
    return false;
  }

  loader->buffer_prev_item = loader->buffer_cur_item;
  return true;
}

static int read_data(ghost_loader_t *loader, int type, void *data,
                     size_t size) {
  if (!loader->file) {
    fprintf(stderr, "ghost_loader: File not open\n");
    return 1;
  }

  if (type < 0 || type >= 256) {
    fprintf(stderr, "ghost_loader: Type invalid\n");
    return 1;
  }

  if (size <= 0 || size > MAX_ITEM_SIZE || size % sizeof(uint32_t) != 0) {
    fprintf(stderr, "ghost_loader: Size invalid\n");
    return 1;
  }

  if ((size_t)(loader->buffer_end - loader->buffer_pos) < size) {
    fprintf(stderr,
            "ghost_loader: Failed to read ghost file '%s': not enough data "
            "(type='%d', got='%zu', wanted='%zu')\n",
            loader->filename, type,
            (size_t)(loader->buffer_end - loader->buffer_pos), size);
    return 1;
  }

  ghost_item_t item_data;
  item_data.type = type;
  if (loader->last_item.type == item_data.type) {
    undiff_item((const uint32_t *)loader->last_item.data,
                (const uint32_t *)loader->buffer_pos,
                (uint32_t *)item_data.data, size / sizeof(uint32_t));
  } else {
    memcpy(item_data.data, loader->buffer_pos, size);
  }

  memcpy(data, item_data.data, size);

  loader->last_item = item_data;
  loader->buffer_pos += size;
  loader->buffer_cur_item++;
  return 0;
}

static void close_ghost_loader(ghost_loader_t *loader) {
  if (!loader->file) {
    return;
  }

  fclose(loader->file);
  loader->file = NULL;
  loader->filename[0] = '\0';
}

static ghost_loader_t new_ghost_loader(void) {
  ghost_loader_t loader;
  loader.file = NULL;
  loader.filename[0] = '\0';
  reset_loader_buffer(&loader);
  return loader;
}

static ghost_loader_t init_ghost_loader(const char *filename) {
  ghost_loader_t loader;
  io_handle_t file = read_header(&loader.header, filename);
  if (!file) {
    loader.file = NULL;
    return loader;
  }

  if (loader.header.version < 6)
    io_seek(file, -(int)sizeof(sha256_digest_t));

  loader.file = file;
  strcpy(loader.filename, filename);
  loader.info = to_ghost_info(&loader.header);
  loader.last_item.type = -1;
  reset_loader_buffer(&loader);
  return loader;
}

ghost_character_t *ghost_get_snap(const ghost_path_t *path, int index) {
  if (!path || !path->chunks || index < 0 || index >= path->num_items)
    return NULL;

  int chunk = index / path->chunk_size;
  int pos = index % path->chunk_size;
  return &path->chunks[chunk][pos];
}

static void str_to_ints(int *ints, size_t num_ints, const char *str) {
  const size_t str_size = strlen(str) + 1;

  for (size_t i = 0; i < num_ints; i++) {
    char buf[sizeof(int)] = {0, 0, 0, 0};
    for (size_t c = 0; c < sizeof(int) && i * sizeof(int) + c < str_size; c++) {
      buf[c] = str[i * sizeof(int) + c];
    }

    ints[i] = ((buf[0] + 128) << 24) | ((buf[1] + 128) << 16) |
              ((buf[2] + 128) << 8) | (buf[3] + 128);
  }

  ints[num_ints - 1] &= 0xFFFFFF00;
}

static bool ints_to_str(const int *ints, size_t num_ints, char *str,
                        size_t str_size) {
  size_t str_index = 0;
  for (size_t int_index = 0; int_index < num_ints; int_index++) {
    const int current_int = ints[int_index];
    str[str_index] = ((current_int >> 24) & 0xff) - 128;
    str_index++;
    str[str_index] = ((current_int >> 16) & 0xff) - 128;
    str_index++;
    str[str_index] = ((current_int >> 8) & 0xff) - 128;
    str_index++;
    str[str_index] = (current_int & 0xff) - 128;
    str_index++;
  }

  str[str_index - 1] = '\0';
  return true;
}

static void reset_ghost_path(ghost_path_t *path) {
  if (!path->chunks) {
    path->num_items = 0;
    return;
  }
  int chunks = (path->num_items + path->chunk_size - 1) / path->chunk_size;
  if (chunks < 0)
    chunks = 0;
  for (int i = 0; i < chunks; ++i)
    free(path->chunks[i]);
  free(path->chunks);
  path->num_items = 0;
  path->chunks = NULL;
}

static void set_ghost_path_size(ghost_path_t *path, int items) {
  reset_ghost_path(path);

  if (items <= 0)
    return;

  int needed_chunks = (items + path->chunk_size - 1) / path->chunk_size;
  path->chunks =
      (ghost_character_t **)calloc(needed_chunks, sizeof(ghost_character_t *));
  if (!path->chunks) {
    path->num_items = 0;
    return;
  }
  for (int i = 0; i < needed_chunks; i++) {
    path->chunks[i] = (ghost_character_t *)calloc(path->chunk_size,
                                                  sizeof(ghost_character_t));
    if (!path->chunks[i]) {
      for (int j = 0; j < i; j++)
        free(path->chunks[j]);
      free(path->chunks);
      path->num_items = 0;
      path->chunks = NULL;
      return;
    }
  }
  path->num_items = items;
}

static void reset_ghost(ghost_t *ghost) {
  reset_ghost_path(&ghost->path);
  ghost->start_tick = -1;
  ghost->playback_pos = -1;
}

ghost_t *ghost_load(const char *filename) {
  ghost_loader_t loader = new_ghost_loader();
  loader = init_ghost_loader(filename);
  if (!loader.file)
    return NULL;

  ghost_t *ghost = (ghost_t *)calloc(1, sizeof(ghost_t));
  if (!ghost) {
    close_ghost_loader(&loader);
    return NULL;
  }
  ghost->path.chunk_size = 25 * 60;

  const ghost_info_t *info = &loader.info;

  reset_ghost(ghost);
  set_ghost_path_size(&ghost->path, info->num_ticks);
  if (ghost->path.num_items != info->num_ticks) {
    fprintf(stderr, "ghost: Failed to allocate memory for path\n");
    close_ghost_loader(&loader);
    ghost_free(ghost);
    return NULL;
  }

  strcpy(ghost->player, info->owner);
  strcpy(ghost->map, info->map);
  ghost->time = info->time;

  int index = 0;
  bool found_skin = false;
  bool no_tick = false;
  bool error = false;

  int type;
  while (!error && read_next_type(&loader, &type)) {
    if (index == info->num_ticks &&
        (type == GHOSTDATA_TYPE_CHARACTER ||
         type == GHOSTDATA_TYPE_CHARACTER_NO_TICK)) {
      error = true;
      break;
    }

    if (type == GHOSTDATA_TYPE_SKIN && !found_skin) {
      found_skin = true;
      if (read_data(&loader, type, &ghost->skin, sizeof(ghost_skin_t) - 24))
        error = true;
      else {
        ints_to_str(ghost->skin.skin, 6, ghost->skin.skin_name, 24);
      }

    } else if (type == GHOSTDATA_TYPE_CHARACTER_NO_TICK) {
      no_tick = true;
      if (read_data(&loader, type, ghost_get_snap(&ghost->path, index++),
                    sizeof(ghost_character_t) - sizeof(int)))
        error = true;
    } else if (type == GHOSTDATA_TYPE_CHARACTER) {
      if (read_data(&loader, type, ghost_get_snap(&ghost->path, index++),
                    sizeof(ghost_character_t)))
        error = true;
    } else if (type == GHOSTDATA_TYPE_START_TICK) {
      if (read_data(&loader, type, &ghost->start_tick, sizeof(int)))
        error = true;
    }
  }

  close_ghost_loader(&loader);

  if (error || index != info->num_ticks) {
    fprintf(stderr,
            "ghost: Failed to read all ghost data (error='%d', got '%d' ticks, "
            "wanted '%d' ticks)\n",
            error, index, info->num_ticks);
    ghost_free(ghost);
    return NULL;
  }

  if (no_tick) {
    int start_tick = 0;
    for (int i = 1; i < info->num_ticks; i++)
      if (ghost_get_snap(&ghost->path, i)->attack_tick !=
          ghost_get_snap(&ghost->path, i - 1)->attack_tick)
        start_tick = ghost_get_snap(&ghost->path, i)->attack_tick - i;
    for (int i = 0; i < info->num_ticks; i++)
      ghost_get_snap(&ghost->path, i - 1)->tick = start_tick + i;
  }

  if (ghost->start_tick == -1 && ghost->path.num_items > 0)
    ghost->start_tick = ghost_get_snap(&ghost->path, 0)->tick;

  if (!found_skin) {
    ghost_set_skin(ghost, "default", 0, 0, 0);
  }

  return ghost;
}

void ghost_free(ghost_t *ghost) {
  if (!ghost)
    return;
  reset_ghost_path(&ghost->path);
  free(ghost);
}

int huffman_compress(const huffman_context_t *ctx, const void *input,
                     int in_size, void *output, int out_size) {
  const unsigned char *src = (const unsigned char *)input;
  const unsigned char *src_end = src + in_size;
  unsigned char *dst = (unsigned char *)output;
  unsigned char *dst_end = dst + out_size;

  unsigned bits = 0;
  unsigned bitcount = 0;

  if (in_size == 0) {
    const huffman_node_t *node = &ctx->nodes[HUFFMAN_EOF_SYMBOL];
    bits |= (unsigned)node->bits << bitcount;
    bitcount += node->num_bits;

    while (bitcount >= 8) {
      if (dst >= dst_end)
        return -1;
      *dst++ = bits & 0xff;
      bits >>= 8;
      bitcount -= 8;
    }
    if (dst >= dst_end)
      return -1;
    *dst++ = bits & 0xff;
    return (int)(dst - (unsigned char *)output);
  }

  int symbol = *src++;

  while (src != src_end) {
    const huffman_node_t *node = &ctx->nodes[symbol];
    bits |= (unsigned)node->bits << bitcount;
    bitcount += node->num_bits;

    symbol = *src++;

    while (bitcount >= 8) {
      if (dst >= dst_end)
        return -1;
      *dst++ = (unsigned char)(bits & 0xff);
      bits >>= 8;
      bitcount -= 8;
    }
  }

  const huffman_node_t *last_node = &ctx->nodes[symbol];
  bits |= (unsigned)last_node->bits << bitcount;
  bitcount += last_node->num_bits;
  while (bitcount >= 8) {
    if (dst >= dst_end)
      return -1;
    *dst++ = (unsigned char)(bits & 0xff);
    bits >>= 8;
    bitcount -= 8;
  }

  const huffman_node_t *eof_node = &ctx->nodes[HUFFMAN_EOF_SYMBOL];
  bits |= (unsigned)eof_node->bits << bitcount;
  bitcount += eof_node->num_bits;
  while (bitcount >= 8) {
    if (dst >= dst_end)
      return -1;
    *dst++ = (unsigned char)(bits & 0xff);
    bits >>= 8;
    bitcount -= 8;
  }

  if (dst >= dst_end)
    return -1;
  *dst++ = bits;

  return (int)(dst - (unsigned char *)output);
}

static unsigned char *var_pack(unsigned char *dst, int i, int dst_size) {
  if (dst_size <= 0)
    return NULL;

  dst_size--;
  *dst = 0;
  if (i < 0) {
    *dst |= 0x40;
    i = ~i;
  }

  *dst |= i & 0x3F;
  i >>= 6;

  while (i) {
    if (dst_size <= 0)
      return NULL;
    *dst |= 0x80;
    dst_size--;
    dst++;
    *dst = i & 0x7F;
    i >>= 7;
  }

  dst++;
  return dst;
}

static long var_compress(const void *src_void, int src_size, void *dst_void,
                         int dst_size) {
  if (src_size % sizeof(int) != 0) {
    return -1;
  }

  const int *src = (const int *)src_void;
  const int *src_end = src + src_size / sizeof(int);
  unsigned char *dst = (unsigned char *)dst_void;
  const unsigned char *dst_start = (const unsigned char *)dst_void;
  const unsigned char *dst_end = dst + dst_size;

  while (src < src_end) {
    dst = var_pack(dst, *src, dst_end - dst);
    if (!dst)
      return -1;
    src++;
  }

  return (long)(dst - dst_start);
}

static void diff_item(const uint32_t *past, const uint32_t *current,
                      uint32_t *out, size_t size) {
  while (size) {
    *out = *current - *past;
    out++;
    past++;
    current++;
    size--;
  }
}

static void uint_to_bytes_be(unsigned char *bytes, unsigned val) {
  bytes[0] = (val >> 24) & 0xff;
  bytes[1] = (val >> 16) & 0xff;
  bytes[2] = (val >> 8) & 0xff;
  bytes[3] = val & 0xff;
}

typedef struct ghost_saver_t {
  FILE *file;
  char filename[IO_MAX_PATH_LENGTH];
  huffman_context_t *huffman;

  unsigned char buffer[MAX_CHUNK_SIZE];

  unsigned char buffer_temp[MAX_CHUNK_SIZE * 2];
  unsigned char compress_buffer[MAX_CHUNK_SIZE * 2];

  unsigned char *buffer_pos;
  int buffer_num_items;

  ghost_item_t last_item;
} ghost_saver_t;

static void reset_saver_buffer(ghost_saver_t *saver) {
  saver->buffer_pos = saver->buffer;
  saver->buffer_num_items = 0;
}

static bool flush_chunk(ghost_saver_t *saver) {
  if (saver->buffer_num_items == 0)
    return true;

  int raw_size = (int)((unsigned char *)saver->buffer_pos -
                       (unsigned char *)saver->buffer);

  if (raw_size == 0) {
    reset_saver_buffer(saver);
    saver->last_item.type = -1;
    return true;
  }

  long var_size = var_compress(saver->buffer, raw_size, saver->buffer_temp,
                               sizeof(saver->buffer_temp));
  if (var_size < 0) {
    fprintf(
        stderr,
        "ghost_saver: Failed to write ghost file '%s': varcompress failed\n",
        saver->filename);
    return false;
  }

  int compressed_size =
      huffman_compress(saver->huffman, saver->buffer_temp, (int)var_size,
                       saver->compress_buffer, sizeof(saver->compress_buffer));
  if (compressed_size < 0) {
    fprintf(stderr,
            "ghost_saver: Failed to write ghost file '%s': huffman compression "
            "failed\n",
            saver->filename);
    return false;
  }

  unsigned char chunk_header[4];
  chunk_header[0] = saver->last_item.type;
  chunk_header[1] = saver->buffer_num_items;
  chunk_header[2] = (compressed_size >> 8) & 0xff;
  chunk_header[3] = compressed_size & 0xff;

  if (fwrite(chunk_header, sizeof(chunk_header), 1, saver->file) != 1) {
    fprintf(stderr,
            "ghost_saver: Failed to write ghost file '%s': error writing chunk "
            "header\n",
            saver->filename);
    return false;
  }
  if (fwrite(saver->compress_buffer, compressed_size, 1, saver->file) != 1) {
    fprintf(stderr,
            "ghost_saver: Failed to write ghost file '%s': error writing chunk "
            "data\n",
            saver->filename);
    return false;
  }

  reset_saver_buffer(saver);
  saver->last_item.type = -1;
  return true;
}

static bool write_data(ghost_saver_t *saver, int type, const void *data,
                       size_t size) {
  if ((size_t)((unsigned char *)saver->buffer + sizeof(saver->buffer) -
               (unsigned char *)saver->buffer_pos) < size) {
    if (!flush_chunk(saver))
      return false;
  }

  ghost_item_t item_data;
  item_data.type = type;
  memcpy(item_data.data, data, size);

  if (saver->last_item.type == item_data.type) {
    diff_item((const uint32_t *)saver->last_item.data,
              (const uint32_t *)item_data.data, (uint32_t *)saver->buffer_pos,
              size / sizeof(uint32_t));
  } else {
    if (!flush_chunk(saver))
      return false;
    memcpy(saver->buffer_pos, item_data.data, size);
  }

  saver->last_item = item_data;
  saver->buffer_pos += size;
  saver->buffer_num_items++;

  if (saver->buffer_num_items >= NUM_ITEMS_PER_CHUNK) {
    if (!flush_chunk(saver))
      return false;
  }

  return true;
}

static bool write_header(FILE *file, const ghost_t *ghost) {
  ghost_header_t header;
  memset(&header, 0, sizeof(header));

  memcpy(header.marker, header_marker, sizeof(header_marker));
  header.version = current_version;
  strncpy(header.owner, ghost->player, sizeof(header.owner));
  strncpy(header.map, ghost->map, sizeof(header.map));
  uint_to_bytes_be(header.num_ticks, ghost->path.num_items);
  uint_to_bytes_be(header.time, ghost->time);

  if (fwrite(&header, sizeof(header), 1, file) != 1) {
    return false;
  }
  return true;
}

int ghost_save(const ghost_t *ghost, const char *filename) {
  FILE *file = fopen(filename, "wb");
  if (!file) {
    fprintf(stderr, "ghost_saver: Failed to open ghost file '%s' for writing\n",
            filename);
    return -1;
  }

  if (!write_header(file, ghost)) {
    fprintf(stderr,
            "ghost_saver: Failed to write ghost file '%s': failed to write "
            "header\n",
            filename);
    fclose(file);
    return -1;
  }

  huffman_context_t ctx;
  huffman_init(&ctx);

  ghost_saver_t saver;
  memset(&saver, 0, sizeof(saver));
  saver.file = file;
  strncpy(saver.filename, filename, sizeof(saver.filename) - 1);
  saver.huffman = &ctx;
  saver.last_item.type = -1;
  reset_saver_buffer(&saver);

  bool error = false;

  if (!write_data(&saver, GHOSTDATA_TYPE_SKIN, &ghost->skin,
                  sizeof(ghost_skin_t) - 24)) {
    error = true;
  }

  if (!error && !write_data(&saver, GHOSTDATA_TYPE_START_TICK,
                            &ghost->start_tick, sizeof(int))) {
    error = true;
  }

  for (int i = 0; i < ghost->path.num_items; i++) {
    if (error)
      break;
    ghost_character_t *character = ghost_get_snap(&ghost->path, i);
    if (!write_data(&saver, GHOSTDATA_TYPE_CHARACTER, character,
                    sizeof(ghost_character_t))) {
      error = true;
    }
  }

  if (!error && !flush_chunk(&saver)) {
    error = true;
  }

  fclose(file);

  if (error) {
    fprintf(stderr,
            "ghost_saver: An error occurred while writing ghost data to '%s'\n",
            filename);
    return -1;
  }

  return 0;
}

ghost_t *ghost_create(void) {
  ghost_t *ghost = (ghost_t *)calloc(1, sizeof(ghost_t));
  if (!ghost)
    return NULL;

  ghost->path.chunk_size = 25 * 60;
  ghost->start_tick = -1;
  ghost->playback_pos = -1;

  ghost_set_skin(ghost, "default", 0, 0, 0);
  return ghost;
}

void ghost_set_meta(ghost_t *ghost, const char *player, const char *map,
                    int time_ms) {
  if (!ghost)
    return;
  strncpy(ghost->player, player, sizeof(ghost->player) - 1);
  ghost->player[sizeof(ghost->player) - 1] = '\0';
  strncpy(ghost->map, map, sizeof(ghost->map) - 1);
  ghost->map[sizeof(ghost->map) - 1] = '\0';
  ghost->time = time_ms;
}

void ghost_set_skin(ghost_t *ghost, const char *skin_name, int use_custom_color,
                    int color_body, int color_feet) {
  if (!ghost)
    return;
  strncpy(ghost->skin.skin_name, skin_name, sizeof(ghost->skin.skin_name) - 1);
  ghost->skin.skin_name[sizeof(ghost->skin.skin_name) - 1] = '\0';
  str_to_ints(ghost->skin.skin, 6, skin_name);
  ghost->skin.use_custom_color = use_custom_color;
  ghost->skin.color_body = color_body;
  ghost->skin.color_feet = color_feet;
}

void ghost_add_snap(ghost_t *ghost, const ghost_character_t *snap) {
  if (!ghost || !snap)
    return;

  if (ghost->start_tick == -1 && snap->tick > 0)
    ghost->start_tick = snap->tick;

  int chunk = ghost->path.num_items / ghost->path.chunk_size;
  int pos = ghost->path.num_items % ghost->path.chunk_size;

  if (pos == 0) {
    int num_chunks = chunk + 1;
    ghost_character_t **new_chunks = (ghost_character_t **)realloc(
        ghost->path.chunks, num_chunks * sizeof(ghost_character_t *));
    if (!new_chunks) {
      fprintf(stderr, "ghost: Failed to realloc chunks\n");
      return;
    }
    ghost->path.chunks = new_chunks;

    ghost->path.chunks[chunk] = (ghost_character_t *)calloc(
        ghost->path.chunk_size, sizeof(ghost_character_t));
    if (!ghost->path.chunks[chunk]) {
      fprintf(stderr, "ghost: Failed to calloc chunk\n");
      return;
    }
  }

  memcpy(&ghost->path.chunks[chunk][pos], snap, sizeof(ghost_character_t));
  ghost->path.num_items++;
}
