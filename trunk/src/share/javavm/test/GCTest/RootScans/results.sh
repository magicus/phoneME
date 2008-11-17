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
# @(#)results.sh	1.7 06/10/10
#
#

#!/bin/sh

echo ""
echo "Collecting Results in RootScans"
echo "-------------------------------"
echo ""

if [ -f ../Defs.mk ]
then
        . ../Defs.mk
else
        echo "Please Create Defs.mk file. Look at README for info"
        exit 1
fi

if [ -f build.log ]
then
	if grep error build.log
	then
		echo "RootScans build FAILED"
		echo "For more detail look in build.log"
	else
		echo "RootScans build PASSED"
	fi
else
	echo "There is no build.log to collect build results"
fi

echo ""
echo ""

if [ -f run.$CVM_TARGET.log ]
then
	FAIL=`grep -c FAIL run.$CVM_TARGET.log`
	PASS=`grep -c PASS run.$CVM_TARGET.log`
	UNRESOLVED=`grep -c UNRESOLVED run.$CVM_TARGET.log`

	if [ $FAIL -eq 0 ] && [ $UNRESOLVED -eq 0 ]
	then
		echo "RootScans run PASSED"
	fi

	if [ $FAIL -ne 0 ]
	then
		echo "RootScans run FAILED"
		echo ""
	fi

	if [ $UNRESOLVED -ne 0 ]
	then
		echo "RootScans run has UNRESOLVED test cases"
		echo ""
	fi

	echo ""
	echo "Number of PASSES     = $PASS"
	echo "Number of FAILS      = $FAIL"
	echo "Number of UNRESOLVED = $UNRESOLVED"
	echo ""

	if [ $FAIL -ne 0 ]
	then
		echo ""
		echo "The following are the tests which failed"
		echo "For more details please look in run.$CVM_TARGET.log"
		echo ""
		grep FAIL run.$CVM_TARGET.log
		echo ""
	fi

	if [ $UNRESOLVED -ne 0 ]
	then
		echo ""
		echo "The following are the unresolved tests"
		echo "For more detail look in run.$CVM_TARGET.log"
		echo ""
		grep UNRESOLVED run.$CVM_TARGET.log
		echo ""
	fi
else
	echo "There is no run.$CVM_TARGET.log to collect run results"
fi

echo ""
