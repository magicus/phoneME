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
# @(#)jcc.mk	1.116 06/10/27
#

#
# Makefile for building JavaCodeCompact(tm) and romjava.c
#

####################
# Exported variables
####################

# this is exported for the cleanjcc rule
CVM_JCC_CLASSPATH	= $(CVM_BUILD_TOP)/classes.jcc

#CVM_ROMJAVA_O 		= romjava.o
CVM_ROMJAVA_O 		= libromjava.a
CVM_ROMJAVA_CPATTERN	= $(CVM_DERIVEDROOT)/javavm/runtime/romjava
CVM_ROMJAVA_LIST   	= $(CVM_ROMJAVA_CPATTERN)List
CVM_ROMJAVA_CLASSES_PER_FILE = 400

CVM_BUILDDIRS	+= $(CVM_JCC_CLASSPATH)

#################
# Local variables
#################

CVM_JCC_SRCPATH	= $(CVM_TOP)/src/share/javavm/jcc

##############################################
# We have a big set of files derived from 
# opcodes.list.
##############################################

CVM_GENOPCODE_DEPEND	= $(CVM_JCC_CLASSPATH)/GenOpcodes.class
CVM_OPCODE_LIST		= $(CVM_SHAREROOT)/javavm/include/opcodes.list
CVM_GENOPCODE_TARGETS	= \
	$(CVM_DERIVEDROOT)/javavm/include/gen_opcodes.h \
	$(CVM_DERIVEDROOT)/javavm/runtime/gen_opcodes.c \
	$(CVM_DERIVEDROOT)/javavm/runtime/bcattr.c \
	$(CVM_DERIVEDROOT)/javavm/runtime/opcodelen.c \
	$(CVM_DERIVEDROOT)/javavm/include/opcodeLabels.h \
	$(CVM_DERIVEDROOT)/javavm/include/opcodeSimplification.h \
	$(CVM_DERIVEDROOT)/javavm/runtime/opcodeconsts/OpcodeConst.java

ifeq ($(CVM_JIT), true)
CVM_GENOPCODE_TARGETS	+= \
	$(CVM_DERIVEDROOT)/javavm/runtime/jit/jitopcodemap.c
CVM_JITOPCODEMAP	 = \
	-opcodeMap $(CVM_DERIVEDROOT)/javavm/runtime/jit/jitopcodemap.c
endif

$(CVM_GENOPCODE_TARGETS): $(CVM_OPCODE_LIST) $(CVM_GENOPCODE_DEPEND)
	@echo ... $(CVM_OPCODE_LIST)
	$(AT)export CLASSPATH; \
	CLASSPATH=$(CVM_JCC_CLASSPATH); \
	$(CVM_JAVA) GenOpcodes $(CVM_OPCODE_LIST) \
	    -h $(CVM_DERIVEDROOT)/javavm/include/gen_opcodes.h \
	    -c $(CVM_DERIVEDROOT)/javavm/runtime/gen_opcodes.c \
	    -bcAttr $(CVM_DERIVEDROOT)/javavm/runtime/bcattr.c \
	    -opcodeLengths $(CVM_DERIVEDROOT)/javavm/runtime/opcodelen.c \
	    $(CVM_JITOPCODEMAP) \
	    -label $(CVM_DERIVEDROOT)/javavm/include/opcodeLabels.h \
	    -javaConst $(CVM_DERIVEDROOT)/javavm/runtime/opcodeconsts/OpcodeConst.java \
	    -simplification $(CVM_DERIVEDROOT)/javavm/include/opcodeSimplification.h

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


CVM_JCC_DEPEND	= $(CVM_JCC_CLASSPATH)/JavaCodeCompact.class

#
# Set JCC_EXCLUDES to something like "-excludeFile <path_to_file>" if you
# want to exclude a list of classes from romization.
#
JCC_EXCLUDES +=

