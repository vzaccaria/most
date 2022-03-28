/* @STSHELL_LICENSE_START@
 *
 *      __  _______  ___________ 
 *     /  |/  / __ \/ ___/_  __/ 
 *    / /|_/ / / / /\__ \ / /    
 *   / /  / / /_/ /___/ // /    
 *  /_/  /_/\____//____//_/     
 * 
 * Multi-Objective System Tuner 
 * Copyright (c) 2007-2022 Politecnico di Milano
 * 
 * Development leader: Vittorio Zaccaria
 * Main developers: Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro
 *  
 * 
 * @STSHELL_LICENSE_END@ */
 
#include <iostream>
#include <st_object.h>
#include <st_map.h>
#include <st_vector.h>
#include <st_shell_variables.h>
#include <st_parser.h>
#include <st_optimizer.h>
#include <st_opt_utils.h>
#include <math.h>
#include <st_doe.h>
#include <st_vector.h>
#include <st_rand.h>

class st_random_replicate: public st_doe
{
    public:
        st_random_replicate() 
        {
        }
        ~st_random_replicate()
        {
        }
        string get_information();
        st_vector *generate_doe(st_env *env);
};


string st_random_replicate::get_information()
{
	return "Replicate DoE - (source_db, max_num_of_points)";
}

static bool probabilistic_accept(double probability)
{   
    double x = rnd_flat_float();
    if(x <= probability)
        return true;    else 
        return false;
}

st_vector *st_random_replicate::generate_doe(st_env *env)
{
    st_assert(env->current_driver);
	prs_display_message("Replicating DoE");

    st_vector *doe = new st_vector();

    int max_num_of_points = 0;
	
	string source_db;
	if(!env->shell_variables.get_string("source_db",source_db)) 
    {
        throw std::logic_error("source_db not defined");
    }
    if(!env->available_dbs.count(source_db))
    {
        throw std::logic_error("source_db not valid");
    }
    if(!env->shell_variables.get_integer("max_num_of_points", max_num_of_points))
        max_num_of_points = 0;

    st_point_set *s = env->available_dbs[source_db]->get_set_of_points(); 

 
    int points = 0; 
    st_point_set::iterator i;
    
    if(max_num_of_points > 0)
    {
        for(;max_num_of_points>0;max_num_of_points--){
		i=s->begin(); 
		std::advance(i,st_rnd_flat(0,s->get_size()));
        	st_point actual_point ((const vector<int>&)i->first);
        	doe->insert(points, actual_point);
        	points++;
	}
    }
    else
    {
        for(i=s->begin(); i!=s->end(); i++){
       	    st_point actual_point ((const vector<int>&)i->first);
       	    doe->insert(points, actual_point);
            points++;
   	}
    }
    return doe;
}

extern "C" 
{
    st_doe *doe_generate_doe()
    {
        return new st_random_replicate();
    }
    st_command *get_help() { 
        const char *ref[]={"source_db", "max_num_of_points", NULL};
        const char *ref_help[]={"Source database", "Maximum number of design points to be replicated", NULL};

        st_command *help = 
            new st_command(multiple_opts, 
                "This design of experiments generates a set of design points that randomly replicate those existing in database ^opt(0). ^opt(1) is mandatory and specifies the maximum number of points to be replicated. Design points are extracted randomly. ",
                "", ref, ref_help, 
                "This design of experiments generates a set of design points that replicate those existing in a database.",
                STDRET);
        help->alt_command_name      = "Random Replica Design of Experiments";
        help->alt_command_synopsys  = "doe_load_doe st_random_replicate";
        help->alt_command_type      = "doe";
        
        return help; };
}
