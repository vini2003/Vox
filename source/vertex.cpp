#include "vertex.h"
#include "util.h"

namespace vox {
    const std::array<VertexAttribute, 3> Vertex::attributes = {{
        {offsetof(vox::Vertex, pos), vox::toVkFormat<glm::vec3>()},
        {offsetof(vox::Vertex, color), vox::toVkFormat<glm::vec3>()},
        {offsetof(vox::Vertex, texCoord), vox::toVkFormat<glm::vec2>()}
    }};
}
