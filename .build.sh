#!/bin/sh
set -ex

mkdir -p build

# Install libarchive and libmpg123 for host
apt-get update
apt-get -y install libarchive-dev libmpg123-dev

# Install libarchive for 3ds
git clone https://github.com/Cruel/3ds_portlibs.git
cd 3ds_portlibs
make libarchive
make install

# Install libmpg123 for 3ds
wget -O libmpg123-dev.tar.gz  https://notabug.org/attachments/216a6d61-f167-4f65-84dc-fa98c2247fc1
tar -xaf libmpg123-dev.tar.gz -C $DEVKITPRO/portlibs/3ds

cd ../build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EMULATOR=ON -DBUILD_TESTS=ON ..
make -j4
../bin/freeshop-tests
