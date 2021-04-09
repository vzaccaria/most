/**
 * This is a very simple example of RSM plug-in.
 */

#include "../../gpl_xdr_api.h"
#include <iostream>
#include <fstream>
#include <string>
#include "parameters.h"
#include "r_wrapper.hpp"

using namespace std;
void help();

int main(int argc, char **argv) {
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  string training_data_file_name;
  string prediction_set_file_name;
  string predicted_data_file_name;
  Parameters param;

  {
    string option;
    for (unsigned int i = 1; i < argc; i++) {
      option.clear();
      option = argv[i];

      if ((option == "-h") || (option == "--help")) {
        param.setOption<std::string>("help", "1");
      } else {
        if (option == "--trai") {
          param.setOption<std::string>("training", argv[++i]);
          continue;
        } else {
          if (option == "--pred") {
            param.setOption<std::string>("prediction", argv[++i]);
            continue;
          } else {
            if (option == "--inte") {
              param.setOption<std::string>("interaction", argv[++i]);
              continue;
            } else {
              if (option == "--subs") {
                param.setOption<std::string>("subset", argv[++i]);
                const double th = param.getOption<double>("subset");
                if (th > 0.1 || th < 0.01) {
                  cout << "subset allowable values are between 0.01 and 0.1\n";
                  return -1;
                }
                continue;
              } else {
                if (option == "--clean") {
                  param.setOption<std::string>("clean", "1");
                  continue;
                } else {
                  if (option == "--lmco") {
                    param.setOption<std::string>("linearmodelcomparison", "1");
                    continue;
                  } else {
                    if (option == "--eran") {
                      param.setOption<std::string>("eran", "1");
                      continue;
                    } else {
                      if (option == "--output") {
                        param.setOption<std::string>("out", argv[++i]);
                        continue;
                      }
                      cout << "ERROR: Parameters error\n";
                      return -1;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    // Eventualy show help and exit
    if (param.isOption("help") || argc == 1) {
      param.help();
      return 0;
    }
    // Parameters rules
    if (!(param.isOption("training"))) {
      cout << "ERROR: Need to specify the training set file\n";
      return -1;
    }
    if (param.isOption("prediction") &&
        param.isOption("linearmodelcomparison")) {
      cout << "ERROR: prediction and linear model comparison could not be "
              "asserted at the same time\n";
      return -1;
    }
    if (!param.isOption("prediction") && !param.isOption("subset")) {
      cout << "Default subset=0.09 selected\n";
      param.setOption<std::string>("subset", "0.09");
    }
    if (param.isOption("prediction") && param.isOption("subset")) {
      cout << "Prediction set specified...subset flag is ignored...usage "
              "training set = 100%\n";
    }
    if (param.isOption("prediction") && param.isOption("eran")) {
      cout << "ERROR: Error analysis file could not be generated if a "
              "prediction set file is specified\n";
      return -1;
    }
    if (!param.isOption("out")) {
      param.setOption<std::string>("out", "result.db");
    }
    // Show parameters prefersnces
    if (param.isOption("training"))
      cout << "Training file: " << param.getOption<string>("training") << "\n";
    if (param.isOption("prediction")) {
      cout << "Prediction file: " << param.getOption<string>("prediction")
           << "\n";
    }
    if (param.isOption("interaction")) {
      cout << "Interaction file: " << param.getOption<string>("interaction")
           << "\n";
    }
    if (param.isOption("linearmodelcomparison")) {
      cout << "Comparison among spline and linear models requested\n";
    }
    if (param.isOption("eran")) {
      cout << "Error analysis file will be produced\n";
    }
    if ((param.isOption("out"))) {
      cout << "Results will be produced in: " << param.getOption<string>("out")
           << "\n";
    }
  }

  training_data_file_name = param.getOption<string>("training");

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  vector<vector<double> > training_configurations;
  vector<vector<double> > training_observations;

  vector<double> training_conf;
  vector<double> observations;

  ifstream inp(training_data_file_name.c_str());

  cout << "Reading training set from " << training_data_file_name << endl;

  while (gpl_xdr_read_design(inp, training_conf, observations)) {
    training_configurations.push_back(training_conf);
    training_observations.push_back(observations);
  }
  inp.close();
  cout << "Read " << training_configurations.size()
       << "training configurations " << endl;
  if (training_configurations.size() == 0) {
    cout << "Error, at least a training configuration should be given. "
            "Quitting. "
         << endl;
    return 0;
  }

  /* Now we have the following data:
   *
   *    TRAINING_CONFIGURATIONS         TRAINING_OBSERVATIONS
   *  [ x1_1 x1_2 .....    x1_n ]       [ f1_1 f1_2 ... f1_k ]
   *  [ x2_1 x2_2 .....    x2_n ]       [ f2_1 f2_2 ... f2_k ]
   *  [ x3_1 x3_2 .....    x3_n ]       [ f3_1 f3_2 ... f3_k ]
   *  ......
   *  [ xj_1 xj_2 .....    xj_n ]       [ fj_1 fj_2 ... fj_k ]
   */

  vector<vector<double> > prediction_configurations;
  if (param.isOption("prediction")) {
    prediction_set_file_name = param.getOption<std::string>("prediction");
    cout << "Reading prediction set from " << prediction_set_file_name << endl;
    inp.open(prediction_set_file_name.c_str());

    vector<double> empty;
    vector<double> prediction_conf;

    while (gpl_xdr_read_design(inp, prediction_conf, empty)) {
      prediction_configurations.push_back(prediction_conf);
    }
    inp.close();
    cout << "Read " << prediction_configurations.size() << " configurations "
         << endl;
  }
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // creates entry files for R

  if (param.isOption("prediction")) {
    r_wrapper R(param, training_configurations, training_observations,
                prediction_configurations);
    R.start();
  } else {
    r_wrapper R(param, training_configurations, training_observations);
    R.start();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  cout << "Exiting." << endl;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
