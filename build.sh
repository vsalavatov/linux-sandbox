#!/usr/bin/env sh
mkdir -p build
mkdir -p rootfs
cd build
curl -o rootfs.tar.xz -L https://file-store.rosalinux.ru/api/v1/file_stores/d3cedd1e74949e3cb2638c4bb483adde4c815837
tar -xvf rootfs.tar.xz -C ../rootfs
cmake .. && cmake --build .
cd ..
sudo ./tests.py
