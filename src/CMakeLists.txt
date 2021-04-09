# git commit hash macro
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(config.h.in config.h)

set(CMAKE_VERBOSE_MAKEFILE ON)


flex_target(lexer st_tokens.ll "${CMAKE_CURRENT_BINARY_DIR}/st_tokens.cc")
bison_target(parser st_grammar.yy "${CMAKE_CURRENT_BINARY_DIR}/st_grammar.cc")

add_compile_definitions(__MOST_NO_LICENSE__)


add_executable(most st_arg.cc st_design_space.cc st_list.cc st_object_utils.cc st_rsm_help.cc st_ast.cc st_doe_utils.cc st_objectives_constraints.cc  st_shell.cc st_vector.cc st_commands.cc st_driver_utils.cc st_opt_utils.cc st_shell_command.cc st_xdr_api.cc st_common_utils.cc st_env.cc st_map.cc st_parser.cc st_shell_variables.cc st_companion_metrics.cc st_exception.cc st_point.cc  st_conv.cc st_gen_man.cc st_mpi_utils.cc  st_rand.cc st_signal.cc st_database.cc st_job_dispatcher.cc  st_object.cc st_rsm.cc st_sim_utils.cc st_lm_utils.cc st_lm.cc st_sign.cc st_tokens.cc st_grammar.cc) 

target_link_libraries(most "-rdynamic")

target_link_libraries(most ${CMAKE_DL_LIBS})
target_link_libraries(most ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(most ${LIBXML2_LIBRARIES})
target_link_libraries(most ${Readline_LIBRARY})
target_link_libraries(most MPI::MPI_CXX)
target_link_libraries(most ${OPENSSL_LIBRARIES})

message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")

target_include_directories(most PRIVATE .)
target_include_directories(most PRIVATE "${PROJECT_BINARY_DIR}/src")

install(TARGETS most DESTINATION bin)

