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
# @(#)GNUmakefile	1.10 06/10/10
#
# GNUmakefile for vxworks-sparc-SPARC target
#

#
# platform specific architecture flags
#
ASM_ARCH_FLAGS		=
CC_ARCH_FLAGS		=
CC_ARCH_FLAGS_FDLIB	=
CC_ARCH_FLAGS_LOOP	=
LINK_ARCH_FLAGS 	= 
LINK_ARCH_LIBS		= $(LIB_GCC)

# overrides of some values in build/vxworks/config.mk. LIB_GCC is needed
# for softfloat support.
export LIB_GCC		= $(WIND_BASE)/host/$(WIND_HOST_TYPE)/lib/gcc-lib/sparc-wrs-vxworks/cygnus-2.7.2-960126/v8/libgcc.a
export TORNADO2DIR	?= $(WIND_BASE)/target/proj/Project0

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true
# assume no floating point hardware on this device, unless told otherwise
CVM_JIT_USE_FP_HARDWARE ?= false

# no CAS suport on the plain vanilla v8 arch
CVM_HAS_CAS ?= false

include ../share/top.mk
