#include <iostream>
#include <fstream>
#include <map>

#include "linalg.hpp"
#include "rbf.hpp"
#include "error.hpp"
#include "utils.hpp"
#include "interpolator.hpp"
#include "gpl_xdr_api.h"

using namespace std;
using namespace linalg;
using namespace rbf;

//*******************************************************************
//*		 						Prototipes							*
//*******************************************************************
void print_prompt();
matrix read_matrix_from_xdr(std::string filename);
std::map<linalg::point, std::vector<double> > read_all_training_from_xdr(
		std::string filename);
map<point, double> training_of_one_metric(
		map<point, std::vector<double> > trainingAll, int intersted_metric);

template<typename RBF> void write_RBF_param_file(int typeRBF, double paramRBF,
		std::vector<RBF> &rbfsComputed, std::vector<double> &lambdaComputed,
		string outParam);

/*
 * 
 * AVAILABLE RADIAL BASIS FUNCTION
 *
 *  

 /// r^n with n odd
 class piecewise_polynomial : public piecewise_smooth_rbf;

 /// r^n log(r) with n even
 class thin_plate_spline : public piecewise_smooth_rbf;

 /// sqrt(1+(eps*r)^2) with eps > 0
 class multiquadric : public c_infty_rbf;

 /// 1/sqrt(1 + (eps*r)^2) with eps > 0
 class inverse_multiquadric : public c_infty_rbf;

 /// exp(- (eps*r)^2) with eps > 0.
 class gaussian : public c_infty_rbf;

 */

