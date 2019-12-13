FROM debian:stretch
LABEL maintainer="EPMCoin Developers <dev@epmcoin.org>"
LABEL description="Dockerised EPMCoinCore, built from Travis"

RUN apt-get update && apt-get -y upgrade && apt-get clean && rm -fr /var/cache/apt/*

COPY bin/* /usr/bin/
