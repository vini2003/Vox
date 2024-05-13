#include "texture_manager.h"

#include <filesystem>
#include <iostream>

vox::Texture &vox::TextureManager::getTexture(uint32_t textureId) {
    return textures.at(textureId);
}

vox::TextureAtlas &vox::TextureManager::getAtlas(uint32_t atlasId) {
    return atlases.at(atlasId);
}

void vox::TextureManager::addTexture(uint32_t textureId, vox::Texture texture) {
    textures.emplace(textureId, std::move(texture));
}

void vox::TextureManager::addAtlas(uint32_t atlasId, vox::TextureAtlas atlas) {
    atlases.emplace(atlasId, std::move(atlas));
}

void vox::TextureManager::removeTexture(uint32_t textureId) {
    textures.erase(textureId);
}

void vox::TextureManager::removeAtlas(uint32_t atlasId) {
    atlases.erase(atlasId);
}

void vox::TextureManager::loadAll() {
    const std::filesystem::directory_iterator textureIterator("textures");

    for (const auto& textureEntry : textureIterator) {
        if (textureEntry.path().extension() == ".png") {
            auto texture = Texture(textures.size() + 1, textureEntry.path().string());

            std::cout << "[Vulkan] Loaded texture file: " << textureEntry.path().stem() << ".png\n" << std::flush;

            textures.emplace(texture.getId(), std::move(texture));
        }
    }
}
