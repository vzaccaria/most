/* @STSHELL_LICENSE_START@
 *
 *      __  _______  ___________
 *     /  |/  / __ \/ ___/_  __/
 *    / /|_/ / / / /\__ \ / /
 *   / /  / / /_/ /___/ // /
 *  /_/  /_/\____//____//_/
 *
 * Multi-Objective System Tuner
 * Copyright (c) 2007-2011 Politecnico di Milano
 *
 * Development leader: Vittorio Zaccaria
 * Main developers: Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro
 *
 * This file is confidential property of Politecnico di Milano.
 *
 * @STSHELL_LICENSE_END@ */
/* @M3EXPLORER_LICENSE_START@
 *
 * This file is part of the Multicube Explorer tool.
 *
 * Authors: Vittorio Zaccaria, Gianluca Palermo, Giovanni Mariani, Fabrizio
 * Castro, Stefano Bolli (2009) Copyright (c) 2008-2009, Politecnico di Milano,
 * Universita' della Svizzera italiana All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * Neither the name of Politecnico di Milano nor Universita' della Svizzera
 * Italiana nor the names of its contributors may be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This work was supported by the EC under grant MULTICUBE FP7-216693.
 *
 * @M3EXPLORER_LICENSE_END@ */

/* @additional_authors @, Stefano Bolli (2009)@*/

#include "st_design_space.h"
#include <st_doe.h>
#include <st_parser.h>
#include <st_shell_variables.h>

class st_scrambled_doe : public st_doe {
public:
  st_vector *generate_doe(st_env *env);
  string get_information();
};

st_vector *st_scrambled_doe::generate_doe(st_env *env) {

  int ds_size = env->current_design_space->ds_parameters.size();
  int max = (1 << ds_size) - 1;
  int min = 0;
  int num_point;
  st_vector *doe = new st_vector();
  int n = 0;

  for (int j = min; j <= max; j++) {
    string res;
    vector<st_point> actual_points;

    env->current_design_space
        ->convert_two_level_factorial_representation_from_int_multi(
            env, actual_points, j, res);

    // cout<<"DOE actual_points size: "<<actual_points.size()<<endl;
    for (int gg = 0; gg < actual_points.size(); gg++) {
      // cout<<"index value: "<<gg<<" actual_points value:
      // "<<actual_points.size()<<endl;
      doe->insert(n++, actual_points[gg]);
    }
  }

  return doe;
}

string st_scrambled_doe::get_information() { return "Scrambled DoE"; }

extern "C" {
st_doe *doe_generate_doe() { return new st_scrambled_doe(); }
st_command *get_help() {
  const char *ref[] = {NULL};
  const char *ref_help[] = {NULL};

  st_command *help =
      new st_command(multiple_opts,
                     "This design of experiments is meant to be a full "
                     "factorial DoE for masks and permutations. ",
                     "", ref, ref_help,
                     "This design of experiments generates Full Factorial-like "
                     "designs of experiments for masks and permutations. ",
                     STDRET);
  help->alt_command_name = "Scrambled Design of Experiments";
  help->alt_command_synopsys = "doe_load_doe st_scrambled_doe";
  help->alt_command_type = "doe";

  return help;
};
}
