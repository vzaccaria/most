# db_clear 
## OVERVIEW
 Delete the contents of a database.
## SYNOPSYS
`db_clear DBNAME`
## DESCRIPTION
Deletes the contents of database `DBNAME`.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_compute_ADRS 
## OVERVIEW
 Compute the ADRS of a database with respect to a reference.
## SYNOPSYS
`db_compute_ADRS DB [ options ] `
## DESCRIPTION
Compute the Average Distance From Reference Set between the database `REFERENCE` (ideally an exact Pareto database) and database `DB`. Note that the objectives should be set before calling this command.
## OPTIONS


* `--reference`=`REFERENCE`. 

 `REFERENCE` is an ideal pareto front database
## RETURN VALUE
On success it returns the ADRS, `false` otherwise (i.e., whenever the databases do not exist or REFERENCE is empty).
# db_compute_corr 
## OVERVIEW
 Compute a correlation between parameters and metrics
## SYNOPSYS
`db_compute_corr DB`
## DESCRIPTION
Compute correlation for database `DB`.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_compute_coverage 
## OVERVIEW
 Computes the coverage of two databases.
## SYNOPSYS
`db_compute_coverage DB [ options ] `
## DESCRIPTION
Compute the coverage of database `REFERENCE` over database `DB`. Note that the objectives should be set before calling this command.
## OPTIONS


* `--reference`=`REFERENCE`. 

 `REFERENCE` is an ideal pareto front database


* `--count`=`BOOL`. 

 Instead of computing pure coverage, forces counting how many points of `DB` are in REFERENCE
## RETURN VALUE
Returns the coverage as a float number between 0 and 1. Lower is better.
# db_compute_distance 
## OVERVIEW
 Compute the euclidean distance
## SYNOPSYS
`db_compute_distance DB [ options ] `
## DESCRIPTION
Compute the average euclidean distance between the `REFERENCE` database (ideally a full Pareto db) and database `DB`. Note that the objectives should be set before calling this command.
## OPTIONS


* `--reference`=`REFERENCE`. 

 `REFERENCE` is an ideal pareto front database
## RETURN VALUE
On success it returns the euclidean distance, `false` otherwise (i.e., whenever the databases do not exist or REFERENCE is empty).
# db_compute_kmeans_clusters 
## OVERVIEW
 Creates clusters of points within a database
## SYNOPSYS
`db_compute_kmeans_clusters DB [ options ] `
## DESCRIPTION
Clusters points of database `DB` based on the objectives previously specified.
## OPTIONS


* `--clusters`=`K`. 

 `K` is the number of clusters


* `--iterations`=`N`. 

 `N` is the number of iterations used by the clustering algorithm
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_compute_median_distance 
## OVERVIEW
 Computes the median distance
## SYNOPSYS
`db_compute_median_distance DB [ options ] `
## DESCRIPTION
Compute the median distance between the database named `REFERENCE` (ideally a full pareto db) and database `DB`. Note that the objectives should be set before calling this command.
## OPTIONS


* `--reference`=`REFERENCE`. 

 `REFERENCE` is an ideal pareto front database
## RETURN VALUE
On success it returns the euclidean distance, `false` otherwise (i.e., whenever the databases do not exist or REFERENCE is empty).
# db_copy 
## OVERVIEW
 Copy or merge a database.
## SYNOPSYS
`db_copy SRC [ options ] `
## DESCRIPTION
Copy a database `SRC` into the destination database `DST`. If `--attach` is set to true, it merges `SRC` and `DST` into `DST`.
## OPTIONS


* `--destination`=`DST`. 

 `DST` is an existing destination database. Note that `DST` will be erased (if attach not specified).


* `--attach`=`BOOL`. 

 If `true`, the destination database is not erased and SRC is copied in it
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_create 
## OVERVIEW
 Creates a new, empty database
## SYNOPSYS
`db_create DB_NAME`
## DESCRIPTION
Create a design point database named `DB_NAME` in memory. Note, on-line databases are volatile; to preserve the data please use the db_write command.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_export 
## OVERVIEW
 Export a database
## SYNOPSYS
`db_export DB [ options ] `
## DESCRIPTION
Export `DB` into a csv file named `NAME`.
## OPTIONS


* `--mode_frontier`=`BOOL`. 

 BOOL is true if exporting for mode_frontier


* `--data_tank`=`BOOL`. 

 BOOL is true if exporting to data_tank


* `--file_name`=`NAME`. 

 NAME is the name of the output file
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_export_xml 
## OVERVIEW
 Export a database
## SYNOPSYS
`db_export_xml DB [ options ] `
## DESCRIPTION
Export `DB` into a csv file named `NAME`.
## OPTIONS


