#include "mesh_3d.h"
#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb/stb_image.h"

Mesh3D::Mesh3D(const char* meshFile, const char* textureFile) {
    debugger.consoleMessage("\nBegin loading in Mesh3D...", false);
    debugger.consoleMessage("Begin loading in texture image...", false);
    int texWidth, texHeight, texChannels;
    texturePixels =
        stbi_load(textureFile, &texWidth,
                  &texHeight, &texChannels, STBI_rgb_alpha);

    if (!texturePixels) {
        debugger.consoleMessage("Failed to load texture image!", true);
    } else {
        debugger.consoleMessage("Successfully loaded texture image", false);
    }

    mipLevels = static_cast<uint32_t>(
                    std::floor(std::log2(std::max(texWidth, texHeight)))) +
                1;
}