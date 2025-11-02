#ifndef LIB_GHOST_H
#define LIB_GHOST_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GhostSkin {
  int m_aSkin[6];
  int m_UseCustomColor;
  int m_ColorBody;
  int m_ColorFeet;
  char m_aSkinName[24];
} SGhostSkin;

typedef struct GhostCharacter {
  int m_X;
  int m_Y;
  int m_VelX;
  int m_VelY;
  int m_Angle;
  int m_Direction;
  int m_Weapon;
  int m_HookState;
  int m_HookX;
  int m_HookY;
  int m_AttackTick;
  int m_Tick;
} SGhostCharacter;

typedef struct GhostPath {
  int m_ChunkSize;
  int m_NumItems;
  SGhostCharacter **m_vpChunks;
} SGhostPath;

typedef struct Ghost {
  SGhostSkin m_Skin;
  SGhostPath m_Path;
  int m_StartTick;
  char m_aPlayer[16];
  int m_PlaybackPos;
  char m_aMap[64];
  int m_Time;
} SGhost;

SGhostCharacter *ghost_path_get(SGhostPath *pPath, int Index);
int load_ghost(SGhost *pGhost, const char *pFilename);
int save_ghost(SGhost *pGhost, const char *pFilename);
void free_ghost(SGhost *pGhost);

#ifdef __cplusplus
}
#endif

#endif // LIB_GHOST_H
