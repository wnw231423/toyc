# run "docker build -t ubuntu-cpp-env ." to build the image.

FROM ubuntu:20.04

RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get -y install tzdata

RUN apt-get update \
  && apt-get install -y build-essential \
      gcc \
      g++ \
      gdb \
      clang \
      make \
      ninja-build \
      cmake \
      autoconf \
      automake \
      libtool \
      valgrind \
      locales-all \
      dos2unix \
      rsync \
      tar \
      python \
      python-dev \
      flex \
      bison \
  && apt-get clean