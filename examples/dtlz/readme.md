# DTLZ1 Optimization problem Example  
A box-constrained continuous n-dimensional n-objective problem with a linear Pareto-optimal front. [More details here](https://pymoo.org/problems/many/dtlz.html)  
In this example, *n* = 3: the design space has 3 dimensions (*x1, x2, x3*) and 3 objectives (*f1, f2, f3*) to minimize.

## Integer representation of the design space
While the original DTLZ1 problem is based on a continuous design space (in which *x1, x2, x3* can take real values in the interval [0, 1]), in this example we provide an integer formulation of the design space (in the file *dtlz_dse.xml*): the three variables (*x1, x2, x3*) are represented as integers whose values can span between 0 and 10.  

The python script *dtlz.py* is the *Use Case Simulator* : it parses the values of *x1, x2, x3* from the XML produced by *MOST*, converts them to *float* values in the interval [0, 1] (by dividing them by 10), computes the output metrics *f1, f2, f3* and writes them to the output XML.

## Launch a full (integer) search
The script *dtlz_fullsearch.scr* implements a simple full search strategy on the integer design space, and performs pareto filtering on the final results.  
It can be launched with the following command:  

    most -x dtlz_dse.xml -f dtlz_fullsearch.scr  
Befaure launching the example, please modify the *simulator_executable path* in *dtlz_dse.xml*.  