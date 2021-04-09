# Introduction

The generic driver (GD) is a MOST driver which can be used to hook up a wide
range of simulators to MOST. GD instantiates a design space and generates,
accordingly, simulator input files by using an input template. Besides, it
parses back the results from the simulator by using a set of rules specified by
the user.

# Configuration file

GD needs a configuration file to specify the following information:

- the design space,

- the system metrics,

- the rules,

- the inputs,

- the output, and

- the simulator command line,

## The design space

The design space parameters section lets the user specify the system design
space parameters. The design space is defined into an appropriate section marked
with the following tags:

- `##DESIGN_SPACE_PARAMETERS_SECTION_BEGIN`

- `##DESIGN_SPACE_PARAMETERS_SECTION_END`

The generic driver supports three types of parameters: _integer_, _exponential
(base two)_ and _string_. A parameter is specified assigning an association map
to an identifier. An association map (or _key value pair_) is defined as (\*
key="value" \*), where _value_ can be any string (quotes should be escaped
\\\").

**Integer parameters**. An integer parameter has values with incremental
representation, starting from a minimum value up to a maximum value with an
arbitrary, integer step. The association map for the integer type has four keys:

- **type**, with value integer,

- **min**, with the integer minimum value,

- **max**, with the integer maximum value,

- **step**, with the step incremental (integer) value.

The **step** key is optional (default is 1), the other keys are mandatory.

As an example, the following parameter definition defines an integer whose
possible values are 2, 3 and 4: processors_number = (_ type="integer" min="2"
max="4" step="1" _) # comment

**Exponential (base two) parameters**. An exponential of base two parementer has
a minimum value and a maximum value:

- **type**, with value exp2,

- **min**, with the integer minimum value (power of 2),

- **max**, with the integer maximum value (power of 2).

The following parameter defines a scalar value which can assume 2, 4, 8 and 16:

    instruction_cache_size = (* type="exp2" min="2" max="16" *)

**String parameters**. String parameters represent categorical values where each
value is represented by an association map value_n=... .

The design space parameters section supports comments, and each parameter has to
be specified in a new line.

Based on the dummy use case defined in the related section, we can define the
corresponding section like this:

    ##DESIGN_SPACE_PARAMETERS_SECTION_BEGIN
    #The following two parameters are related to cache sizes
    instruction_cache_size = (* type="exp2" min="2" max="16" *)
    data_cache_size = (* type="exp2" min="2" max="16" *)
    processors_number = (* type="integer" min="2" max="4" step="1" *)  # comment
    benchmark = (* type = "string"	value_1 = "benchmark1"	value_2 = "benchmark2" *)
    ##DESIGN_SPACE_PARAMETERS_SECTION_END

The design space parameters section is mandatory. In the next subsection will be
discussed the metrics specification.

Suppose that we have a system with four parameters:

- processors number, that can assume the values 2, 3 and 4,

- instruction cache size, that can assume the values 2KB, 4KB, 8KB, and 16KB,

- data cache size, that can assume the values 2KB, 4KB, 8KB, and 16KB, and

- benchmark name, that can assume the values "benchmark1", "benchmark2".

Suppose also that we want an instruction cache of the same size of the data
cache.

From such a system, we want to know two metrics:

- the execution time, and

- the energy required.

Suppose that we have a simulator able to estimates the metrics we are
intrerested in, passing the parameters in the following way:

- instruction cache size and data cache size by the command line,

- processors number is written in a software specific configuration file, and

- benchmark name is written in another software specific configuration file.

The software prints the metrics value on the standard output. For example, at a
certain point will print "The exectuion time is: 0.15s", and next "The energy
required is: 30J".

The next section will rely on this use case to define the driver configure file.

## The design space parameters section

## The metrics section

The metrics section allows the user specify the metric values that MOST has to
readback to control the exploration flow. The metrics are written from the
simulator driven by MOST to output streams. The generic driver assumes that all
the output streams are collected into a single output file. The metric values
are assumed to be integer or real values (either in ordinary decimal notation or
scientific notation). The starting tag is `##METRIC_SECTION_BEGIN`, while the
ending tag is `##METRIC_SECTION_END`, between this tags it is possible to
specify the system metrics. A metric is specified assigning an association map
to an identifier, where the metric identifier has to be unique into the metrics
section. The association map for the integer type has the following keys:

- unit (mandatory), with a string value describing the unit of measurement,

- description (optional), with a string value describing the metric meaning,

- a (optional) key that can either be error_pattern or error_command, both with
  a string value,

- a (mandatory) key that can either be pattern or command, both with a string
  value.

The meaning of this last key is that it is possible to ask to the generic driver
to readback the metric value directly from the output file (pattern key) or by
mean an external program (command key).

When the readback is entrusted to the generic driver, the user has to specify
the pattern where find the metric value into the output file, marking the
position of the metric value with a special tag into (`<<METRIC_VALUE>>`) the
string assigned to the pattern key.

It is possible to increase the pattern matching complexity expressing a pattern
that uses regular expression syntax. If it is desired an external program for
extracting the metric value from the output stream it is possible to assign to
the key command a command line command to recall the program. The generic driver
requires that the metric value would be written alone into a file. In such a
case, it is desiderable to give to the external program the output filename of
the file containing all the output stream, and the name of the file where to
write the metric value extracted. The generic driver automatically generates the
filename of the file where to put the extracted metric value, while the output
filename is known by the driver (the details regarding the output file are
described later in this document). So, into the string assigned to the command
key the user has to specify the tag related to the output file
(`<<SIMULATOR_OUTPUT>>`) and the tag related to the filename for reading back
the metric value (`<<METRIC_READBACK>>`).

In some cases, the simulator prints informations on results correctness that it
is useful to readback. In that cases, it is possible to specify an error pattern
to match that if it is found marks the related configuration with an error. The
error pattern is specified by the key error_pattern, a string value that
represents a regular expression to find into the simulator output. The syntax of
the regular expression is the same of the GNU regex library. If it is desired an
external program for extracting the correctness of the results it's possible to
assign to the key error_command a command line command to recall the program.
The generic driver requires that the evaluation result ( a boolean value
represented by a string described by one of the following regular expressions:
[Tt][rr][Uu][ee] or [Ff][aa][Ll][ss][Ee] ) would be written alone into a file. A
true value means that an error occours. If the external program output file is
wrong the considered result is a false. The string value associated to the key
error_command has to contains two tags, one describing the simulator output
filename (<<SIMULATOR_OUTPUT>>), and another describing the external program
output filename (<<ERROR_READBACK>>).

Referring to the dummy use case, we can express the metrics section like
follows:

    ##METRICS_SECTION_BEGIN
    execution_time = (* unit="s" description="..." pattern="The execution time is: <<METRIC_VALUE>>s" *)
    energy_required	= (* unit="J" description=".." command="program_path <<SIMULATOR_OUTPUT>> > <<METRIC_READBACK>>" *)
    ##METRICS_SECTION_END

In such an example, the metric execution_time is extracted directly by the
generic driver from the output file, where the metric value position is marked
and the driver can extract that value by pattern matching of the text before the
value and the text that follows the metric value. The energy_required metric is
extracted (in the previous example) by an external program that receives the
output filename, extract the metric value and prints such a value onto the
standard output redirected to a file. At runtime the driver substitutes the tags
with the actual and correct values.

The metric section is mandatory and supports comments. The next section
describes the rules specification.

## The rules section

The rules section let the user specify the valid design space points examining
the design space parameters values. The starting tag for this section is
`##RULES_SECTION_BEGIN`, while the ending tag is `##RULES_SECTION_END`, between
this tags it is possible to specify the system rules. A rule is specified
assigning an associations map to an identifier, where the rule identifier has to
be unique into the rules section. The associations map is composed of only one
key that can be the rule key or the command key, both with string values. When
using the rule key, the generic driver uses the GNU bc program to evaluate the
expression assigned to the rule key. In particular, the user has to write the
expression of the if statement for evaluating each point validity. For referring
to a design space parameter value just put the parameter name between `<<` and
`>>`, the generic driver at runtime will substitutes that tags with the actual
values. It is also possible to use an external program to evaluates the rule
using the command key. In such a case, the external program has to write into a
file a string specifying the rule evaluation result, that can be only true or
false, or better:

- [Tt][rr][Uu][ee], or
- [Ff][aa][Ll][ss][Ee].

Into the string assigned to the command key it is possible to specify the design
space parameters values in the same way as for the rule key, but furthermore the
external filename has be given. To give the filename of the file where to put
the result, can be used the `<<RULE_READBACK>>` tag.

Referring to the dummy use case, we can express the rules section like this:

    ##RULES_SECTION_BEGIN
    cache_sizes = (* rule = "<<instruction_cache_size>> == <<data_cache_size>>" *)
    ##RULES_SECTION_END

or, if we use an external program for evaluating the rule, like this:

    ##RULES_SECTION_BEGIN
    cache_sizes = (* command = "/home/user/external_program <<instruction_cache_size>> <<data_cache_size>> <<RULE_READBACK>>" *)
    ##RULES_SECTION_END

In this example the external program reads the parameters value from the command
line, and writes the evaluation result into the file with the filename passed by
command line. The rules section is optional and supports comments. The driver at
runtime automatically substitutes the parameters value and the readback file
with the actual values.

## The input configuration files

Many simulators need parameters value written into configuration files. In these
cases, before to call the simulator, the user has to set the actual values into
each file, and then run the software. With the generic driver this process is
done automatically. It is possible to specify a single configuration file
content directly inside the driver into a dedicated section, and it is also
possible to specify a list of files where to set the parameters value.

If it is needed to specify the content of a configuration file inside the driver
configuration file, just set up a section that starts with
`##CONFIGURATION_FILE_SECTION_BEGIN` and ends with
`##CONFIGURATION_FILE_SECTION_END`, the configuration file section. Between
these tags just put the simulator configuration file content where the
parameters value are substitued with special tags like for rules section.

Referring to the dummy use case, the processors_number could be specified into a
configuration file like this:

    ##CONFIGURATION_FILE_SECTION_BEGIN

    ...some text...

    Processors := <<processors_number>>

    ...some text...

    ##CONFIGURATION_FILE_SECTION_END

When the driver driven by MOST needs to call the simulator, it substitutes
`<<processors_number>>` with the actual value, writes all the configuration text
into a file (the filename of this file will be discussed later in this
document), and then calls the software. Such a section is optional.

If one needs to specify several configuration files, can use another section
that starts with `##EXTERNAL_CONFIGURATION_FILES_SECTION_BEGIN` and ends with
`##EXTERNAL_CONFIGURATION_FILES_SECTION_END`, the external configuration files
section. In such a section one can specify a list of configuration files
templates that at runtime will be generated as actual configuration file and
given to the simulator. To add a configuration file to the list just specify an
identifier for the configuration file, and assign an associations map to that
identifier. The association map has two mandatory key:

- template, and

- configuration_filename.

The template key specify a filename of a file that contains the desired
configuration file where the parameters values are substituted as for the
configuration file section previously mentioned. The configuration_filename key
specify the filename of the file into which write the template configuration
file with the parameters value replaced.

Referring to the dummy use case, suppose that the simulator has an input file
with name input_file.cfg, into which the benchmark name is written, for example,
like this one:

    __________________________________________
    ... some text ...

    The benchmark name is = "benchmark1"

    ... some text ...
    __________________________________________

For letting the driver managing the input configuration file, we can create a
file with name, for example, input_file_template.cfg under `path_to_template`,
with the following content:

    __________________________________________
    ... some text ...

    The benchmark name is = <<benchmark>>

    ... some text ...
    __________________________________________

Where the parameter value is specified like before in this section. Suppose that
the software needs such a configuration file into the specific location
`path_to_location`. Now we can specify the section like this:

    ##EXTERNAL_CONFIGURATION_FILES_SECTION_BEGIN
    external_configuration_file = (* template="/path_to_template/input_file_template.cfg" configuration_filename="/path_to_location/input_file.cfg" *)
    ##EXTERNAL_CONFIGURATION_FILES_SECTION_END

At runtime, the driver, driven by MOST, will writes the configuration file with
the `<<benchmark>>` tag substituted with the actual value of the benchmark
parameter.

The external configuration files section and configuration file section are
optionals and support comments.

In the next section will be discussed the command line section for setting up
the simulator call.

## The command line section

In order to call the simulator to perform computation and retrieve the metrics,
it is necessary to specify the simulator call, and optionally the program
parameters and the output filename. For doing this work exists a specific
section, the command line section, that starts with the tag
\#\#COMMAND_LINE_SECTION_BEGIN and ends with the tag
\#\#COMMAND_LINE_SECTION_END.

Referring to the dummy use case, suppose that:

- the program name that perform the metrics estimation is metrics_estimator and
  it located under /home/user/, and

- the input filename of the configuration file specified into the configuration
  file section is passed to the simulator by the command line.

Given the previous suppositions, we can define the command line section like
this:

    ##COMMAND_LINE_SECTION_BEGIN
    /home/user/metrics_estimator <<instruction_cache_size>> <<data_cache_size>> <<SIMULATOR_INPUT>> > <<SIMULATOR_OUTPUT>>
    ##COMMAND_LINE_SECTION_END

where:

- `<<instruction_cache_size>>` and `<<data_cache_size>>` will be replaced with
  the actual value of the corresponding parameters,

- `<<SIMULATOR_INPUT>>` will be replaced at runtime with the input configuration
  filename, and

- `<<SIMULATOR_OUTPUT>>` will be replaced at runtime with the output filename.

If the input configuration filename isn't passed by command line, it is possible
to specify where to write the file by setting a shell variable, the details are
described later in this document. If the program writes the output into a
predetermined file, it is possible to specify the output filename setting a
shell variable, the details are described later in this document.

The command line section is mandatory and supports comments.

# The driver related MOST shell variables

To set up the use of the generic driver input configuration file (the
configuration file described in this document), the user has to specify the
filename into the input_config_file variable. To customize some details of the
generic driver interaction with the simulator can be specified some shell
variables:

- generic_driver_simulator_input, string value, used to customize the simulator
  configuration filename specified into the configuration file section,

- generic_driver_simulator_output, string value, used to customize the simulator
  output filename.

In order to help the user to verify the successful connection of generic driver
and simulator and configuration file settings some shell variables can be
specified:

- generic_driver_warning_enabled, boolean value (true or false), if true enables
  the printing of warning messages,

- generic_driver_debug_mode, boolean value (true or false), if true disables the
  deleting of temporary files (rules evaluation results and pattern matching of
  metrics values)

- generic_driver_test, boolean value (true or false), if true enables the
  generation and evaluation of a random valid point when the driver is loaded.
