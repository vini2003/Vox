#ifndef VOX_MESH_H
#define VOX_MESH_H

#include <vector>
#include <cstdint>

#include "vertex.h"
#include "material.h"

namespace vox {
    class Mesh {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        Material material;

    public:
        Mesh() = default;

        [[nodiscard]] Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, Material material)
                : vertices(std::move(vertices)),
                  indices(std::move(indices)),
                  material(std::move(material)) {
        }

        [[nodiscard]] std::vector<Vertex>& getVertices();
        [[nodiscard]] std::vector<uint32_t>& getIndices();

        [[nodiscard]] Material& getMaterial();

        void setMaterial(Material material);
    };
}

#endif
