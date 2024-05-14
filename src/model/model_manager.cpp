#include "model_manager.h"

#include <fstream>
#include <iostream>

vox::Model &vox::ModelManager::get(const std::string &name) {
    return models[name];
}

std::unordered_map<std::string, vox::Model> &vox::ModelManager::getAll() {
    return models;
}

void vox::ModelManager::add(const std::string &name, vox::Model model) {
    models.emplace(name, std::move(model));
}

void vox::ModelManager::remove(const std::string &name) {
    models.erase(name);
}

void vox::ModelManager::loadAll() {
    const std::filesystem::directory_iterator modelIterator("models");

    for (const auto& metadataEntry : modelIterator) {
        if (metadataEntry.path().extension() == ".obj") {
            std::ifstream file(metadataEntry.path());

            if (!file.is_open()) {
                throw std::runtime_error("[Vulkan] Failed to load model file: " + metadataEntry.path().filename().string() + "\n");
            }


            std::string id = metadataEntry.path().stem().string();
            auto model = Model(id, metadataEntry.path());
            model.load();

            std::cout << "[Vulkan] Loaded model file: " << id << ".json\n" << std::flush;

            models.emplace(id, model);
        }
    }
}
