#!/bin/bash
cd /home/zxq/os-workbench/kernel
make clean
make
qemu-system-x86_64 \
	-s -S \
	-serial mon:stdio\
	-machine accel=tcg\
	-smp ""\
	-drive format=raw,file=/home/zxq/os-workbench/kernel/build/kernel-x86_64-qemu \
    -nographic
# serial 将串口connect到我们的标准输入输出流
