//
// Created by vini2003 on 13/05/2024.
//

#include <ranges>
#include "texture_manager.h"
#include "texture_atlas.h"

#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void vox::TextureAtlas::addTexture(uint32_t textureId) {
    textureIds.push_back(textureId);
}

void vox::TextureAtlas::removeTexture(uint32_t textureId) {
    textureIds.erase(std::remove(textureIds.begin(), textureIds.end(), textureId), textureIds.end());
}

// This is a horizontal atlas, for now.
void vox::TextureAtlas::stitchTextures(TextureManager& textureManager) {
    int totalWidth = 0;
    int totalHeight = 0;

    const auto textures = textureIds | std::ranges::views::transform([&textureManager](uint32_t textureId) {
        return textureManager.getTexture(textureId);
    });

    for (const auto& texture : textures) {
        int width;
        int height;
        int channels;

        stbi_info(texture.getPath().c_str(), &width, &height, &channels);

        totalWidth = std::max(totalWidth, width);
        totalHeight = std::max(totalHeight, height);
    }

    totalWidth *= textures.size();

    std::vector<uint8_t> data(totalWidth * totalHeight * 4);

    int xOffset = 0;

    for (const auto& texture : textures) {
        int width;
        int height;
        int channels;

        stbi_uc* image = stbi_load(texture.getPath().c_str(), &width, &height, &channels, 4);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < 4; c++) {
                    data[(y * totalWidth + x + xOffset) * 4 + c] = image[(y * width + x) * 4 + c];
                }
            }
        }

        xOffset += width;

        stbi_image_free(image);
    }

    stbi_write_png("atlas.png", totalWidth, totalHeight, 4, data.data(), totalWidth * 4);
}
