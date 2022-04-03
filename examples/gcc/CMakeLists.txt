# git commit hash macro

set(bdir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..")
set(idir "${CMAKE_INSTALL_PREFIX}")
set(sdir ${CMAKE_SOURCE_DIR})

# configure_file(do_tests.in do_tests)
configure_file(gcc_ds.xml.in gcc_ds.xml)

# install(FILES gcc_most.py DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gcc)
# install(FILES linpack.c DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gcc)
# install(FILES ../../build/examples/gcc/gcc_ds.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/examples/gcc)


