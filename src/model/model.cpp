#include "model.h"

#include <tiny_obj_loader.h>

namespace vox {
    void Model::load() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

        for (const auto& [name, _mesh, lines, points] : shapes) {
            Mesh mesh = {};

            for (const auto& [vertexIndex, normalIndex, texCoordIndex] : _mesh.indices) {
                Vertex vertex = {};

                vertex.pos = {
                        attrib.vertices[3 * vertexIndex + 0],
                        attrib.vertices[3 * vertexIndex + 1],
                        attrib.vertices[3 * vertexIndex + 2]
                };

                vertex.texCoord = {
                        attrib.texcoords[2 * texCoordIndex + 0],
                        1.0f - attrib.texcoords[2 * texCoordIndex + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(mesh.getVertices().size());
                    mesh.getVertices().push_back(vertex);
                }

                mesh.getIndices().push_back(uniqueVertices[vertex]);
            }

            addMesh(std::move(mesh));
        }
    }

    void Model::upload(std::vector<Vertex> *vertices, std::vector<uint32_t> *indices) {
        for (auto& mesh : meshes) {
            vertices->insert(vertices->end(), mesh.getVertices().begin(), mesh.getVertices().end());
            indices->insert(indices->end(), mesh.getIndices().begin(), mesh.getIndices().end());
        }
    }

    std::string Model::getId() { return id; }

    std::filesystem::path Model::getPath() { return path; }

    const std::vector<Mesh> &Model::getMeshes() const {
        return meshes;
    }

    const std::vector<Texture> &Model::getTextures() const {
        return textures;
    }

    void Model::addMesh(Mesh &&mesh) {
        meshes.push_back(std::move(mesh));
    }

    void Model::addMesh(const Mesh &mesh) {
        meshes.push_back(mesh);
    }

    void Model::removeMesh(const Mesh& mesh) {
        meshes.erase(
                std::remove_if(meshes.begin(), meshes.end(),
                               [&mesh](const Mesh& m) { return &m == &mesh; }),
                meshes.end()
        );
    }
}