cmake_minimum_required(VERSION 3.16)

project(TunDeviceWorkspace VERSION 1.0.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Keep generated artifacts under build/{bin,lib}
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(lib/LibTUN)

add_executable(tun_proxy_ui
    src/app.c
    src/proxy_client.c
)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    target_sources(tun_proxy_ui PRIVATE src/checksum_x86_64.S)
    target_compile_definitions(tun_proxy_ui PRIVATE TUNPROXY_HAVE_ASM_CHECKSUM=1)
else()
    target_sources(tun_proxy_ui PRIVATE src/checksum_fallback.c)
endif()

target_include_directories(tun_proxy_ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/LibTUN/include
)

target_link_libraries(tun_proxy_ui PRIVATE tunlib)

target_compile_options(tun_proxy_ui PRIVATE -Wall -Wextra -Wpedantic)

message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Binary output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Library output: ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
