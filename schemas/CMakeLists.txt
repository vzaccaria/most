# git commit hash macro

set(bdir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..")
set(sdir ${CMAKE_SOURCE_DIR})

configure_file(multicube_design_space_def.xsd multicube_design_space_def.xsd)
configure_file(multicube_simulator_interface.xsd multicube_simulator_interface.xsd)

