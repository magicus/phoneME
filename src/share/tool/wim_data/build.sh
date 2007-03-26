#!/bin/sh
#
#   
#
# Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 only, as published by the Free Software Foundation. 
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License version 2 for more details (a copy is
# included at /legal/license.txt). 
# 
# You should have received a copy of the GNU General Public License
# version 2 along with this work; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA 
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 or visit www.sun.com if you need additional
# information or have any questions. 
#
######################################################################
#

TMP_DIR=/tmp/`logname`/wim_data
CLASSES_DIR=$TMP_DIR/classes

if [ "$JAVA_HOME" == "" ]; then
    JAVA_HOME=/usr/lib/jvm/jdk1.5.0_04
fi

JAVA="$JAVA_HOME/bin/java"
JAVAC="$JAVA_HOME/bin/javac -source 1.4 -encoding utf-8"
COMPILE="$JAVAC -d $CLASSES_DIR -classpath $CLASSES_DIR"
RUN="$JAVA -classpath $CLASSES_DIR"

rm -rf $CLASSES_DIR
mkdir -p $CLASSES_DIR

rm -f Data.java
$COMPILE FileSystem.java Key.java Main.java PIN.java TLV.java Utils.java
if [ $? -ne 0 ]; then
    exit
fi

rm -f listing.txt
$RUN wim_data.Main
