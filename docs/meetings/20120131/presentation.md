# New MOST R1.1 Features

- Partial functional programming

  - Functions passed as parameters

  - Anonymous functions

* More on map/reduce algorithms on database points

* Simplified access to simulation database

* Pretty print

* Advanced Plots

# Functions passed as parameters

`db_apply` is a **map** operator. It maps `count` to all the points:

```
    	set_procedure display(x)
    	      echo perf($x) >> "data.out"
    	done

    	db_read "databases/FULL.db" --destination="D"
    	db_apply("D", ^display)
```

# Reduce functions

`db_reduce` is a **reduce** operator.

```
    	set_function count(p, v) = $v+1
    	set size = db_reduce("D", ^count, 0)
```

- Initializes accumulator `v` to 0
- Updates accumulator `v` for each point in database "D" with `count(p, v)`

# Anonymous functions

    	set size = db_reduce("D", ^(p, v)=$v+1 , 0)

- `^(p, v)=$v+1` is an anonymous function equivalent to `count`.
- To define an anonymous function, use the operator `^` and the list of
  parameters.

# Predicated Map/Reduce

    set c = db_reduce_p("D", ^count , 0, ^(x)=M3($x)>285 )

- `db_reduce_p` and `db_apply_p` receive an optional **predicate**.
- They act selectively only when the predicate is true.
- The predicate is another (anonymous) function.

# Simulation path stored in database

- The simulation path is now written/read to/from a database

- Use `--show_path=true` in `db_report` to print out the path:

`db_report "D" --only_objectives=true --show_path=true`

- It is also available as a property/function for each point (useful for
  map/reduce)

- Use `--describe=true` in `db_report` to print descriptive statistics about the
  data.

# Advanced plots: level plot

    db_plot "D" --plevel="lg2CacheSize" --override="ovrde.scr"

- Added `--plevel=PAR` option to `db_plot` to highlight different values of the
  parameters. Appropriate template files should be used to associate the
  linestyle to a gradient palette color for efficient reading of plots.

- or you can use the report package:

`report_pdf("D", "override.scr")`

# Advanced plots: effect plot and random_doe_effect DoE

    set random_doe_solutions_number = 400
    set par = "lg2Sets"
    set random_doe_effect = $par
    doe_load_doe "st_random_effect"

...simulate

    db_plot "D" --override="override.scr" --effect=$par

Generates a plot with arrows corresponding to points associated with minimum and
maximum value of `$par`

# Other options

- Added -n option. Non interactive sessions stop as soon as a command fails or
  if it the XML driver cannot be found.

- Added -j option. It allows to create an independent session of MOST (copy the
  current dir and launch MOST into it). Sessions are named and a pid is
  attached.

- Added `--override=FILE` option to `db_plot*` to override the default gnuplot
  template.
