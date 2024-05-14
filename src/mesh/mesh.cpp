#include "mesh.h"
#include "../vertex/vertex.h"

std::vector<vox::Vertex> &vox::Mesh::getVertices() {
    return vertices;
}

std::vector<uint32_t> &vox::Mesh::getIndices() {
    return indices;
}

vox::Material &vox::Mesh::getMaterial() {
    return material;
}

void vox::Mesh::setMaterial(vox::Material material) {
    this->material = std::move(material);
}
