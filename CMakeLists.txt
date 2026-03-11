cmake_minimum_required(VERSION 3.16)

project(TunDevice VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

add_library(tunlib SHARED
    src/tun_device.cpp
    src/tun_c.cpp
)

add_library(TunDevice::tunlib ALIAS tunlib)

target_include_directories(tunlib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_options(tunlib PRIVATE -Wall -Wextra -Wpedantic)

set_target_properties(tunlib PROPERTIES
    OUTPUT_NAME tunlib
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

add_executable(tun_cpp_example examples/cpp_signal_safe_example.cpp)
target_link_libraries(tun_cpp_example PRIVATE tunlib)

add_executable(tun_c_example examples/c_signal_safe_example.c)
target_link_libraries(tun_c_example PRIVATE tunlib)

install(TARGETS tunlib tun_cpp_example tun_c_example
    EXPORT TunDeviceTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT TunDeviceTargets
    FILE TunDeviceTargets.cmake
    NAMESPACE TunDevice::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/TunDevice
)

find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating HTML documentation with Doxygen"
        VERBATIM
    )
else()
    add_custom_target(doc
        COMMAND ${CMAKE_COMMAND} -E echo "Doxygen not found: install doxygen to build docs"
    )
endif()

configure_file(cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
