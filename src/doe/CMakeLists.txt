
set(CMAKE_VERBOSE_MAKEFILE ON)

set(srclist  st_full_search st_random st_scrambled_doe st_box_behnken st_full_factorial st_neighbor        st_random_effect st_ccd_fc st_full_factorial_extended st_plackett_burman st_replicate) 

# st_fractional_factorial excluded as it uses a different API (?!)

foreach(x ${srclist})
        add_library(${x} SHARED lib${x}.cc)
        target_include_directories(${x} PRIVATE .. PRIVATE "${PROJECT_BINARY_DIR}/src")
        install(TARGETS ${x} LIBRARY DESTINATION lib)
endforeach()

