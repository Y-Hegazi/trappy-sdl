cmake_minimum_required(VERSION 3.16)
project(RageBaitGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(TINYXML2 REQUIRED tinyxml2)

# Collect source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${TINYXML2_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${TINYXML2_LIBRARIES}
)

# Compiler-specific options
target_compile_options(${PROJECT_NAME} PRIVATE
    ${SDL2_CFLAGS_OTHER}
    ${SDL2_IMAGE_CFLAGS_OTHER}
    ${SDL2_MIXER_CFLAGS_OTHER}
    ${SDL2_TTF_CFLAGS_OTHER}
    ${TINYXML2_CFLAGS_OTHER}
)

# Windows-specific settings
if(WIN32)
    # Copy SDL2 DLLs to output directory on Windows
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/Debug)
    else()
        set(OUTPUT_DIR ${CMAKE_BINARY_DIR}/Release)
    endif()
    
    # Set console subsystem for debugging
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
endif()

# macOS-specific settings
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()

# Copy resources to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources 
     DESTINATION ${CMAKE_BINARY_DIR})

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources/
    DESTINATION bin/resources
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "RageBaitGame")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SDL2 Platform Game")
set(CPACK_PACKAGE_VENDOR "Y-Hegazi")

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
