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
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <string>

using namespace std;

/* Signature verify functions */
void st_initialize_ossl();
bool st_verify_signature(string clear_text, string signature);
bool st_verify_script(string &file_name, string &key_name);

bool st_load_data(string &file_name, string &data);

/* Common functions between the node manager and most */
key_t st_get_unique_most_key();

/* The following is obsolete */
long int st_get_host_id();

int st_code(int val);

long int st_get_hw_addr_from_s(string s);

long int st_get_hw_addr(string &);
