cmake_minimum_required(VERSION 3.20)
project(SdlExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# If using vcpkg, add this in CLion CMake options:
# -DCMAKE_TOOLCHAIN_FILE=`C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`

find_package(SDL2 CONFIG REQUIRED)

add_executable(SdlExample main.cpp)

target_link_libraries(SdlExample PRIVATE SDL2::SDL2 SDL2::SDL2main)
target_include_directories(SdlExample PRIVATE ${SDL2_INCLUDE_DIRS})