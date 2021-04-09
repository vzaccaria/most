/**
 * GPL Regressor.
 */

#include "gpl_xdr_api.h"
#include <iostream>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_cdf.h>
#include <assert.h>
#include <algorithm>

void compute_F_and_p_and_Radj_value(vector<double> &coeff,
                                    vector<vector<double> > &designs,
                                    vector<double> &y, double &F, double &pval,
                                    double &Radj, vector<double> &se,
                                    vector<double> &t, vector<double> &p_t);

void compute_residuals_and_zj(vector<double> &coeff,
                              vector<vector<double> > &designs,
                              vector<double> &y, vector<double> &residuals,
                              vector<double> &zj);

int main(int argc, char **argv) {
  cout << "Information: Regressor - (c) 2008 Politecnico di Milano" << endl;
  if (argc < 3) {
    cout << "Error: Please specify input and output file" << endl;
    return 127;
  }
  string input = argv[1];
  string output = argv[2];

  string command = string("rm -f ") + argv[2];
  system(command.c_str());

  vector<vector<double> > design_matrix;
  vector<vector<double> > metrics_matrix;

  vector<double> design;
  vector<double> metrics;

  ifstream inp(argv[1]);
  while (gpl_xdr_read_design(inp, design, metrics)) {
    // cout << "Regressor found " << design.size() << " " << metrics.size() <<
    // endl;
    design_matrix.push_back(design);
    metrics_matrix.push_back(metrics);
  }
  inp.close();
  if (design_matrix.size() == 0) {
    cout << "Error: Please specify a suitable input file " << endl;
    return 127;
  }

  int dim = design_matrix[0].size();
  int n = design_matrix.size();

  assert(n == metrics_matrix.size());

  /** These are only computed for the last element... This assumes
   * metrics_matrix is a vector :-)*/
  vector<double> residuals;
  vector<double> zj;

  vector<vector<double> > coeffs;
  for (int i = 0; i < metrics_matrix[0].size(); i++) {
    gsl_matrix *X, *cov;
    gsl_vector *y, *c;
    vector<double> current_observations;

    X = gsl_matrix_alloc(n, dim + 1);
    y = gsl_vector_alloc(n);
    c = gsl_vector_alloc(dim + 1);
    cov = gsl_matrix_alloc(dim + 1, dim + 1);

    for (int des = 0; des < n; des++) {
      gsl_matrix_set(X, des, 0, 1.0);
      for (int j = 1; j < dim + 1; j++) {
        assert((j - 1) < design_matrix[des].size());
        // current_design.push_back(design_matrix[des][j-1]);

        gsl_matrix_set(X, des, j, design_matrix[des][j - 1]);
      }
      assert(des < metrics_matrix.size());
      assert(i < metrics_matrix[des].size());
      // cout << "Information: data in " << metrics_matrix[des][i] << endl;
      gsl_vector_set(y, des, metrics_matrix[des][i]);
      current_observations.push_back(metrics_matrix[des][i]);
    }
    double chisq;

    gsl_multifit_linear_workspace *work = gsl_multifit_linear_alloc(n, dim + 1);
    gsl_multifit_linear(X, y, c, cov, &chisq, work);
    gsl_multifit_linear_free(work);

    vector<double> se;
#define COV(i) (gsl_matrix_get(cov, (i), (i)))
    for (int s = 0; s < (dim + 1); s++) {
      se.push_back(sqrt(COV(s)));
    }

    vector<double> coeff;

    for (int z = 0; z < dim + 1; z++) {
      // cout << "Information: Coefficient found " << z << ": " <<
      // gsl_vector_get(c,z) << endl;
      coeff.push_back(gsl_vector_get(c, z));
    }

    double F;
    double p;
    double Radj;
    vector<double> t;
    vector<double> p_t;
    compute_F_and_p_and_Radj_value(coeff, design_matrix, current_observations,
                                   F, p, Radj, se, t, p_t);

    compute_residuals_and_zj(coeff, design_matrix, current_observations,
                             residuals, zj);

    coeff.push_back(F);
    coeff.push_back(p);
    coeff.push_back(Radj);

    for (int i = 0; i < t.size(); i++) {
      coeff.push_back(t[i]);
      coeff.push_back(p_t[i]);
    }

    coeffs.push_back(coeff);

    gsl_matrix_free(X);
    gsl_vector_free(y);
    gsl_vector_free(c);
    gsl_matrix_free(cov);
  }
  ofstream fout(argv[2]);
  for (int j = 0; j < coeffs.size(); j++) {
    gpl_xdr_write_vector(fout, coeffs[j]);
  }
  gpl_xdr_write_vector(fout, residuals);
  gpl_xdr_write_vector(fout, zj);
  fout.close();

  return 0;
}

