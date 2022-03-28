/* @STSHELL_LICENSE_START@
 *
 *      __  _______  ___________ 
 *     /  |/  / __ \/ ___/_  __/ 
 *    / /|_/ / / / /\__ \ / /    
 *   / /  / / /_/ /___/ // /    
 *  /_/  /_/\____//____//_/     
 * 
 * Multi-Objective System Tuner 
 * Copyright (c) 2007-2022 Politecnico di Milano
 * 
 * Development leader: Vittorio Zaccaria
 * Main developers: Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro
 *  
 * 
 * @STSHELL_LICENSE_END@ */

#define __NEED_ONLY_COMMAND_DEF__
#include <st_command_list.h>

map<string, st_command> rsm_c_help;

vector<string> st_rsm_get_available_rsms() {
  vector<string> v;
  map<string, st_command>::iterator i;
  for (i = rsm_c_help.begin(); i != rsm_c_help.end(); i++) {
    v.push_back(i->first);
  }
  return v;
}

void st_init_rsm_command_help() {
  {
    const char *ref[] = {"--order=ORDER", "--interaction=BOOL",
                         "--normax=BOOL", "--preprocess=PRE",
                         "--filter=FL",   "--model=\"LINEAR\"",
                         "--source=SRC",  NULL};
    const char *ref_help[] = {
        "! can be 1 or 2",
        "if #t the model will consider interaction effects",
        "if #t the normalization will be performed with the maximum value "
        "instead of the variance",
        "Preprocessing Box-Cox transform; it can be a float value or \"log\"",
        "Percentage of points of the target design space to be randomly used "
        "for generating DEST. If not specified, all the design space will be "
        "considered",
        "",
        "Source database to be used for training",
        NULL};

    rsm_c_help["LINEAR"] = st_command(
        multiple_opts,
        "Computes the linear RSM by using the training database specified with "
        "the source option. Predictions are comput ed over all the design "
        "space and put into database @. ",
        "DEST", ref, ref_help,
        "Creates a linear regression-based prediction model for the entire "
        "design space. "
        "Linear regression is a method that models a linear relationship "
        "between a dependent response function and some independent variables. "
        "In the general class of regression models, the response is modeled as "
        "a weighted sum of independent variables plus random noise. Since the "
        "basic linear estimates may not adequately capture nuances in the "
        "response-independent variable relationship, the implemented plugin "
        "takes into account also the interactions between the independent "
        "variables (the design parameters) as well as quadratic behaviour with "
        "respect to a single parameter. ",
        STDRET);
    rsm_c_help["LINEAR"].alt_command_name = "Linear Response Surface Model";
    rsm_c_help["LINEAR"].alt_command_synopsys = "db_train_rsm";
  }
  {
    const char *ref[] = {"--power=ORDER", "--preprocess=PRE",
                         "--model=\"SHEPARD\"", "--source=SRC", NULL};
    const char *ref_help[] = {
        "! is the float power value used for the shepard interpolation",
        "Preprocessing Box-Cox transform; it can be a float value or \"log\"",
        "", "Source database to be used for training", NULL};

    rsm_c_help["SHEPARD"] = st_command(
        multiple_opts,
        "Computes the shepard RSM by using the training database specified "
        "with the source option. Predictions are computed over all the design "
        "space and put into database @.",
        "DEST", ref, ref_help,
        "Computes a Shepard-interpolation based model for the entire design "
        "space. The Shepard’s technique is a well known method for "
        "multivariate interpolation. This technique is also called Inverse "
        "Distance Weighting (IDW) method because the value of the response "
        "function in unknown points is the the sum of the value of the "
        "response function in known points weighted with the inverse of the "
        "distance.",
        STDRET);
    rsm_c_help["SHEPARD"].alt_command_name = "Shepard Interpolation Model";
    rsm_c_help["SHEPARD"].alt_command_synopsys = "db_train_rsm";
  }
  {
    const char *ref[] = {"--power=ORDER", "--preprocess=PRE",
                         "--model=\"SHEPARD\"", "--source=SRC", NULL};
    const char *ref_help[] = {
        "! is the float power value used for the shepard interpolation",
        "Preprocessing Box-Cox transform; it can be a float value or \"log\"",
        "", "Source database to be used for training", NULL};

    rsm_c_help["SHEPARD_ON_DOE"] = st_command(
        multiple_opts,
        "Computes the SHEPARD RSM by using the training database specified "
        "with the source option. Predictions are computed on the DoE and put "
        "into database @.",
        "DEST", ref, ref_help,
        "The Shepard technique is a well known method for multivariate "
        "interpolation. This technique is also called Inverse Distance "
        "Weighting (IDW) method because the value of the response function in "
        "unknown points is the the sum of the value of the response function "
        "in known points weighted with the inverse of the distance. The "
        "predictions for this model are made only for points specified with a "
        "design of experiment module.",
        STDRET);
    rsm_c_help["SHEPARD_ON_DOE"].alt_command_name =
        "Shepard Interpolation Model (predictions made only on DoE)";
    rsm_c_help["SHEPARD_ON_DOE"].alt_command_synopsys = "db_train_rsm";
  }

  {
    const char *ref[] = {"--type=TYPE",      "--parameter=PAR",
                         "--preprocess=PRE", "--model=\"RBF_ON_DOE\"",
                         "--source=SRC",     NULL};
    const char *ref_help[] = {
        "! can be \"power\", \"power_log\", \"sqrt\", \"inv_sqrt\" or \"exp\"",
        "! is the integer parameter value of the selected RBF",
        "Preprocessing Box-Cox transform; it can be a float value or \"log\"",
        "",
        "Source database to be used for training",
        NULL};

    rsm_c_help["RBF_ON_DOE"] = st_command(
        multiple_opts,
        "Computes the RBF RSM by using the training database specified with "
        "the source option. Predictions are computed on the DoE and put into "
        "database @.",
        "DEST", ref, ref_help,
        "Computes the RBF model for design points specified with a design of "
        "experiment module. Radial Basis Functions (RBF) represent a widely "
        "used interpolation/approximation model, whose values depends only on "
        "the distance from the origin or alternatively on the distance from "
        "some other point called center. Any radial function is suitable as "
        "distance function. Interesting radial functions definitions are: "
        "linear, thin plate spline, multiquadric, inverse multiquadric and "
        "gaussian. The approximating function is represented as a sum of "
        "radial basis functions, each associated with a center and weighted by "
        "an appropriate coefficient. The implemented approach is based on: "
        "M.J.D. Powell. The theory of radial basis functions approximation in "
        "1990, W.A. Light (Ed.), Advances in Numerical Analysis II: Wavelets, "
        "Subdivision, Algorithms, and Radial Basis Functions, Oxford "
        "University Press, Oxford. pp. 105-210, 1992.",
        STDRET);
    rsm_c_help["RBF_ON_DOE"].alt_command_name =
        "Radial Basis Functions Model (predictions made only on DoE)";
    rsm_c_help["RBF_ON_DOE"].alt_command_synopsys = "db_train_rsm";
  }
  {
    const char *ref[] = {"--effort=TYPE", "--preprocess=PRE",
                         "--model=\"NN_ON_DOE\"", "--source=SRC", NULL};
    const char *ref_help[] = {
        "! can be \"fast\", \"low\", \"medium\", \"high\"",
        "Preprocessing Box-Cox transform; it can be a float value or \"log\"",
        "", "Source database to be used for training", NULL};

    rsm_c_help["NN_ON_DOE"] = st_command(
        multiple_opts,
        "Computes an Artificial Neural Network RSM by using the training "
        "database specified with the source option. Predictions are computed "
        "for design points specified with the DoE module and put into database "
        "@.",
        "DEST", ref, ref_help,
        "Computes an artificial neural network model. An Artificial Neural "
        "Network (ANN) is a mathematical model or computational model that "
        "tries to simulate the structure and/or functional aspects of "
        "biological neural networks. It consists of an interconnected group of "
        "artificial neurons. An artificial neuron is a mathematical function "
        "that models a biological neuron. The artificial neuron receives one "
        "or more inputs (dendrites) and sums them to produce an output "
        "(synapse). Usually the sums of each node are weighted, and the sum is "
        "passed through a non-linear function known as an activation function "
        "or transfer function. In most cases an ANN is an adaptive system that "
        "changes its structure based on external or internal information that "
        "flows through the network during the learning phase. The learning "
        "method implemented is based on the Cascade 2 architecture: "
        "S.E.Fahlman, D.Baker, J.Boyan, The Cascade2 learning architecture, "
        "Technical Report, CMU-CS-TR-96-184, Carnegie Mellon University, "
        "1996. ",
        STDRET);
    rsm_c_help["NN_ON_DOE"].alt_command_name =
        "Neural Network Model (predictions made only on DoE)";
    rsm_c_help["NN_ON_DOE"].alt_command_synopsys = "db_train_rsm";
  }
  {
    const char *ref[] = {"--preprocess=PRE", "--model=\"SPLINE_ON_DOE\"",
                         "--source=SRC", NULL};
    const char *ref_help[] = {
        "Preprocessing Box-Cox transform; it can be a float value or \"log\"",
        "", "Source database to be used for training", NULL};

    rsm_c_help["SPLINE_ON_DOE"] = st_command(
        multiple_opts,
        "Computes a Spline based RSM by using the training database specified "
        "with the source option. Predictions are computed on the DoE and put "
        "into database @.",
        "DEST", ref, ref_help,
        "Computes a SPLINE prediction for the design points specified in the "
        "Design of Experiments. Interpolation is the process of assigning "
        "values to unknown points by using a small set of known points and "
        "does not produce any error on the known data. Spline is a form of "
        "interpolation where the interpolant function is divided into "
        "intervals defining multiple different continuous polynomials with "
        "endpoints called knots. The implemented approach is based on: "
        "Benjamin C. Lee, David M. Brooks, Regression Modeling Stategies for "
        "Microarchitectural Performance and Power Prediction, Report No. "
        "TR-08-06, Division of Engineering and Applied Sciences Harvard "
        "University, March 2006.",
        STDRET);
    rsm_c_help["SPLINE_ON_DOE"].alt_command_name =
        "SPLINE response surface model (predictions made only on DoE)";
    rsm_c_help["SPLINE_ON_DOE"].alt_command_synopsys = "db_train_rsm";
  }
}

st_command *rsm_command_help(string rsm_name) {
  if (rsm_c_help.count(rsm_name))
    return &(rsm_c_help[rsm_name]);

  return NULL;
}
