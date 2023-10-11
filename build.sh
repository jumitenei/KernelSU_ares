#!/bin/bash

source ~/.bashrc && source ~/.profile
export LC_ALL=C && export USE_CCACHE=1
ccache -M 32G
export ARCH=arm64
export KBUILD_BUILD_HOST=khanra17
export KBUILD_BUILD_USER="khanra17"

if ! [ -d "out" ]; then
echo "Kernel OUT Directory Not Found . Making Again"
mkdir out
fi

make O=out ARCH=arm64 ares_user_defconfig

PATH="${PWD}/../clang-18_neutron/bin:${PATH}" \
make -j$(nproc --all) O=out \
                      ARCH=arm64 \
                      CC="clang" \
                      CROSS_COMPILE="aarch64-linux-gnu-" \
                      CROSS_COMPILE_ARM32="arm-linux-gnueabi-" \
                      STRIP=llvm-strip \
                      AS=llvm-as \
                      AR=llvm-ar \
                      NM=llvm-nm \
                      OBJCOPY=llvm-objcopy \
                      OBJDUMP=llvm-objdump \
                      CONFIG_NO_ERROR_ON_MISMATCH=y 2>&1 | tee error.log 
