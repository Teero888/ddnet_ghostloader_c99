// vim:foldmethod=marker

#include "ghost.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Huffman {{{

#define HUFFMAN_EOF_SYMBOL 256
#define HUFFMAN_MAX_SYMBOLS (HUFFMAN_EOF_SYMBOL + 1)
#define HUFFMAN_MAX_NODES (HUFFMAN_MAX_SYMBOLS * 2 - 1)
#define HUFFMAN_LUTBITS 10
#define HUFFMAN_LUTSIZE (1 << HUFFMAN_LUTBITS)
#define HUFFMAN_LUTMASK (HUFFMAN_LUTSIZE - 1)

typedef struct Node {
  unsigned m_Bits;
  unsigned m_NumBits;
  unsigned short m_aLeafs[2];
  unsigned char m_Symbol;
} Node;

typedef struct HuffmanContext {
  Node m_aNodes[HUFFMAN_MAX_NODES];
  Node *m_apDecodeLut[HUFFMAN_LUTSIZE];
  Node *m_pStartNode;
  int m_NumNodes;
} HuffmanContext;

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

typedef struct {
  unsigned short m_NodeId;
  int m_Frequency;
} ConstructNode;

static int compare_nodes(const void *a, const void *b) {
  const ConstructNode *na = *(const ConstructNode **)a;
  const ConstructNode *nb = *(const ConstructNode **)b;
  return nb->m_Frequency - na->m_Frequency;
}

static void set_bits_recursive(Node *nodes, Node *node, int bits,
                               unsigned depth) {
  if (node->m_aLeafs[1] != 0xffff)
    set_bits_recursive(nodes, &nodes[node->m_aLeafs[1]], bits | (1 << depth),
                       depth + 1);
  if (node->m_aLeafs[0] != 0xffff)
    set_bits_recursive(nodes, &nodes[node->m_aLeafs[0]], bits, depth + 1);

  if (node->m_NumBits) {
    node->m_Bits = bits;
    node->m_NumBits = depth;
  }
}

static void construct_tree(HuffmanContext *ctx, const unsigned *frequencies) {
  ConstructNode nodes_left_storage[HUFFMAN_MAX_SYMBOLS];
  ConstructNode *nodes_left[HUFFMAN_MAX_SYMBOLS];
  int num_nodes_left = HUFFMAN_MAX_SYMBOLS;

  // Initialize nodes
  for (int i = 0; i < HUFFMAN_MAX_SYMBOLS; i++) {
    ctx->m_aNodes[i].m_NumBits = 0xFFFFFFFF;
    ctx->m_aNodes[i].m_Symbol = i;
    ctx->m_aNodes[i].m_aLeafs[0] = 0xffff;
    ctx->m_aNodes[i].m_aLeafs[1] = 0xffff;

    nodes_left_storage[i].m_Frequency =
        (i == HUFFMAN_EOF_SYMBOL) ? 1 : frequencies[i];
    nodes_left_storage[i].m_NodeId = i;
    nodes_left[i] = &nodes_left_storage[i];
  }

  ctx->m_NumNodes = HUFFMAN_MAX_SYMBOLS;

  // Build tree
  while (num_nodes_left > 1) {
    qsort(nodes_left, num_nodes_left, sizeof(ConstructNode *), compare_nodes);

    Node *new_node = &ctx->m_aNodes[ctx->m_NumNodes];
    new_node->m_NumBits = 0;
    new_node->m_aLeafs[0] = nodes_left[num_nodes_left - 1]->m_NodeId;
    new_node->m_aLeafs[1] = nodes_left[num_nodes_left - 2]->m_NodeId;

    nodes_left[num_nodes_left - 2]->m_NodeId = ctx->m_NumNodes;
    nodes_left[num_nodes_left - 2]->m_Frequency +=
        nodes_left[num_nodes_left - 1]->m_Frequency;

    ctx->m_NumNodes++;
    num_nodes_left--;
  }

  // Set start node and build bits
  ctx->m_pStartNode = &ctx->m_aNodes[ctx->m_NumNodes - 1];
  set_bits_recursive(ctx->m_aNodes, ctx->m_pStartNode, 0, 0);
}

