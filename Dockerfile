FROM ubuntu:21.04


ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update
RUN apt-get -y upgrade

RUN apt-get install -y \
    curl \ 
    git \
    lsb-core


WORKDIR /root


RUN apt-get install -y libedit-dev libxml2-dev silversearcher-ag bison flex mpich
RUN apt-get install -y build-essential
RUN apt-get install -y cmake
RUN apt-get install -y libreadline-dev libssl-dev libgsl-dev


WORKDIR /local

RUN apt-get install -y python3
RUN ln -s /usr/bin/python3 /usr/bin/python
RUN apt-get install -y libxml2-utils

WORKDIR /root
RUN git clone https://github.com/libfann/fann.git
WORKDIR /root/fann
RUN cmake .
RUN make install

WORKDIR /root
RUN apt-get install -y wget
RUN apt-get install -y r-base r-cran-hmisc
RUN apt-get install -y gnuplot



WORKDIR /local
