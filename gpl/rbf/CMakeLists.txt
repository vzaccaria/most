set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(rbf_interpolator bvp.cpp ddm.cpp diff_op.cpp error.cpp linalg.cpp utils.cpp func.cpp interpolator.cpp  interpolatorRBF.cpp  rbf.cpp ../gpl_xdr_api.cc)

target_link_libraries(rbf_interpolator ${GSL_LIBRARIES})

install(TARGETS rbf_interpolator DESTINATION bin)