void huffman_init(HuffmanContext *ctx) {
  memset(ctx, 0, sizeof(*ctx));
  construct_tree(ctx, huffman_freq_table);

  // Build decode LUT
  for (int i = 0; i < HUFFMAN_LUTSIZE; i++) {
    unsigned bits = i;
    Node *node = ctx->m_pStartNode;

    for (int k = 0; k < HUFFMAN_LUTBITS; k++) {
      node = &ctx->m_aNodes[node->m_aLeafs[bits & 1]];
      bits >>= 1;

      if (node->m_NumBits) {
        ctx->m_apDecodeLut[i] = node;
        break;
      }
    }

    if (!ctx->m_apDecodeLut[i])
      ctx->m_apDecodeLut[i] = node;
  }
}

int huffman_decompress(const HuffmanContext *ctx, const void *input,
                       int in_size, void *output, int out_size) {
  const unsigned char *src = (const unsigned char *)input;
  const unsigned char *src_end = src + in_size;
  unsigned char *dst = (unsigned char *)output;
  unsigned char *dst_end = dst + out_size;

  unsigned bits = 0;
  unsigned bitcount = 0;
  const Node *eof = &ctx->m_aNodes[HUFFMAN_EOF_SYMBOL];

  while (1) {
    const Node *node = NULL;

    if (bitcount >= HUFFMAN_LUTBITS)
      node = ctx->m_apDecodeLut[bits & HUFFMAN_LUTMASK];

    while (bitcount < 24 && src < src_end) {
      bits |= (*src++) << bitcount;
      bitcount += 8;
    }

    if (!node)
      node = ctx->m_apDecodeLut[bits & HUFFMAN_LUTMASK];

    if (!node)
      return -1;

    if (node->m_NumBits) {
      bits >>= node->m_NumBits;
      bitcount -= node->m_NumBits;
    } else {
      bits >>= HUFFMAN_LUTBITS;
      bitcount -= HUFFMAN_LUTBITS;

      while (1) {
        node = &ctx->m_aNodes[node->m_aLeafs[bits & 1]];
        bits >>= 1;
        bitcount--;

        if (node->m_NumBits)
          break;
        if (bitcount == 0)
          return -1;
      }
    }

    if (node == eof)
      break;
    if (dst >= dst_end)
      return -1;
    *dst++ = node->m_Symbol;
  }

  return (int)(dst - (unsigned char *)output);
}

// }}}

enum {
  MAX_ITEM_SIZE = 128,
  NUM_ITEMS_PER_CHUNK = 50,
  MAX_CHUNK_SIZE = MAX_ITEM_SIZE * NUM_ITEMS_PER_CHUNK,
  MAX_NAME_LENGTH = 16,
  IO_MAX_PATH_LENGTH = 512,
  SHA256_DIGEST_LENGTH = 256 / 8,
  SHA256_MAXSTRSIZE = 2 * SHA256_DIGEST_LENGTH + 1,
};

struct SHA256_DIGEST {
  unsigned char data[SHA256_DIGEST_LENGTH];
} typedef SHA256_DIGEST;

struct GhostInfo {
  char m_aOwner[MAX_NAME_LENGTH];
  char m_aMap[64];
  int m_NumTicks;
  int m_Time;
} typedef SGhostInfo;

// version 4-6
struct GhostHeader {
  unsigned char m_aMarker[8];
  unsigned char m_Version;
  char m_aOwner[MAX_NAME_LENGTH];
  char m_aMap[64];
  unsigned char m_aZeroes[sizeof(int32_t)]; // Crc before version 6
  unsigned char m_aNumTicks[sizeof(int32_t)];
  unsigned char m_aTime[sizeof(int32_t)];
  SHA256_DIGEST m_MapSha256;
} typedef SGhostHeader;

