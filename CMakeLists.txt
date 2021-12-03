cmake_minimum_required(VERSION 3.15)
project(tsexplainC)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-O3 -g")
#set(CMAKE_CXX_FLAGS "-O1 -g")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpqxx -lpq -g")


include_directories(include)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "include/*.h")

add_executable(tsexplainC ${SOURCE_FILES})
