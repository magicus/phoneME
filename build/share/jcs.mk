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
# @(#)jcs.mk	1.38 06/10/10
#

#
# Makefile for building JavaCodeSelector) and jitcodegen.c
#

####################
# Exported variables
####################

# this is exported for the cleanjcc rule
CVM_JCS_BUILDDIR	= $(CVM_BUILD_TOP)/jcs
CVM_JCS			= $(CVM_JCS_BUILDDIR)/jcs

CVM_CODEGEN_CPATTERN	= $(CVM_DERIVEDROOT)/javavm/runtime/jit/jitcodegen
CVM_CODEGEN_HFILE	= $(CVM_DERIVEDROOT)/javavm/include/jit/jitcodegen.h
CVM_CODEGEN_CFILES	= $(CVM_CODEGEN_CPATTERN).c \
			  $(CVM_CODEGEN_CPATTERN)table.c

#################
# Local variables
#################

CVM_JCS_SRCDIR	= $(CVM_TOP)/src/share/javavm/jcs


CVM_JCS_OPTIONS = 

ifeq ($(CVM_OPTIMIZED), true)
    CVM_JCS_OPTIONS += -e
endif

CVM_JCS_INPUT = 

#
# -lsupc++ should never be needed now that we link with g++ rather than
# gcc, but the ability to add it on the command line is left in just in case.
#
ifeq ($(JCS_NEEDS_SUPC_LIB), true)
LINKLIBS_JCS += -lsupc++
endif

#################
# JavaCodeSelect
#################

CVM_JCS_DERIVED_OBJS0 = scan.o tbl.o

CVM_JCS_COBJS0 = \
        compress.o \
        compute_states.o \
        debug.o \
        hash.o \
        invert.o \
        item.o \
        main.o \
        matchset.o \
        output.o \
        POINTERLIST.o \
        pool_alloc.o \
        rule.o \
        state.o \
        statemap.o \
        symbol.o \
        transition.o \
        wordlist.o \
	$(CVM_JCS_DERIVED_OBJS0)

CVM_JCS_COBJS = $(patsubst %.o,$(CVM_JCS_BUILDDIR)/%.o,$(CVM_JCS_COBJS0))

CVM_JCS_DERIVED_OBJS = $(patsubst %.o,$(CVM_JCS_BUILDDIR)/%.o,$(CVM_JCS_DERIVED_OBJS0))

$(CVM_JCS_BUILDDIR)/%.o: $(CVM_JCS_SRCDIR)/%.cc
	@echo host c++ $@
	$(AT)$(HOST_CCC) $(CFLAGS_JCS) -c $< -o $@

#
# Derived objects should really be put in generated/ subtree,
# but for now...
#
CVM_TBL_CC=$(CVM_JCS_BUILDDIR)/tbl.cc
CVM_TBL_CC_H=$(CVM_TBL_CC).h
$(CVM_TBL_CC) $(CVM_TBL_CC_H) : $(CVM_JCS_SRCDIR)/tbl.y
	@echo "bison $(CVM_TBL_CC)"
	$(AT)$(BISON) --no-lines -d $(CVM_JCS_SRCDIR)/tbl.y -o $(CVM_TBL_CC)
	@if [ ! -f $(CVM_TBL_CC_H) -a -f $(CVM_JCS_BUILDDIR)/tbl.hh ]; then \
	    mv $(CVM_JCS_BUILDDIR)/tbl.hh $(CVM_TBL_CC_H); \
	fi

$(CVM_JCS_BUILDDIR)/scan.cc: $(CVM_TBL_CC_H) $(CVM_JCS_SRCDIR)/scan.l
	@echo "flex $@"
	$(AT)$(FLEX) -L -o$@ $(CVM_JCS_SRCDIR)/scan.l

$(CVM_JCS_DERIVED_OBJS) : %.o: %.cc
	@echo host c++ $@
	$(AT)$(HOST_CCC) $(CFLAGS_JCS) -I$(CVM_JCS_SRCDIR) -I$(CVM_JCS_BUILDDIR) -c $< -o $@

$(CVM_JCS): $(CVM_JCS_COBJS)
	@echo host cc  $@
	$(AT)$(HOST_CCC) $(CFLAGS_JCS) -o $@ $^ $(LINKLIBS_JCS)

$(CVM_CODEGEN_CFILES): $(CVM_JCS) $(CVM_JCS_INPUT_FILES)
	@echo ... running jcs
	$(AT)$(CVM_JCS) -a  $(CVM_JCS_OPTIONS) \
	    $(call abs2rel,$(CVM_JCS_INPUT_FILES)) \
	    -o $(call abs2rel,$(CVM_CODEGEN_CFILES) $(CVM_CODEGEN_HFILE))