struct GhostItem {
  uint32_t m_aData[MAX_ITEM_SIZE];
  int m_Type;
} typedef SGhostItem;

enum {
  GHOSTDATA_TYPE_SKIN = 0,
  GHOSTDATA_TYPE_CHARACTER_NO_TICK,
  GHOSTDATA_TYPE_CHARACTER,
  GHOSTDATA_TYPE_START_TICK
};

struct GhostLoader {
  void *m_File;
  char m_aFilename[IO_MAX_PATH_LENGTH];

  SGhostHeader m_Header;
  SGhostInfo m_Info;

  uint32_t m_aBuffer[MAX_CHUNK_SIZE];
  uint32_t m_aBufferTemp[MAX_CHUNK_SIZE];
  uint32_t *m_pBufferPos;
  uint32_t *m_pBufferEnd;
  int m_BufferNumItems;
  int m_BufferCurItem;
  int m_BufferPrevItem;
  SGhostItem m_LastItem;
} typedef SGhostLoader;

static const unsigned char gs_aHeaderMarker[8] = {'T', 'W', 'G', 'H',
                                                  'O', 'S', 'T', 0};
static const unsigned char gs_CurVersion = 6;

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

static int GetTicks(const SGhostHeader *pHeader) {
  return bytes_be_to_uint(pHeader->m_aNumTicks);
}

static int GetTime(const SGhostHeader *pHeader) {
  return bytes_be_to_uint(pHeader->m_aTime);
}

static bool ValidateHeader(const SGhostHeader *pHeader, const char *pFilename) {
  if (memcmp(pHeader->m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) !=
      0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': invalid header marker\n",
        pFilename);
    return false;
  }

  if (pHeader->m_Version < 4 || pHeader->m_Version > gs_CurVersion) {
    fprintf(stderr,
            "ghost_loader: Failed to read ghost file '%s': ghost version '%d' "
            "is not supported\n",
            pFilename, pHeader->m_Version);
    return false;
  }

  if (!mem_has_null(pHeader->m_aOwner, sizeof(pHeader->m_aOwner))) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': owner name is invalid\n",
        pFilename);
    return false;
  }

  if (!mem_has_null(pHeader->m_aMap, sizeof(pHeader->m_aMap))) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': map name is invalid\n",
        pFilename);
    return false;
  }

  const int NumTicks = GetTicks(pHeader);
  if (NumTicks <= 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': number of ticks '%d' "
        "is invalid\n",
        pFilename, NumTicks);
    return false;
  }

  const int Time = GetTime(pHeader);
  if (Time <= 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': time '%d' is invalid\n",
        pFilename, Time);
    return false;
  }

  return true;
}

typedef void *IOHANDLE;
static IOHANDLE ReadHeader(SGhostHeader *pHeader, const char *pFilename) {
  FILE *pFile = fopen(pFilename, "r");
  if (!pFile) {
    fprintf(stderr,
            "ghost_loader: Failed to open ghost file '%s' for reading\n",
            pFilename);
    return NULL;
  }

  if (fread(pHeader, sizeof(*pHeader), 1, pFile) != 1) {
    fprintf(stderr,
            "ghost_loader: Failed to read ghost file '%s': failed to read "
            "header\n",
            pFilename);
    fclose(pFile);
    return NULL;
  }

  if (!ValidateHeader(pHeader, pFilename)) {
    fclose(pFile);
    return NULL;
  }

  return pFile;
}

static int io_seek(IOHANDLE io, int64_t offset) {
#if defined(CONF_FAMILY_WINDOWS)
  return _fseeki64((FILE *)io, offset, SEEK_CUR);
#else
  return fseek((FILE *)io, offset, SEEK_CUR);
#endif
}

