//
// Created by vini2003 on 14/03/2024.
//

#include "vertex.h"

const std::array<vox::VertexAttribute, 3> vox::Vertex::attributes = {{
    {offsetof(vox::Vertex, pos), toVkFormat<glm::vec3>()},
    {offsetof(vox::Vertex, color), toVkFormat<glm::vec3>()},
    {offsetof(vox::Vertex, texCoord), toVkFormat<glm::vec2>()}
}};