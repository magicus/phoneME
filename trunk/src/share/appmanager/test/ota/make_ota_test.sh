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
# @(#)make_ota_test.sh	1.7 05/10/20
#
# A build script for the OtaTest class.
#
${DBG_SH}

[ -z "$BUILDFLAVOR" ] && BUILDFLAVOR=linux-x86-suse
# Set AM_HOME appropriately
AM_HOME=${CDCAMS_HOME}/src/share/appmanager
# If you built in a subdir, specify it here
#SUBDIR=<subdir_value>

[ -z "$CDCAMS_HOME" ] && echo "CDCAMS_HOME not set!"
[ -z "$JAVA_HOME" ] && echo "JAVA_HOME not set!" 

JAVAC=$JAVA_HOME/bin/javac

CP=$CDCAMS_HOME/build/$BUILDFLAVOR
[ -n "$SUBDIR" ] && CP=${CP}/${SUBDIR}


$JAVAC -bootclasspath $CP/btclasses.zip \
       -classpath $CP/lib/appmanager.jar:$CP/lib/personal.jar \
       OtaTest.java

exit $?
