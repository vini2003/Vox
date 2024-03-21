//
// Created by vini2003 on 21/03/2024.
//

#include "model.h"

#include <tiny_obj_loader.h>

void Model::load() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<vox::Vertex, uint32_t> uniqueVertices = {};

    for (const auto& [name, mesh, lines, points] : shapes) {
        for (const auto& [vertexIndex, normalIndex, texCoordIndex] : mesh.indices) {
            vox::Vertex vertex = {};

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

            vertices.push_back(vertex);

            if (!uniqueVertices.contains(vertex)) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void Model::upload(std::vector<vox::Vertex> *vertices, std::vector<uint32_t> *indices) {
    vertices->insert(vertices->end(), this->vertices.begin(), this->vertices.end());
    indices->insert(indices->end(), this->indices.begin(), this->indices.end());
}

std::string Model::getId() { return id; }
std::vector<vox::Vertex> Model::getVertices() { return vertices; }
std::vector<uint32_t> Model::getIndices() { return indices; }
