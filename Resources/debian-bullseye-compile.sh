#!/bin/bash

##
## This script compiles the plugin for Debian bullseye, in static
## mode.
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

# Always make sure we use the latest version of Debian bullseye
docker pull debian:bullseye

mkdir -p ${ROOT_DIR}/debian-bullseye

docker run -t ${DOCKER_FLAGS} --rm \
    -v ${ROOT_DIR}:/source:ro \
    -v ${ROOT_DIR}/debian-bullseye:/target:rw \
    debian:bullseye \
    bash /source/Resources/debian-internal.sh $1 $(id -u) $(id -g)

ls -l ${ROOT_DIR}/debian-bullseye/