* `--objectives`=`BOOL`. 

 BOOL if exporting objectives instead of metrics


* `--file_name`=`NAME`. 

 NAME is the name of the output file
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_filter_cluster 
## OVERVIEW
 Eliminates points not belonging to a cluster.
## SYNOPSYS
`db_filter_cluster DB`
## DESCRIPTION
Eliminates from `DB` the points that do not belong to cluster `K`.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_filter_pareto 
## OVERVIEW
 Filter dominated points.
## SYNOPSYS
`db_filter_pareto DB [ options ] `
## DESCRIPTION
Filter database `DB` for Pareto points. Points which violate the constraints are still kept in the database if the `--valid` option is not used or `false`. Objectives should be previously specified. Pareto ranking can be specified with the `--level` option.
## OPTIONS


* `--level`=`N`. 

 N is the level of depth of the pareto curve


* `--valid`=`BOOL`. 

 `true` if only valid points should be considered
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_filter_valid 
## OVERVIEW
 Eliminates invalid points.
## SYNOPSYS
`db_filter_valid DB`
## DESCRIPTION
Eliminates from database `DB` the points that have errors.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_import 
## OVERVIEW
 Import a .csv file into a database
## SYNOPSYS
`db_import DB [ options ] `
## DESCRIPTION
Import a .csv file named `NAME` into `DB`.
## OPTIONS


* `--file_name`=`NAME`. 

 NAME is the name of the file to import


* `--use_symbols`=`BOOL`. 

 BOOL is true if data in the .csv file represents symbols
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_normalize 
## OVERVIEW
 Equalize two databases.
## SYNOPSYS
`db_normalize DBR [ options ] `
## DESCRIPTION
Filters the input databases `DBR` and the database `DB`) in order to have the same points.
## OPTIONS


* `--with`=`DB`. 

 `DBR` is the reference database for computing normalization


* `--valid`=`BOOL`. 

 `true` if only valid points should be retained in the databases
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_plot 
## OVERVIEW
 Creates a plot from a database
## SYNOPSYS
`db_plot DB [ options ] `
## DESCRIPTION
Plot a graph of database `DB` for given metrics. `DB` can be also a list of database names
## OPTIONS


* `--xaxis`=`XMETRIC`. 

 XMETRIC is a feasible metric of the design


* `--yaxis`=`YMETRIC`. 

 YMETRIC is a feasible metric of the design


* `--zaxis`=`ZMETRIC`. 

 ZMETRIC is a feasible metric of the design. This is optional.


* `--output`=`FILENAME`. 

 FILENAME is the prefix of the postscript file. Optional


* `--clusters`=`NCLUS`. 

 NCLUS is the number of clusters to be displayed


* `--color_violating`=`BOOL`. 

 BOOL is true if constraint violators should be highlighted


* `--key`=`KEY`. 

 String indicating the position of the legend


* `--onepage`=`BOOL`. 

 TRUE if plot should be in a single page


* `--bubble`=`BOOL`. 

 TRUE if plotting a bubble plot instead of a 3D plot


* `--plevel`=`par`. 

 highlights different levels of `par`


* `--override`=`FILE`. 

 Use `FILE` to override default options
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_plot_vector 
## OVERVIEW
 Creates a scatter plot from a given set of vectors or a box-plot from a list of maps.
## SYNOPSYS
`db_plot_vector vec [ options ] `
## DESCRIPTION
Plot a scatter graph for `vec`. `vec` should be composed by a list of \[ `v1, v2, name`\]  vectors, where `v1` contains the x-axis coords, `v2` contains the y-axis coords and `name` is used as a label for the corresponding (x,y) points. It is possible to plot a box-plot graph for `vec`, where `vec` is a list of maps with the following keys: "x", "sample_min", "Q1", "Med", "Q3", "sample_max" and "outliers". The object related to "outliers" must be a vector of numbers, while the objects related to the remaining keys must be numbers.
## OPTIONS


* `--xaxis`=`xmetric`. 

 `xmetric` is the label to be associated to the x-axis


* `--yaxis`=`ymetric`. 

 `ymetric` is the label to be associated to the y-axis


* `--output`=`filename`. 

 `filename` is the prefix of the postscript file (optional)


* `--key`=`key`. 

 `key` indicates the position of the legend within the plot


* `--onepage`=`cond`. 

 `cond` is `true` if the plot should be put in a single page


* `--box`=`cond`. 

 `cond` is `true` if the desired plot is box-plot


* `--yrange`=`y_axis_range`. 

 `y_axis_range` is a list of two numbers representing the minimum (the first number) and the maximum (the second number) value of the y axis


