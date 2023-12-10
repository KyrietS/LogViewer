cmake_minimum_required(VERSION 3.15)

project(LogAnnihilator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES "src/**.cpp" "src/**.hpp")
add_executable(LogAnnihilator ${SOURCES})
