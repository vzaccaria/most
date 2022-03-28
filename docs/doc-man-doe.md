# Box Behnken Design of Experiments 
## OVERVIEW
 This design of experiments generates design by using the Box Behnken technique. 
## SYNOPSYS
`doe_load_doe st_box_behnken`
## DESCRIPTION
This design of experiments generates design by using the Box Behnken technique. The Box–Behnken DoE is suitable for constructing RSM quadratic models where parameter combinations that are at the center of the edges of the design space in addition to a design with all the parameters at the center. The main advantage is that the parameter combinations avoid taking extreme values taken at the same time (in contrast with the central composite design). This may be suitable to avoid singular points in the generation of the response surface, which would deteriorate it. 
## SHELL VARIABLES
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Central Composite Design Face-Centered DoE 
## OVERVIEW
 This design of experiments generates design by using the Central Composite Design technique. 
## SYNOPSYS
`doe_load_doe st_ccd_fc`
## DESCRIPTION
A central composite design is an experimental design specifically targeted to the construction of response surfaces of the second order (quadratic) without requiring a three-level full or fractional factorial DoE. The design consists of the following three distinct sets of experimental runs: 1) a two-level full or fractional factorial design; 2) a set of center points, i.e., experimental runs whose values of each parameter are the medians of the values used in the factorial portion; and 3) a set of axial points, i.e., experimental runs that are identical to the center points except for one parameter. In the general central composite design, this parameter will take on values both below and above the median of the two levels. 
## SHELL VARIABLES
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Full Factorial Design of Experiments 
## OVERVIEW
 This design of experiments generates Full Factorial design of experiments. 
## SYNOPSYS
`doe_load_doe st_full_factorial`
## DESCRIPTION
In statistics, a two-level full factorial experiment is an experiment whose design consists of two or more parameters, each with discrete possible values or **levels** and whose experimental units take on all possible combinations of the minimum and maximum levels for such parameters. Such an experiment enables the evaluation of the effects of each parameter on the response variable, as well as the effects of interactions between parameters on the response variable. 
## SHELL VARIABLES
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Full Factorial Design of Experiments Extended 
## OVERVIEW
 This design of experiments generates Full Factorial Extended design of experiments. It is similar to the plain Full factorial design but with a different twist on the generation of opposite values for permutations and masks. 
## SYNOPSYS
`doe_load_doe st_full_factorial_extended`
## DESCRIPTION
In statistics, a two-level full factorial experiment is an experiment whose design consists of two or more parameters, each with discrete possible values or **levels** and whose experimental units take on all possible combinations of the minimum and maximum levels for such parameters. Such an experiment enables the evaluation of the effects of each parameter on the response variable, as well as the effects of interactions between parameters on the response variable. 
## SHELL VARIABLES
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Full Search 
## OVERVIEW
 This design of experiments generates all the designs of the design space.
## SYNOPSYS
`doe_load_doe st_full_search`
## DESCRIPTION
This design of experiments generates all the designs of the design space (by taking into account optional boundaries for each parameter). 
## SHELL VARIABLES
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Neighborhood Design of Experiments 
## OVERVIEW
 This design of experiments generates all the designs in the neighborhood of a design point.
## SYNOPSYS
`doe_load_doe st_neighbor`
## DESCRIPTION
This design of experiments generates all the designs in the neighborhood range `neighbor_range` of a design point `neighbor_starting_point`. 
## SHELL VARIABLES


* `neighbor_starting_point`. 

 Design point to be considered (see the overall documentation)


* `neighbor_range`. 

 Neighborhood of the design point
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Placket Burman Design of Experiments 
## OVERVIEW
 This design of experiments generates design by using the PLACKETT BURMAN technique. 
## SYNOPSYS
`doe_load_doe st_placket_burman`
## DESCRIPTION
 Plackett–Burman DoEs are experimental designs presented in 1946 by Robin L. Plackett and J. P. Burman. Their goal was to find experimental designs for investigating the dependence of some measured quantity on a number of independent variables (factors), each taking L levels, in such a way as to minimize the variance of the estimates of these dependencies using a limited number of experiments. Interactions between the factors were considered negligible. The solution to this problem is to find an experimental design where each combination of levels for any pair of factors appears the same number of times, throughout all the experimental runs (refer table). A complete factorial design would satisfy this criterion, but the idea was to find smaller design. 
## SHELL VARIABLES
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Random Design of Experiments 
## OVERVIEW
 This design of experiments generates a set of random design points.
## SYNOPSYS
`doe_load_doe st_random`
## DESCRIPTION
This design of experiments generates `random_doe_solutions_number` design points out of the design space. 
## SHELL VARIABLES


* `random_doe_solutions_number`. 

 Number of random design points to be generated


* `random_doe_no_replicate`. 

 If 1, no replication is allowed
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Random Design of Experiments 
## OVERVIEW
 This design of experiments generates a set of random design points.
## SYNOPSYS
`doe_load_doe st_random`
## DESCRIPTION
This design of experiments generates `random_doe_solutions_number` design points out of the design space. You can specify also an effect parameter for which both high and low levels should be generated. Useful for db_plot --effects=parameter
## SHELL VARIABLES


* `random_doe_solutions_number`. 

 Number of random design points to be generated


* `random_doe_no_replicate`. 

 If true, no replication is allowed


* `random_doe_effect`. 

 Specified a scalar parameter for which both high and low levels should be considered. Only if random_doe_no_replicate is not specified.
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Random Replica Design of Experiments 
## OVERVIEW
 This design of experiments generates a set of design points that replicate those existing in a database.
## SYNOPSYS
`doe_load_doe st_random_replicate`
## DESCRIPTION
This design of experiments generates a set of design points that randomly replicate those existing in database `source_db`. `max_num_of_points` is mandatory and specifies the maximum number of points to be replicated. Design points are extracted randomly. 
## SHELL VARIABLES


* `source_db`. 

 Source database


* `max_num_of_points`. 

 Maximum number of design points to be replicated
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Scrambled Design of Experiments 
## OVERVIEW
 This design of experiments generates Full Factorial-like designs of experiments for masks and permutations. 
## SYNOPSYS
`doe_load_doe st_scrambled_doe`
## DESCRIPTION
This design of experiments is meant to be a full factorial DoE for masks and permutations. 
## SHELL VARIABLES
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
