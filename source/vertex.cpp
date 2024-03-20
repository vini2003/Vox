#include "vertex.h"
#include "util.h"

namespace vox {
    const std::array<VertexAttribute, 3> Vertex::attributes = {{
        {offsetof(Vertex, pos), toVkFormat<glm::vec3>()},
        {offsetof(Vertex, color), toVkFormat<glm::vec3>()},
        {offsetof(Vertex, texCoord), toVkFormat<glm::vec2>()}
    }};
}
