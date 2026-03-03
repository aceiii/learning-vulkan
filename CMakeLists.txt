cmake_minimum_required(VERSION 3.15)

project(learning-vulkan VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCE_FILES
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(Vulkan REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
