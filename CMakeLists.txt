cmake_minimum_required(VERSION 3.15)
include(FetchContent)

FetchContent_Declare(
        fltk
        GIT_REPOSITORY https://github.com/fltk/fltk.git
        GIT_TAG 957becbe2836d1bc4a8be76b7ca4abb445a181f3
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT LogViewer)

project(LogViewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FLTK_BUILD_FLUID OFF)
set(FLTK_BUILD_FLTK_OPTIONS OFF)
set(FLTK_BUILD_TEST OFF)
FetchContent_MakeAvailable(
        fltk
)

file(GLOB_RECURSE SOURCES "src/**.cpp" "src/**.hpp")
add_executable(LogViewer ${SOURCES})
target_include_directories(LogViewer PRIVATE src)
target_include_directories(LogViewer PRIVATE ${fltk_SOURCE_DIR})
target_include_directories(LogViewer PRIVATE ${fltk_BINARY_DIR})
target_link_libraries(LogViewer fltk)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src FILES ${SOURCES})

if (MSVC)
    target_compile_options(LogViewer PRIVATE /W4)
else ()
    target_compile_options(LogViewer PRIVATE -Wall -Wextra -pedantic)
endif ()

# Move all FLTK targets to the Dependencies folder
set(FLTK_TARGETS fltk fltk_images fltk_jpeg fltk_png fltk_z fltk_forms fltk_gl uninstall)
foreach (fltk_target ${FLTK_TARGETS})
    if (TARGET ${fltk_target})
        set_target_properties(${fltk_target} PROPERTIES FOLDER "Dependencies")
    endif ()
endforeach ()

# Copy pan-tadeusz.txt to binary folder after compiling
add_custom_command(TARGET LogViewer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/pan-tadeusz.txt
        $<TARGET_FILE_DIR:LogViewer>)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pan-tadeusz.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