static SGhostInfo ToGhostInfo(SGhostHeader *pHeader) {
  SGhostInfo Result;
  strcpy(Result.m_aOwner, pHeader->m_aOwner);
  strcpy(Result.m_aMap, pHeader->m_aMap);
  Result.m_NumTicks = GetTicks(pHeader);
  Result.m_Time = GetTime(pHeader);
  return Result;
}

static void ResetBuffer(SGhostLoader *pLoader) {
  pLoader->m_pBufferPos = pLoader->m_aBuffer;
  pLoader->m_pBufferEnd = pLoader->m_aBuffer;
  pLoader->m_BufferNumItems = 0;
  pLoader->m_BufferCurItem = 0;
  pLoader->m_BufferPrevItem = -1;
}

static void UndiffItem(const uint32_t *pPast, const uint32_t *pDiff,
                       uint32_t *pOut, size_t Size) {
  while (Size) {
    *pOut = *pPast + *pDiff;
    pOut++;
    pPast++;
    pDiff++;
    Size--;
  }
}

static const unsigned char *VarUnpack(const unsigned char *pSrc, int *pInOut,
                                      int SrcSize) {
  if (SrcSize <= 0)
    return NULL;

  const int Sign = (*pSrc >> 6) & 1;
  *pInOut = *pSrc & 0x3F;
  SrcSize--;

  static const int s_aMasks[] = {0x7F, 0x7F, 0x7F, 0x0F};
  static const int s_aShifts[] = {6, 6 + 7, 6 + 7 + 7, 6 + 7 + 7 + 7};

  for (unsigned i = 0; i < 4; i++) {
    if (!(*pSrc & 0x80))
      break;
    if (SrcSize <= 0)
      return NULL;
    SrcSize--;
    pSrc++;
    *pInOut |= (*pSrc & s_aMasks[i]) << s_aShifts[i];
  }

  pSrc++;
  *pInOut ^= -Sign; // if(sign) *i = ~(*i)
  return pSrc;
}

static long VarDecompress(const void *pSrc_, int SrcSize, void *pDst_,
                          int DstSize) {
  if (DstSize % sizeof(int) != 0) {
    return -1;
    fprintf(stderr, "Variable int invalid bounds\n");
  }

  const unsigned char *pSrc = (unsigned char *)pSrc_;
  const unsigned char *pSrcEnd = pSrc + SrcSize;
  int *pDst = (int *)pDst_;
  const int *pDstEnd = pDst + DstSize / sizeof(int);
  while (pSrc < pSrcEnd) {
    if (pDst >= pDstEnd)
      return -1;
    pSrc = VarUnpack(pSrc, pDst, pSrcEnd - pSrc);
    if (!pSrc)
      return -1;
    pDst++;
  }

  return (long)((unsigned char *)pDst - (unsigned char *)pDst_);
}

static bool ReadChunk(SGhostLoader *pLoader, int *pType) {
  if (pLoader->m_Header.m_Version != 4) {
    pLoader->m_LastItem.m_Type = -1;
  }

  ResetBuffer(pLoader);

  unsigned char aChunkHeader[4];
  if (fread(aChunkHeader, sizeof(aChunkHeader), 1, pLoader->m_File) != 1) {
    return false; // EOF
  }

  *pType = aChunkHeader[0];
  int Size = (aChunkHeader[2] << 8) | aChunkHeader[3];
  pLoader->m_BufferNumItems = aChunkHeader[1];

  if (Size <= 0 || Size > MAX_CHUNK_SIZE) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': invalid chunk header "
        "size\n",
        pLoader->m_aFilename);
    return false;
  }

  if (fread(pLoader->m_aBuffer, Size, 1, pLoader->m_File) != 1) {
    fprintf(stderr,
            "ghost_loader: Failed to read ghost file '%s': error reading chunk "
            "data\n",
            pLoader->m_aFilename);
    return false;
  }

  HuffmanContext ctx;
  huffman_init(&ctx);

  Size =
      huffman_decompress(&ctx, pLoader->m_aBuffer, Size, pLoader->m_aBufferTemp,
                         sizeof(pLoader->m_aBufferTemp));
  if (Size < 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': error during network "
        "decompression\n",
        pLoader->m_aFilename);
    return false;
  }

  Size = VarDecompress(pLoader->m_aBufferTemp, Size, pLoader->m_aBuffer,
                       sizeof(pLoader->m_aBuffer));
  if (Size < 0) {
    fprintf(
        stderr,
        "ghost_loader: Failed to read ghost file '%s': error during intpack "
        "decompression\n",
        pLoader->m_aFilename);
    return false;
  }

  pLoader->m_pBufferEnd = pLoader->m_aBuffer + Size;
  return true;
}

