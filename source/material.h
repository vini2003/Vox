#ifndef VOX_MATERIAL_H
#define VOX_MATERIAL_H

#include <string>

#include <glm/glm.hpp>

namespace vox {
    class Material {
        std::string id = "default";

        glm::vec3 albedo = glm::vec3(0.5f);
        glm::vec3 emissivity = glm::vec3(0.5f);
        glm::vec3 reflectance = glm::vec3(0.5f);

        float roughness = 0.5f;

    public:
        Material() = default;

        Material(std::string id, glm::vec3 ambient, glm::vec3 diffuse, float shininess)
            : id(std::move(id)),
              albedo(ambient),
              emissivity(diffuse),
              roughness(shininess) {
        }

        [[nodiscard]] const std::string& getId() const;

        [[nodiscard]] const glm::vec3& getAlbedo() const;
        [[nodiscard]] const glm::vec3& getEmissivity() const;
        [[nodiscard]] const glm::vec3& getReflectance() const;

        [[nodiscard]] float getRoughness() const;
    };
}

#endif
