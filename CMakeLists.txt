cmake_minimum_required(VERSION 3.28)
project(Vox)

set(CMAKE_CXX_STANDARD 26)
set(PROJECT_NAME Vox)
set(SOURCE_DIRECTORY "src")

add_executable(${PROJECT_NAME}
        "${SOURCE_DIRECTORY}/main.cpp"
        "${SOURCE_DIRECTORY}/misc/util.cpp"
        "${SOURCE_DIRECTORY}/misc/util.h"
        "${SOURCE_DIRECTORY}/vertex/vertex.cpp"
        "${SOURCE_DIRECTORY}/vertex/vertex.h"
        "${SOURCE_DIRECTORY}/shader/shader.cpp"
        "${SOURCE_DIRECTORY}/shader/shader.h"
        "${SOURCE_DIRECTORY}/application/application.cpp"
        "${SOURCE_DIRECTORY}/application/application.h"
        "${SOURCE_DIRECTORY}/misc/constants.cpp"
        "${SOURCE_DIRECTORY}/misc/constants.h"
        "${SOURCE_DIRECTORY}/camera/camera.cpp"
        "${SOURCE_DIRECTORY}/camera/camera.h"
        "${SOURCE_DIRECTORY}/model/model.cpp"
        "${SOURCE_DIRECTORY}/model/model.h"
        "${SOURCE_DIRECTORY}/mesh/mesh.cpp"
        "${SOURCE_DIRECTORY}/mesh/mesh.h"
        "${SOURCE_DIRECTORY}/texture/texture.cpp"
        "${SOURCE_DIRECTORY}/texture/texture.h"
        "${SOURCE_DIRECTORY}/material/material.cpp"
        "${SOURCE_DIRECTORY}/material/material.h"
        "${SOURCE_DIRECTORY}/texture/texture_manager.cpp"
        "${SOURCE_DIRECTORY}/texture/texture_manager.h"
        "${SOURCE_DIRECTORY}/texture/texture_atlas.cpp"
        "${SOURCE_DIRECTORY}/texture/texture_atlas.h"
        "${SOURCE_DIRECTORY}/model/model_manager.cpp"
        "${SOURCE_DIRECTORY}/model/model_manager.h"
        "${SOURCE_DIRECTORY}/shader/shader_manager.cpp"
        "${SOURCE_DIRECTORY}/shader/shader_manager.h"
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/shaders" "shaders"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/textures" "textures"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/models" "models"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/fonts" "fonts"
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