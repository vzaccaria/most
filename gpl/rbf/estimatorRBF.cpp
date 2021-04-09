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
using namespace bvp;

// ATTENZIONE TUTTO IL FILE E' UN PO' SPORCO, sarebbe bello usare i template, ma
// c'è errore da qualche parte nella gerarchia delle classi...

//*******************************************************************
//*		 						Prototipes
//*
//*******************************************************************
void print_prompt();
matrix read_matrix_from_xdr(std::string filename);
std::map<linalg::point, std::vector<double> >
read_all_training_from_xdr(std::string filename);
map<point, double>
training_of_one_metric(map<point, std::vector<double> > trainingAll,
                       int intersted_metric);
void string_split(string str, string delim, std::vector<string> &results);

template <typename RBF>
void read_RBF_param_file(std::vector<int> &typeRBF,
                         std::vector<double> &paramRBF,
                         std::vector<RBF> &rbfsComputed,
                         std::vector<double> &lambdaComputed,
                         ifstream &filePar);

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

  cout << "RBF ESTIMATOR - (c) 2008 Politecnico di Milano" << endl;

  if (argc != 5) {
    print_prompt();
    exit(1);
  }
  // std::cout<<"1"<<std::endl;
  // Ricavo dai parametri in ingresso il numero di metriche su cui dovro'
  // lavorare
  int numMetrics = atoi(argv[1]);
  // Salvo in stringhe i nomi dei file cui dovro' accedere in lettura e/
  // scrittura
  string inputParam = argv[2];
  string inputTarget = argv[3];
  string output = argv[4];
  // Elimino il file su cui scrivero' l'output qualora fosse già presente
  string command = string("rm -f ") + output;
  system(command.c_str());

  // Dichiaro i vector per salvare i tipi di RBF ed i loro parametri che
  // recupero dal file
  std::vector<int> inputRBFtype;
  std::vector<double> inputRBFparam;
  // Struttura dati contenente i risultati
  std::vector<std::vector<double> > metrics_matrix_estimated;

  // Qui sotto funziona, ma soluzione sporca...
  //	std::vector <piecewise_polynomial> rbfsComputed_poly;

  /*std::vector <thin_plate_spline> rbfsComputed_thin;
   std::vector <multiquadric> rbfsComputed_multi;
   std::vector <inverse_multiquadric> rbfsComputed_inv_multi;
   std::vector <gaussian> rbfsComputed_gauss;
   */
  //	std::cout<<"2"<<std::endl;
  gsl_set_error_handler(&error_handling::errorHandler);
  try {

    // Leggo i dati di target dal file
    matrix target = read_matrix_from_xdr(inputTarget);
    //	std::cout<<"3"<<std::endl;

    /*
     // Apro lo stream per leggere i parametri della RBF da usare
     std::ifstream fileParams(inputParam.c_str(), ios::in);
     std::cout<<"4 stream apeto"<<std::endl;

     // Per ogni metrica:
     for(int i=0; i<numMetrics;i++) {
     std::cout<<"5-"<<i<<std::endl;
     // Struttura contenente i valori di lambda
     std::vector <double> inputLambda;
     inputLambda.clear();

     // Questa era una buona soluzione, ma non funziona nella gerarchia della
     classi un qualche cosa
     //std::vector <radial_basis_function> rbfsComputed;
     //rbfsComputed.clear();

     rbfsComputed_poly.clear();

     /*rbfsComputed_thin.clear();
     rbfsComputed_multi.clear();
     rbfsComputed_inv_multi.clear();
     rbfsComputed_gauss.clear();
     std::cout<<"4"<<std::endl;
     //Leggo la prima riga...
     // e poi decido in base a quella che tipo di RBF reader usare...

     // Leggo dal file i parametri della RBF che docro' utilizzare...
     string fileLine;
     std::vector <string> dataInFileLine;

     dataInFileLine.clear();
     // Leggo la riga g-esima del file ed esco se è EOF
     if (getline(fileParams, fileLine).eof()) {
     std::cout<<"End of RBF."<<std::endl;
     break;
     }

     // Spezzo la riga in più stringhe
     string_split(fileLine, " ", dataInFileLine);

     std::cout<<"6-spezzata"<<std::endl;

     if (strcmp("RBF", dataInFileLine[0].c_str())==0) {
     // Se inizia con RBF allora è l'intestazione di una nuova RBF e ne rilevo
     quindi i parametri e il tipo
     // 1° dato è il tipo della RBF che uso

     inputRBFtype.push_back(atoi(dataInFileLine[1].c_str()));
     // 2° dato è il parametro della RBF che uso
     inputRBFparam.push_back(atof(dataInFileLine[2].c_str()));
     // 3° è la dimensione
     //int dimension = atoi(dataInFileLine[3].c_str());
     //radial_basis_function::set_dimension(dimension);

     std::cout<<"6-prima switch"<<std::endl;
     switch(inputRBFtype[i]) {
     case 1: read_RBF_param_file(inputRBFtype, inputRBFparam, rbfsComputed_poly,
     inputLambda, fileParams); break;

     /*case 2: read_RBF_param_file(inputRBFtype, inputRBFparam,
     rbfsComputed_thin, inputLambda, fileParams); break; case 3:
     read_RBF_param_file(inputRBFtype, inputRBFparam, rbfsComputed_multi,
     inputLambda, fileParams); break; case 4: read_RBF_param_file(inputRBFtype,
     inputRBFparam, rbfsComputed_inv_multi, inputLambda, fileParams);break; case
     5: read_RBF_param_file(inputRBFtype, inputRBFparam, rbfsComputed_gauss,
     inputLambda, fileParams);break;

     }

     }
     std::cout<<"7-dopo read"<<std::endl;

     */

    std::ifstream fileParams(inputParam.c_str(), ios::in);

    // cout<<"3"<<endl;
    std::vector<piecewise_polynomial> rbfsComputed_poly;
    std::vector<thin_plate_spline> rbfsComputed_thin;
    std::vector<multiquadric> rbfsComputed_multi;
    std::vector<inverse_multiquadric> rbfsComputed_inv_multi;
    std::vector<gaussian> rbfsComputed_gauss;

    // cout<<"4"<<endl;
    // Per ogni metrica:
    for (int i = 0; i < numMetrics; i++) {

      string fileLine;
      std::vector<string> dataInFileLine;
      std::vector<double> inputLambda;
      inputLambda.clear();
      int dimension;
      //	cout<<"prima di leggere file"<<endl;
      bool ender = false;
      int g = 0;
      do {
        dataInFileLine.clear();
        // leggo la riga i-esima del file
        if (getline(fileParams, fileLine).eof())
          break;

        //	cout<<"letta riga"<<endl;

        // spezzo la riga
        string_split(fileLine, " ", dataInFileLine);
        if (strcmp("RBF", dataInFileLine[0].c_str()) == 0) {
          //	cout<<"in RBF"<<endl;
          // 1° dato è il tipo della RBF che uso
          inputRBFtype.push_back(atoi(dataInFileLine[1].c_str()));

          // 2° dato è il parametro della RBF che uso
          inputRBFparam.push_back(atof(dataInFileLine[2].c_str()));

          // 3° è la dimensione
          dimension = atoi(dataInFileLine[3].c_str());
        } else if (strcmp("END_RBF", dataInFileLine[0].c_str()) == 0) {
          // cout<<"in RBF_END"<<endl;
          ender = true;
        } else if (fileParams != false) {
          //	cout<<"in recupero dati"<<endl;
          point p(dimension);
          int f;
          // costruisco il vettore dei parametri per la data RBF
          for (f = 1; f <= dimension; f++) {
            p(f) = atof(dataInFileLine[f - 1].c_str());
          }
          //	cout<<"iz: "<<p.size()<<endl;

          switch (inputRBFtype[i]) {
          case 1: {
            piecewise_polynomial::set_dimension(p.size());
            piecewise_polynomial r(p);
            rbfsComputed_poly.push_back(r);
            break;
          }

          case 2: {
            thin_plate_spline::set_dimension(p.size());
            thin_plate_spline r(p);
            rbfsComputed_thin.push_back(r);
            break;
          }
          case 3: {
            multiquadric::set_dimension(p.size());
            multiquadric r(p);
            rbfsComputed_multi.push_back(r);
            break;
          }
          case 4: {
            inverse_multiquadric::set_dimension(p.size());
            inverse_multiquadric r(p);
            rbfsComputed_inv_multi.push_back(r);
            break;
          }
          case 5: {
            gaussian::set_dimension(p.size());
            gaussian r(p);
            rbfsComputed_gauss.push_back(r);
            break;
          }
          }

          // rbfsComputed[g].set_dimension(p.size());

          // carico la lambda corrispondente...

          inputLambda.push_back(atof(dataInFileLine[f - 1].c_str()));
          g++;
        } // else

      } while (ender == false);
      //	cout<<"dopop di leggere file"<<endl;

      // Struttura dati contenente le stime per una singola metrica, usata come
      // struttura temporanea
      std::vector<double> metrics_computed;
      metrics_computed.clear();

      cout << "## Metrics: " << i << " ##" << endl;

      switch (inputRBFtype[i]) {
      case 1: {
        cout << "RBF chosen: r^n, with n integer odd" << endl;
        cout << "n = " << (int)inputRBFparam[i] << endl;
        cout << "...working..." << endl;
        /// r^n with n odd
        if ((int)inputRBFparam[i] % 2 == 0) {
          cout << "For this type of RBF n MUST be ODD" << endl;
          // Salto la stima di questa metrica, visto l'errore nei parametri
          continue;
        }

        // Define interpolator given the lambda and RBF computed set.
        // std::vector<piecewise_polynomial>
        // rbfsComputed_temp(rbfsComputed.begin(), rbfsComputed.end());

        // Settare il parametro della radial basisi function di interesse
        piecewise_polynomial::set_n((size_t)inputRBFparam[i]);
        bvp::interpolator<piecewise_polynomial> u(inputLambda,
                                                  rbfsComputed_poly, false);
        // Calcolo la stima e salvo i dati
        for (size_t i = 1; i <= target.rows(); i++) {
          slice s(1, target.cols());
          metrics_computed.push_back(u(target(i, s)));
        }
        metrics_matrix_estimated.push_back(metrics_computed);

        break;
      }
      case 2: {
        /// r^n log(r) with n even
        cout << "RBF chosen: r^n log(r), with n integer even" << endl;
        cout << "n = " << (int)inputRBFparam[i] << endl;
        cout << "...working..." << endl;
        if ((int)inputRBFparam[i] % 2 != 0) {
          cout << "For this type of RBF n MUST be EVEN" << endl;
          continue;
        }

        // settare il parametro della radial basisi function di interesse
        thin_plate_spline::set_n((size_t)inputRBFparam[i]);

        // Define interpolator given the training set.
        // bvp::interpolator<thin_plate_spline> u(training);
        // bvp::interpolator<piecewise_polynomial> u(inputLambda,
        // rbfsComputed_poly, false);
        bvp::interpolator<thin_plate_spline> u(inputLambda, rbfsComputed_thin,
                                               false);

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
        cout << "RBF chosen: sqrt(1+(eps*r)^2), with eps > 0 real" << endl;
        cout << "eps = " << inputRBFparam[i] << endl;
        cout << "...working..." << endl;
        if (inputRBFparam[i] <= 0.0) {
          cout << "For this type of RBF MUST be eps >0" << endl;
          continue;
        }

        // Change the parameter of the rbf.
        c_infty_rbf::set_epsilon(inputRBFparam[i]);

        // Define interpolator given the training set.
        bvp::interpolator<multiquadric> u(inputLambda, rbfsComputed_multi,
                                          false);

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
        cout << "RBF chosen: 1/sqrt(1 + (eps*r)^2), with eps > 0 real" << endl;
        cout << "eps = " << inputRBFparam[i] << endl;
        cout << "...working..." << endl;
        if (inputRBFparam[i] <= 0.0) {
          cout << "For this type of RBF MUST be eps >0" << endl;
          continue;
        }

        // Change the parameter of the rbf.
        c_infty_rbf::set_epsilon(inputRBFparam[i]);

        // Define interpolator given the training set.
        // bvp::interpolator<inverse_multiquadric> u(training);
        bvp::interpolator<inverse_multiquadric> u(
            inputLambda, rbfsComputed_inv_multi, false);

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
        cout << "RBF chosen: exp(- (eps*r)^2), with eps > 0 real" << endl;
        cout << "eps = " << inputRBFparam[i] << endl;
        cout << "...working..." << endl;
        if (inputRBFparam[i] <= 0.0) {
          cout << "For this type of RBF MUST be eps >0" << endl;
          continue;
        }

        // Change the parameter of the rbf.
        c_infty_rbf::set_epsilon(inputRBFparam[i]);

        // Define interpolator given the training set.
        bvp::interpolator<gaussian> u(inputLambda, rbfsComputed_gauss, false);
        // bvp::interpolator<gaussian> u(training);

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
        cout << "Error !";

      } // switch

      cout << "Done." << endl << endl;
    }

    // scrittura dell'output
    // qui ci sono parti ridondanti...

    // Output interpolated results.
    std::cout << "Writing file with interpolated values... " << endl;

    std::vector<double> design;
    std::vector<double> metrics;
    std::ofstream fileOut(output.c_str());
    std::ifstream inp(inputTarget.c_str());

    std::vector<std::vector<double> > design_matrix;

    while (gpl_xdr_read_design(inp, design, metrics)) {
      design_matrix.push_back(design);
    }

    for (int i = 0; i < design_matrix.size(); i++) {
      std::vector<double> union_vector;

      union_vector.push_back(design_matrix[i].size());

      // avendo in lib solo funzione che stampa il solo vect, ma nn sarebbe
      // tuile per il TS avere che stampa dati i 2 vettori?
      // poichè solo i param mi sono utili per i punti da stimare, ma per il TS
      // mi serve tutto !!! vedere...

      for (int j = 0; j < design_matrix[i].size(); j++) {
        union_vector.push_back(design_matrix[i][j]);
      }

      union_vector.push_back(metrics_matrix_estimated.size());

      for (int k = 0; k < metrics_matrix_estimated.size(); k++) {
        union_vector.push_back(metrics_matrix_estimated[k][i]);
      }

      gpl_xdr_write_vector(fileOut, union_vector);
    }

    fileOut.close();

    cout << "...file " << output << " written." << endl;
    return 0;
  } catch (error_handling::error &exc) {
    utils::show_exception(exc);
    return 1;
  }
}

