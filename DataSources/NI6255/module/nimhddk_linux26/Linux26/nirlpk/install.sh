#!/bin/sh
#
#  install.sh
#     Installation script for nirlpk kernel driver
#
#  (c) Copyright 2006,
#  National Instruments Corporation.
#  All Rights reserved.
#
#--------------------------------------------------

# kernel configuration

source Makefile.in
nirlpModPath=$NIMODPATH/nirlp

# check if running as root

if [ `id -ur` != 0 ]; then
    echo "$0 : must be run as root"
    exit 1
fi

# check that nirplk.ko exists

if [ ! $1 ]; then
    echo "$0 : no object directory specified (install.sh <objdir>)"
    exit 1
fi

if [ ! -e $1/nirlpk.ko ]; then
    echo "$1/nirlpk.ko not found"
    exit 1
fi

# install - copy nirlpk.ko to kernel's module directory

echo "installing nirplk in $nirlpModPath"

install -d $nirlpModPath
install -m 0666 $1/nirlpk.ko $nirlpModPath

# load the kernel module

echo "loading kernel module..."

/sbin/depmod -a >/dev/null 2>&1
./nirlp start
