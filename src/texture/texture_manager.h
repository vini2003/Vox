#ifndef VOX_TEXTURE_MANAGER_H
#define VOX_TEXTURE_MANAGER_H

/**
 * Textures are loaded once, during start-up.
 * They are then stitched into a texture atlas.
 * This atlas is then used to build a texture image,
 * which is then uploaded to the GPU and used by
 * texture sampler.
 *
 * This class is responsible for managing textures.
 *
 * Textures have an ID and path.
 */

#include <unordered_map>
#include <cstdint>
#include "texture_atlas.h"
#include "texture.h"

namespace vox {
    class TextureManager {
    private:
        std::unordered_map<uint32_t, TextureAtlas> atlases = {};
        std::unordered_map<uint32_t, Texture> textures = {};

    public:
        TextureManager() = default;

        TextureManager(const TextureManager& other) = delete;

        TextureManager(TextureManager&& other) noexcept = delete;

        TextureManager& operator=(const TextureManager& other) = delete;

        TextureManager& operator=(TextureManager&& other) = delete;

        ~TextureManager() = default;

        Texture &getTexture(uint32_t textureId);

        TextureAtlas &getAtlas(uint32_t atlasId);

        void addTexture(uint32_t textureId, Texture texture);

        void addAtlas(uint32_t atlasId, TextureAtlas atlas);

        void removeTexture(uint32_t textureId);

        void removeAtlas(uint32_t atlasId);

        void loadAll();
    };
}

#endif
