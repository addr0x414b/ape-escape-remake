#include "scene.h"

Scene::Scene() {
    debugger.consoleMessage("\nBegin loading in Scene...", false);
    dennis = Mesh3D((std::string(ASSET_PATH) + "/models/dennis.obj").c_str(),
                    (std::string(ASSET_PATH) + "/textures/dennis.png").c_str());
}