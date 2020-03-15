#include "vulkan/VkApp.hpp"

int main() {
	VkApp application;

	try {
		application.run();
	} catch (const std::exception& exception) {
		std::cerr << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}