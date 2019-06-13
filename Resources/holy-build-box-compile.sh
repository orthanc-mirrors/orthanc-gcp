#!/bin/bash
set -ex

ROOT_DIR=`dirname $(readlink -f $0)`/..

docker run -t -i --rm \
       -v ${ROOT_DIR}:/source:ro \
       -v ${ROOT_DIR}/holy-build-box:/target:rw \
       phusion/holy-build-box-64:2.0.1 \
       bash /source/Resources/holy-build-box-internal.sh

ls -l ${ROOT_DIR}/holy-build-box/
