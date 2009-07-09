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
# @(#)GNUmakefile	1.4 06/10/10
#

#
# GNUmakefile for linux-arm-thumb target using GCC 3.x and VFP
# same as linux-arm-xscale, but with thumb-target compilation
# for some files.
#
# Requires static linking. This is because of a shortcoming in
# the gld that generates the stubs for intercalling.
#

#
# platform specific architecture flags
#
# NOTE: -msoft-float is enabled in linux-arm/defs.mk automatically if
# CVM_FORCE_HARD_FLOAT is not true, which is the default.
#
# -static and -DCVM_STATIC_LINK is required only as workarounds for gld
# shortcomings. CVM_STATIC_LINK is used only in segvhandler_arch.c
# Delete them when you can.
#
ASM_ARCH_FLAGS		= -mthumb-interwork
CC_ARCH_FLAGS		= -mcpu=xscale  -mthumb-interwork -DCVM_STATIC_LINK
CC_ARCH_FLAGS_FDLIB	=
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_FLAGS		= -mthumb-interwork -static
# TODO: Why do we need to specify libgcc.a?
LINK_ARCH_LIBS		= $(CVM_TOOLS_DIR)/lib/libgcc.a

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true

# Only enable USE_GCC2 if GCC 3.x or later is *not* being used
USE_GCC2 ?= false

include ../share/top.mk

#
# Make a list of all files we want to compile to Thumb mode.
# Start with the 'SPACE' objects, subtracting out thing that
# contain the program entry point, constructors, or a lot of
# float code. Add in a few other things that we think want to
# go fast but still work ok as thumb code (note that this has
# the potential of uncovering new compiler bugs, since it
# passes gcc -O4 -mthumb)
UNTHUMBABLE     := float_fdlibm.o java_md.o linux_close.o net_util_md.o
THUMB_OBJS      = $(filter-out $(UNTHUMBABLE), $(CVM_OBJECTS_SPACE0))
THUMB_OBJS      := $(THUMB_OBJS) stackmaps.o interpreter.o jit_common.o gc_stat.o

# Now add the -mthumb flag to the compilation line for each of the above
# (The way this rule works is by adding the patsubst expression to
# the CCFLAGS for all compilations. If the target of that particular
# compilation is in the THUMB_OBJS list, then -mthumb will be the result.
# Otherwise, nothing will.)
CCFLAGS         += $(patsubst %,-mthumb,$(findstring $(@F), $(THUMB_OBJS)))