static bool ReadNextType(SGhostLoader *pLoader, int *pType) {
  if (!pLoader->m_File) {
    fprintf(stderr, "ghost_loader: File not open\n");
    return 1;
  }

  if (pLoader->m_BufferCurItem != pLoader->m_BufferPrevItem &&
      pLoader->m_BufferCurItem < pLoader->m_BufferNumItems) {
    *pType = pLoader->m_LastItem.m_Type;
  } else if (!ReadChunk(pLoader, pType)) {
    return false; // error or EOF
  }

  pLoader->m_BufferPrevItem = pLoader->m_BufferCurItem;
  return true;
}

static int ReadData(SGhostLoader *pLoader, int Type, void *pData, size_t Size) {
  if (!pLoader->m_File) {
    fprintf(stderr, "ghost_loader: File not open\n");
    return 1;
  }

  if (Type < 0 || Type >= 256) {
    fprintf(stderr, "ghost_loader: Type invalid\n");
    return 1;
  }

  if (Size <= 0 || Size > MAX_ITEM_SIZE || Size % sizeof(uint32_t) != 0) {
    fprintf(stderr, "ghost_loader: Size invalid\n");
    return 1;
  }

  if ((size_t)(pLoader->m_pBufferEnd - pLoader->m_pBufferPos) < Size) {
    fprintf(stderr,
            "ghost_loader: Failed to read ghost file '%s': not enough data "
            "(type='%d', got='%zu', wanted='%zu')\n",
            pLoader->m_aFilename, Type,
            (size_t)(pLoader->m_pBufferEnd - pLoader->m_pBufferPos), Size);
    return 1;
  }

  SGhostItem Data;
  Data.m_Type = Type;
  if (pLoader->m_LastItem.m_Type == Data.m_Type) {
    UndiffItem((const uint32_t *)pLoader->m_LastItem.m_aData,
               (const uint32_t *)pLoader->m_pBufferPos,
               (uint32_t *)Data.m_aData, Size / sizeof(uint32_t));
  } else {
    memcpy(Data.m_aData, pLoader->m_pBufferPos, Size);
  }

  memcpy(pData, Data.m_aData, Size);

  pLoader->m_LastItem = Data;
  pLoader->m_pBufferPos += Size;
  pLoader->m_BufferCurItem++;
  return 0;
}

static void ResetGhostLoader(SGhostLoader *pLoader) {
  pLoader->m_pBufferPos = pLoader->m_aBuffer;
  pLoader->m_pBufferEnd = pLoader->m_aBuffer;
  pLoader->m_BufferNumItems = 0;
  pLoader->m_BufferCurItem = 0;
  pLoader->m_BufferPrevItem = -1;
}

static void Close(SGhostLoader *pLoader) {
  if (!pLoader->m_File) {
    return;
  }

  fclose(pLoader->m_File);
  pLoader->m_File = NULL;
  pLoader->m_aFilename[0] = '\0';
}

static SGhostLoader NewGhostLoader(void) {
  SGhostLoader Loader;
  Loader.m_File = NULL;
  Loader.m_aFilename[0] = '\0';
  ResetGhostLoader(&Loader);
  return Loader;
}

