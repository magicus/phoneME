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
# @(#)GNUmakefile	1.12 06/10/10
#
# GNUmakefile for linux-powerpc-tivo target
#

#
# platform specific architecture flags
#
CC_ARCH_FLAGS   	= -mcpu=403 -msoft-float
CC_ARCH_FLAGS_FDLIB	= -mno-fused-madd
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_FLAGS		= -msoft-float
LINK_ARCH_LIBS		= -lm

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true
# assume no floating point hardware on this device unless told otherwise
CVM_JIT_USE_FP_HARDWARE ?= false

include ../share/top.mk
