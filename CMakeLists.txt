cmake_minimum_required(VERSION 3.13.0)
project(Vox)

set(CMAKE_CXX_STANDARD 20)

set(GLM_VERSION glm)
set(GLFW_VERSION glfw-3.3.2-x86)
set(VULKAN_VERSION vulkan-sdk-1.1.130.0)

set(LIBRARY_DIRECTORY C:/Users/vini2003/vsprojects/Libraries)
set(VULKAN_DIRECTORY source/vulkan)
set(SOURCE_DIRECTORY source)

set(GDI_LIBRARY gdi32)
set(GLFW_LIBRARY glfw3)
set(VULKAN_LIBRARY vulkan-1)

add_executable(${PROJECT_NAME} ${SOURCE_DIRECTORY}/Main.cpp "${VULKAN_DIRECTORY}/VkApi.cpp"
              "${VULKAN_DIRECTORY}/VkApp.cpp"
              "${VULKAN_DIRECTORY}/VkUtils.cpp")

target_include_directories(${PROJECT_NAME} PUBLIC
                          "${LIBRARY_DIRECTORY}/${GLM_VERSION}"
                          "${LIBRARY_DIRECTORY}/${VULKAN_VERSION}/include"
                          "${LIBRARY_DIRECTORY}/${GLFW_VERSION}/include")

target_link_directories(${PROJECT_NAME} PUBLIC
                       "${LIBRARY_DIRECTORY}/${VULKAN_VERSION}/Lib32"
                       "${LIBRARY_DIRECTORY}/${GLFW_VERSION}/lib-mingw")

target_link_libraries(${PROJECT_NAME} PUBLIC
                      "${GDI_LIBRARY}"
                      "${GLFW_LIBRARY}"
                      "${VULKAN_LIBRARY}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders shaders)