#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <string>
#include <map>
#include <fstream>
#include <stdexcept>
#include <sstream>

  template<typename T> T to_string( const T & obj )
         {
          std::stringstream os; 
          os << obj; 
          return os.str(); 
         } 

  template<typename T> T from_string(const std::string & str) 
         {
          std::istringstream is( str ); 
          T res;
          is >> std::dec >> res;
          return res; 
         } 
/**
* \brief Implement Generic parameter collection
*/
class Parameters{
private:
	std::map<std::string, std::string> Options;



public:
	/**
	* \brief Costruct empty collection
	*/
	Parameters(){};
	/**
	* \brief Return value for given parameter as specified type
	* \return the option's value
	*/
	template<typename G> G getOption(const std::string& name) const{
		if(Options.find(name) == Options.end()) throw std::runtime_error("Option \"" + name + "\" not stored");
		return from_string<G>(Options.find(name)->second);
	}

	/**
	* \brief Return value for given parameter as specified type
	* \return the option's value
	*/
	template<typename G> G getOption(const char* name) const{
		return getOption<G>(std::string(name));
	}

	/**
	* \brief add parameter to Collection
	*/
	template<typename G> void setOption(const std::string& name, const G& value){
                
		Options[name] =  to_string<G>(value);
	}

	/**
	* \brief add parameter to Collection
	*/
	template<typename G> void setOption(const char* name, const G& value){     
		Options[std::string(name)] =  to_string<G>(value);
	}
	
	/**
	* \brief Check if parameter is available
	* \return return true if parameter is available
	*/
	bool isOption(const std::string& name) const;

	/**
	* \brief Check if parameter is available
	* \return return true if parameter is available
	*/
	bool isOption(const char* name) const;
        
        /**
        * \print help
        */
        void help();

};


#endif
