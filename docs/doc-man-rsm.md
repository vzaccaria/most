# Linear Response Surface Model 
## OVERVIEW
 Creates a linear regression-based prediction model for the entire design space. Linear regression is a method that models a linear relationship between a dependent response function and some independent variables. In the general class of regression models, the response is modeled as a weighted sum of independent variables plus random noise. Since the basic linear estimates may not adequately capture nuances in the response-independent variable relationship, the implemented plugin takes into account also the interactions between the independent variables (the design parameters) as well as quadratic behaviour with respect to a single parameter. 
## SYNOPSYS
`db_train_rsm DEST [ options ] `
## DESCRIPTION
Computes the linear RSM by using the training database specified with the source option. Predictions are comput ed over all the design space and put into database `DEST`. 
## OPTIONS


* `--order`=`ORDER`. 

 `ORDER` can be 1 or 2


* `--interaction`=`BOOL`. 

 if `true` the model will consider interaction effects


* `--normax`=`BOOL`. 

 if `true` the normalization will be performed with the maximum value instead of the variance


* `--preprocess`=`PRE`. 

 Preprocessing Box-Cox transform; it can be a float value or "log"


* `--filter`=`FL`. 

 Percentage of points of the target design space to be randomly used for generating DEST. If not specified, all the design space will be considered


* `--model`=`"LINEAR"`. 

 


* `--source`=`SRC`. 

 Source database to be used for training
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Neural Network Model (predictions made only on DoE) 
## OVERVIEW
 Computes an artificial neural network model. An Artificial Neural Network (ANN) is a mathematical model or computational model that tries to simulate the structure and/or functional aspects of biological neural networks. It consists of an interconnected group of artificial neurons. An artificial neuron is a mathematical function that models a biological neuron. The artificial neuron receives one or more inputs (dendrites) and sums them to produce an output (synapse). Usually the sums of each node are weighted, and the sum is passed through a non-linear function known as an activation function or transfer function. In most cases an ANN is an adaptive system that changes its structure based on external or internal information that flows through the network during the learning phase. The learning method implemented is based on the Cascade 2 architecture: S.E.Fahlman, D.Baker, J.Boyan, The Cascade2 learning architecture, Technical Report, CMU-CS-TR-96-184, Carnegie Mellon University, 1996. 
## SYNOPSYS
`db_train_rsm DEST [ options ] `
## DESCRIPTION
Computes an Artificial Neural Network RSM by using the training database specified with the source option. Predictions are computed for design points specified with the DoE module and put into database `DEST`.
## OPTIONS


* `--effort`=`TYPE`. 

 `TYPE` can be "fast", "low", "medium", "high"


* `--preprocess`=`PRE`. 

 Preprocessing Box-Cox transform; it can be a float value or "log"


* `--model`=`"NN_ON_DOE"`. 

 


* `--source`=`SRC`. 

 Source database to be used for training
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Radial Basis Functions Model (predictions made only on DoE) 
## OVERVIEW
 Computes the RBF model for design points specified with a design of experiment module. Radial Basis Functions (RBF) represent a widely used interpolation/approximation model, whose values depends only on the distance from the origin or alternatively on the distance from some other point called center. Any radial function is suitable as distance function. Interesting radial functions definitions are: linear, thin plate spline, multiquadric, inverse multiquadric and gaussian. The approximating function is represented as a sum of radial basis functions, each associated with a center and weighted by an appropriate coefficient. The implemented approach is based on: M.J.D. Powell. The theory of radial basis functions approximation in 1990, W.A. Light (Ed.), Advances in Numerical Analysis II: Wavelets, Subdivision, Algorithms, and Radial Basis Functions, Oxford University Press, Oxford. pp. 105-210, 1992.
## SYNOPSYS
`db_train_rsm DEST [ options ] `
## DESCRIPTION
Computes the RBF RSM by using the training database specified with the source option. Predictions are computed on the DoE and put into database `DEST`.
## OPTIONS


* `--type`=`TYPE`. 

 `TYPE` can be "power", "power_log", "sqrt", "inv_sqrt" or "exp"


* `--parameter`=`PAR`. 

 `PAR` is the integer parameter value of the selected RBF


* `--preprocess`=`PRE`. 

 Preprocessing Box-Cox transform; it can be a float value or "log"


* `--model`=`"RBF_ON_DOE"`. 

 


* `--source`=`SRC`. 

 Source database to be used for training
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Shepard Interpolation Model 
## OVERVIEW
 Computes a Shepard-interpolation based model for the entire design space. The Shepard’s technique is a well known method for multivariate interpolation. This technique is also called Inverse Distance Weighting (IDW) method because the value of the response function in unknown points is the the sum of the value of the response function in known points weighted with the inverse of the distance.
## SYNOPSYS
`db_train_rsm DEST [ options ] `
## DESCRIPTION
Computes the shepard RSM by using the training database specified with the source option. Predictions are computed over all the design space and put into database `DEST`.
## OPTIONS


* `--power`=`ORDER`. 

 `ORDER` is the float power value used for the shepard interpolation


* `--preprocess`=`PRE`. 

 Preprocessing Box-Cox transform; it can be a float value or "log"


* `--model`=`"SHEPARD"`. 

 


* `--source`=`SRC`. 

 Source database to be used for training
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# Shepard Interpolation Model (predictions made only on DoE) 
## OVERVIEW
 The Shepard technique is a well known method for multivariate interpolation. This technique is also called Inverse Distance Weighting (IDW) method because the value of the response function in unknown points is the the sum of the value of the response function in known points weighted with the inverse of the distance. The predictions for this model are made only for points specified with a design of experiment module.
## SYNOPSYS
`db_train_rsm DEST [ options ] `
## DESCRIPTION
Computes the SHEPARD RSM by using the training database specified with the source option. Predictions are computed on the DoE and put into database `DEST`.
## OPTIONS


* `--power`=`ORDER`. 

 `ORDER` is the float power value used for the shepard interpolation


* `--preprocess`=`PRE`. 

 Preprocessing Box-Cox transform; it can be a float value or "log"


* `--model`=`"SHEPARD"`. 

 


* `--source`=`SRC`. 

 Source database to be used for training
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
# SPLINE response surface model (predictions made only on DoE) 
## OVERVIEW
 Computes a SPLINE prediction for the design points specified in the Design of Experiments. Interpolation is the process of assigning values to unknown points by using a small set of known points and does not produce any error on the known data. Spline is a form of interpolation where the interpolant function is divided into intervals defining multiple different continuous polynomials with endpoints called knots. The implemented approach is based on: Benjamin C. Lee, David M. Brooks, Regression Modeling Stategies for Microarchitectural Performance and Power Prediction, Report No. TR-08-06, Division of Engineering and Applied Sciences Harvard University, March 2006.
## SYNOPSYS
`db_train_rsm DEST [ options ] `
## DESCRIPTION
Computes a Spline based RSM by using the training database specified with the source option. Predictions are computed on the DoE and put into database `DEST`.
## OPTIONS


* `--preprocess`=`PRE`. 

 Preprocessing Box-Cox transform; it can be a float value or "log"


* `--model`=`"SPLINE_ON_DOE"`. 

 


* `--source`=`SRC`. 

 Source database to be used for training
## RETURN VALUE
The return variable $? is set to `true` on success, `false` otherwise.
