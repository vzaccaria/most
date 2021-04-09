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
    gsl_rng_set(rng,0);

    /*
    ifstream inp(".normal_rng_state");
    if(inp.good())
    {
        void * state = gsl_rng_state (rng);
        size_t n = gsl_rng_size (rng);
        inp.read((char *)state, n);
        if(!inp.good())
        {
            cout << "Bad rng state" << endl;
            gsl_rng_set(rng,0);
        }
    }

    inp.close();
    */
}

void save_state()
{
    /*
    system("rm -rf .normal_rng_state");
    ofstream out(".normal_rng_state");
    void * state = gsl_rng_state (rng);
    size_t n = gsl_rng_size (rng); 
    out.write((char *) state,n);
    out.close();
    */
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
   
#define MEAN 0 
#define STD  1 
#define RHO  2
    vector<double> xstat;
    vector<double> ystat;

    load_state();

    vector<double> data;

    ifstream inp(argv[1]);
    ofstream fout(argv[2]);
    while(gpl_xdr_read_design(inp, xstat, ystat))
    {
        data.clear();
        double x;
        double y;
        gsl_ran_bivariate_gaussian (rng, xstat[STD], ystat[STD], xstat[RHO], &x, &y);

        data.push_back(x+xstat[MEAN]);
        data.push_back(y+ystat[MEAN]);


        gpl_xdr_write_vector(fout, data);
    }
    inp.close();
    fout.close();
    save_state();
    gsl_rng_free(rng);
    return 0;
}

