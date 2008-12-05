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
######################################################################
#
# Makefile for building and packaging.
#
######################################################################

USE_JAVACALL=true

ifeq ($(USE_DEBUG), true)
g = _g
else
g =
endif


JAVACALL_INCLUDE_DIR=$(JAVACALL_OUTPUT_DIR)/inc
JAVACALL_LIB_DIR=$(JAVACALL_OUTPUT_DIR)/lib
JAVACALL_OBJ_DIR=$(JAVACALL_OUTPUT_DIR)/obj$(g)

# Make MIDP depend on Javacall library so that MIDP is re-linked automatically on any Javacall change
# Note: it is assumed that Javacall library itself is rebuilt by another Makefile, as there's no explicit
# rule for building it from MIDP.

MIDP_DEPS += $(JAVACALL_LIB_DIR)/$(LIB_PREFIX)javacall$(g)$(r)$(LIB_SUFFIX)

EXTRA_CFLAGS += -I$(JAVACALL_INCLUDE_DIR)

include Platform.gmk

# Workspace directory
MIDP_DIR               := $(shell dirname $(shell dirname $(CURDIR)))

# Build output directory
ifndef MIDP_OUTPUT_DIR
MIDP_OUTPUT_DIR         = $(CURDIR)/output
endif

# Build options should be include before shared gmk
include Options.gmk

#overwrite  USE_JAVAUTIL_LOG_IMPLEMENTATION=false from Top.gmk
USE_JAVAUTIL_LOG_IMPLEMENTATION=true

# All rules are defined in shared makefiles
include $(MIDP_DIR)/build/common/Top.gmk


# Including the part that should appear after Top.gmk
include Platform_post.gmk