//*******************************************************************
//*		 						Functions
//*
//*******************************************************************
void print_prompt() {

  cout << "Error: Please specify correct parameters !" << endl
       << "Usage: ./estimatorRBF <num_of_metrics> "
          "<file_precalculated_RBF_parameters> <file_target_set> <output_file>"
       << endl;
}

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
      v(j, i) = design_matrix[j - 1][i - 1];
    }
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
// Passata una stringa ed un'altra stringa come delimitatore scrive nel terzo
// parametro "result", che è passato come indirizzo, le componenti della
// stringa originale separate dal deliminitatore indicato.

void string_split(string str, string delim, std::vector<string> &results) {
  // using namespace std;

  unsigned int cutAt;
  while ((cutAt = str.find_first_of(delim)) != str.npos) {
    if (cutAt > 0) {
      results.push_back(str.substr(0, cutAt));
    }
    str = str.substr(cutAt + 1);
  }
  if (str.length() > 0) {
    results.push_back(str);
  }
}
/*
 template<typename RBF> void read_RBF_param_file(std::vector <int> &typeRBF,
 std::vector<double> &paramRBF, std::vector<RBF> &rbfsComputed,
 std::vector<double> &lambdaComputed, ifstream &filePar) {

 int dimension, temp_typeRBF;
 bool ender=false;
 // Stringa temporanea leggendo il file riga per riga
 string fileLine;
 // Contiene la stringa letta spezzandone i singoli campi
 std::vector <string> dataInFileLine1;
 std::cout<<"intro 0"<<std::endl;
 do {
 // Pulisco da righe già lette
 dataInFileLine1.clear();
 // Leggo la riga g-esima del file ed esco se è EOF
 if (getline(filePar, fileLine).eof()) {
 std::cout<<"End of RBF."<<std::endl;
 break;
 }
 std::cout<<"intro dopo getline"<<std::endl;
 // Spezzo la riga in più stringhe
 string_split(fileLine, " ", dataInFileLine1);

 if (strcmp("RBF", dataInFileLine1[0].c_str())==0) {
 // Se inizia con RBF allora è l'intestazione di una nuova RBF e ne rilevo
 quindi i parametri e il tipo
 // 1° dato è il tipo della RBF che uso
 temp_typeRBF = atoi(dataInFileLine1[1].c_str());
 typeRBF.push_back(temp_typeRBF);
 // 2° dato è il parametro della RBF che uso
 paramRBF.push_back(atof(dataInFileLine1[2].c_str()));
 // 3° è la dimensione
 // dimension = atoi(dataInFileLine[3].c_str());
 //RBF::set_dimension(dimension);

 } else if (strcmp("END_RBF", dataInFileLine1[0].c_str())==0) {
 // Se inizia con END_RBF allora è la riga finale della RBF
 ender =true;
 } else if (filePar!=false) {
 // Altrimenti è una riga contenente i parametri della RBF stessa
 point p(dimension);
 int f;
 std::cout<<"intro 00"<<std::endl;
 // Costruisco il vettore dei parametri per la data RBF
 for (f = 1; f<=dimension; f++) {
 p(f) =atof(dataInFileLine1[f-1].c_str());
 }

 // qui il casino !!!!
 std::cout<<"intro 1"<<std::endl;
 RBF::set_dimension(p.size());
 RBF r(p);
 rbfsComputed.push_back(r);

 std::cout<<"intro 2"<<std::endl;

 // Carico la lambda corrispondente...partendo da 1 la f...
 f--;
 lambdaComputed.push_back(atof(dataInFileLine1[f].c_str()));

 }

 } while (ender==false);

 }

 */
