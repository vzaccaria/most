# see https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1

cmake_minimum_required(VERSION 3.15)
project(most LANGUAGES CXX VERSION 1.2)
set(CMAKE_CXX_STANDARD 11 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# For integration with clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# For additional cmake macros
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# Build structure
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(LibXml2 REQUIRED)
find_package(Readline REQUIRED)
find_package(MPI REQUIRED
             COMPONENTS CXX)
find_package(Threads REQUIRED)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GSL REQUIRED)


include_directories(${LIBXML2_INCLUDE_DIR} ${MPI_INCLUDE_PATH} ${READLINE_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${GSL_INCLUDE_DIRS})

add_subdirectory(src)
add_subdirectory(src/optimizers)
add_subdirectory(src/doe)
add_subdirectory(src/drivers)
add_subdirectory(gpl)
add_subdirectory(gpl/nn)
add_subdirectory(gpl/rbf)
add_subdirectory(gpl/splines/src)
add_subdirectory(tests)
add_subdirectory(tests/xml)
add_subdirectory(validation)
add_subdirectory(schemas)

file(COPY packages DESTINATION .)
file(COPY examples DESTINATION .)

# Installation

set(CMAKE_INSTALL_PREFIX "/opt" CACHE PATH "..." FORCE)
install(DIRECTORY packages DESTINATION .)
install(DIRECTORY examples DESTINATION .)


# if doing cmake 
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

