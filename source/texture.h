#ifndef VOX_TEXTURE_H
#define VOX_TEXTURE_H

#include <cstdint>
#include <string>
#include <utility>

namespace vox {
    class Texture {
        const uint32_t id;
        uint32_t atlasId;

        const std::string path;

        float minU = -1.0f;
        float minV = -1.0f;

        float maxU = -1.0f;
        float maxV = -1.0f;

        uint32_t width = -1;
        uint32_t height = -1;

    public:
        Texture() = delete;

        Texture(uint32_t id, std::string path)
                : id(id), path(std::move(path)) {
        }

        Texture(const Texture& other)
                : id(other.id), path(other.path) {
        }

        Texture(Texture&& other) noexcept
                : id(other.id), path(std::move(other.path)) {
        }

        Texture& operator=(const Texture& other) = delete;

        Texture& operator=(Texture&& other) = delete;

        [[nodiscard]] uint32_t getId() const;
        [[nodiscard]] uint32_t getAtlasId() const;

        [[nodiscard]] const std::string& getPath() const;

        [[nodiscard]] float getMinU() const;
        [[nodiscard]] float getMinV() const;

        [[nodiscard]] float getMaxU() const;
        [[nodiscard]] float getMaxV() const;

        [[nodiscard]] uint32_t getWidth() const;
        [[nodiscard]] uint32_t getHeight() const;

        void updateAfterLoaded(uint32_t width, uint32_t height) {
            this->width = width;
            this->height = height;
        }

        void updateAfterUploaded(uint32_t atlasId, float minU, float minV, float maxU, float maxV) {
            this->atlasId = atlasId;

            this->minU = minU;
            this->minV = minV;
            this->maxU = maxU;
            this->maxV = maxV;
        }
    };
}

#endif
