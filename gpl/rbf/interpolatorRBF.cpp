#include <iostream>
#include <fstream>
#include <map>

#include "linalg.hpp"
#include "rbf.hpp"
#include "error.hpp"
#include "utils.hpp"
#include "interpolator.hpp"
#include "../gpl_xdr_api.h"

using namespace std;
using namespace linalg;
using namespace rbf;

//*******************************************************************
//*		 						Prototipes
//*
//*******************************************************************
matrix read_matrix_from_xdr(std::string filename);
std::map<linalg::point, std::vector<double> >
read_all_training_from_xdr(std::string filename);
map<point, double>
training_of_one_metric(map<point, std::vector<double> > trainingAll,
                       int intersted_metric);

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

  cout << "RBF INTERPOLATOR - (c) 2008 Politecnico di Milano" << endl;

  if (argc < 2) {
    cout << "Wrong number of parameters" << endl;
    return 127;
  }

  int numMetrics = atoi(argv[1]);
  if (argc != (1 + 1 + numMetrics * 2 + 3)) {
    cout
        << "Error: Please specify the correct parameters !" << endl
        << "Usage: ./interpolatorRBF <num_of_metrics> "
           "(<choosen_RBF>)^num_of_metrics (<parameter_of_RBF>)^num_of_metrics "
           "<file_training_set> <file_target_set> <output_file>"
        << endl
        << endl
        << "AVAILABLE RBF:" << endl
        << "1		r^n with n odd" << endl
        << "2		r^n log(r) with n even" << endl
        << "3		sqrt(1+(eps*r)^2) with eps > 0" << endl
        << "4		1/sqrt(1 + (eps*r)^2) with eps > 0" << endl
        << "5 		exp(- (eps*r)^2) with eps > 0." << endl;
    return 127;
  }

  // vedere di sistemare aui tuta la gestione dei parametri di ingresso da linea
  // di comando Quante metriche
  // int numMetrics = atoi(argv[1]); è istanziato prima per fare un minimo di
  // controllo di errori.
  // Compilo il vettore delle RBF da usare
  std::vector<int> inputRBFtype;
  for (int i = 0; i < numMetrics; i++)
    inputRBFtype.push_back(atoi(argv[1 + 1 + i]));
  // Compilo il vettore dei parametri da usare con le RBF
  std::vector<double> inputRBFparam;
  for (int i = 0; i < numMetrics; i++)
    inputRBFparam.push_back(atof(argv[1 + numMetrics + 1 + i]));

  string inputTraining = argv[1 + 2 * numMetrics + 1];
  string inputTarget = argv[1 + 2 * numMetrics + 1 + 1];
  string output = argv[1 + 2 * numMetrics + 1 + 1 + 1];

  string command = string("rm -f ") + output;
  system(command.c_str());

  // struttura dati contenente i risultati
  std::vector<std::vector<double> > metrics_matrix_estimated;

  // poi dovrai settare quante sono effettivamente le metriche
  // int numMetrics=2;
  //
  cout << "Information: Computing predictions " << endl;

  gsl_set_error_handler(&error_handling::errorHandler);
  try {

    // Read data from files.

    // ok, now working with XDR
    map<point, std::vector<double> > trainingAll =
        read_all_training_from_xdr(inputTraining);
    map<point, double> training;

    // ok, now working form XDR directly
    matrix target = read_matrix_from_xdr(inputTarget);

    // Per ogni metrica:
    for (int i = 0; i < numMetrics; i++) {

      std::vector<double> metrics_computed;
      metrics_computed.clear();

      // Costruisco la map di training con la sola metrica in esame
      // Calcolo l'interpolatore di interesse
      // Eseguo la stima del target e ne salvo i risultati
      // in metrics_matrix_estimated
      cout << "Information: Computing prediction for metric " << i << endl;

      training = training_of_one_metric(trainingAll, i);

      switch (inputRBFtype[i]) {
      case 1: {
        // cout<<"RBF chosen: r^n, with n integer odd"<<endl;
        // cout<<"n = "<<(int)inputRBFparam[i]<<endl;
        // cout<<"...working..."<<endl;
        /// r^n with n odd
        if ((int)inputRBFparam[i] % 2 == 0) {
          cout << "Error: the parameter should be odd " << endl;
          // Salto la stima di questa metrica, visto l'errore nei parametri
          return 127;
        }
        // Settare il parametro della radial basisi function di interesse
        piecewise_polynomial::set_n(inputRBFparam[i]);

        // Define interpolator given the training set.
        bvp::interpolator<piecewise_polynomial> u(training);
        bvp::interpolator<piecewise_polynomial> riletto;

        // se qui vado a salvare u..
        // non va bene perchè se non sbaglio è un puntatore a memoria,
        // quindi andrei a salvare l'inidirizzo del puntatore...
        // NON SONO SICURO QUI PERO'

        /*
        cout<<"sono qui 1"<<endl;
        //	class Data {
        //		int key;
        //		double value;
        //	};

        //Data x;
        //Data *y = new Data[10];
        //std::ifstream ifs(filename.c_str());
        std::fstream myFile ("data/salvaMatrice.bin", ios::in | ios::out |
        ios::binary);
        //myFile.seekp (location1);
        cout<<"sono qui 2"<<endl;
        myFile.write ((char*)&u, sizeof
        (bvp::interpolator<piecewise_polynomial>)); cout<<"sono qui 3"<<endl;
        myFile.seekg (0);
        // infatti qui si blocca perchè...alla fine non ho salvato i dati, ma un
        puntatore...

        //myFile.read ((char*)&riletto, sizeof
        (bvp::interpolator<piecewise_polynomial>)); cout<<"sono qui 4"<<endl;
        myFile.close();
        cout<<"sono qui 5"<<endl;
        */

        // Calcolo la stima e salvo i dati
        for (size_t i = 1; i <= target.rows(); i++) {
          slice s(1, target.cols());
          metrics_computed.push_back(u(target(i, s)));

          // NON VA...ora non uso quello letto da file
          // metrics_computed.push_back(riletto(target(i,s)));

          // cout<<"sono qui 7"<<endl;
          // fileOut<<target(i,s)<<u(target(i,s))<<endl;
        }
        metrics_matrix_estimated.push_back(metrics_computed);
        break;
      }
      case 2: {
        /// r^n log(r) with n even
        // cout<<"RBF chosen: r^n log(r), with n integer even"<<endl;
        // cout<<"n = "<<(int)inputRBFparam[i]<<endl;
        // cout<<"...working..."<<endl;
        if ((int)inputRBFparam[i] % 2 != 0) {
          cout << "Error: the parameter should be even " << endl;
          // cout<<"For this type of RBF n MUST be EVEN"<<endl;
          return 127;
        }

        // settare il parametro della radial basisi function di interesse
        thin_plate_spline::set_n(inputRBFparam[i]);

        // Define interpolator given the training set.
        bvp::interpolator<thin_plate_spline> u(training);

        for (size_t i = 1; i <= target.rows(); i++) {
          slice s(1, target.cols());
          metrics_computed.push_back(u(target(i, s)));
          // fileOut<<target(i,s)<<u(target(i,s))<<endl;
        }
        metrics_matrix_estimated.push_back(metrics_computed);
        break;

        break;
      }

      case 3: {

        /// sqrt(1+(eps*r)^2) with eps > 0
        // cout<<"RBF chosen: sqrt(1+(eps*r)^2), with eps > 0 real"<<endl;
        // cout<<"eps = "<<inputRBFparam[i]<<endl;
        // cout<<"...working..."<<endl;
        if (inputRBFparam[i] <= 0.0) {
          // cout<<"For this type of RBF MUST be eps >0"<<endl;
          cout << "Error: the parameter should be greater than 0" << endl;
          return 127;
        }

        // Change the parameter of the rbf.
        c_infty_rbf::set_epsilon(inputRBFparam[i]);

        // Define interpolator given the training set.
        bvp::interpolator<multiquadric> u(training);

        for (size_t i = 1; i <= target.rows(); i++) {
          slice s(1, target.cols());
          metrics_computed.push_back(u(target(i, s)));
          // fileOut<<target(i,s)<<u(target(i,s))<<endl;
        }
        metrics_matrix_estimated.push_back(metrics_computed);
        break;

        break;
      }

      case 4: {
        /// 1/sqrt(1 + (eps*r)^2) with eps > 0
        // cout<<"RBF chosen: 1/sqrt(1 + (eps*r)^2), with eps > 0 real"<<endl;
        // cout<<"eps = "<<inputRBFparam[i]<<endl;
        // cout<<"...working..."<<endl;
        if (inputRBFparam[i] <= 0.0) {
          cout << "Error: the parameter should be greater than 0" << endl;
          // cout<<"For this type of RBF MUST be eps >0"<<endl;
          return 127;
        }

        // Change the parameter of the rbf.
        c_infty_rbf::set_epsilon(inputRBFparam[i]);

        // Define interpolator given the training set.
        bvp::interpolator<inverse_multiquadric> u(training);

        for (size_t i = 1; i <= target.rows(); i++) {
          slice s(1, target.cols());
          metrics_computed.push_back(u(target(i, s)));
          // fileOut<<target(i,s)<<u(target(i,s))<<endl;
        }
        metrics_matrix_estimated.push_back(metrics_computed);
        break;

        break;
      }

      case 5: {
        /// exp(- (eps*r)^2) with eps > 0.
        // cout<<"RBF chosen: exp(- (eps*r)^2), with eps > 0 real"<<endl;
        // cout<<"eps = "<<inputRBFparam[i]<<endl;
        // cout<<"...working..."<<endl;
        if (inputRBFparam[i] <= 0.0) {
          cout << "Error: the parameter should be greater than 0" << endl;
          // cout<<"For this type of RBF MUST be eps >0"<<endl;
          return 127;
        }

        // Change the parameter of the rbf.
        c_infty_rbf::set_epsilon(inputRBFparam[i]);

        // Define interpolator given the training set.
        bvp::interpolator<gaussian> u(training);

        for (size_t i = 1; i <= target.rows(); i++) {
          slice s(1, target.cols());
          metrics_computed.push_back(u(target(i, s)));
          // fileOut<<target(i,s)<<u(target(i,s))<<endl;
        }
        metrics_matrix_estimated.push_back(metrics_computed);
        break;

        break;
      }
      default:
        cout << "Error: Unknown RBF chosen";
        return 128;
        break;
      }
    }

    // scrittura dell'output
    // qui ci sono parti ridondanti...

    // Output interpolated results.
    cout << endl << "Information: Writing predictions" << endl;

    std::vector<double> design;
    std::vector<double> metrics;
    std::ofstream fileOut(output.c_str());
    std::ifstream inp(inputTarget.c_str());

    std::vector<std::vector<double> > design_matrix;

    while (gpl_xdr_read_design(inp, design, metrics)) {
      design_matrix.push_back(design);
    }

    for (int i = 0; i < design_matrix.size(); i++) {
      std::vector<double> prediction_vector;
      for (int k = 0; k < metrics_matrix_estimated.size(); k++) {
        prediction_vector.push_back(metrics_matrix_estimated[k][i]);
      }
      gpl_xdr_write_raw_data(fileOut, design_matrix[i], prediction_vector);
    }

    fileOut.close();

    // cout << "...file "<<output<<" written." << endl;
    return 0;
  } catch (error_handling::error &exc) {
    utils::show_exception(exc);
    return 127;
  }
  return 0;
}