# 
#  At this time, all natives would be JNI,
#  and we are ROM only -- no impure constant pools allowed,
#  so no constant-pool type-table required.
# 
# NOTE: CVM_PROFILE_JCC_OPTIONS are for JCC options that the profile specific
#       makefiles may wish to add (e.g. profile specific classpaths).  It can
#       be left empty (i.e. undefined) if the profile does not have any options
#       to add.
#
CVM_GENERATE_OFFSETS	= $(patsubst %,-extraHeaders CVMOffsets %,$(CVM_OFFSETS_CLASSES))
CVM_GENERATE_NATIVES	= $(patsubst %,-nativesType CNI %,$(CVM_CNI_CLASSES))
ifeq ($(CVM_JIT), true)
CVM_JCC_OPTIONS = -jit
endif
CVM_JCC_OPTIONS += \
		  $(CVM_GENERATE_NATIVES) \
		  -nativesType JNI "-*" \
		  -headersDir CNI $(CVM_DERIVEDROOT)/cni \
		  -headersDir JNI $(CVM_DERIVEDROOT)/jni \
		  -headersDir CVMOffsets $(CVM_DERIVEDROOT)/offsets \
		  $(JCC_EXCLUDES) \
		  $(CVM_PROFILE_JCC_OPTIONS) \
	          $(CVM_GENERATE_OFFSETS)

ifeq ($(CVM_JVMTI_ROM), true)
	override CVM_LOSSLESS_OPCODES = true
endif

ifeq ($(CVM_JVMPI_TRACE_INSTRUCTION), true)
	override CVM_LOSSLESS_OPCODES = true
endif

ifeq ($(CVM_LOSSLESS_OPCODES), true)
CVM_JCC_OPTIONS += -qlossless
endif

ifneq ($(CVM_CLASSLOADING), true)
CVM_JCC_OPTIONS += -imageAttribute noClassLoading
endif

ifeq ($(CVM_NO_CODE_COMPACTION), true)
CVM_JCC_OPTIONS += -noCodeCompaction
else
CVM_JCC_OPTIONS += -sharedCP
endif

CVM_JCC_INPUT += $(CVM_BUILDTIME_CLASSESZIP)

#
# For purposes of this workspace, ROMize the kBench benchmarks, right
# out of the jar file
#
ifeq ($(CVM_PRELOAD_TEST), true)
CVM_JCC_CL_SYS_INPUT = -cl:sys:boot
CVM_JCC_CL_SYS_INPUT += $(KBENCH_JAR)
CVM_JCC_CL_SYS_INPUT += $(CVM_TEST_CLASSESZIP)
endif

ifeq ($(CVM_DEBUG_CLASSINFO), true)
CVM_JCC_OPTIONS += -g
endif
# Allow breakpoints in ROMized code
ifeq ($(CVM_JVMTI_ROM), true)
CVM_JCC_OPTIONS += -imageAttribute noPureCode
endif
ifeq ($(CVM_ALLOW_UNRESOLVED), true)
CVM_JCC_OPTIONS += -allowUnresolved
endif

###########
# romjava.c files
###########

CVM_JCC_INPUT_FILES = $(filter-out -%,$(CVM_JCC_INPUT))
CVM_JCC_INPUT_FILES += $(filter-out -%,$(CVM_JCC_CL_SYS_INPUT))
CVM_JCC_INPUT_FILES += $(filter-out -%,$(CVM_JCC_CL_MIDP_INPUT))
CVM_JCC_INPUT_FILES += $(filter-out -%,$(CVM_JCC_CL_MISC_INPUT))

$(CVM_ROMJAVA_LIST): $(CVM_JCC_INPUT_FILES) $(CVM_JCC_DEPEND)
	@echo "jcc romjava.c files"
	$(AT)$(CVM_JAVA) -cp $(CVM_JCC_CLASSPATH) -Xmx256m JavaCodeCompact \
		$(CVM_JCC_OPTIONS) \
		-maxSegmentSize $(CVM_ROMJAVA_CLASSES_PER_FILE) \
		-o $(CVM_ROMJAVA_CPATTERN) \
		$(call POSIX2HOST,$(CVM_JCC_INPUT)) \
		$(CVM_JCC_CL_SYS_INPUT) $(CVM_JCC_CL_MIDP_INPUT) $(CVM_JCC_CL_MISC_INPUT) $(CVM_JCC_APILISTER_OPTIONS)

