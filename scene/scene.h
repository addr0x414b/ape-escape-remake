#ifndef SCENE_H
#define SCENE_H

#include "3d/mesh_3d.h"

// This is defined in the CMakelists.txt file
// Doing this simply to get rid of the intellisense error
#ifndef ASSET_PATH
#define ASSET_PATH "${CMAKE_BINARY_DIR}/assets}"
#endif

class Scene {
   public:
    Scene();

    Mesh3D dennis;

   private:
    Debugger debugger;
};

#endif