static SGhostLoader Load(const char *pFilename) {
  SGhostLoader Loader;
  IOHANDLE File = ReadHeader(&Loader.m_Header, pFilename);
  if (!File)
    return Loader;

  if (Loader.m_Header.m_Version < 6)
    io_seek(File, -(int)sizeof(SHA256_DIGEST));

  Loader.m_File = File;
  strcpy(Loader.m_aFilename, pFilename);
  Loader.m_Info = ToGhostInfo(&Loader.m_Header);
  Loader.m_LastItem.m_Type = -1;
  ResetBuffer(&Loader);
  return Loader;
}

SGhostCharacter *ghost_path_get(SGhostPath *pPath, int Index) {
  if (Index < 0 || Index >= pPath->m_NumItems)
    return NULL;

  int Chunk = Index / pPath->m_ChunkSize;
  int Pos = Index % pPath->m_ChunkSize;
  return &pPath->m_vpChunks[Chunk][Pos];
}

static void StrToInts(int *pInts, size_t NumInts, const char *pStr) {
  const size_t StrSize = strlen(pStr) + 1;

  for (size_t i = 0; i < NumInts; i++) {
    // Copy to temporary buffer to ensure we don't read past the end of the
    // input string
    char aBuf[sizeof(int)] = {0, 0, 0, 0};
    for (size_t c = 0; c < sizeof(int) && i * sizeof(int) + c < StrSize; c++) {
      aBuf[c] = pStr[i * sizeof(int) + c];
    }

    pInts[i] = ((aBuf[0] + 128) << 24) | ((aBuf[1] + 128) << 16) |
               ((aBuf[2] + 128) << 8) | (aBuf[3] + 128);
  }

  // Last byte is always zero and unused in this format
  pInts[NumInts - 1] &= 0xFFFFFF00;
}

static bool IntsToStr(const int *pInts, size_t NumInts, char *pStr,
                      size_t StrSize) {
  // Unpack string without validation
  size_t StrIndex = 0;
  for (size_t IntIndex = 0; IntIndex < NumInts; IntIndex++) {
    const int CurrentInt = pInts[IntIndex];
    pStr[StrIndex] = ((CurrentInt >> 24) & 0xff) - 128;
    StrIndex++;
    pStr[StrIndex] = ((CurrentInt >> 16) & 0xff) - 128;
    StrIndex++;
    pStr[StrIndex] = ((CurrentInt >> 8) & 0xff) - 128;
    StrIndex++;
    pStr[StrIndex] = (CurrentInt & 0xff) - 128;
    StrIndex++;
  }

  // Ensure null-termination
  pStr[StrIndex - 1] = '\0';
  return true;
}

static void SetGhostSkinData(SGhostSkin *pSkin, const char *pSkinName,
                             int UseCustomColor, int ColorBody, int ColorFeet) {
  strcpy(pSkin->m_aSkinName, pSkinName);
  StrToInts(pSkin->m_aSkin, sizeof(pSkin->m_aSkin) / sizeof(pSkin->m_aSkin[0]),
            pSkinName);
  pSkin->m_UseCustomColor = UseCustomColor;
  pSkin->m_ColorBody = ColorBody;
  pSkin->m_ColorFeet = ColorFeet;
}

static void ResetGhostPath(SGhostPath *pPath) {
  pPath->m_NumItems = 0;
  pPath->m_vpChunks = NULL;
  pPath->m_ChunkSize = 25 * 60;
}

static void SetGhostPathSize(SGhostPath *pPath, int Items) {
  int Chunks =
      (pPath->m_NumItems + pPath->m_ChunkSize - 1) / pPath->m_ChunkSize;
  for (int i = 0; i < Chunks; ++i)
    free(pPath->m_vpChunks[i]);
  free(pPath->m_vpChunks);

  int NeededChunks = (Items + pPath->m_ChunkSize - 1) / pPath->m_ChunkSize;
  pPath->m_vpChunks = malloc(NeededChunks * sizeof(pPath->m_vpChunks));
  for (int i = 0; i < NeededChunks; i++)
    pPath->m_vpChunks[i] =
        (SGhostCharacter *)calloc(pPath->m_ChunkSize, sizeof(SGhostCharacter));

  pPath->m_NumItems = Items;
}

