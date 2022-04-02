**MOST** (Multi-Objective System Optimizer) is a tool for architectural design
space exploration. **MOST** is an interactive program that lets the designer
explore a design space of configurations for a particular architecture for which
an executable model (or _driver_) exists. **MOST** can be extended by
introducing new optimization algorithms such as Monte Carlo optimization,
sensitivity based optimization and, as an example, Taguchi design of
experiments. All of this by using an appropriate API.

# Installation

At the moment the best way to install and run MOST is through a docker container
with the available MOST development and test Docker image:

```shell
docker run -ti -v ${PWD}:/local vzaccaria/most:latest

# in the container (root@....)
mkdir build
cd build
source ../bootstrap.sh
make
make install

# To test
# make test

# now you will find the tool installed in /opt/bin
/opt/bin/most

# output
#      __  _______  ___________
#     /  |/  / __ \/ ___/_  __/
#    / /|_/ / / / /\__ \ / /
#   / /  / / /_/ /___/ // /
#  /_/  /_/\____//____//_/
#
#  Multi-Objective System Tuner - Version R1.2  - Build date: Feb 25 2022 17:16:19 - Git hash: 4c5da95
#  Developed at Politecnico di Milano - 2001-2022
#  Send bug reports to vittorio.zaccaria@polimi.it
#  --
#
```

# Example

Once installed, you can run the `gcc` example shown in the docs using:

```shell
/opt/bin/most -x /opt/examples/gcc/gcc_ds.xml -f /opt/examples/gcc/gcc_full_dse.scr
```

# Documentation

- [Introduction and overview](./docs/doc-most.md)

**Interfacing**:

- [Simulator interface (xml)](./docs/doc-xml.md)
- [Simulator interface (generic driver)](./docs/doc-gend.md)

**Reference manuals**:

- [Language commands](./docs/doc-man-commands.md)
- [Optimization algorithms](./docs/doc-man-opt.md)
- [Design of experiments](./docs/doc-man-doe.md)
- [Response Surface Modeling](./docs/doc-man-rsm.md)

# TODO

- Use LUA as default language

# Acknowledgements

MOST is currently updated and maintained by Vittorio Zaccaria. Other
contributors are Gianluca Palermo, Fabrizio Castro, Giovanni Mariani, Vailati
Mattia, and Franzini Paolo.

MOST is derived from two projects: the System Tuning Shell and Multicube
Explorer. The System Tuning Shell was originally designed and coded internally
at Politecnico di Milano by Vittorio Zaccaria while Lorenzo Salvemini, Simone
Valsecchi and Gianluca Palermo contributed to various aspects of the
implementation and testing. The prefix `st_` in front of source code files is an
heritage coming from the System Tuning Shell.

Multicube Explorer was a tool developed for the Multicube project (MULTICUBE
FP7-216693) by Politecnico di Milano and Università della Svizzera Italiana. Its
authors were Vittorio Zaccaria, Gianluca Palermo, Giovanni Mariani, Fabrizio
Castro.

If you use this tool for academic research, please reference the following work:

```bibtex
@inproceedings{mdf,
  author = {C. Silvano and W. Fornaciari and G. Palermo and V. Zaccaria and F. Castro and M. Martinez and S. Bocchio and R. Zafalon and P. Avasare and G. Vanmeerbeeck and C. Ykman-Couvreur and M. Wouters and C. Kavka and L. Onesti and A. Turco and U. Bondi and G. Mariani and H. Posadas and E. Villar and C. Wu and F. Dongrui and Z. Hao},
  booktitle = {Multi-objective Design Space Exploration of Multiprocessor SoC Architectures, Cristina Silvano, William Fornaciari, Eugenio Villar (Eds.)},
  isbn = {ISBN 978-1-4419-8836-2},
  pages = {3-17},
  publisher = {Springer Science+Businness Media},
  title = {The MULTICUBE Design Flow},
  year = {2011}
}
```

# WARRANTY

NO WARRANTY

THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.
EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER
PARTIES PROVIDE THE PROGRAM "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE
QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE
DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY
COPYRIGHT HOLDER BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL,
INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE
THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE
PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY
HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

# AUTHORS

```
      __  _______  ___________
     /  |/  / __ \/ ___/_  __/
    / /|_/ / / / /\__ \ / /
   / /  / / /_/ /___/ // /
  /_/  /_/\____//____//_/

  Multi-Objective System Tuner - Politecnico di Milano

MOST is derived from System Tuning Shell and Multicube Explorer.
As of may 2010, the development team is composed of the following people:

Vittorio Zaccaria, Gianluca Palermo, Fabrizio Castro

--
ST-Shell

The System Tuning Shell was originally designed and coded by Vittorio Zaccaria.

Lorenzo Salvemini, Simone Valsecchi and Gianluca Palermo contributed to
various aspects of the implementation and testing of System Tuning Shell.
The System Tuning Shell is currently updated and maintained by Vittorio Zaccaria
and Gianluca Palermo.

Other contributors:
Giovanni Mariani, Politecnico di Milano and ALaRI
Vailati Mattia, Franzini Paolo, Politecnico di Milano

--
Multicube Explorer

Authors: Vittorio Zaccaria, Gianluca Palermo, Giovanni Mariani, Fabrizio Castro
Copyright (c) 2008-2009, Politecnico di Milano, Universita' della Svizzera italiana
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

Neither the name of Politecnico di Milano nor Universita' della Svizzera Italiana
nor the names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

Multicube Explorer was supported by the EC under grant MULTICUBE FP7-216693.
```
