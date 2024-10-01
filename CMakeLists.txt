cmake_minimum_required(VERSION 3.10)

project(StockEstimator VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Find LWS
find_package(libwebsockets REQUIRED)
include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})

include_directories("${PROJECT_SOURCE_DIR}/include")

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.c")

add_executable(main ${SOURCES})

#Link with LWS
target_link_libraries(main ${LIBWEBSOCKETS_LIBRARIES})
target_link_libraries(main m)
target_compile_options(main PRIVATE -O3 -Wall -Wextra -lssl)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
