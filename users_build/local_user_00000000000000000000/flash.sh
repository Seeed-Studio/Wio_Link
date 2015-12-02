#!/bin/bash

esptool.py -p /dev/tty.SLAB_USBtoUART -b576000 write_flash 0x1000 ./user1.bin 
