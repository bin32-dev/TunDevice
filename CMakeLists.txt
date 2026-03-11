cmake_minimum_required(VERSION 3.10)

project(tunlib VERSION 1.0 LANGUAGES C CXX)

# ==========================
# Compiler Settings
# ==========================

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================
# Output directories
# ==========================

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# ==========================
# Include directories
# ==========================

include_directories(${PROJECT_SOURCE_DIR}/include)

# ==========================
# Source files
# ==========================

set(SOURCES
    src/tun_device.cpp
    src/tun_c.cpp
)

# ==========================
# Create shared library
# ==========================

add_library(tunlib SHARED ${SOURCES})

target_include_directories(tunlib
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# ==========================
# Library properties
# ==========================

set_target_properties(tunlib PROPERTIES

    OUTPUT_NAME "tunlib"

    VERSION ${PROJECT_VERSION}

    SOVERSION 1

    POSITION_INDEPENDENT_CODE ON
)

# ==========================
# Install rules
# ==========================

install(TARGETS tunlib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
)

# ==========================
# Custom clean target
# ==========================

add_custom_target(clean-all
    COMMAND rm -rf build lib bin
)