//*******************************************************************
//*		 						Functions
//*
//*******************************************************************

// ok, working
matrix read_matrix_from_xdr(std::string filename) {

  std::ifstream ifs(filename.c_str());
  if (!ifs) {
    error_handling::badArgument exc;
    exc.reason = "Cannot open file ";
    exc.reason += filename;
    exc.line = __LINE__;
    exc.file = __FILE__;
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

  for (int j = 1; j <= numRow; j++) {
    for (int i = 1; i <= numCol; i++) {
      // cout<<"sn 4";
      v(j, i) = design_matrix[j - 1][i - 1];
      //	cout<<v(j, i)<<" ";
    }
    // cout<<endl;
  }

  ifs.close();

  return v;
}

// ok, working
std::map<linalg::point, std::vector<double> >
read_all_training_from_xdr(std::string filename) {
  std::ifstream ifs(filename.c_str());
  if (!ifs) {
    error_handling::badArgument exc;
    exc.reason = "Cannot open file ";
    exc.reason += filename;
    exc.line = __LINE__;
    exc.file = __FILE__;
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

  for (int j = 1; j <= numRow; j++) {
    for (int i = 1; i <= numColPoint; i++) {
      // cout<<"sn 4";
      M(j, i) = design_matrix[j - 1][i - 1];
      // cout<<M(j, i)<<" ";
    }
    // cout<<endl;
  }

  if (M.cols() < 2) {
    error_handling::badArgument exc;
    exc.reason = "Input matrix to read_pd_map is too narrow. \n"
                 "Need at least two columns in the input matrix";
    exc.line = __LINE__;
    exc.file = __FILE__;
    throw exc;
  }

  std::map<linalg::point, std::vector<double> > result;

  size_t m = M.cols();
  // come etichetta uso l'intera riga contenente tutti i punti
  linalg::slice s(1, m);

  for (int j = 1; j <= M.rows(); j++) {
    for (int i = 1; i <= numColMetrics; i++) {
      //	for(size_t i = 1; i <= M.rows(); i++)
      result[M(j, s)] = metrics_matrix[j - 1];
    }
    // cout<<" "<< (result[M(j,s)]).at(0);
    // cout<<" "<< (result[M(j,s)]).at(1);
    // cout<<endl;
  }
  return result;
}

// Passata la mappa contenente tutte le metriche registrate per ogni coordinata
// ritorna una mappa con una sola metrica scelta (parametro interested_metric)
// per ogni coordinata.
// In questo modo posso la map ottenuta viene direttamente mandata
// all'interpolator.
map<point, double>
training_of_one_metric(map<point, std::vector<double> > trainingAll,
                       int intersted_metric) {

  map<point, double> train;

  for (map<point, std::vector<double> >::const_iterator i = trainingAll.begin();
       i != trainingAll.end(); ++i) {

    // assegnamento alla mappa della sola metrica specificata da parametro
    train[i->first] = (i->second).at(intersted_metric);
  }
  return train;
}
