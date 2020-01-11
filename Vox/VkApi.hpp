#pragma once
#include "VkApp.hpp"
#include "VkUtils.hpp"
#include "glm/glm.hpp"

class VkApi {
public:
	VkApi();

	VkApp* vkApp;

	void putTriangle(VkUtils::VkTriangle vkTriangle);
};