###########
# romjava.o  is made by compiling all the .c files and linking the result
###########
#
# How to get from a list of C files to a single ROMJAVA_O
#
# 1) make CVM_ROMJAVA_O depend on CVM_ROMJAVA_LIST, which contains
#    a list of all the C files created by a JCC run (when segmenting
#    output file).
# 2) Convert the contents of CVM_ROMJAVA_LIST to CVM_ROMJAVA_LIST_O,
#    a list of object files needed to create ROMJAVA_O.
# 3) Recursively call make to compile the object files in CVM_ROMJAVA_LIST_O
# 4) Link files in CVM_ROMJAVA_LIST_O to create ROMJAVA_O.
#

# Build list of romjava object files from the list of C files generated by JCC.
CVM_ROMJAVA_LIST_C = $(shell cat $(CVM_ROMJAVA_LIST))
CVM_ROMJAVA_LIST_C_NODIR = $(notdir $(CVM_ROMJAVA_LIST_C))
CVM_ROMJAVA_LIST_O = $(patsubst %.c,$(CVM_OBJDIR)/%.o,$(CVM_ROMJAVA_LIST_C_NODIR))

$(CVM_OBJDIR)/$(CVM_ROMJAVA_O): $(CVM_ROMJAVA_LIST) \
		$(CVM_TOP)/src/share/javavm/include/defs.h \
		$(CVM_TOP)/src/share/javavm/include/objects.h \
		$(CVM_TOP)/src/share/javavm/include/classes.h \
		$(CVM_TOP)/src/share/javavm/include/interpreter.h \
		$(CVM_TOP)/src/share/javavm/include/jni_impl.h \
		$(CVM_TOP)/src/share/javavm/include/typeid.h \
		$(CVM_TOP)/src/share/javavm/include/typeid_impl.h \
		$(CVM_TOP)/src/share/javavm/include/string_impl.h \
		$(CVM_TOP)/src/share/javavm/include/preloader_impl.h \
		$(CVM_TOP)/src/share/javavm/include/porting/endianness.h
	$(AT)$(MAKE) $(MAKE_NO_PRINT_DIRECTORY) $(CVM_ROMJAVA_LIST_O)
	$(AT)echo Linking $@
	$(AT)$(call TARGET_AR_CREATE,$@) $(call POSIX2HOST,$(CVM_ROMJAVA_LIST_O))
	$(AT)$(call TARGET_AR_UPDATE,$@)

#################
# JavaCodeCompact
#################

CVM_JCC_CLASSES0 = \
	JavaCodeCompact.java \
	runtime/CVMOffsetsHeader.java \
	runtime/CNIHeader.java \
	runtime/JNIHeader.java \
	runtime/CVMWriter.java

CVM_JCC_CLASSES1 = $(patsubst %,$(CVM_JCC_SRCPATH)/%,$(CVM_JCC_CLASSES0))
CVM_JCC_CLASSES  = $(subst /,$(CVM_FILESEP),$(CVM_JCC_CLASSES1))

$(CVM_JCC_DEPEND) :: $(CVM_JCC_CLASSPATH)
$(CVM_JCC_DEPEND) :: $(CVM_DERIVEDROOT)/javavm/runtime/opcodeconsts/OpcodeConst.java
$(CVM_JCC_DEPEND) :: $(CVM_JCC_SRCPATH)/JavaCodeCompact.java \
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
	    $(CVM_JCC_CLASSES)
	@rm -f $(CVM_JCC_CLASSPATH)/JCCMessage.properties; \
	cp $(CVM_JCC_SRCPATH)/JCCMessage.properties $(CVM_JCC_CLASSPATH)/JCCMessage.properties
