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
# @(#)jal.mk	1.7	06/10/10
#
# Makefile for building JavaAPILister and the text file(s) it
# produces.
#
# NOTICE: This is NOT part of the usual CVM makefile hierarchy.
# This is NOT to be run as a usual part of the CVM build process.
# The resulting file(s), MIDPFilterConfig.txt and (as of this writing)
# MIDPPermittedClasses.txt do not need to be rederived
# with every build. They are dependent only on the CLDC version that
# we wish to support with the dual stack classloader.
#
# This makefile borrows heavily from jcc.mk.
#
# To use:
# Create a subdirectory of the 'build' directory.
# Obtain a copy of the CLDC zip file you wish to support.
# Determine which java tools you wish to use to build and
# execute JavaAPILister.
# find a make or gnumake to use.
# run make -f ../share/jal.mk <variables from below list>
# after examining the files produced, check them into the directory
# src/share/lib
#
# important variables:
# CVM_TOOLS_BIN	-- where to find the javac and java to run this program
# CVM_CLDC_ZIP	-- the name of the cldc.zip file to process
#
# other variables:
# CVM_FILESEP	-- file component separator
# PS		-- path separator. ; on Windows



####################
# Imported variables
####################
CVM_TOOLS_BIN ?= $(CVM_TOOLS_DIR)/java/jdk1.3/bin
CVM_JAVA = $(CVM_TOOLS_BIN)/java
CVM_JAVAC = $(CVM_TOOLS_BIN)/javac
CVM_FILESEP ?= /
PS ?= :



#################
# Local variables
# These probably have to be revisited for a Windows host build.
#################

CVM_TOP 	= ../..
CVM_DERIVEDROOT = ./derived
CVM_SHAREROOT	=$(CVM_TOP)/src/share
CVM_JCC_SRCPATH	= $(CVM_TOP)/src/share/javavm/jcc
CVM_JAL_INPUT 	+= $(CVM_CLDC_ZIP)
CVM_OPCODECONSTDIR = $(CVM_DERIVEDROOT)/javavm/runtime/opcodeconsts
CVM_JCC_CLASSPATH= $(CVM_BUILD_TOP)/classes.jcc
CVM_JAL_DEPEND	= $(CVM_JCC_CLASSPATH)/JavaAPILister.class

#################
# Default target
#################
all: MIDPFilterConfig.txt MIDPPermittedClasses.txt

#################
# Clean target
#################
clean:
	rm -rf $(CVM_BUILD_TOP)/classes.jcc derived
	rm -f MIDPFilterConfig.txt MIDPPermittedClasses.txt

#################
# Directory targets
#################
$(CVM_JCC_CLASSPATH):
	mkdir $(CVM_JCC_CLASSPATH)
$(CVM_OPCODECONSTDIR):
	mkdir -p $(CVM_OPCODECONSTDIR)

##############################################
# To successfully compile some of the components of
# jal, we need OpcodeConst.java, which is derived from
# opcodes.list.
##############################################

CVM_GENOPCODE_DEPEND	= $(CVM_JCC_CLASSPATH)/GenOpcodes.class
CVM_OPCODE_LIST		= $(CVM_SHAREROOT)/javavm/include/opcodes.list
CVM_GENOPCODE_TARGETS	= $(CVM_OPCODECONSTDIR)/OpcodeConst.java

$(CVM_GENOPCODE_TARGETS): $(CVM_OPCODE_LIST) $(CVM_GENOPCODE_DEPEND) $(CVM_OPCODECONSTDIR)
	@echo ... $(CVM_OPCODE_LIST)
	$(AT)export CLASSPATH; \
	CLASSPATH=$(CVM_JCC_CLASSPATH); \
	$(CVM_JAVA) GenOpcodes $(CVM_OPCODE_LIST) \
	    -javaConst $(CVM_OPCODECONSTDIR)/OpcodeConst.java 

$(CVM_GENOPCODE_DEPEND) :: $(CVM_JCC_CLASSPATH)
$(CVM_GENOPCODE_DEPEND) :: $(CVM_JCC_SRCPATH)/GenOpcodes.java \
		 $(CVM_JCC_SRCPATH)/text/*.java \
		 $(CVM_JCC_SRCPATH)/util/*.java \
		 $(CVM_JCC_SRCPATH)/JCCMessage.properties
	@echo "... $@"
	$(AT)CLASSPATH=$(CVM_JCC_SRCPATH); export CLASSPATH; \
	$(CVM_JAVAC) $(JAVAC_OPTIONS) -d $(CVM_JCC_CLASSPATH) \
	    $(subst /,$(CVM_FILESEP),$(CVM_JCC_SRCPATH)/GenOpcodes.java)
	$(AT)rm -f $(CVM_JCC_CLASSPATH)/JCCMessage.properties; \
	cp $(CVM_JCC_SRCPATH)/JCCMessage.properties $(CVM_JCC_CLASSPATH)/JCCMessage.properties

####################################

CVM_JAL_OPTIONS= -i 'java/*' -i 'javax/*'

MIDPFilterConfig.txt MIDPPermittedClasses.txt: $(CVM_JAL_INPUT) $(CVM_JAL_DEPEND)
	@echo "jal files"
	$(AT)export CLASSPATH; \
	CLASSPATH=$(CVM_JCC_CLASSPATH); \
	$(CVM_JAVA) JavaAPILister $(CVM_JAL_OPTIONS) \
		$(CVM_JAL_INPUT) \
		-mout MIDPFilterConfig.txt -cout MIDPPermittedClasses.txt

#################
# JavaAPILister
#################


CVM_JAL_CLASSES  = $(CVM_JCC_SRCPATH)/JavaAPILister.java


$(CVM_JAL_DEPEND) :: $(CVM_JCC_CLASSPATH)
$(CVM_JAL_DEPEND) :: $(CVM_OPCODECONSTDIR)/OpcodeConst.java
$(CVM_JAL_DEPEND) :: $(CVM_JCC_SRCPATH)/JavaCodeCompact.java \
		 $(CVM_JCC_SRCPATH)/components/*.java \
		 $(CVM_JCC_SRCPATH)/dependenceAnalyzer/*.java \
		 $(CVM_JCC_SRCPATH)/jcc/*.java \
		 $(CVM_JCC_SRCPATH)/runtime/*.java \
		 $(CVM_JCC_SRCPATH)/text/*.java \
		 $(CVM_JCC_SRCPATH)/util/*.java \
		 $(CVM_JCC_SRCPATH)/vm/*.java \
		 $(CVM_JCC_SRCPATH)/JCCMessage.properties
	@echo "... $@"
	@CLASSPATH=$(CVM_JCC_CLASSPATH)$(PS)$(CVM_JCC_SRCPATH)$(PS)$(CVM_DERIVEDROOT)/javavm/runtime; export CLASSPATH; \
	$(CVM_JAVAC) $(JAVAC_OPTIONS) -d $(CVM_JCC_CLASSPATH) \
	    $(CVM_JAL_CLASSES)
	@rm -f $(CVM_JCC_CLASSPATH)/JCCMessage.properties; \
	cp $(CVM_JCC_SRCPATH)/JCCMessage.properties $(CVM_JCC_CLASSPATH)/JCCMessage.properties
