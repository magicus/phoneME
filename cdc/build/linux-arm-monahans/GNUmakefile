#
# Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
# Reserved.  Use is subject to license terms.
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
# Copyright 2005 Intel Corporation. All rights reserved.  
#
# @(#)GNUmakefile	1.2 05/05/17
#
# GNUmakefile for linux-arm-monahans target using GCC 3.x,
# AAPCS and VFP
#

#
# platform specific architecture flags
#
ASM_ARCH_FLAGS		= -Wa,-mcpu=iwmmxt
CC_ARCH_FLAGS		= -Wa,-mcpu=iwmmxt
CC_ARCH_FLAGS_FDLIB	=
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_FLAGS		=
LINK_ARCH_LIBS		=

# Bulverde has a wmmx coprocessor, but there is a WMMX/MontaVista
# incompatiblity that prevents us from using it.
CVM_DEFINES += -DCVM_ARM_HAS_WMMX

# GCC 3.4.3 uses AAPCS calling conventions on monohans
USE_AAPCS ?= true

# These options are not enabled for linux-arm by default
CVM_DEFINES += -DIAI_CACHEDCONSTANT -DIAI_CACHEDCONSTANT_INLINING
CVM_DEFINES += -DIAI_ARRAY_INIT_BOUNDS_CHECK_ELIMINATION
CVM_DEFINES += -DIAI_FIXUP_FRAME_VMFUNC_INLINING

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true

# enable code scheduling by default
# NOTE: Code Scheduling appears to be broken, so it is no longer enabled.
# CVM_JIT_CODE_SCHED ?= true

# Only enable USE_GCC2 if GCC 3.x or later is *not* being used
USE_GCC2 ?= false

include ../share/top.mk
