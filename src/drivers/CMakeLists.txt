
set(CMAKE_VERBOSE_MAKEFILE ON)

set(srclist "st_generic" "st_sesc"  "st_test"  "st_xml")

foreach(x ${srclist})
        add_library(${x} SHARED lib${x}.cc)
        target_include_directories(${x} PRIVATE .. PRIVATE "${PROJECT_BINARY_DIR}/src")
        install(TARGETS ${x} LIBRARY DESTINATION lib)
endforeach()

