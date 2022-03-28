# Multi-Objective Simulated Annealing 
## OVERVIEW
 Simulated annealing is a Monte Carlo approach for minimizing multivariate functions. The term simulated annealing derives from the analogy with the physical process of heating and then slowly cooling a substance to obtain a strong crystalline structure. In the Simulated Annealing algorithm a new configuration is constructed by imposing a random displacement. If the cost function of this new state is less than the previous one, the change is accepted unconditionally and the system is updated. If the cost function is greater, the new configuration is accepted probabilistically; the acceptance possibility decreases with the temperature (optimization time). This procedure allows the system to move consistently towards lower cost function states, thus jumping out of local minima due to the probabilistic acceptance of some upward moves. This optimizer is derived by: Smith, K. I.; Everson, R. M.; Fieldsend, J. E.; Murphy, C.; Misra, R., Dominance-Based Multiobjective Simulated Annealing,IEEE Transaction on Evolutionary Computation, 12(3): 323-342 - 2008
## SYNOPSYS
`opt_load_optimizer st_mosa`
## DESCRIPTION
Once loaded with the **opt_load_optimizer** command, the algorithm is invoked by using the **opt_tune** command (see manual).Each epoch is composed by iterations characterized by the same temperature. The number of iterations per epoch is defined bythe `epoch_length` shell variable, while the number of epochs is defined by the `epochs` shell variable. The temperature is internally defined and impacts on the randomness of generated configurations. The shell variable `t_decrease_coefficient` defines the temperature ratio between consecutive epochs.
## SHELL VARIABLES


* `epochs`. 

 number of macro-iterations with different temperature coefficients


* `epoch_length`. 

 number of iterations with the same temperature


* `t_decrease_coefficient`. 

 multiplier used to compute the next temperature (ideally it should be <1, default is 0.19)
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Parallel DoE. 
## OVERVIEW
 This algorithm evaluates all the designs specified by the current DoE (as defined by **doe_load_doe**). 
## SYNOPSYS
`opt_load_optimizer st_parallel_doe`
## DESCRIPTION
Once loaded with the **opt_load_optimizer** command, the algorithm is invoked by using the **opt_tune** command (see manual). This algorithm evaluates all the designs specified by the current DoE (as defined by **doe_load_doe**). All evaluated designs are stored in the destination database specified as the **opt_tune** command. When MOST is run with MPI on parallel nodes, evaluations are performed concurrently. The number of concurrent evaluations is equal to the number of MPI nodes specified in the command line. A temporary database to store the incremental results of the evaluations can be specified with `parallel_doe_temp_database`.
## SHELL VARIABLES


* `parallel_doe_temp_database`. 

 name of the temporary database file to store incremental results


* `parallel_doe_tempdb_granularity`. 

 number of evaluations after which the temporary database is written
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Parallel Pareto DoE. 
## OVERVIEW
 This algorithm evaluates all the designs specified by the current DoE (as defined by **doe_load_doe**) by maintaining only non-dominated points (when objectives have been specified). 
## SYNOPSYS
`opt_load_optimizer st_parallel_pareto_doe`
## DESCRIPTION
Once loaded with the **opt_load_optimizer** command, the algorithm is invoked by using the **opt_tune** command (see manual). This algorithm evaluates all the designs specified by the current DoE (as defined by **doe_load_doe**). If the problem objectives have been specified, at every evaluation dominated designs are removed from the destination database (i.e., only the Pareto frontier is maintained in the database). All evaluated designs are stored in the destination database specified as the **opt_tune** command. When MOST is run with MPI on parallel nodes, evaluations are performed concurrently. The number of concurrent evaluations is equal to the number of MPI nodes specified in the command line. A temporary database to store the incremental results of the evaluations can be specified with `parallel_doe_temp_database`.
## SHELL VARIABLES


* `parallel_doe_temp_database`. 

 name of the temporary database file to store incremental results


* `parallel_doe_tempdb_granularity`. 

 number of evaluations after which the temporary database is written
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Steepest descent 
## OVERVIEW
 Greedy, single objective optimization algorithm. It first evaluates all the designs specified by the current DoE (as defined by **doe_load_doe**) and then starts to greedly move within the design space towards a local minimum. 
## SYNOPSYS
`opt_load_optimizer st_parallel_steepest_descent`
## DESCRIPTION
Once loaded with the **opt_load_optimizer** command, the algorithm is invoked by using the **opt_tune** command (see manual). This algorithm evaluates all the designs specified by the current DoE (as defined by **doe_load_doe**) and then starts to greedly move within the design space through neighborhood points trying to minimize a single objective. Either a single objective should be specified with **set_objective** or `target_objective` should be used to define the unique objective to be minimized.
## SHELL VARIABLES


* `parallel_instances`. 

 If specified, overrides the number of simultaneous evaluations set at the command line of MOST


* `target_objective`. 

 If multiple objectives have been set, this options specifies the one that should be minimized by the algorithm. 
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
