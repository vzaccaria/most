#include "parameters.h"
#include <iostream>
using namespace std;


bool Parameters::isOption(const std::string& name) const{
	return Options.find(name) != Options.end();
}

bool Parameters::isOption(const char* name) const{
	return isOption(std::string(name));
}

void Parameters::help()
{
 cout << "R spline script generator\n"; 
 cout << "Usage: rsm [options] file..\n";
 cout << "Options:\n";
 cout << "--help 			: show this help\n";
 cout << "--trai 			: specify the training set file (required)\n";
 cout << "--pred 			: specify the prediction set file\n";
 cout << "--inte 			: specify the interaction file\n";
 cout << "--subs 			: specify a subset of the training set file to be used as training, the remainig is used as prediction set\n";
 cout << "--lmco 			: specify to compare spline model with linear models of the 1st and 2nd order\n";
 cout << "--eran			: produce error analysis file. Available only when the prediction is not specified\n";
 cout << "--output		: specify output file name. Default result.db\n";
 cout << "--clean			: delete unuseful file at the end of the process\n";
}
