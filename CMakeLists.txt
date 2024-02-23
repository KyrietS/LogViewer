cmake_minimum_required(VERSION 3.15)
include(FetchContent)

FetchContent_Declare(
        fltk
        GIT_REPOSITORY https://github.com/fltk/fltk.git
        GIT_TAG 957becbe2836d1bc4a8be76b7ca4abb445a181f3
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT LogAlligator)

project(LogAlligator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FLTK_BUILD_FLUID OFF)
set(FLTK_BUILD_FLTK_OPTIONS OFF)
set(FLTK_BUILD_TEST OFF)
FetchContent_MakeAvailable(
        fltk
)

file(GLOB_RECURSE SOURCES "src/**.cpp" "src/**.hpp")
add_executable(LogAlligator ${SOURCES})
target_include_directories(LogAlligator PRIVATE src)
target_include_directories(LogAlligator PRIVATE ${fltk_SOURCE_DIR})
target_include_directories(LogAlligator PRIVATE ${fltk_BINARY_DIR})
target_link_libraries(LogAlligator fltk)

if (MSVC)
    target_compile_options(LogAlligator PRIVATE /W4)
else ()
    target_compile_options(LogAlligator PRIVATE -Wall -Wextra -pedantic)
endif ()

# Move all FLTK targets to the Dependencies folder
set(FLTK_TARGETS fltk fltk_images fltk_jpeg fltk_png fltk_z fltk_forms fltk_gl uninstall)
foreach (fltk_target ${FLTK_TARGETS})
    if (TARGET ${fltk_target})
        set_target_properties(${fltk_target} PROPERTIES FOLDER "Dependencies")
    endif ()
endforeach ()

# Copy pan-tadeusz.txt to binary folder after compiling
add_custom_command(TARGET LogAlligator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/pan-tadeusz.txt
        $<TARGET_FILE_DIR:LogAlligator>)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pan-tadeusz.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