double pred(vector<double> &coeff, vector<double> &design) {
  assert(coeff.size() == design.size() + 1);

  double res = coeff[0];
  // cout << "c0: " << res << endl;
  for (int i = 0; i < design.size(); i++) {
    // cout << "res (" << res << ") += ("<< design[i] << "*" << coeff[i+1] << ")
    // " ;
    res = res + design[i] * coeff[i + 1];
    // cout << "=> " << res << endl;
  }
  return res;
}

void compute_residuals_and_zj(vector<double> &coeff,
                              vector<vector<double> > &designs,
                              vector<double> &y, vector<double> &residuals,
                              vector<double> &zj) {
  assert(y.size());
  residuals.clear();
  for (int i = 0; i < designs.size(); i++) {
    double y_hat = pred(coeff, designs[i]);
    // cout << "pred: " << y_hat << " " << y[i] << endl;
    residuals.push_back(y_hat - y[i]);
  }
  sort(residuals.begin(), residuals.end());
  double mean = 0;
  for (int i = 0; i < residuals.size(); i++) {
    mean += residuals[i];
  }
  mean = mean / y.size();
  double variance = 0;
  for (int i = 0; i < residuals.size(); i++) {
    variance += pow(residuals[i] - mean, 2);
  }
  variance = variance / y.size();
  zj.clear();
  for (int i = 1; i <= residuals.size(); i++) {
    double prob = (i - 0.5) / residuals.size();
    /*double zig = gsl_cdf_gaussian_Pinv(prob, sqrt(variance));*/
    double zig = gsl_cdf_ugaussian_Pinv(prob);
    zj.push_back(zig);
  }
}

void compute_F_and_p_and_Radj_value(vector<double> &coeff,
                                    vector<vector<double> > &designs,
                                    vector<double> &y, double &F, double &pval,
                                    double &Radj, vector<double> &se,
                                    vector<double> &t, vector<double> &p_t) {
  assert(y.size());
  double mean = 0;
  for (int j = 0; j < y.size(); j++) {
    mean += y[j];
  }
  mean = mean / y.size();
  double SST = 0;
  double SSE = 0;
  for (int i = 0; i < designs.size(); i++) {
    double y_hat = pred(coeff, designs[i]);
    // cout << "pred: " << y_hat << " " << y[i] << endl;
    SST += pow(y[i] - mean, 2);
    SSE += pow(y_hat - y[i], 2);
  }
  int n = y.size();
  int p = coeff.size();
  F = ((SST - SSE) / (p - 1)) / (SSE / (n - p));
  pval = gsl_cdf_fdist_Q(F, p - 1, n - p);
  Radj = 1 - (SSE / (n - p)) / (SST / (n - 1));

  for (int i = 0; i < se.size(); i++) {
    if (se[i] == 0) {
      cout << "Warning: Coefficient estimation maybe incorrect, standard "
              "deviation found zero for predictor "
           << i << endl;
      cout << "Warning: Assuming a very small variance " << endl;
      se[i] = 0.000001;
    }
    double tval = coeff[i] / se[i];
    double ptval = 2.0 * gsl_cdf_tdist_Q(fabs(tval), n - p);
    t.push_back(tval);
    p_t.push_back(ptval);
  }
}
