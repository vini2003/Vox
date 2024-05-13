#ifndef VOX_MODEL_MANAGER_H
#define VOX_MODEL_MANAGER_H

#include "model.h"

#include <unordered_map>
#include <string>

namespace vox {
    class ModelManager {
    private:
        std::unordered_map<std::string, Model> models = {};

    public:
        ModelManager() = default;

        ModelManager(const ModelManager& other) = delete;

        ModelManager(ModelManager&& other) noexcept = delete;

        ModelManager& operator=(const ModelManager& other) = delete;

        ModelManager& operator=(ModelManager&& other) = delete;

        ~ModelManager() = default;

        Model &get(const std::string &name);

        void add(const std::string &name, Model model);

        void remove(const std::string &name);

        void loadAll();
    };
}


#endif
