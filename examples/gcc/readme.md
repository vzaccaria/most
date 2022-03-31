# Exploration of GCC Optimization parameters
In this example, the goal is to explore the effect of the
basic GCC options used for compiling a target application. The metrics of
interest in such a case will be the time needed for the compilation process, size of the generated binary file and the time needed for the execution of the compiled application.  

## Design space definition
Regarding the definition of the design space, two parameters (optimization `par_opt` and debug `par_dbg` options) are explored:

- Optimization options parameter values:

  - “-O”: The compiler tries to reduce code size and execution time, without
    performing any optimizations that take a great deal of compilation time;

  - “-O0”: Reduce compilation time and make debugging produce the expected
    results. This is the default during GCC compilation;

  - “-O1”: Optimizing compilation takes somewhat more time, and a lot more
    memory for a large function;

  - “-O2”: GCC performs nearly all supported optimizations that do not involve a
    space-speed tradeoff. As compared to -O, this option usually increases both
    compilation time and the performance of the generated code;

  - “-O3”: Optimize yet more. -O3 turns on all optimizations specified by -O2
    and also turns on the -finline-functions, -funswitch-loops,
    -fpredictive-commoning, -fgcse-after-reload, -ftree-vectorize and
    -fipa-cp-clone options;

  - “-Os”: Optimize for size. -Os enables all -O2 optimizations that do not
    typically increase code size. It also performs further optimizations
    designed to reduce code size.

- Debug options parameter values:

  - “-g”: Produce debugging information in the operating system's native format
    (stabs, COFF, XCOFF, or DWARF 2). GDB can work with this debugging
    information. GCC allows you to use -g with -O. The shortcuts taken by
    optimized code may occasionally produce surprising results: some variables
    you declared may not exist at all; flow of control may briefly move where
    you did not expect it; some statements may not be executed because they
    compute constant results or their values were already at hand; some
    statements may execute in different places because they were moved out of
    loops.

  - “ ”: None. The debugging options are not enabled.

The __metrics__ contained with the system model are:

  - Compilation Time: Time needed only for the compilation process;

  - Execution Time: Time needed only to execute the compiled target application;

  - Code Size: Size of the generated binary file.  

The XML file that encodes this design space for MOST is *gcc_ds.xml*.  
For convenience, we provide also a test C program (*linpack.c*) that can be compiled with gcc and can be used for benchmarking in this example. ([Reference](https://github.com/ereyes01/linpack))   

## Interfacing with MOST
Regarding the creation of a compliant interface with the **MOST** framework, we wrapped the GCC compilation process and the evaluation of the metrics within a python script (*gcc_most.py*), which is intended to be called as follows:  

```shell
python gcc_most.py --file=<target_filename>.c  \
      --xml_system_configuration=<input_filename>.xml \
      --xml_system_metrics=<output_filename>.xml
```
## Launch a full DSE with MOST
The script *gcc_full_dse.scr* contains the commands to perform a full search with *MOST*, evaulating the *execution time* and *code size* as objectives of the optimization, while the compilation time is used only as an additional metric for analysis purposes.  
This exploration can be launched with the following command:  

    most -x gcc_ds.xml -f gcc_full_dse.scr  
Befaure launching the example, please modify the *simulator_executable path* in *gcc_full_dse.xml*.