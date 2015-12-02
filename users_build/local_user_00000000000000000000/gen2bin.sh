#!/bin/bash

set -e; make clean; make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=4; make clean; make COMPILE=gcc BOOT=new APP=2 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=4

mkdir -p ./build_temp

mv *.d ./build_temp
mv *.dump ./build_temp
mv *.S ./build_temp
mv *.hex ./build_temp
mv *.elf ./build_temp
