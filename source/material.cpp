#include "material.h"

const std::string &vox::Material::getId() const {
    return id;
}

const glm::vec3 &vox::Material::getAlbedo() const {
    return albedo;
}

const glm::vec3 &vox::Material::getEmissivity() const {
    return emissivity;
}

const glm::vec3 &vox::Material::getReflectance() const {
    return reflectance;
}

float vox::Material::getRoughness() const {
    return roughness;
}
