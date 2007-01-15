#
#
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
######################################################################
#
# Makefile for building and packaging.
#
######################################################################

INVALID_JAVACALL_OUTPUT_DIR = (\
echo ===================================================================== ; \
echo JAVACALL_OUTPUT_DIR \(environment or GNUmake variable\) must be set.  ; \
echo Please set it such that it is possible to access                      ; \
echo \$${JAVACALL_OUTPUT_DIR}/inc and \$${JAVACALL_OUTPUT_DIR}/lib         ; \
echo directories.                                                          ; \
echo ===================================================================== )

ifeq ($(JAVACALL_OUTPUT_DIR),)
  $(shell $(INVALID_JAVACALL_OUTPUT_DIR) 1>&2)
  $(error JAVACALL_OUTPUT_DIR is not set)
endif
ifneq ($(shell if test -d $(JAVACALL_OUTPUT_DIR); then echo valid; fi),valid)
  $(shell $(INVALID_JAVACALL_OUTPUT_DIR) 1>&2)
  $(error $${JAVACALL_OUTPUT_DIR} does not point to a directory)
endif
ifneq ($(shell if test -d $(JAVACALL_OUTPUT_DIR)/inc; then echo valid; fi),valid)
  $(shell $(INVALID_JAVACALL_OUTPUT_DIR) 1>&2)
  $(error $${JAVACALL_OUTPUT_DIR/inc} is not a directory)
endif
ifneq ($(shell if test -d $(JAVACALL_OUTPUT_DIR)/lib; then echo valid; fi),valid)
  $(shell $(INVALID_JAVACALL_OUTPUT_DIR) 1>&2)
  $(error $${JAVACALL_OUTPUT_DIR/lib} is not a directory)
endif

JAVACALL_INCLUDE_DIR=$(JAVACALL_OUTPUT_DIR)/inc
JAVACALL_LIB_DIR=$(JAVACALL_OUTPUT_DIR)/lib

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

# All rules are defined in shared makefiles
include $(MIDP_DIR)/build/common/Top.gmk
