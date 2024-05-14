#ifndef MODEL_H
#define MODEL_H

#include <filesystem>
#include <string>
#include <vector>

#include "../vertex/vertex.h"
#include "../mesh/mesh.h"
#include "../texture/texture.h"

namespace vox {
    class Model {
        std::string id;

        std::filesystem::path path;

        std::vector<vox::Mesh> meshes;
        std::vector<vox::Texture> textures;

    public:
        Model() = default;

        [[nodiscard]] Model(std::string id, std::filesystem::path path)
                : id(std::move(id)),
                  path(std::move(path)) {
        }

        void load();

        void upload(std::vector<vox::Vertex>* vertices, std::vector<uint32_t>* indices);

        std::string getId();

        std::filesystem::path getPath();

        [[nodiscard]] const std::vector<Mesh>& getMeshes() const;

        [[nodiscard]] const std::vector<Texture>& getTextures() const;

    private:
        void addMesh(Mesh&& mesh);
        void addMesh(const Mesh& mesh);

        void removeMesh(const Mesh &mesh);
    };
}

#endif
