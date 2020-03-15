cmake_minimum_required(VERSION 3.13.0)
project(Vox)

set(CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} main.cpp "main.cpp" "VkApi.cpp" "VkApp.cpp" "VkUtils.cpp")

target_include_directories(${PROJECT_NAME} PUBLIC
        "C:/Users/vini2003/vsprojects/Libraries/glm"
        "C:/VulkanSDK/1.1.130.0/Include"
        "C:/Users/vini2003/vsprojects/Libraries/glfw-3.3.2-x86/include")

target_link_directories(${PROJECT_NAME} PUBLIC
        "C:/VulkanSDK/1.1.130.0/Lib32;"
        "C:/Users/vini2003/vsprojects/Libraries/glfw-3.3.2-x86/lib-mingw")

set(ADDITIONAL_LIBRARY_DEPENDENCIES "gdi32" "glfw3" "vulkan-1")

target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/shaders shaders)