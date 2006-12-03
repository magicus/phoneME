#!/bin/sh
#
# Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#
# Sun TCK Auto Tester Script
#
# 1. Install/Update test suite
# 2. if suite installed successfully, then run suite and loop back to step 1
# 3. remove installed test suite
#

# Change the directory to the one of this script
DIR=${0%/*}
cd $DIR

if [ "x$1" = "x" ]
then
  echo "usage: autotest.sh <HTTP URL> [<domain for unsigned suites>]"
  exit
fi

prev_midlet_args="0"

while :
do
  #install and remove any output lines that do not have a "["
  install_output=`./runMidlet -1 com.sun.midp.scriptutil.HttpJadInstaller \
    $* | sed "/\[/!d"`

  #remove any text before and including the [
  install_output=${install_output#*[}

  #to get the final arg string remove any text after and including the ]
  midlet_args=${install_output%]*}

  #if the args are blank the install failed and we done
  if [ "x${midlet_args}" = "x" ] 
  then
    break
  fi

  ./runMidlet $midlet_args

  prev_midlet_args=$midlet_args
done

./runMidlet -1 com.sun.midp.scriptutil.SuiteRemover ${prev_midlet_args}

