#include "application.h"

int main() {
	try {
		vox::Application application;
		application.run();
	} catch (const std::exception& exception) {
		std::cerr << exception.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}