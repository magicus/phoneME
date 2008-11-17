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
# @(#)GNUmakefile	1.6 06/10/10
#

#
# GNUmakefile for linux-arm-strongarm target using GCC 3.x and VFP
#

#
# platform specific architecture flags
#
# NOTE: -msoft-float is enabled in linux-arm/defs.mk automatically if
# CVM_FORCE_HARD_FLOAT is not true, which is the default.
#
ASM_ARCH_FLAGS		=
CC_ARCH_FLAGS		= -mcpu=strongarm110
CC_ARCH_FLAGS_FDLIB	=
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_FLAGS		=
LINK_ARCH_LIBS		=

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true

# Only enable USE_GCC2 if GCC 3.x or later is *not* being used
USE_GCC2 ?= false

include ../share/top.mk
