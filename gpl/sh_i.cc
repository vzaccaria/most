/**
 *  GPL SHEPARD'S Interpolation 
 *  Authors: Gianluca Palermo and Vittorio Zaccaria
 *  
 **/


#include <gpl_xdr_api.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_cdf.h>
#include <iostream>
#include <fstream>

/** This is a simple test */
int main(int argc, char **argv)
{
    if(argc<4)
    {
        cout << "SHEPARD'S Interpolation" << endl;
        cout << "Please define TS, PS and PD filenames and power value" << endl;
        return 0;
    }

    string training_data_file_name = argv[1];
    string prediction_set_file_name = argv[2];
    string predicted_data_file_name = argv[3];
    int sh_power =atoi(argv[4]);

    vector<vector<double> > training_configurations;
    vector<vector<double> > training_observations;

    vector<double> training_conf;
    vector<double> observations;

    ifstream inp(training_data_file_name.c_str());

    cout << "Reading training set from " << training_data_file_name << endl;

    while(gpl_xdr_read_design(inp, training_conf, observations))
    {
        training_configurations.push_back(training_conf);
        training_observations.push_back(observations);
    }
    inp.close();
    cout << "Read " << training_configurations.size() << " configurations " << endl;
    if(training_configurations.size()==0)
    {
        cout << "Error, at least a training configuration should be given. Quitting. " << endl;
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
    
    cout << "Reading prediction set from " << prediction_set_file_name << endl;
    inp.open(prediction_set_file_name.c_str());

    vector<double> empty;
    vector<double> prediction_conf;
    
    vector<vector<double> > prediction_configurations;

    while(gpl_xdr_read_design(inp, prediction_conf, empty))
    {
        prediction_configurations.push_back(prediction_conf);
    }
    inp.close();
    
    cout << "Read " << prediction_configurations.size() << " configurations " << endl;

    /** 
     * Now:
     *
     * - Use training_configurations and training_observations to compute K models, 
     *   each associated to a column of the training observations.
     *
     * - Use the K models to compute a K-wide prediction row for each of 
     *   the prediction_configurations. 
     *
     * - Use  gpl_xdr_write_raw_data(file, prediction_configuration, prediction_row);
     *   to write out, for each prediction_configuration, the prediction_row.
     *   The file name is predicted_data_file_name.
     *
     */
    
    cout << "Writing predictions into " << predicted_data_file_name << endl;

    /** 
     * AS AN EXAMPLE, we write, for each prediction_configuration, a K-wide null prediction_row:
     */

    ofstream out(predicted_data_file_name.c_str());

    int K = training_observations[0].size();
    vector<vector<double> > prediction_observations;
    vector<double> empty_vector(K,0);
     
    int neg_power = 0 - sh_power;
    double distance_from_actual;
    double overall_weighted_distance;

    for(int pc_i=0; pc_i<prediction_configurations.size();pc_i++)
    {
        prediction_observations.push_back(empty_vector);
        overall_weighted_distance=0;
        for(int tc_i=0; tc_i<training_configurations.size();tc_i++)
        {
            distance_from_actual=0;
            for(int pce_i=0; pce_i<prediction_configurations[0].size();pce_i++)
            {
                distance_from_actual += gsl_pow_int((prediction_configurations[pc_i][pce_i] - training_configurations[tc_i][pce_i]), 2);                
            }
            if (distance_from_actual==0)
            {
                prediction_observations[pc_i]=training_observations[tc_i];
                overall_weighted_distance=1;
                break;
            }
            else{
                distance_from_actual = pow( sqrt(distance_from_actual), neg_power);
                overall_weighted_distance += distance_from_actual;
                for(int poe_i=0; poe_i<prediction_observations[0].size();poe_i++)
                {
                    prediction_observations[pc_i][poe_i] += distance_from_actual*training_observations[tc_i][poe_i];
                }
            }
        }
        for(int poe_i=0; poe_i<prediction_observations[0].size();poe_i++)
        {
            prediction_observations[pc_i][poe_i] = prediction_observations[pc_i][poe_i]/overall_weighted_distance;
        }
        gpl_xdr_write_raw_data(out, prediction_configurations[pc_i], prediction_observations[pc_i]);
    }
    cout << "Exiting." << endl;
}
