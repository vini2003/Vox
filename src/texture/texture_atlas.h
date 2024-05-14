//
// Created by vini2003 on 13/05/2024.
//

#ifndef VOX_TEXTURE_ATLAS_H
#define VOX_TEXTURE_ATLAS_H


#include <cstdint>
#include <vector>
#include <algorithm>
#include "texture_manager.h"

namespace vox {
    class TextureAtlas {
        const uint32_t id;

        uint32_t width;
        uint32_t height;

        std::vector<uint32_t> textureIds = {};

        void addTexture(uint32_t textureId);

        void removeTexture(uint32_t textureId);

        void stitchTextures(TextureManager& textureManager);
    };
}


#endif //VOX_TEXTURE_ATLAS_H
