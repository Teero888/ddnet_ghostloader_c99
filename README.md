# DDNet Ghost Loader
This is a minimalistic C99-compatible ghost file loader, inspired by the ghost file loading code from [DDNet](https://github.com/ddnet/ddnet).

## Features
- Loads ghost files in a simple and isolated manner.
- The only dependency is libc.

## Usage
To use the ghost loader in your project, simply include the `ghost_loader.h` file and compile the `ghost_loader.c` file along with your project.

### Example
```c
#include "ghost_loader.h"
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
         "\tfinish time: %f\n"
         "\tnum snaps: %d\n",
         Ghost.m_aPlayer, Ghost.m_Skin.m_aSkinName, Ghost.m_Skin.m_ColorBody,
         Ghost.m_Skin.m_ColorFeet, Ghost.m_Skin.m_UseCustomColor,
         Ghost.m_StartTick, Ghost.m_Time / 1000.f, Ghost.m_Path.m_NumItems);

  free_ghost(&Ghost);
  return 0;
}
```
## Output
```bash
> ./build/example
Snap: 0
        Pos: 44,43
        Vel: 30,0
        Angle: 939
Snap: 50
        Pos: 148,56
        Vel: 47,0
        Angle: 322
Snap: 100
        Pos: 195,112
        Vel: -35,0
        Angle: 1003
Snap: 150
        Pos: 160,160
        Vel: 28,0
        Angle: 125
Snap: 200
        Pos: 263,183
        Vel: 59,0
        Angle: -282
Snap: 250
        Pos: 391,213
        Vel: 61,0
        Angle: 800
Ghost player: Teero
        skin: glow_mermyfox
        color body: 16449290
        color feet: 16777215
        use custom color: 1
        start tick: 127783
        finish time: 11.625000
        num snaps: 291
```  