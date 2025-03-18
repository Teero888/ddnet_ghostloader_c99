#include "ghost.h"
#include <stdio.h>

int main(void) {

  SGhost Ghost;
  if (load_ghost(&Ghost, "run_dead_silence.gho")) {
    printf("Ghost file could not be loaded\n");
    return 1;
  }

  for (int i = 0; i < Ghost.m_Path.m_NumItems; i += 50) {
    SGhostCharacter *pChar = ghost_path_get(&Ghost.m_Path, i);
    printf("Snap: %d\n"
           "\tPos: %d,%d\n"
           "\tVel: %d,%d\n"
           "\tAngle: %d\n",
           i, pChar->m_X / 32, pChar->m_Y / 32, pChar->m_VelX / 256, pChar->m_VelY / 256,
           pChar->m_Angle);
  }

  printf("Ghost player: %s\n"
         "\tskin: %s\n"
         "\tcolor body: %d\n"
         "\tcolor feet: %d\n"
         "\tuse custom color: %d\n"
         "\tstart tick: %d\n"
         "\ttime: %f\n"
         "\tnum snaps: %d\n",
         Ghost.m_aPlayer, Ghost.m_Skin.m_aSkinName, Ghost.m_Skin.m_ColorBody,
         Ghost.m_Skin.m_ColorFeet, Ghost.m_Skin.m_UseCustomColor,
         Ghost.m_StartTick, Ghost.m_Time / 1000.f, Ghost.m_Path.m_NumItems);

  free_ghost(&Ghost);
  return 0;
}