* `--xrange`=`x_axis_range`. 

 `x_axis_range` is a list of two numbers representing the minimum (the first number) and the maximum (the second number) value of the x axis


* `--use_classes`=`cond`. 

 `cond` is true if the tics on the x axis should be equispaced over the axis. This option is available only for box plots.


* `--override`=`FILE`. 

 Use `FILE` to override default options
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_read 
## OVERVIEW
 Read a database from disk
## SYNOPSYS
`db_read FILE [ options ] `
## DESCRIPTION
Read database from disk file `FILE`. The contents of the file are inserted into the database `DB` specified with the `--destination` option. The database file should have been previously written with the db_write command. 
## OPTIONS


* `--destination`=`DB`. 

 `DB` is the string containing the name of the destination database
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_report 
## OVERVIEW
 Print out the contents of the database.
## SYNOPSYS
`db_report DB [ options ] `
## DESCRIPTION
Reports the contents of database `DB`. For each configuration, it shows the corresponding metrics and statistics toghether with the associated objectives. The cluster number for each configuration can be shown by enabling the corresponding option.
## OPTIONS


* `--only_size`=`BOOL`. 

 Dump only the size if `BOOL` is `true`


* `--only_objectives`=`BOOL`. 

 Dump only the objectives if `BOOL` is `true`


* `--show_cluster`=`BOOL`. 

 Show the cluster number if `BOOL` is `true`
## RETURN VALUE

# db_report_html 
## OVERVIEW
 Creates an HTML report of the specified database
## SYNOPSYS
`db_report_html DB [ options ] `
## DESCRIPTION
Invokes Multicube Explorer R1.0 to create an HTML report of database '`DB`' into the current directory. The Multicube Explorer path should be available in the 'multicube_explorer_path' shell variable. The current driver should be the XML driver.
## OPTIONS


* `--name`=`NAME`. 

 Create a report named NAME


* `--objectives`=`BOOL`. 

 BOOL should be true if only if the report should contain only objectives
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_train_rsm 
## OVERVIEW
 Trains and RSM
## SYNOPSYS
`db_train_rsm DEST [ options ] `
## DESCRIPTION
Train a RSM by using model `MODEL` and creates a set of predictions (over the full space or DoE depending on the model type)  which are put in database `DEST`. Additional options depend on the RSM used. Use "help db_train_rsm --model=`MODEL`" for a  detailed help on the selected `MODEL`. `SOURCEDB` may be used as an alternative source for training design points.
## OPTIONS


* `--model`=`MODEL`. 

 `MODEL` should be one of the existing models.


* `--source`=`SOURCEDB`. 

 `SOURCEDB` is the name of the source DB
## AVAILABLE MODELS
LINEAR,
NN_ON_DOE,
RBF_ON_DOE,
SHEPARD,
SHEPARD_ON_DOE,
SPLINE_ON_DOE

## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# db_write 
## OVERVIEW
 Write a database on disk.
## SYNOPSYS
`db_write DB [ options ] `
## DESCRIPTION
Write database `DB` into the disk file `FILE` specified with the `--file_name` option. The database is written into a text file which can be manually inspected. This may change in future versions but backward compatibility will be ensured.
## OPTIONS


* `--file_name`=`FILE`. 

 `FILE` is the name of the destination file
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# doe_load_doe 
## OVERVIEW
 Load the specified optimization module.
## SYNOPSYS
`doe_load_doe DOE_NAME`
## DESCRIPTION
Load the Design Of Experiments module module `DOE_NAME`. See the considerations on the  search path with 'help opt_load_optimizer'. 
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# drv_load_driver 
## OVERVIEW
 Load the specified driver module.
## SYNOPSYS
`drv_load_driver DRV_NAME`
## DESCRIPTION
Load the driver module named `DRV_NAME`. See the considerations on the search path with 'help opt_load_optimizer'. 
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# drv_write_xml 
## OVERVIEW
 Writes the current design space into a file.
## SYNOPSYS
`drv_write_xml FILENAME`
## DESCRIPTION
Writes the current design space into file `FILENAME`. A design space should be instantiated before invoking this command.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# echo 
## OVERVIEW
 Prints the value of an expression.
## SYNOPSYS
`echo EXPR`
## DESCRIPTION
Prints the value of expression `EXPR`. Values are printed according to the MOST language syntax.
## RETURN VALUE

# exit 
## OVERVIEW
 Quits the current session.
## SYNOPSYS
`exit`
## DESCRIPTION
Quits the current most session.
## RETURN VALUE

# help 
## OVERVIEW
 Prints a general help of MOST R1.2
