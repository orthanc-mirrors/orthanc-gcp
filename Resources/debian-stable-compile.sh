#!/bin/bash

##
## This script compiles the plugin for Debian stable, in static
## mode. The resulting binaries can be used in the Docker images of
## Orthanc, as those are also based on Debian stable:
## https://github.com/jodogne/OrthancDocker/blob/master/orthanc/Dockerfile
##

set -ex

if [ "$1" != "Debug" -a "$1" != "Release" ]; then
    echo "Please provide build type: Debug or Release"
    exit -1
fi

if [ -t 1 ]; then
    # TTY is available => use interactive mode
    DOCKER_FLAGS='-i'
fi

ROOT_DIR=`dirname $(readlink -f $0)`/..

# Always make sure we use the latest version of Debian stable
docker pull debian:stable

mkdir -p ${ROOT_DIR}/debian-stable

docker run -t ${DOCKER_FLAGS} --rm \
    -v ${ROOT_DIR}:/source:ro \
    -v ${ROOT_DIR}/debian-stable:/target:rw \
    debian:stable \
    bash /source/Resources/debian-stable-internal.sh $1 $(id -u) $(id -g)

ls -l ${ROOT_DIR}/debian-stable/
