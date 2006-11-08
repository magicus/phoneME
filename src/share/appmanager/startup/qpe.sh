#!/bin/sh

#
# Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 only,
# as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# version 2 for more details (a copy is included at /legal/license.txt).
# 
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
# CA 95054 or visit www.sun.com if you need additional information or have
# any questions.
#

#
# @(#)qpe.sh	1.2 05/10/20
#

if [ -f /proc/sys/vm/freepg_signal_watermarks ]; then
    #echo '120 160 340' > /proc/sys/vm/freepg_signal_watermarks
    echo '200 400 800' > /proc/sys/vm/freepg_signal_watermarks
fi
 
chvt 2
export SHELL=/bin/bash
export QWS_DISPLAY="Transformed:Rot270:Vga:0"
export QTDIR=/home/QtPalmtop
export QPEDIR=/home/QtPalmtop
export PATH=/root/bin:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin
export PATH=$QPEDIR/bin:$PATH
export LD_LIBRARY_PATH=$QTDIR/lib
export HOME=/home/zaurus
export CRL_DIC_PATH=/home/QtPalmtop/dic
#export UNICODEMAP_JP="nec-vdc,ibm-vdc,udc"
export UNICODEMAP_JP="open-19970715-ms,cp932,nec-vdc,ibm-vdc,udc"
 
export QPEUSER=zaurus
export QPEGROUP=qpe
export RES_OPTIONS=insecure1
 
while true ; do
    sdisp /home/QtPalmtop/pics144/Startup_screen.bmp &
    cd
    if [ -f /etc/restorefile ]; then
        export LD_LIBRARY_PATH=/usr/QtPalmtop.rom/lib
    else
        export LD_LIBRARY_PATH=$QTDIR/lib
    fi
    qectrl -c
    export QWS_DISPLAY="Transformed:Rot270:Vga:0"
    chkhinge
    if [ $? -eq 2 ]
    then
      export QWS_DISPLAY="Transformed:Rot0:Vga:0"
    fi
    chmod a+rw /tmp
    setdevperm $QPEUSER root
    if [ -f /mnt/card/failsafe-mode ] ; then
      rm -f /tmp/appmanager_boot
      /home/QtPalmtop/bin/runappmanager server > /tmp/appmanager_boot 2>&1
    fi
    nice survive -l 6 runqpe $QPEUSER $QPEGROUP >/dev/null 2>&1
#    nice survive -l 6 runqpe $QPEUSER $QPEGROUP >/home/zaurus/log.`date +06/21/06M%S` 2>&1
    if [ ! -f /etc/quickboot ] && [ ! -f /etc/backupfile ] && [ ! -f /etc/restorefile ]; then
        chvt 1; exit 0
    fi
done
 
chvt 1
exit 0

