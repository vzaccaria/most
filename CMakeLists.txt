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
set(CMAKE_INSTALL_PREFIX "/opt" CACHE PATH "..." FORCE)

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
add_subdirectory(tests/testx1)
add_subdirectory(tests/testx2)
add_subdirectory(tests/testx3)
add_subdirectory(tests/testx4)
add_subdirectory(tests/testx5)
add_subdirectory(examples/gcc)
add_subdirectory(validation)
add_subdirectory(schemas)

file(COPY packages DESTINATION .)
file(COPY examples DESTINATION .)

# Installation

install(DIRECTORY packages DESTINATION .)
install(DIRECTORY ${PROJECT_BINARY_DIR}/examples/gcc DESTINATION examples)
install(DIRECTORY schemas DESTINATION .)


# if doing cmake 
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

# Testing
enable_testing()
find_program (BASH_PROGRAM bash)
if (BASH_PROGRAM)
        add_test (most-lang-1 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test1)
        add_test (most-lang-2 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test2)
        add_test (most-w-db ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test3 -d test3_output_db.txt)
        add_test (most-rw-db-1 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test4)
        add_test (most-rw-db-2 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test8 -d out.db -c)
        add_test (most-lang-3 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test5)
        add_test (most-lang-4 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test6)
        add_test (most-lang-5 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test7)
        add_test (most-lang-6 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test9)
        add_test (most-mpi ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test10 -m -d par.db)
        add_test (most-pareto-1 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test12)
        add_test (most-lang-7 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test13)
        add_test (most-lang-8 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test15 -p)
        add_test (most-lang-9 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test16 -p)
        add_test (most-lang-10 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t test17 -p)
        add_test (most-xml-1 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t testx1 -x)
        add_test (most-xml-2 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t testx2 -x)
        add_test (most-xml-3 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t testx3 -x)
        add_test (most-xml-4 ${CMAKE_BINARY_DIR}/tests/most_test.sh -t testx4 -x)
endif (BASH_PROGRAM)

