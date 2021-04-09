FROM ubuntu:21.04


ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update
RUN apt-get -y upgrade

RUN apt-get install -y \
    curl \ 
    git \
    lsb-core


WORKDIR /root

RUN echo "4" 
RUN git clone https://github.com/vzaccaria/filesdot.git
WORKDIR /root/filesdot 

RUN /usr/bin/bash ./install.sh 

ENV POWERLEVEL9K_DISABLE_CONFIGURATION_WIZARD true


WORKDIR /root

RUN apt-get install -y npm clang libedit-dev libxml2-dev silversearcher-ag bison flex mpich 
RUN npm install -g diff-so-fancy
RUN apt-get install -y clangd
RUN npm install coc-clangd && nvim --headless +"CocInstall -sync coc-clangd" +qall
RUN apt-get install -y cmake
RUN apt-get install -y libreadline-dev
RUN apt-get install -y libgsl-dev

COPY ./overrides/most /root
RUN apt-get install -y clang-format

WORKDIR /local

RUN apt-get install -y python2
RUN ln -s /usr/bin/python2 /usr/bin/python
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
