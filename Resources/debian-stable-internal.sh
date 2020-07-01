#!/bin/bash

set -ex

BUILD_TYPE=$1
USER_ID=$2
GROUP_ID=$3

# Create the same user and group than the one who is running the
# "./debian-stable-compile.sh" script on the hosting system (*)
groupadd -g ${GROUP_ID} -r orthanc
useradd -u ${USER_ID} -r -g orthanc orthanc

# Static build using the root user
apt-get update
apt-get install -y cmake build-essential unzip mercurial

mkdir /tmp/build
cd /tmp/build

cp -r /source /tmp/source-writeable
cmake /tmp/source-writeable \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DSTATIC_BUILD=ON \
    -DCMAKE_INSTALL_PREFIX=/target 

make -j`nproc`

# Copy the installation to the host filesystem, using the
# newly-created user "orthanc" (*) that corresponds to the user who is
# running "./debian-stable-compile.sh" script. This allows to avoid
# files owned by the "root" user on the host filesystem.
su -c "cp /tmp/build/libOrthancGoogleCloudPlatform.so /target" orthanc
