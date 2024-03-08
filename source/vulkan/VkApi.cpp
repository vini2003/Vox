#include "VkApi.hpp"
#include "VkUtils.hpp"
#include "glm/glm.hpp"

VkApi::VkApi() {

}

void VkApi::putTriangle(VkUtils::VkTriangle vkTriangle) {
	vkApp->vkApiPutTriangle(vkTriangle);
}