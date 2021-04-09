# git commit hash macro

set(bdir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..")
set(sdir ${CMAKE_SOURCE_DIR})

configure_file(do_tests.in do_tests)
configure_file(multicube_design_space_test.xml.in  multicube_design_space_test.xml)
configure_file(multicube_design_space_test_err.xml.in multicube_design_space_test_err.xml)
configure_file(multicube_design_space_test_R1.4.xml.in multicube_design_space_test_R1.4.xml)
configure_file(multicube_design_space_test_dump.xml.in multicube_design_space_test_dump.xml)

