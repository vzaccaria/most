
set(CMAKE_VERBOSE_MAKEFILE ON)

set(srclist "st_mosa" "st_nsgaii" "st_parallel_doe" "st_parallel_pareto_doe" "st_parallel_steepest_descent")

foreach(x ${srclist})
        add_library(${x} SHARED lib${x}.cc)
        target_include_directories(${x} PRIVATE .. PRIVATE "${PROJECT_BINARY_DIR}/src")
        install(TARGETS ${x} LIBRARY DESTINATION lib)
endforeach()

