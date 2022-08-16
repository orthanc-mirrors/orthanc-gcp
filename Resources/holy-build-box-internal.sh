#!/bin/bash
set -e

# Holy Build Box doesn't contain "hg", there's no "pip" or
# "easy_install" in Python 2.6 (the version of Holy Build Box) and
# we're running as a standard user. So we have to install Mercurial
# from source.
MERCURIAL=mercurial-5.4.1
cd /tmp
curl https://www.mercurial-scm.org/release/${MERCURIAL}.tar.gz > ${MERCURIAL}.tar.gz
tar xvf ${MERCURIAL}.tar.gz

# Activate Holy Build Box environment.
source /hbb_exe/activate

set -x

mkdir /tmp/build
cd /tmp/build

# Holy Build Box defines LDFLAGS as "-L/hbb_exe/lib
# -static-libstdc++". The "-L/hbb_exe/lib" option results in linking
# errors "undefined reference" to `std::__once_callable',
# 'std::__once_call' and '__once_proxy'.
export LDFLAGS=-static-libstdc++
unset LDPATHFLAGS
unset SHLIB_LDFLAGS
unset LD_LIBRARY_PATH
unset LIBRARY_PATH
export PATH=${PATH}:/tmp/${MERCURIAL}/

cp -r /source /tmp/source-writeable
cmake /tmp/source-writeable \
    -DUSE_LEGACY_BOOST=ON \
    -DCMAKE_BUILD_TYPE=$1 -DSTATIC_BUILD=ON \
    -DCMAKE_INSTALL_PREFIX=/target 

make -j`nproc`

if [ "$1" == "Release" ]; then
    strip ./libOrthancGoogleCloudPlatform.so
fi

make install
