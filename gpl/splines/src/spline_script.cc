#include "spline_script.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <set>
#include <map>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <cstdlib>

using namespace std;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
spline_script::spline_script(Parameters &params) {
  this->param = params;
  this->pred_index.clear();
  this->var_prob.clear();
  this->trouble = false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
spline_script::~spline_script() {}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void spline_script::generate_spline_script(
    vector<vector<double> > configuration,
    vector<vector<double> > observation) {
  vector<unsigned int> index;
  if (param.isOption("subset") && !param.isOption("prediction")) {
    index = locate_configurations(configuration, observation);
    cout << "Configurations used " << index.size() << "\n";
  } else {
    calc_cardinalities(configuration);
  }

  const unsigned int nfun = observation.at(0).size();
  ofstream configs("spline_script.txt");

  configs << "## Loading necessary libraries\n";
  configs << "library(methods); library(survival);library(splines);\n";
  configs
      << "library(Hmisc); library(Design); library(lattice); library(grid);\n";
  configs << "## Loading file containing observation for training\n";
  configs << "data_model.df = read.csv(\"observation.csv\", header=TRUE);\n";
  configs << "dd = datadist(data_model.df);\n";
  configs << "options(datadist = 'dd');\n";
  configs << "describe(data_model.df); \n";

  const unsigned int nvar = configuration.at(0).size();
  for (unsigned int i = 0; i < nfun; i++) {
    configs << "#####################################################\n\n";
    configs << "## Model Spline Specification\n";
    configs << "m" << i << " = (f" << i << "~(## first-order effects\n";
    bool first_written = false;
    // variable with none domains are not used bacause they have just the value
    // 0 and this will stop the analysis becouse the corresponding matrix will
    // be singular and not invertible linear domain are those who has less than
    // 3 value in the domain, and in case just one, that is not equal to 0
    // spline domains has 3 or more values in the domain. Higher is the
    // cardinality, higher will be the degree examined
    {
      int card = 3; // this varible is used to specify the nuber of knots to use
                    // in spline,from 3 to max 5
      for (unsigned int j = 0; j < nvar; j++) {
        if (!first_written) {
          switch (this->domains.at(j)) {
          case none:
            break;
          case spline:
            if (this->trouble) {
              if (this->var_trouble.at(j) && this->poly.at(j) > 0) {
                configs << "	               pol(x" << j << ","
                        << this->poly.at(j) << ")\n";
                first_written = true;
              } else {
                if (this->cardinalities.at(j) > 5)
                  card = 5;
                else
                  card = cardinalities.at(j);

                configs << "	               rcs(x" << j << ",nk=" << card
                        << ")\n";
                first_written = true;
              }
            } else {
              if (this->cardinalities.at(j) > 5)
                card = 5;
              else
                card = this->cardinalities.at(j);

              configs << "	               rcs(x" << j << ",nk=" << card
                      << ")\n";
              first_written = true;
            }

            break;
          case linear:
            configs << "	               x" << j << "\n";
            first_written = true;
            break;
          }
        } else {
          switch (this->domains.at(j)) {
          case none:
            break;
          case spline:
            if (this->trouble) {
              if (this->var_trouble.at(j) && this->poly.at(j) > 0) {
                configs << "	               +pol(x" << j << ","
                        << this->poly.at(j) << ")\n";
                first_written = true;
              } else {
                if (this->cardinalities.at(j) > 5)
                  card = 5;
                else
                  card = this->cardinalities.at(j);

                configs << "	               +rcs(x" << j << ",nk=" << card
                        << ")\n";
                first_written = true;
              }
            } else {
              if (this->cardinalities.at(j) > 5)
                card = 5;
              else
                card = this->cardinalities.at(j);

              configs << "	               +rcs(x" << j << ",nk=" << card
                      << ")\n";
              first_written = true;
            }
            break;
          case linear:
            configs << "	               +x" << j << "\n";
            break;
          }
        }
      }
    }
    configs << "                   ## second-order effects\n";
    {
      // if no interaction file has been specified, the default is no
      // interaction at all
      if (param.isOption("interaction")) {
        vector<vector<unsigned int> > interaction;
        // interaction of the funcion are loaded from its corresponding xdr
        interaction =
            check_interaction(param.getOption<std::string>("interaction"), i);
        unsigned int ncorr = interaction.size();
        cout << "Loaded " << ncorr << " interactions for function f" << i
             << "\n";
        unsigned int nscorr = 0;
        vector<unsigned int> single_corr;
        bool write_on;
        for (unsigned int y = 0; y < ncorr; y++) {
          write_on = true;
          single_corr = interaction.at(y);
          nscorr = single_corr.size();
          // if one of the variables in the interaction examinated has only one
          // value in its domain and the value is zero then the entire
          // interaction is considered null and not written, this is needed to
          // avoid errors with R
          for (unsigned int j = 0; j < nscorr; j++) {
            if (this->domains.at(single_corr.at(j)) == none) {
              write_on = false;
              break;
            }
          }
          if (write_on) {
            configs << "	               +";
            {
              int card = 3;
              for (unsigned int j = 0; j < nscorr; j++) {
                if (this->domains.at(single_corr.at(j)) == spline) {
                  if (trouble) {
                    if (this->var_trouble.at(j) && this->poly.at(j) > 0)
                      configs << "pol(x" << j << "," << poly.at(j) << ")";
                    else {
                      if (cardinalities.at(j) > 5)
                        card = 5;
                      else
                        card = cardinalities.at(j);
                      configs << "rcs(x" << j << ",nk=" << card << ")";
                    }
                  } else {
                    if (cardinalities.at(j) > 5)
                      card = 5;
                    else
                      card = cardinalities.at(j);
                    configs << "rcs(x" << j << ",nk=" << card << ")";
                  }
                } else
                  configs << "x" << single_corr.at(j);
                if (j + 1 == nscorr)
                  configs << "\n";
                else
                  configs << " \%ia\% ";
              }
            }
          }
        }
      }
    }

    configs << "                  ));\n";

    if (param.isOption("linearmodelcomparison") &&
        !param.isOption("prediction")) {
      configs << "##Linear first order Model Specification\n";
      configs << "m" << i << "_l1 = (f" << i << "~(## first-order effects\n";
      for (unsigned int j = 0; j < nvar; j++) {
        if (this->domains.at(j) == none)
          continue;
        else
          configs << "	               x" << j;
        if (j + 1 == nvar)
          configs << "\n";
        else
          configs << "+\n";
      }
      configs << "                   ## second-order effects\n";
      {
        // if no interaction file has been specified, the default is no
        // interaction at all
        if (param.isOption("interaction")) {
          vector<vector<unsigned int> > interaction;
          // interaction of the funcion are loaded from its corresponding xdr
          interaction =
              check_interaction(param.getOption<std::string>("interaction"), i);
          unsigned int ncorr = interaction.size();
          unsigned int nscorr = 0;
          vector<unsigned int> single_corr;
          bool write_on;
          for (unsigned int y = 0; y < ncorr; y++) {
            write_on = true;
            single_corr = interaction.at(y);
            nscorr = single_corr.size();
            for (unsigned int j = 0; j < nscorr; j++) {
              if (this->domains.at(single_corr.at(j)) == none) {
                write_on = false;
                break;
              }
            }
            if (write_on) {
              configs << "	               +";
              for (unsigned int j = 0; j < nscorr; j++) {
                configs << "x" << single_corr.at(j);
                if (j + 1 == nscorr)
                  configs << "\n";
                else
                  configs << " \%ia\% ";
              }
            }
          }
        }
      }
      configs << "));\n";

      configs << "##Linear second order Model Specification\n";
      configs << "m" << i << "_l2 = (f" << i << "~(## first-order effects\n";
      for (unsigned int j = 0; j < nvar; j++) {
        if (this->domains.at(j) == none)
          continue;
        else
          configs << "	               pol(x" << j << ",2)";
        if (j + 1 == nvar)
          configs << "\n";
        else
          configs << "+\n";
      }
      configs << "                   ## second-order effects\n";
      {
        // if no interaction file has been specified, the default is no
        // interaction at all
        if (param.isOption("interaction")) {
          vector<vector<unsigned int> > interaction;
          // interaction of the funcion are loaded from its corresponding xdr
          interaction =
              check_interaction(param.getOption<std::string>("interaction"), i);
          unsigned int ncorr = interaction.size();
          unsigned int nscorr = 0;
          vector<unsigned int> single_corr;
          bool write_on;
          for (unsigned int y = 0; y < ncorr; y++) {
            write_on = true;
            single_corr = interaction.at(y);
            nscorr = single_corr.size();
            for (unsigned int j = 0; j < nscorr; j++) {
              if (this->domains.at(single_corr.at(j)) == none) {
                write_on = false;
                break;
              }
            }
            if (write_on) {
              configs << "	               +";
              for (unsigned int j = 0; j < nscorr; j++) {
                configs << "pol(x" << single_corr.at(j) << ",2)";
                if (j + 1 == nscorr)
                  configs << "\n";
                else
                  configs << " \%ia\% ";
              }
            }
          }
        }
      }
      configs << "));\n";
    }
  }
  configs << "#####################################################\n\n";
  configs << "data_train.df = data_model.df;\n";
  if (!param.isOption("prediction")) {
    configs << "#Loading Prediction Configurations already predicted\n";
    configs << "data_prediction_conf.df = "
               "read.csv(\"configuration_predicted.csv\", header=TRUE);\n";
  }
  configs << "#Loading Prediction configurations\n";
  configs << "prediction_conf.df = read.csv(\"pred_conf.csv\", header=TRUE);\n";

  for (unsigned int i = 0; i < nfun; i++) {
    configs << "#####################################################\n\n";
    configs << "##Spline Model\n";
    configs << "#Perform Ordinary Least Squares\n";
    configs << "fm" << i << " = ols(m" << i << ",data=data_train.df);\n";
    configs << "summary(fm" << i << ");\n";
    configs << "summary.lm(fm" << i << ");\n";
    configs << "#Perform prediction\n";
    configs << "p" << i << "=predict(object=fm" << i
            << ",newdata=prediction_conf.df);\n";
    // Error alnalysis could be performed only in model evaluation
    if (!param.isOption("prediction") && param.isOption("eran")) {
      configs << "o" << i << " = data_prediction_conf.df$f" << i << ";\n";
      // relative error
      configs << "e" << i << " = (o" << i << "-p" << i << ")/o" << i << ";\n";
      configs << "results = cbind(o" << i << ", p" << i << ", e" << i
              << "); \n";
      configs << "emax_" << i << "=max(e" << i << ");\n";
      configs << "emin_" << i << "=min(e" << i << ");\n";
      configs << "erm_" << i << " = mean(e" << i << ");\n";
      configs << "erm_abs_" << i << " = mean(abs(e" << i << "));\n";
      configs << "SST_" << i << " = sum((o" << i << "- mean(o" << i
              << "))^2)\n";
      configs << "SSE_" << i << " = sum((o" << i << "-p" << i << ")^2);\n";
      configs << "R2_" << i << " =1 -  SSE_" << i << "/SST_" << i << ";\n";
      configs << "MSE_" << i << " = mean((o" << i << "-p" << i << ")^2);\n";
      configs << "write.table(results, file=\"ope_f" << i
              << "_spline.csv\", "
                 "sep=\",\",row.names=FALSE,col.names=c(\"observed\","
                 "\"predicted\",\"error\"));\n\n";
    }

    if (param.isOption("linearmodelcomparison") &&
        !param.isOption("prediction") && param.isOption("eran")) {
      configs << "##First Linear Model\n";
      configs << "fm" << i << "_l1 = ols(m" << i
              << "_l1,data=data_train.df);\n";
      configs << "summary(fm" << i << "_l1);\n";
      configs << "summary.lm(fm" << i << "_l1);\n";
      configs << "p" << i << "_l1=predict(object=fm" << i
              << "_l1,newdata=prediction_conf.df);\n";
      configs << "o" << i << "_l1 = data_prediction_conf.df$f" << i << ";\n";
      // relative error
      configs << "e" << i << "_l1 = (o" << i << "_l1-p" << i << "_l1)/o" << i
              << "_l1;\n";
      configs << "results_l1 = cbind(o" << i << "_l1, p" << i << "_l1, e" << i
              << "_l1); \n";
      configs << "emax_" << i << "_l1=max(e" << i << "_l1);\n";
      configs << "emin_" << i << "_l1=min(e" << i << "_l1);\n";
      configs << "erm_" << i << "_l1 = mean(e" << i << "_l1);\n";
      configs << "erm_abs_" << i << "_l1 = mean(abs(e" << i << "_l1));\n";
      configs << "SST_" << i << "_l1 = sum((o" << i << "_l1- mean(o" << i
              << "_l1))^2);\n";
      configs << "SSE_" << i << "_l1 = sum((o" << i << "_l1-p" << i
              << "_l1)^2);\n";
      configs << "R2_" << i << "_l1 =1 -  SSE_" << i << "_l1/SST_" << i
              << "_l1;\n";
      configs << "MSE_" << i << "_l1 = mean((o" << i << "_l1-p" << i
              << "_l1)^2);\n";
      configs << "write.table(results_l1, file=\"ope_f" << i
              << "_l1.csv\", "
                 "sep=\",\",row.names=FALSE,col.names=c(\"observed\","
                 "\"predicted\",\"error\"));\n\n";
      configs << "##Second Linear Model\n";
      configs << "fm" << i << "_l2 = ols(m" << i
              << "_l2,data=data_train.df);\n";
      configs << "summary(fm" << i << "_l2);\n";
      configs << "summary.lm(fm" << i << "_l2);\n";
      configs << "p" << i << "_l2=predict(object=fm" << i
              << "_l2,newdata=prediction_conf.df);\n";
      configs << "o" << i << "_l2 = data_prediction_conf.df$f" << i << ";\n";
      // relative error
      configs << "e" << i << "_l2 = (o" << i << "_l2-p" << i << "_l2)/o" << i
              << "_l2;\n";
      configs << "results_l2 = cbind(o" << i << "_l2, p" << i << "_l2, e" << i
              << "_l2); \n";
      configs << "emax_" << i << "_l2=max(e" << i << "_l2);\n";
      configs << "emin_" << i << "_l2=min(e" << i << "_l2);\n";
      configs << "erm_" << i << "_l2 = mean(e" << i << "_l2);\n";
      configs << "erm_abs_" << i << "_l2 = mean(abs(e" << i << "_l2));\n";
      configs << "SST_" << i << "_l2 = sum((o" << i << "_l2- mean(o" << i
              << "_l2))^2);\n";
      configs << "SSE_" << i << "_l2 = sum((o" << i << "_l2-p" << i
              << "_l2)^2);\n";
      configs << "R2_" << i << "_l2 =1 -  SSE_" << i << "_l2/SST_" << i
              << "_l2\n";
      configs << "MSE_" << i << "_l2 = mean((o" << i << "_l2-p" << i
              << "_l2)^2);\n";
      configs << "write.table(results_l2, file=\"ope_f" << i
              << "_l2.csv\", "
                 "sep=\",\",row.names=FALSE,col.names=c(\"observed\","
                 "\"predicted\",\"error\"));\n";
      configs << "#####################################################\n\n";
    }
  }
  // all configuration and prediction are combined in order to write the
  // xdr_result containing the prediction of every configuration requested
  configs << "#####################################################\n\n";
  configs << "#Write Output File\n";
  configs << "out = cbind(" << configuration.at(0).size()
          << ",prediction_conf.df," << nfun << ",";
  for (unsigned int i = 0; i < nfun; i++) {
    configs << "p" << i;
    if (i + 1 == nfun) {
      configs << ");\n";
    } else {
      configs << ",";
    }
  }
  configs << "write.table(out, file=\"xdr_temp.db\",quote=FALSE, sep=\" "
             "\",row.names=FALSE,col.name=FALSE);\n";
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // spline model
  if (!param.isOption("prediction") && param.isOption("eran")) {
    configs << "err_spline = cbind(";
    for (unsigned int i = 0; i < nfun; i++) {
      configs << "R2_" << i << ",MSE_" << i << ",erm_abs_" << i << ",erm_" << i
              << ",emin_" << i << ",emax_" << i;
      if (i + 1 == nfun) {
        configs << ");\n";
      } else {
        configs << ",";
      }
    }
    configs << "write.table(err_spline, "
               "file=\"error_analysis_spline.csv\",quote=FALSE, "
               "sep=\",\",row.names=FALSE,col.name=TRUE);\n";
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  if (param.isOption("linearmodelcomparison") &&
      !param.isOption("prediction") && param.isOption("eran")) {
    // first order linear
    configs << "err_l1 = cbind(";
    for (unsigned int i = 0; i < nfun; i++) {
      configs << "R2_" << i << "_l1,MSE_" << i << "_l1,erm_abs_" << i
              << "_l1,erm_" << i << "_l1,emin_" << i << "_l1,emax_" << i
              << "_l1";
      if (i + 1 == nfun) {
        configs << ");\n";
      } else {
        configs << ",";
      }
    }
    configs
        << "write.table(err_l1, file=\"error_analysis_l1.csv\",quote=FALSE, "
           "sep=\",\",row.names=FALSE,col.name=TRUE);\n";
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // second order linear
    configs << "err_l2 = cbind(";
    for (unsigned int i = 0; i < nfun; i++) {
      configs << "R2_" << i << "_l2,MSE_" << i << "_l2,erm_abs_" << i
              << "_l2,erm_" << i << "_l2,emin_" << i << "_l2,emax_" << i
              << "_l2";
      if (i + 1 == nfun) {
        configs << ");\n";
      } else {
        configs << ",";
      }
    }
    configs
        << "write.table(err_l2, file=\"error_analysis_l2.csv\",quote=FALSE, "
           "sep=\",\",row.names=FALSE,col.name=TRUE);\n";
  }
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  configs.close();

  // write the script that will be used to run R
  cout << "\nWriting luncher script\n\n";
  ofstream script("r_wrapper.sh");
  script << "#!/bin/bash\n";
  script
      << "Rscript spline_script.txt > spline_script.log\n"; // 2>error.log\n";
  script << "cat spline_script.log | grep -e \"           NA         NA      "
            "NA       NA\" >> prob.log\n";
  for (unsigned int i = 0; i < nvar; i++)
    script << "cat prob.log | grep \"x" << i << "\" -o -m 1 >> var.log\n";
  script.close();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void spline_script::run_spline_script() {
  pid_t child;
  int status;
  child = fork();
  if (child == 0) {
    cout << "Processing the script\n\n";
    execlp("bash", "-f", "r_wrapper.sh", (char *)0);
  } else {
    waitpid(child, &status, 0);
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
vector<unsigned int>
spline_script::locate_configurations(vector<vector<double> > configuration,
                                     vector<vector<double> > observation) {
  set<unsigned int> index; // contains the indexes of the configuration that are
                           // to be used in the approximation
  vector<set<double> > dom;  // constins the domain value of each domain
  set<double> single_domain; // used as aux for the previous... used only in the
                             // initialization
  vector<double> single;     // used as aux for configurations
  int before;
  const unsigned int conf_size = configuration.size();
  const unsigned int nvar = configuration.at(0).size();

  single_domain.clear();
  for (unsigned int j = 0; j < nvar; j++) {
    dom.push_back(single_domain);
  }

  /////////////////////////////////////
  // randomize the training set
  {
    cout << "Randomize the training set\n";
    unsigned int r1 = 0, r2 = 0;
    srand(time(NULL));
    for (unsigned int i = 0; i < conf_size; i++) {
      do {
        r1 = (rand() % (conf_size - 1));
        r2 = (rand() % (conf_size - 1));
      } while (r1 == r2);
      swap(observation.at(r1), observation.at(r2));
      swap(configuration.at(r1), configuration.at(r2));
    }
  }
  /////////////////////////////////////
  for (unsigned int i = 0; i < conf_size; i++) {
    single = configuration.at(i);
    // spline need at least one point for every value of the variable's domain
    // in order to work
    for (unsigned int j = 0; j < nvar; j++) {
      before = dom.at(j).size();
      dom.at(j).insert(single.at(j));
      // if some new value is found then the position is recorded
      if (before < dom.at(j).size()) {
        index.insert(i);
      }
    }
  }
  // evaluates cardianlities in order to decide if use the rcs, variable only or
  // novariable in the model
  this->cardinalities.clear();
  {
    unsigned int dom_size;
    for (unsigned int j = 0; j < nvar; j++) {
      dom_size = dom.at(j).size();
      cardinalities.push_back(dom_size);
      cout << dom_size << " ";
      if (dom_size >= 3) {
        this->domains.push_back(spline);
      } else {
        if (dom_size > 1) {
          this->domains.push_back(linear);
        } else {
          this->domains.push_back(none);
        }
      }
    }
    cout << std::endl;
  }
  // genereate the returning vector with configurations

  set<unsigned int>::iterator set_end = index.end();
  // fill the training set till a given treshold
  if (param.isOption("subset")) // use a given amount of data
  {
    srand(time(NULL));
    const double th = param.getOption<double>("subset");
    cout << "Trining set percentage usage : " << th * 100 << "\%\n";
    while (index.size() <= conf_size * th) {
      index.insert(rand() % (conf_size - 1));
    }
    // write the observation file with the random configurations
    {
      ofstream obc("observation.csv");
      const char sep = ',';
      for (unsigned int i = 0; i < nvar; i++) {
        obc << "x" << i << sep;
      }
      const unsigned int nfun = observation.at(0).size();
      for (unsigned int i = 0; i < nfun; i++) {
        obc << "f" << i;
        if (i + 1 == nfun)
          obc << std::endl;
        else
          obc << sep;
      }
      for (unsigned int i = 0; i < conf_size; i++) {
        if (index.count(i) > 0) {
          for (unsigned int j = 0; j < nvar; j++) // write configuration
            obc << configuration.at(i).at(j) << sep;
          for (unsigned int j = 0; j < nfun; j++) // write observation
          {
            obc << (observation.at(i).at(j));
            if (j + 1 == nfun)
              obc << std::endl;
            else
              obc << sep;
          }
        }
      }
      obc.close();
    }
    ////////////////////////////////end writing observation_file randomized
    // if a prediction_file entry is not specified then the remaining 1-th
    // percentage of the trining set will be predicted
    if (!param.isOption("prediction")) {
      ofstream pred("pred_conf.csv");
      ofstream conf_pred("configuration_predicted.csv");
      const char sep = ',';
      for (unsigned int i = 0; i < nvar; i++) {
        pred << "x" << i;
        conf_pred << "x" << i << sep;
        if (i + 1 == nvar)
          pred << std::endl;
        else
          pred << sep;
      }
      const unsigned int nfun = observation.at(0).size();
      for (unsigned int i = 0; i < nfun; i++) {
        conf_pred << "f" << i;
        if (i + 1 == nfun)
          conf_pred << std::endl;
        else
          conf_pred << sep;
      }

      for (unsigned int i = 0; i < conf_size; i++) {
        if (!index.count(i)) {
          this->pred_index.push_back(i);
          for (unsigned int j = 0; j < nvar; j++) // write configuration
          {
            pred << configuration.at(i).at(j);
            conf_pred << configuration.at(i).at(j) << sep;
            if (j + 1 == nvar) {
              pred << std::endl;
            } else {
              pred << sep;
            }
          }
          for (unsigned int j = 0; j < nfun; j++) // write observation
          {
            conf_pred << (observation.at(i).at(j));
            if (j + 1 == nfun) {
              conf_pred << std::endl;
            } else {
              conf_pred << sep;
            }
          }
        }
      }
      pred.close();
      conf_pred.close();
    }
  }
  vector<unsigned int> res;
  set_end = index.end();
  for (set<unsigned int>::iterator set_it = index.begin(); set_it != set_end;
       set_it++) {
    res.push_back(*set_it);
  }
  return res;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
vector<vector<unsigned int> >
spline_script::check_interaction(string interact_file_name, unsigned int fnum) {
  ifstream interact_file(interact_file_name.c_str());
  vector<vector<unsigned int> > interaction;
  vector<unsigned int> single_corr;

  unsigned int i;

  if (!GOON(interact_file)) {
    cout << "INTERACTION LOADING FAILED\n";
    vector<vector<unsigned int> > res;
    return res;
  }
  while (GOON(interact_file)) {
    unsigned int dim = 0;
    unsigned int fn, aux;
    single_corr.clear();
    interact_file >> fn;
    interact_file >> dim;
    if (!GOON(interact_file))
      break;
    for (i = 0; i < dim && GOON(interact_file); i++) {
      interact_file >> aux;
      if (fn == fnum)
        single_corr.push_back(aux);
    }
    if (((single_corr.size() != dim) && (fn == fnum)) || (dim != i)) {
      cout << "INTERACTION LOADING FAILED\n";
      vector<vector<unsigned int> > res;
      return res;
    }
    if (fn == fnum)
      interaction.push_back(single_corr);
  }
  interact_file.close();
  // reorder each interaction vector
  unsigned int ncorr = interaction.size();
  {
    unsigned int nscorr;
    for (unsigned int i = 0; i < ncorr; i++) {
      nscorr = interaction.at(i).size();
      for (unsigned int j = 0; j < nscorr; j++) {
        for (unsigned int z = 0; z < nscorr - 1; z++) {
          if (interaction.at(i).at(z) > interaction.at(i).at(z + 1))
            swap(interaction.at(i).at(z), interaction.at(i).at(z + 1));
        }
      }
    }
  }
  // expand interactions and filter duplicates
  // expand: if x1,x2,x3 are correlated, is not possible to specify x1 %ia% x2
  // %ia% x3 alone, R will halt reporting that x2,x3 is not listed as a main
  // effect, thus is needed to expand the interaction to each subsequent group
  // of variables
  set<vector<unsigned int> > expanded;
  vector<unsigned int> aux;
  unsigned int nscorr;

  for (unsigned int i = 0; i < ncorr; i++) {
    single_corr = interaction.at(i);
    nscorr = single_corr.size();
    for (unsigned int z = 2; z <= nscorr; z++) {
      for (unsigned int j = 0; j < nscorr - z + 1; j++) {
        for (unsigned int l = 0; l < z; l++) {
          aux.push_back(single_corr.at(j + l));
        }
        expanded.insert(aux);
        aux.clear();
      }
    }
  }

  {
    interaction.clear();
    set<vector<unsigned int> >::iterator it_end = expanded.end();
    set<vector<unsigned int> >::iterator it;
    for (it = expanded.begin(); it != it_end; it++) {
      interaction.push_back(*(it));
    }
  }
  // reorder interaction in order to have an increasing size of interactions
  ncorr = interaction.size();
  for (unsigned int i = 0; i < ncorr; i++) {
    for (unsigned int j = 0; j < ncorr - 1; j++) {
      if (interaction.at(j).size() > interaction.at(j + 1).size())
        swap(interaction.at(j), interaction.at(j + 1));
    }
  }
  return interaction;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void spline_script::calc_cardinalities(vector<vector<double> > configuration) {
  vector<set<double> > dom;  // constins the domain value of each domain
  set<double> single_domain; // used as aux for the previous... used only in the
                             // initialization
  vector<double> single_conf;
  const unsigned int conf_size = configuration.size();
  const unsigned int nvar = configuration.at(0).size();

  single_domain.clear();
  for (unsigned int j = 0; j < nvar; j++) {
    dom.push_back(single_domain);
  }

  for (unsigned int i = 0; i < conf_size; i++) {
    single_conf = configuration.at(i);
    for (unsigned int j = 0; j < nvar; j++) {
      dom.at(j).insert(single_conf.at(j));
    }
  }

  // evaluates cardianlities in order to decide if use the rcs, variable only or
  // novariable in the model
  this->cardinalities.clear();
  {
    unsigned int dom_size;
    for (unsigned int j = 0; j < nvar; j++) {
      dom_size = dom.at(j).size();
      cardinalities.push_back(dom_size);
      cout << dom_size << " ";
      if (dom_size >= 3) {
        this->domains.push_back(spline);
      } else {
        if (dom_size > 1) {
          this->domains.push_back(linear);
        } else {
          this->domains.push_back(none);
        }
      }
    }
    cout << std::endl;
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool spline_script::check_problems(unsigned int nvar) {
  ifstream pred_var("var.log");
  if (!GOON(pred_var))
    return false;
  else {
    if (this->poly.size() == 0) // first time usage of this function
    {
      for (unsigned int i = 0; i < nvar; i++) {
        this->poly.push_back(this->cardinalities.at(i));
        this->var_trouble.push_back(false);
      }
    }
    var_prob.clear();
    string compare;
    string aux_s;
    string variable;
    while (GOON(pred_var)) {
      pred_var >> variable;
      for (unsigned int i = 0; i < nvar; i++) {
        compare.clear();
        {
          stringstream sstr;
          sstr << i;
          compare = "x" + sstr.str();
        }
        if (compare == variable) {
          this->var_prob.insert(i);
          this->var_trouble[i] = true;
          break;
        }
      }
    }
    pred_var.close();
    for (unsigned int i = 0; i < nvar; i++) {
      if (this->var_prob.count(i)) {
        if (this->poly.at(i) > 0) {
          const unsigned int aux = this->poly.at(i) - 1;
          this->poly[i] = aux;
        } else {
          this->poly[i] = 0;
        }
      }
    }
    for (unsigned int i = 0; i < nvar; i++) {
      if (this->var_prob.count(i)) {
        this->trouble = true;
        return true;
      }
    }
    return false;
  }
}
