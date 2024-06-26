#!/bin/sh
#
#  uninstall.sh
#     Uninstallation script for nirlpk kernel driver
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

# unload driver

./nirlp stop

# remove driver image from kernel's module directory

if [ -d $nirlpModPath ]; then
    rm -f $nirlpModPath/nirlpk.ko
    rmdir -p --ignore-fail-on-non-empty $nirlpModPath
fi

# update kernel's module dependencies

/sbin/depmod -a > /dev/null 2>&1