int main(int argc, char **argv) {

	// Minimo controllo sui parametri in ingresso
	int numMetrics=0;
	cout << "RBF TRAINER - (c) 2008 Politecnico di Milano" <<endl<<endl;
	if (argc>1)
		numMetrics = atoi(argv[1]);
	else {
		print_prompt();
		exit(1);
	}
	

	// Noto il numero di metriche su cui lavorare...controllo di avere un numero corretto di parametri
	if (argc!=(1+1+numMetrics*2+2)) {
		print_prompt();
		exit(1);
	}
	
	// Salvo in stringhe i nomi dei file cui dovro' accedere in lettura e/o scrittura
	string inputTraining = argv[1+2*numMetrics+1];
	string outputParam = argv[1+2*numMetrics+1+1];
	// Elimino nel caso in cui eventualmente ci sia gia' un file con nome identico a quello dei parametri da salvare
	string command = string("rm -f ")+outputParam;
	system(command.c_str());

	
	// Compilo il vettore delle RBF da usare (una per ogni metrica)
	std::vector<int> inputRBFtype;
	for (int i=0; i<numMetrics; i++)
		inputRBFtype.push_back(atoi(argv[1+1+i]));

	// Compilo il vettore dei parametri da usare con le RBF
	std::vector<double> inputRBFparam;
	for (int i=0; i<numMetrics; i++)
		inputRBFparam.push_back(atof(argv[1+numMetrics+1+i]));

	
	gsl_set_error_handler(&error_handling::errorHandler);
	try {

		// Carica l'intero training set
		map<point, std::vector<double> > trainingAll = read_all_training_from_xdr(inputTraining);
		map<point, double> training;
		

		// Per ogni metrica:
		for(int i=0; i<numMetrics;i++) {
			
			// Dichiaro il vettore contenente i lambda calcolati per la RBF di quella metrica
			std::vector<double> lambdaComputed;
			lambdaComputed.clear();

			// Costruisco la map di training con la sola metrica in esame
			// Calcolo l'interpolatore di interesse e lo salvo su file
			cout<<endl<<"## Metrics: "<<i<<" ##"<<endl;

			training = training_of_one_metric(trainingAll, i);
	
			cout<<"Extracted training data."<<endl;

			switch (inputRBFtype[i]) {
				case 1: {
					cout<<"RBF chosen: r^n, with n integer odd"<<endl;
					cout<<"n = "<<(int)inputRBFparam[i]<<endl;
					cout<<"...working..."<<endl;
					/// r^n with n odd
					if((int)inputRBFparam[i]%2 == 0) {
						cout<<"For this type of RBF n MUST be ODD"<<endl;
						// Salto la stima di questa metrica, visto l'errore nei parametri
						continue;
					}

					// Dichiaro il vettore in cui salvare la rbfs completa
					std::vector <piecewise_polynomial> rbfsComputed;

					// Settare il parametro della radial basis function di interesse
					piecewise_polynomial::set_n((size_t)inputRBFparam[i]);
					// Define interpolator given the training set.
					bvp::interpolator<piecewise_polynomial> u(training, lambdaComputed, rbfsComputed, true);

					// Scrivo su file i risultati 
					write_RBF_param_file(inputRBFtype[i], inputRBFparam[i], rbfsComputed, lambdaComputed, outputParam);
					break;
				}

				case 2: {
					/// r^n log(r) with n even
					cout<<"RBF chosen: r^n log(r), with n integer even"<<endl;
					cout<<"n = "<<(int)inputRBFparam[i]<<endl;
					cout<<"...working..."<<endl;
					if((int)inputRBFparam[i]%2 != 0) {
						cout<<"For this type of RBF n MUST be EVEN"<<endl;
						continue;
					}

					// Dichiaro il vettore in cui salvare la rbfs completa
					std::vector <thin_plate_spline> rbfsComputed;

					// Settare il parametro della radial basis function di interesse
					thin_plate_spline::set_n((size_t)inputRBFparam[i]);
					// Define interpolator given the training set.
					bvp::interpolator<thin_plate_spline> u(training, lambdaComputed, rbfsComputed, true);

					// Scrivo su file i risultati 
					write_RBF_param_file(inputRBFtype[i], inputRBFparam[i], rbfsComputed, lambdaComputed, outputParam);

					break;
				}

				case 3: {

					/// sqrt(1+(eps*r)^2) with eps > 0
					cout<<"RBF chosen: sqrt(1+(eps*r)^2), with eps > 0 real"<<endl;
					cout<<"eps = "<<inputRBFparam[i]<<endl;
					cout<<"...working..."<<endl;
					if(inputRBFparam[i]<= 0.0) {
						cout<<"For this type of RBF MUST be eps >0"<<endl;
						continue;
					}

					// Dichiaro il vettore in cui salvare la rbfs completa
					std::vector <multiquadric> rbfsComputed;

					//Change the parameter of the rbf.
					c_infty_rbf::set_epsilon(inputRBFparam[i]);
					// Define interpolator given the training set.
					bvp::interpolator<multiquadric> u(training, lambdaComputed, rbfsComputed, true);

					// Scrivo su file i risultati 
					write_RBF_param_file(inputRBFtype[i], inputRBFparam[i], rbfsComputed, lambdaComputed, outputParam);

					break;
				}

				case 4: {
					/// 1/sqrt(1 + (eps*r)^2) with eps > 0
					cout<<"RBF chosen: 1/sqrt(1 + (eps*r)^2), with eps > 0 real"<<endl;
					cout<<"eps = "<<inputRBFparam[i]<<endl;
					cout<<"...working..."<<endl;
					if(inputRBFparam[i]<= 0.0) {
						cout<<"For this type of RBF MUST be eps >0"<<endl;
						continue;
					}

					// Dichiaro il vettore in cui salvare la rbfs completa
					std::vector <inverse_multiquadric> rbfsComputed;

					//Change the parameter of the rbf.
					c_infty_rbf::set_epsilon(inputRBFparam[i]);
					// Define interpolator given the training set.
					bvp::interpolator<inverse_multiquadric> u(training, lambdaComputed, rbfsComputed, true);

					// Scrivo su file i risultati 
					write_RBF_param_file(inputRBFtype[i], inputRBFparam[i], rbfsComputed, lambdaComputed, outputParam);

					break;
				}

				case 5: {
					/// exp(- (eps*r)^2) with eps > 0.
					cout<<"RBF chosen: exp(- (eps*r)^2), with eps > 0 real"<<endl;
					cout<<"eps = "<<inputRBFparam[i]<<endl;
					cout<<"...working..."<<endl;
					if(inputRBFparam[i] <= 0.0) {
						cout<<"For this type of RBF MUST be eps >0"<<endl;
						continue;
					}

					// Dichiaro il vettore in cui salvare la rbfs completa
					std::vector <gaussian> rbfsComputed;

					//Change the parameter of the rbf.
					c_infty_rbf::set_epsilon(inputRBFparam[i]);
					// Define interpolator given the training set.
					bvp::interpolator<gaussian> u(training, lambdaComputed, rbfsComputed, true);

					// Scrivo su file i risultati 
					write_RBF_param_file(inputRBFtype[i], inputRBFparam[i], rbfsComputed, lambdaComputed, outputParam);

					break;
				}
				default:cout<<"Error in selection of RBF !";

			}

			cout <<"Done. Data written on file."<<endl<<endl;
		}
		cout << "Writing file with computed RBF values... " << endl;

		cout << "...file "<<outputParam<<" completely written." << endl;
		return 0;
	}
	catch(error_handling::error& exc) {
		utils::show_exception(exc);
		return 1;
	}
}

//*******************************************************************
//*		 						Functions							*
//*******************************************************************


