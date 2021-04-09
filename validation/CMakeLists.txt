# git commit hash macro

set(bdir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/..")
set(sdir ${CMAKE_SOURCE_DIR})

configure_file(full_sesc_ro_rsm_validation.scr.in  full_sesc_ro_rsm_validation.scr)
configure_file(mosa_validation.scr.in    mosa_validation.scr) 
configure_file(respir_validation.scr.in  respir_validation.scr)
configure_file(validate_rsm.in validate_rsm)
configure_file(meth_design_space.scr.in            meth_design_space.scr)
configure_file(nsgaii_validation.scr.in  nsgaii_validation.scr)
configure_file(validate_optimizers.in validate_optimizers)

