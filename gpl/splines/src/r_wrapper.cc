#include "r_wrapper.hpp"
#include "converter.hpp"
#include "spline_script.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
r_wrapper::r_wrapper(Parameters& params,vector<vector<double> > training_configurations, vector<vector<double> > training_observations)
{
 this->param = params;
 this->training_configurations = training_configurations;
 this->training_observations = training_observations;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
r_wrapper::r_wrapper(Parameters& params,vector<vector<double> > training_configurations, vector<vector<double> > training_observations,vector<vector<double> > prediction_configurations)
{
 this->param = params;
 this->training_configurations = training_configurations;
 this->training_observations = training_observations;
 this->prediction_configurations = prediction_configurations;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
r_wrapper::~r_wrapper()
{}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void r_wrapper::start()
{
 converter conv;
 conv.convert_in(this->training_configurations,this->training_observations,this->prediction_configurations);
 spline_script R_luncher(this->param);

 R_luncher.generate_spline_script(this->training_configurations,this->training_observations);
 R_luncher.run_spline_script();

 while(R_luncher.check_problems(this->training_configurations.at(0).size())) //repeat the process till it goes well
 {
  cout << "Some problems have been detected... R relunched\n";
  delete_temp_prob();
  R_luncher.generate_spline_script(this->training_configurations,this->training_observations);
  R_luncher.run_spline_script();
 }

 conv.convert_out(param.getOption<string>("out"));
 if(this->param.isOption("clean"))
  {
   delete_temp();
  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void r_wrapper::delete_temp()
{
 pid_t child;
 int status;
 child = fork();
  if(child == 0)
   {
    cout << "Delete temporary files\n"; 
    if(!param.isOption("prediction"))
     execlp("rm","-f","observation.csv","configuration_predicted.csv","pred_conf.csv","spline_script.txt","xdr_temp.db","r_wrapper.sh",(char*)0); 
    else
     execlp("rm","-f","observation.csv","pred_conf.csv","spline_script.txt","xdr_temp.db","r_wrapper.sh",(char*)0);
   }
 else
   {
    waitpid(child,&status,0);
   }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void r_wrapper::delete_temp_prob()
{
 {
 pid_t child;
 int status;
 child = fork();
  if(child == 0)
   {
    execlp("rm","-f","prob.log","var.log",(char*)0); 
   }
 else
   {
    waitpid(child,&status,0);
   }
}
}
