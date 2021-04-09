/**
 * GPL Normal Random Distribution Computation. 
 */

#include "gpl_xdr_api.h"
#include <iostream>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_cdf.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <assert.h>

const gsl_rng_type *rng_type;
gsl_rng *rng;

void load_state()
{
    gsl_rng_env_setup();
    rng_type = gsl_rng_mt19937;
    rng = gsl_rng_alloc(rng_type);
    gsl_rng_set(rng,time(NULL));
}

void save_state()
{
}

int main(int argc, char **argv)
{
    /*cout << "Information: Normal - (c) 2008 Politecnico di Milano" << endl;*/
    if(argc<3)
    {
        cout << "Error: Please specify input and output file" << endl;
        return 0;
    }
    string input = argv[1];
    string output = argv[2];

    /*cout << argv[1] << " " << argv[2] << endl;*/
   
#define VAL0    0 
#define VAL1    1 
    vector<double> xstat;
    vector<double> ystat;

    load_state();

    vector<double> data;

    ifstream inp(argv[1]);
    ofstream fout(argv[2]);
    while(gpl_xdr_read_design(inp, xstat, ystat))
    {
        data.clear();
        double x = gsl_ran_flat(rng, xstat[VAL0], xstat[VAL1]+1);
        int pos= (int) floor((x));

        data.push_back(pos);

        gpl_xdr_write_vector(fout, data);
    }
    inp.close();
    fout.close();
    gsl_rng_free(rng);
    return 0;
}

