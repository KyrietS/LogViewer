cmake_minimum_required(VERSION 3.15)
include(FetchContent)

FetchContent_Declare(
    fltk
    GIT_REPOSITORY https://github.com/fltk/fltk.git
    GIT_TAG        094ee9e953a7361093a2fa0cff90ee8ce6de8051
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT LogAnnihilator)

project(LogAnnihilator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FLTK_BUILD_FLUID OFF)
set(FLTK_BUILD_FLTK_OPTIONS OFF)
set(FLTK_BUILD_TEST OFF)
FetchContent_MakeAvailable(
    fltk
)

file(GLOB_RECURSE SOURCES "src/**.cpp" "src/**.hpp")
add_executable(LogAnnihilator ${SOURCES})
target_include_directories(LogAnnihilator PRIVATE src)
target_include_directories(LogAnnihilator PRIVATE ${fltk_SOURCE_DIR})
target_include_directories(LogAnnihilator PRIVATE ${fltk_BINARY_DIR})
target_link_libraries(LogAnnihilator fltk)

# Move all FLTK targets to the Dependencies folder
set(FLTK_TARGETS fltk fltk_images fltk_forms fltk_gl uninstall)
foreach(fltk_target ${FLTK_TARGETS})
    set_target_properties(${fltk_target} PROPERTIES FOLDER "Dependencies")
endforeach()

# Move all fltk_images dependencies to the Dependencies folder
get_target_property(dependencies fltk_images INTERFACE_LINK_LIBRARIES)
foreach(dependency ${dependencies})
    set_target_properties(${dependency} PROPERTIES FOLDER "Dependencies")
endforeach()
