#include <fstream>
#include "shader_manager.h"

void vox::ShaderManager::remove(const std::string &name) {
    shaders.erase(name);
}

void vox::ShaderManager::add(const std::string &name, vox::Shader<> shader) {
    shaders[name] = std::move(shader);
}

vox::Shader<> &vox::ShaderManager::get(const std::string &name) {
    return shaders[name];
}

std::unordered_map<std::string, vox::Shader<>> &vox::ShaderManager::getAll() {
    return shaders;
}

void vox::ShaderManager::loadAll() {
    std::map<std::string, ShaderMetadata> shaderMetadata;

    std::map<std::string, std::vector<char>> shaderVertexCode;
    std::map<std::string, std::vector<char>> shaderFragmentCode;

    const std::filesystem::directory_iterator shaderCodeIterator("shaders/spirv");

    for (const auto& shaderEntry : shaderCodeIterator) {
        if (shaderEntry.path().extension() == ".vert" || shaderEntry.path().extension() == ".frag") {
            std::ifstream shaderFile(shaderEntry.path(), std::ios::ate | std::ios::binary);

            if (!shaderFile.is_open()) {
                throw std::runtime_error("[Vulkan] Failed to load shader file: " + shaderEntry.path().filename().string() + "\n");
            }

            std::vector<char> buffer(shaderEntry.file_size());
            shaderFile.seekg(0);
            shaderFile.read(buffer.data(), shaderEntry.file_size());
            shaderFile.close();

            std::string id = shaderEntry.path().stem().string();

            if (shaderEntry.path().extension() == ".vert") {
                shaderVertexCode[id] = buffer;
            } else if (shaderEntry.path().extension() == ".frag") {
                shaderFragmentCode[id] = buffer;
            }

            std::cout << "[Vulkan] Loaded shader file: " << shaderEntry.path().filename().string() << "\n" << std::flush;
        }
    }

    const std::filesystem::directory_iterator shaderMetadataIterator("shaders/metadata");

    for (const auto& metadataEntry : shaderMetadataIterator) {
        if (metadataEntry.path().extension() == ".json") {
            std::ifstream metadataFile(metadataEntry.path());

            if (!metadataFile.is_open()) {
                throw std::runtime_error("[Vulkan] Failed to load shader metadata file: " + metadataEntry.path().filename().string() + "\n");
            }

            nlohmann::json metadata;
            metadataFile >> metadata;
            metadataFile.close();

            std::string id = metadataEntry.path().stem().string();
            shaderMetadata[id] = metadata.get<ShaderMetadata>();

            std::cout << "[Vulkan] Loaded shader metadata file: " << id << ".json\n" << std::flush;
        }
    }


    for (const auto& [id, metadata] : shaderMetadata) {
        shaders[id] = Shader<>(id, metadata, shaderVertexCode[metadata.vertex], shaderFragmentCode[metadata.fragment]);
    }
}
