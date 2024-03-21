//
// Created by vini2003 on 21/03/2024.
//

#ifndef MODEL_H
#define MODEL_H
#include <filesystem>
#include <string>

#include "vertex.h"


class Model {
    std::string id;
    std::filesystem::path path;

public:
    Model() = default;

    [[nodiscard]] Model(std::string id, std::filesystem::path path)
        : id(std::move(id)),
          path(std::move(path)) {
    }

private:
    std::vector<vox::Vertex> vertices;
    std::vector<uint32_t> indices;

public:
    void load();
    void upload(std::vector<vox::Vertex>* vertices, std::vector<uint32_t>* indices);

    std::string getId();
    std::vector<vox::Vertex> getVertices();
    std::vector<uint32_t> getIndices();
};



#endif //MODEL_H
