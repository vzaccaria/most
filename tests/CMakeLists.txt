# git commit hash macro

set(bdir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..")
set(sdir ${CMAKE_SOURCE_DIR})

# configure_file(do_tests.in do_tests)
configure_file(most_test.sh.in most_test.sh)