void print_prompt() {
	cout << "Error: Please specify correct parameters !" << endl
			<< "Usage: ./trainerRBF <num_of_metrics> (<choosen_RBF>)^num_of_metrics (<parameter_of_RBF>)^num_of_metrics <file_training_set> <file_to_save_parameters>"
			<<endl<<endl <<"AVAILABLE RBF:"<<endl <<"1		r^n with n odd" <<endl
			<<"2		r^n log(r) with n even"<<endl
			<<"3		sqrt(1+(eps*r)^2) with eps > 0"<<endl
			<<"4		1/sqrt(1 + (eps*r)^2) with eps > 0"<<endl
			<<"5 		exp(- (eps*r)^2) with eps > 0."<<endl;
}
//ok, working
matrix read_matrix_from_xdr(std::string filename) {

	std::ifstream ifs(filename.c_str());
	if (!ifs) {
		error_handling::badArgument exc;
		exc.reason = "Cannot open file ";
		exc.reason += filename;
		exc.line = __LINE__;
		exc.file =__FILE__;
		throw exc;
	}
	matrix v;
	std::vector<std::vector<double> > design_matrix;

	std::vector<double> design;
	std::vector<double> metrics;

	while (gpl_xdr_read_design(ifs, design, metrics)) {
		design_matrix.push_back(design);
	}

	int numRow = design_matrix.size();
	int numCol = (design_matrix[0]).size();

	v = matrix(numRow, numCol, 0);

	for(int j=1;j<=numRow;j++) {
		for(int i=1; i<=numCol;i++) {
			//cout<<"sn 4";
			v(j, i) = design_matrix[j-1][i-1];
			//	cout<<v(j, i)<<" ";
		}
		//cout<<endl;
	}

	ifs.close();

	return v;
}

// ok, working
std::map<linalg::point, std::vector<double> > read_all_training_from_xdr(
		std::string filename) {
	std::ifstream ifs(filename.c_str());
	if (!ifs) {
		error_handling::badArgument exc;
		exc.reason = "Cannot open file ";
		exc.reason += filename;
		exc.line = __LINE__;
		exc.file =__FILE__;
		throw exc;
	}

	// ogni cella del vettore contiene la mappa punto -> vettore delle metriche
	// prima rilevo i punti, coordinate

	matrix M;
	std::vector<std::vector<double> > design_matrix;
	std::vector<std::vector<double> > metrics_matrix;

	std::vector<double> design;
	std::vector<double> metrics;

	while (gpl_xdr_read_design(ifs, design, metrics)) {
		design_matrix.push_back(design);
		metrics_matrix.push_back(metrics);
	}

	int numRow = design_matrix.size();
	int numColPoint = (design_matrix[0]).size();
	int numColMetrics = (metrics_matrix[0]).size();

	M = matrix(numRow, numColPoint, 0);

	for(int j=1;j<=numRow;j++) {
		for(int i=1; i<=numColPoint;i++) {
			//cout<<"sn 4";
			M(j, i) = design_matrix[j-1][i-1];
			//cout<<M(j, i)<<" ";
		}
		//cout<<endl;
	}

	if(M.cols() < 2) {
		error_handling::badArgument exc;
		exc.reason =
		"Input matrix to read_pd_map is too narrow. \n"
		"Need at least two columns in the input matrix";
		exc.line = __LINE__;
		exc.file = __FILE__;
		throw exc;
	}

	std::map <linalg::point, std::vector<double> > result;

	size_t m = M.cols();
	// come etichetta uso l'intera riga contenente tutti i punti
	linalg::slice s(1,m);

	for(int j=1;j<=M.rows();j++) {
		for(int i=1; i<=numColMetrics;i++) {
			//	for(size_t i = 1; i <= M.rows(); i++)
			result[M(j,s)] = metrics_matrix[j-1];

		}
		//cout<<" "<< (result[M(j,s)]).at(0);
		//cout<<" "<< (result[M(j,s)]).at(1);
		//cout<<endl;
	}
	return result;
}

// Passata la mappa contenente tutte le metriche registrate per ogni coordinata
// ritorna una mappa con una sola metrica scelta (parametro interested_metric)
// per ogni coordinata.
// In questo modo posso la map ottenuta viene direttamente mandata all'interpolator.
map<point, double> training_of_one_metric(
		map<point, std::vector<double> > trainingAll, int intersted_metric) {

	map<point, double> train;

	for (map<point, std::vector<double> >::const_iterator i =
			trainingAll.begin(); i != trainingAll.end(); ++i) {

		// assegnamento alla mappa della sola metrica specificata da parametro
		train[i->first] = (i->second).at(intersted_metric);
	}
	return train;
}

// Scrive su file i dati della RBF
// con: 
// - N diemnsione della RBF (num di coordinate per ogni punto)
// - M numero di dati di training
// FORMATO PER OGNI METRICA NEL FILE:
// RBF typeRBF paramRBF dimensionRBF
// centre1 centreRBF2 ...  centreRBFN lambda1
// centre1 centreRBF2 ...  centreRBFN lambda2
//  ...
// centre1 centreRBF2 ...  centreRBFN lambdaM
// END_RBF
template<typename RBF> void write_RBF_param_file(int typeRBF, double paramRBF,
		std::vector<RBF> &rbfsComputed, std::vector<double> &lambdaComputed,
		string outParam) {

	std::ofstream out(outParam.c_str(), ios::app);
	
	out.precision(20);
	out<<"RBF "<<typeRBF<<" "<<paramRBF<<" " <<rbfsComputed[0].getDimension()
			<<std::endl;
	for (unsigned int g = 0; g<rbfsComputed.size(); g++) {
		out<<" "<<rbfsComputed[g].getCentre()<<" "<<lambdaComputed[g]
				<<std::endl;
	}
	out<<"END_RBF"<<std::endl;
	out.close();

}
