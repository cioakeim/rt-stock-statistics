cmake_minimum_required(VERSION 3.30)

project(RPiStockEstimator)

set(TOOLCHAIN_PATH "${PROJECT_SOURCE_DIR}/toolchain.cmake")

set(OPENSSL_ROOT_DIR "${PROJECT_SOURCE_DIR}/libroot")  # Replace with OpenSSL path
set(LIBWEBSOCKETS_ROOT_DIR "${PROJECT_SOURCE_DIR}/libroot")  # Replace with libwebsockets path


set(CMAKE_FIND_LIBRARY_SUFFIXES .a)   # Use only static libraries (.a)
set(BUILD_SHARED_LIBS OFF)            # Disable dynamic linking

# Specify the exact static library files
set(OPENSSL_LIBRARIES ${OPENSSL_ROOT_DIR}/lib/libssl.a ${OPENSSL_ROOT_DIR}/lib/libcrypto.a)
set(LIBWEBSOCKETS_LIBRARIES ${LIBWEBSOCKETS_ROOT_DIR}/lib/libwebsockets.a)


include_directories(${OPENSSL_ROOT_DIR}/include ${LIBWEBSOCKETS_ROOT_DIR}/include)

include_directories("${PROJECT_SOURCE_DIR}/../include")

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/../src/*.c")

add_executable(main ${SOURCES})

target_link_libraries(main
    ${LIBWEBSOCKETS_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    m
)
target_compile_options(main PRIVATE -O3 -Wall -Wextra)




