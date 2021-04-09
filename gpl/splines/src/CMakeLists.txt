set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(spline converter.cc  parameters.cc  r_wrapper.cc  rsm.cc  spline_script.cc ../../gpl_xdr_api.cc) 

# target_include_directories(rbf_interpolator PRIVATE ${Boost_INCLUDE_DIR})

# target_link_libraries(rbf_interpolator ${Boost_LIBRARIES} ${GSL_LIBRARIES})

install(TARGETS spline DESTINATION bin)


