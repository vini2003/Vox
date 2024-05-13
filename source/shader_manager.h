#ifndef VOX_SHADER_MANAGER_H
#define VOX_SHADER_MANAGER_H

#include "shader.h"

#include <unordered_map>
#include <string>

namespace vox {
    class ShaderManager {
    private:
        std::unordered_map<std::string, Shader<>> shaders = {};

    public:
        ShaderManager() = default;

        ShaderManager(const ShaderManager& other) = delete;

        ShaderManager(ShaderManager&& other) noexcept = delete;

        ShaderManager& operator=(const ShaderManager& other) = delete;

        ShaderManager& operator=(ShaderManager&& other) = delete;

        ~ShaderManager() = default;

        Shader<> &get(const std::string &name);

        void add(const std::string &name, Shader<> shader);

        void remove(const std::string &name);

        void loadAll();
    };
}


#endif