## SYNOPSYS
`help CMD_NAME`
## DESCRIPTION
When no arguments are specified, it prints all available commands on the shell. Otherwise, prints the manual of `CMD_NAME` command.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# opt_estimate 
## OVERVIEW
 Estimates a single point
## SYNOPSYS
`opt_estimate P`
## DESCRIPTION
Estimate the metrics of point `P` by using the current driver. The return value will contain the point with the new estimated metrics. 
## RETURN VALUE
On success, $? contains the point with the new estimated metrics. `false` otherwise.
# opt_load_optimizer 
## OVERVIEW
 Loads the specified optimization module.
## SYNOPSYS
`opt_load_optimizer OPT_NAME`
## DESCRIPTION
Load the optimizer module named `OPT_NAME`. The module should be an .so object located in the search path of MOST. The search path is composed by the `lib` directory of the program distribution package plus the value of the search_path variable (the latter variable can be a string or a list of strings).
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# opt_remove_constraint 
## OVERVIEW
 Removes a constraint
## SYNOPSYS
`opt_remove_constraint CONS`
## DESCRIPTION
Remove the constraint named `CONS`. If the constraint is not specified, removes all the constraints.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# opt_remove_objective 
## OVERVIEW
 Removes an objective
## SYNOPSYS
`opt_remove_objective OBJ`
## DESCRIPTION
Remove the objective named `OBJ`. If the objective is not specified, removes all the objectives.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# opt_tune 
## OVERVIEW
 Start the optimization process.
## SYNOPSYS
`opt_tune DB`
## DESCRIPTION
Invokes the algorithm specified with the command **opt_load_optimizer** (and, optionally, the design of experiment specified with **doe_load_doe**). Each optimization algorithm heuristically tries to identify the Pareto frontier associated with the minimization of the objectives. Objectives should be specified with the command **set_objective** and can be more than 1. The designs associated with the Pareto set are inserted in database named `DB`. If the database exists, it will be overwritten. 
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# opt_update_cache 
## OVERVIEW
 Refreshes the content of the cache
## SYNOPSYS
`opt_update_cache`
## DESCRIPTION
Updates the content of the constraint and objective cache. This command should be invoked whenever objectives or constraints change.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# quit 
## OVERVIEW
 Quit the current session.
## SYNOPSYS
`quit`
## DESCRIPTION
Quits the current most session.
## RETURN VALUE

# read_object 
## OVERVIEW
 Reads a data object from disk.
## SYNOPSYS
`read_object FILE`
## DESCRIPTION
Read a data object from file `FILE`. The data object should have been saved with the write_object command.The data object is one of the basic types available in the MOST type system.
## RETURN VALUE
On success, the object is stored in the return variable $?. Otherwise, the return variable is set to `false`.
# read_script 
## OVERVIEW
 Reads and executes a script
## SYNOPSYS
`read_script FILE`
## DESCRIPTION
Read and executes commands from the script file named `FILE`. It can't be used in MOST interactive mode, i.e., it can be used only in scripts executed with the -f command line of MOST.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# set
## SYNOPSYS
`set var = expr`
## DESCRIPTION
This commands instantiates a variable named `var` and assigns to it the value of expression `expr`. The expression can be composed of any combination of variables (usually denoted by the prefix `$`), constants and operators. The variable may be of one the following allowed types:


* `scalars, floats, strings`. 

 Scalars are usually denoted by numbers without the decimal point (e.g. `7`) while floats do have a decimal point (e.g. `8.3`). Strings, instead, are delimited by quotes (e.g. `"foo"`). While most arithmetic operators work both on scalars and floats, strings can only be concatenated with the "+" operator.


* `lists`. 

 Lists can be created by using the "{}" brackets, by including the values separated with commas. List values can be of any allowed type (i.e. they may contain recursively also other lists). If any variable is specified, it is actually evaluated before being put into the list. 
Lists can be concatenated with the + operator, while each element of a list can be evaluated with the `foreach` control statement. The following example shows a list containing another list:


	* `set l =  { 2 , { 3 , 4 } }`.


* `vectors`. 

 Vectors can be created by using the "[]", by including the values separated with commas. Vector values can be of any allowed type (i.e., they are similar to lists). Vector values can be accessed with the `@` operator.  The following example shows a vector containing, among other values, a list:


	* `set v =  [ "foo", 2 , { 3 , 4 } ]`


* `maps`. 

 Maps are composed of a sequence of key-value pairs. They can be created by using the "(*" and "*)" brackets by indicating the keys and the associated values: 


	* `set map =  (* key1="foo" key2=2 *)`


	* Map's values can be accessed with the `@` operator by indicating the key as a string. For example, `map@"key1"` returns the value associated with key1 in the above map (i.e. `"foo"`).


