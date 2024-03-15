cmake_minimum_required(VERSION 3.28)
project(Vox)

set(CMAKE_CXX_STANDARD 26)
set(PROJECT_NAME Vox)
set(SOURCE_DIRECTORY "source")

add_executable(${PROJECT_NAME} "${SOURCE_DIRECTORY}/main.cpp"
        "${SOURCE_DIRECTORY}/application.cpp"
        "${SOURCE_DIRECTORY}/application.h"
        "${SOURCE_DIRECTORY}/util.cpp"
        "${SOURCE_DIRECTORY}/util.h"
        "${SOURCE_DIRECTORY}/shaders.cpp"
        "${SOURCE_DIRECTORY}/shaders.h"
        source/vertex.cpp
        source/vertex.h
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/shaders shaders
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/textures textures
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/models models
)

find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${Vulkan_LIBRARIES})

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)

find_package(glm CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(Stb REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})

find_package(tinyobjloader CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE tinyobjloader::tinyobjloader)