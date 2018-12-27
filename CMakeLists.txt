cmake_minimum_required(VERSION 3.8.1)
project(graphene-assimp)
include_directories("${PROJECT_SOURCE_DIR}/include")
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_library(graphene-assimp
    "src/assimp.cpp"
)
target_compile_features(graphene-assimp PUBLIC cxx_std_17)
target_link_libraries(graphene-assimp CONAN_PKG::graphene CONAN_PKG::Assimp "stdc++fs")
