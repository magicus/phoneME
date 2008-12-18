#
# Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
# @(#)configure.sh	1.6 06/10/10
#
#

#!/bin/bash

init() {
   CUR_DIR=`pwd`
   cd ../../../../..
   CVM_PREFIX=`pwd`
   cd $CUR_DIR

   echo "CVM_PREFIX=$CVM_PREFIX" > Defs.mk
}

linux_build_flavors() {
NAME=`uname`
if [ "$NAME" = "Linux" ]
then
   MAKE=make
   JDK_HOME=/usr/java/jdk1.3
else
   MAKE=gnumake
   JDK_HOME=/usr/java/jdk1.3
fi

cat << FLAGS >> Defs.mk
MAKE=$MAKE
JDK_HOME=$JDK_HOME
GCTEST_DIR=\${CVM_PREFIX}/src/share/javavm/test/GCTest
CVM_TARGET=$1
CDC_BUILDDIR=\${CVM_PREFIX}/build/\${CVM_TARGET}
BOOT_CLASSPATH=\${CDC_BUILDDIR}/lib/foundation.jar

FLAGS

   echo ""
   echo "Select the interested build flavor from the list"
   while true
   do
      echo "(1) CDCOptimized"
      echo "(2) DebugJVMDI"
      echo "(3) DebugJVMPI"


      read build
      case $build in
         1)
            linux_CDCOptimized
            break;;
         2)
            linux_DebugJVMDI
            break;;
         3)
            linux_DebugJVMPI
            break;;
         *)
            echo "Please select 1, 2 or 3"
            continue;;
      esac
   done
}

solaris_sparc_build_flavors() {
cat << FLAGS >>Defs.mk
MAKE=gnumake
GCTEST_DIR=\${CVM_PREFIX}/src/share/javavm/test/GCTest

CVM_TARGET=solaris-sparc
CDC_BUILDDIR=\${CVM_PREFIX}/build/\${CVM_TARGET}
BOOT_CLASSPATH=\${CDC_BUILDDIR}/lib/foundation.jar

FLAGS

solaris_DevDebugJVMXI
}

#Set build flags particular for the flavors
linux_CDCOptimized() {
cat << FLAGS >> Defs.mk
CVM_OPTIMIZED=true
CVM_PRELOAD_LIB=true
FLAGS
}

linux_DebugJVMDI() {
cat << FLAGS >> Defs.mk
CVM_DEBUG=true
CVM_JVMDI=true
J2ME_CLASSLIB=foundation
FLAGS
}

linux_DebugJVMPI() {
cat << FLAGS >> Defs.mk
CVM_DEBUG=true
CVM_JVMPI=true
J2ME_CLASSLIB=foundation
FLAGS
}

solaris_DevDebugJVMXI() {
cat << FLAGS >> Defs.mk
CVM_DEBUG=true
CVM_JVMDI=true
CVM_JVMPI=true
J2ME_CLASSLIB=foundation
FLAGS
}

#Main program starts here
init

echo ""
echo "Select the interested platform from the list"

while true
do
   echo "(1) Linux_i686"
   echo "(2) Linux_sarm_netwinder"
   echo "(3) Solaris_sparc"

   read platform
   case $platform in
      "1")
         linux_build_flavors linux-i686
         break;;
      "2")
         linux_build_flavors linux-sarm-netwinder
         break;;
      "3")
         solaris_sparc_build_flavors
         break;;
      *)
         echo "Please select 1, 2 or 3"
         continue;;
   esac
done
