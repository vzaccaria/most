set(CMAKE_VERBOSE_MAKEFILE ON)

find_package(OpenMP REQUIRED)


add_executable(nn cascade.cc ../gpl_xdr_api.cc)

target_link_libraries(nn /usr/local/lib/libfann.a OpenMP::OpenMP_CXX)

install(TARGETS nn DESTINATION bin)


