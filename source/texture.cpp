#include "texture.h"

namespace vox {
    uint32_t Texture::getId() const {
        return id;
    }

    uint32_t Texture::getAtlasId() const {
        return atlasId;
    }

    const std::string &Texture::getPath() const {
        return path;
    }

    float Texture::getMinU() const {
        return minU;
    }

    float Texture::getMinV() const {
        return minV;
    }

    float Texture::getMaxU() const {
        return maxU;
    }

    float Texture::getMaxV() const {
        return maxV;
    }

    uint32_t Texture::getWidth() const {
        return width;
    }

    uint32_t Texture::getHeight() const {
        return height;
    }
}