* `points`. 

 Points actual design instances associated with the optimization problem. They are characterized by a number of parameters according to the current design space. Each parameter has a "level", i.e., a scalar value belonging to the parameter range. A point can be constructed by using the % character as a bracket: 


	* `set point =  % 3 2 0 %`


	* In this case the point's three parameters are set to the levels 3, 2 and 0 respectively. Note that these values have a meaning with respect to the actual design space so it is not safe to directly use them.
## RETURN VALUE
The return variable $? is set to the value of `expr` on success, `false` in the case the `expr` is invalid. 
# set_constraint
## SYNOPSYS
``set_constraint cn_name(point) = expr1($point) OP expr2($point)``
## DESCRIPTION

This command sets a boolean constraint or `predicate` for the optimization problem. Whenever the design point `point` fails to meet the specified constraint(s), it is considered `non-feasible` by the optimization algorithm. 
`OP` can be one of the following: >, <, >=, <=, ==. 
In the case of the == operator, a margin of 10e9 is considered between the two expression associated to the constraint. This margin can be changed by modifying the `constraint_precision` shell variable.

Each non-feasible configuration has a `rank` and a `penalty`. The rank is the number of constraints that have been violated. The penalty is the product of the slacks associated to the constraints. The rank and the penalty are taken into account when filtering with the `db_filter_pareto` command.

## EXAMPLES
`set_constraint frame_rate(x) = frame_s($x)/25 > 1`

## SEE ALSO
`opt_remove_constraint`, `opt_tune`, `set`

## RETURN VALUE
The return variable $? is set to the value of true on success, false otherwise.
# set_function
## SYNOPSYS
``set_function fn_name(par1, ..., parn) = expr(par1, ..., parn)``
## DESCRIPTION

This command creates a new function which can be invoked in a script to factor out and perform repetitive computations. Bear in mind that for complex algorithms you may use the set_procedure command.

## EXAMPLES
The following commands defines a function:

`set_function frame_s(x) = ($nframe*$frequency_Hz)/(metrics($x)@1)`

You may use the previous function to create a constraint for the optimization process:

`set_constraint frame_rate(x) = frame_s($x)/25 > 1`

## SEE ALSO
`set`

## RETURN VALUE
The return variable $? is set to the value of true on success, false otherwise. 
# set_metric
## SYNOPSYS
`set_metric m_name(point) = expr(point)`
## DESCRIPTION

This command creates a new `companion` metric `m_name` associated with each design point `point` of the design space. The companion metrics are evaluated whenever all the actual metrics of a point have been returned by the driver, but before objectives and constraints are evaluated. Once evaluated, the companion metrics are stored into the metrics cache for speeding up the optimization process.

## EXAMPLES
`set_metric Pdyn_W(x) = Energy_dyn_J($x)/delay_s($x)`
## SEE ALSO
`set_function`
## RETURN VALUE
The return variable $? is set to the value of true on success, false otherwise. 
# set_objective
## SYNOPSYS
`set_objective objective_name(point) = expr($point)`
## DESCRIPTION
This command sets a minimization objective. The current optimization problem may have more than one objective. 
The objective is a function of a design point `(point)` which is usually instantiated during the optimization phase. 
The objective function is typically derived from the actual metric values returned by the problem driver for the specific point under consideration. The function `metrics` can be used to derive an array of the metric associated to the point to build a complex objective. The following objective is specified as being the second metric described in the design space.:

`set_objective my_obj(x) = metrics($x)@2`

The expression associated to the objective may be a complex expression which takes into account also the actual values of the parameters of the design point (or `levels`), e.g.:

`set_objective my_obj(x) = get_level($x, 2)` (get_level is a built-in function that returns the value of the 3rd parameter of design point $x).

## SEE ALSO
`opt_remove_objective`, `opt_tune`, `set_constraint`, `set_function`, `set_metric`

## RETURN VALUE
The return variable $? is set to the value of true on sucess, false otherwise.
# show_vars 
## OVERVIEW
 Shows the state of the shell.
## SYNOPSYS
`show_vars`
## DESCRIPTION
Shows the state of the current shell, containing the shell variables, the current objectives and constraints and the databases in memory.
## RETURN VALUE

# write_object 
## OVERVIEW
 Write an object on disk.
## SYNOPSYS
`write_object OBJ [ options ] `
## DESCRIPTION
Write the value of expression `OBJ` into disk file `FILE` specified with the `--file_name` option.
## OPTIONS


* `--file_name`=`FILE`. 

 `FILE` is the name of the destination file
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
