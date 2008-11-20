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
# @(#)config.mk	1.27 06/10/21
#
# configurable definitions for VxWorks target
#

#
# Define the variables needed by tornado2 tools.
#

# Define absolute top for reference from outside the workspace. Note this
# assumes the cwd is build/vxworks; be careful if ABSOLUTE_TOP is not
# defined and you invoke this makefile from a different directory level.
ifndef ABSOLUTE_TOP
export ABSOLUTE_TOP := $(shell cd ../.. ; pwd)
endif

# WIND_BASE determines where the Tornado installation lives.
export WIND_BASE	?= $(CVM_TOOLS_DIR)/tornado2

# WIND_HOST_TYPE determines which host architecture to use for executables.
export WIND_HOST_TYPE	?= sun4-solaris2

# This points to the Tornado project directory - not to be confused with the
# Tornado installation base directory in WIND_BASE. Override this if you have
# your own project direcotory.
export TORNADO2DIR	?= $(ABSOLUTE_TOP)/build/vxworks/target/PC

# The := operator expands PATH exactly once to prepend to PATH without
# recursion.  The += operator appends to the PATH.
export PATH		:= $(WIND_BASE)/host/$(WIND_HOST_TYPE)/bin:$(PATH)
export LD_LIBRARY_PATH 	:= $(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib:$(LD_LIBRARY_PATH)
# This must be an absolute path to a version of libgcc.a compiled without -pic
# for vxworks.  See docs for more details on how to get the binary.
export LIB_GCC		?= $(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/lib/libgcc.a

# These must be set for the hard-target and host being used.
ETHER			= elPci
STARTUP			= $(HOME)/.wind/startup
TARGET_ADDR		= 192.168.151.118
HOST_ADDR		= 192.168.151.117
HOSTNAME		= thehost

# Where to find the floppy disk device on the host machine.
DISK_DEVICE = /vol/dev/diskette0/unnamed_floppy
MOUNTED_DISK = /floppy/floppy0