static void ResetGhost(SGhost *pGhost) {
  ResetGhostPath(&pGhost->m_Path);
  pGhost->m_StartTick = -1;
  pGhost->m_PlaybackPos = -1;
}

int load_ghost(SGhost *pGhost, const char *pFilename) {
  SGhostLoader Loader = NewGhostLoader();
  Loader = Load(pFilename);
  if (!Loader.m_File)
    return -1;

  const SGhostInfo *pInfo = &Loader.m_Info;

  // select ghost
  ResetGhost(pGhost);
  SetGhostPathSize(&pGhost->m_Path, pInfo->m_NumTicks);

  strcpy(pGhost->m_aPlayer, pInfo->m_aOwner);
  strcpy(pGhost->m_aMap, pInfo->m_aMap);
  pGhost->m_Time = pInfo->m_Time;

  int Index = 0;
  bool FoundSkin = false;
  bool NoTick = false;
  bool Error = false;

  int Type;
  while (!Error && ReadNextType(&Loader, &Type)) {
    if (Index == pInfo->m_NumTicks &&
        (Type == GHOSTDATA_TYPE_CHARACTER ||
         Type == GHOSTDATA_TYPE_CHARACTER_NO_TICK)) {
      Error = true;
      break;
    }

    if (Type == GHOSTDATA_TYPE_SKIN && !FoundSkin) {
      FoundSkin = true;
      if (ReadData(&Loader, Type, &pGhost->m_Skin, sizeof(SGhostSkin) - 24))
        Error = true;
      else {
        IntsToStr(pGhost->m_Skin.m_aSkin, 6, pGhost->m_Skin.m_aSkinName, 24);
      }

    } else if (Type == GHOSTDATA_TYPE_CHARACTER_NO_TICK) {
      NoTick = true;
      if (ReadData(&Loader, Type, ghost_path_get(&pGhost->m_Path, Index++),
                   sizeof(SGhostCharacter) - sizeof(int)))
        Error = true;
    } else if (Type == GHOSTDATA_TYPE_CHARACTER) {
      if (ReadData(&Loader, Type, ghost_path_get(&pGhost->m_Path, Index++),
                   sizeof(SGhostCharacter)))
        Error = true;
    } else if (Type == GHOSTDATA_TYPE_START_TICK) {
      if (ReadData(&Loader, Type, &pGhost->m_StartTick, sizeof(int)))
        Error = true;
    }
  }

  Close(&Loader);

  if (Error || Index != pInfo->m_NumTicks) {
    fprintf(stderr,
            "ghost: Failed to read all ghost data (error='%d', got '%d' ticks, "
            "wanted '%d' ticks)\n",
            Error, Index, pInfo->m_NumTicks);
    ResetGhost(pGhost);
    return -1;
  }

  if (NoTick) {
    int StartTick = 0;
    for (int i = 1; i < pInfo->m_NumTicks; i++) // estimate start tick
      if (ghost_path_get(&pGhost->m_Path, i)->m_AttackTick !=
          ghost_path_get(&pGhost->m_Path, i - 1)->m_AttackTick)
        StartTick = ghost_path_get(&pGhost->m_Path, i)->m_AttackTick - i;
    for (int i = 0; i < pInfo->m_NumTicks; i++)
      ghost_path_get(&pGhost->m_Path, i - 1)->m_Tick = StartTick + i;
  }

  if (pGhost->m_StartTick == -1)
    pGhost->m_StartTick = ghost_path_get(&pGhost->m_Path, 0)->m_Tick;

  if (!FoundSkin) {
    SetGhostSkinData(&pGhost->m_Skin, "default", 0, 0, 0);
  }

  return 0;
}
