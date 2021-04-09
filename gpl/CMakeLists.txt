set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(rand rand.cc gpl_xdr_api.cc)
add_executable(normal normal.cc gpl_xdr_api.cc)
add_executable(regressor regressor.cc gpl_xdr_api.cc)

target_link_libraries(rand ${GSL_LIBRARIES})
target_link_libraries(normal ${GSL_LIBRARIES})
target_link_libraries(regressor ${GSL_LIBRARIES})


target_include_directories(most PRIVATE "${PROJECT_BINARY_DIR}/gpl")

install(TARGETS rand DESTINATION bin)
install(TARGETS normal DESTINATION bin)
install(TARGETS regressor DESTINATION bin)


