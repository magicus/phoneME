# Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 only,
# as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# version 2 for more details (a copy is included at /legal/license.txt).
# 
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
# CA 95054 or visit www.sun.com if you need additional information or have
# any questions.

#
#  Makefile for building the jdwp tool
#

###############################################################################
# Make definitions:

CVM_JDWP_BUILD_TOP     = $(CVM_BUILD_TOP)/jdwp
CVM_JDWP_OBJDIR        = $(CVM_JDWP_BUILD_TOP)/obj
CVM_JDWP_LIBDIR        ?= $(CVM_LIBDIR)
CVM_JDWP_FLAGSDIR      = $(CVM_JDWP_BUILD_TOP)/flags
CVM_JDWP_CLASSES       = $(CVM_JDWP_BUILD_TOP)/classes

CVM_JDWP_SHAREROOT     = $(CVM_TOP)/src/share/tools/jpda
CVM_JDWP_TARGETROOT    = $(CVM_TOP)/src/$(TARGET_OS)/tools/jpda

CVM_JDWP_BUILDDIRS += \
        $(CVM_JDWP_OBJDIR) \
        $(CVM_JDWP_FLAGSDIR) \
        $(CVM_JDWP_CLASSES)

CVM_JDWP_LIB = libjdwp$(LIB_POSTFIX)

CVM_JDWP_TRANSPORT = socket

#
# Search path for include files:
#
CVM_JDWP_INCLUDES  += \
        -I$(CVM_SHAREROOT)/javavm/export \
	-I$(CVM_JDWP_TARGETROOT)/back \
	-I$(CVM_JDWP_SHAREROOT)/back \
	-I$(CVM_JDWP_SHAREROOT)/transport/export \
	-I$(CVM_JDWP_TARGETROOT)/transport/$(CVM_JDWP_TRANSPORT) \
	-I$(CVM_JDWP_BUILD_TOP)

ifeq ($(CVM_DEBUG), true)
CVM_JDWP_DEFINES += -DDEBUG
endif

JPDA_NO_DLALLOC = true

ifeq ($(JPDA_NO_DLALLOC), true)
CVM_JDWP_DEFINES += -DJPDA_NO_DLALLOC
endif

jdwp : CVM_INCLUDES += $(CVM_JDWP_INCLUDES)
jdwp : CVM_DEFINES += $(CVM_JDWP_DEFINES)

#
# List of object files to build:
#
CVM_JDWP_SHAREOBJS += \
	ArrayReferenceImpl.o \
	ArrayTypeImpl.o \
	ClassTypeImpl.o \
	ClassLoaderReferenceImpl.o \
	ClassObjectReferenceImpl.o \
	EventRequestImpl.o \
	FieldImpl.o \
	MethodImpl.o \
	ObjectReferenceImpl.o \
	ReferenceTypeImpl.o \
	SDE.o \
	StackFrameImpl.o \
	StringReferenceImpl.o \
	ThreadGroupReferenceImpl.o \
	ThreadReferenceImpl.o \
	VirtualMachineImpl.o \
	bag.o \
	classTrack.o \
	commonRef.o \
	debugDispatch.o \
	debugInit.o \
	debugLoop.o \
	eventFilter.o \
	eventHandler.o \
	eventHelper.o \
	inStream.o \
	invoker.o \
	outStream.o \
	popFrames.o \
	standardHandlers.o \
	stepControl.o \
	stream.o \
	threadControl.o \
	transport.o \
	util.o \
	version.o \
	linker_md.o \
	exec_md.o

ifneq ($(JPDA_NO_DLALLOC), true)
CVM_JDWP_SHAREOBJS += dlAlloc.o
endif



CVM_JDWP_OBJECTS0 = $(CVM_JDWP_SHAREOBJS) $(CVM_JDWP_TARGETOBJS)
CVM_JDWP_OBJECTS  = $(patsubst %.o,$(CVM_JDWP_OBJDIR)/%.o,$(CVM_JDWP_OBJECTS0))

CVM_JDWP_SRCDIRS  = \
	$(CVM_JDWP_SHAREROOT)/back \
	$(CVM_JDWP_SHAREROOT)/transport \
	$(CVM_JDWP_SHAREROOT)/transport/$(CVM_JDWP_TRANSPORT) \
	$(CVM_JDWP_TARGETROOT)/back \
	$(CVM_JDWP_TARGETROOT)/transport \
	$(CVM_JDWP_TARGETROOT)/transport/$(CVM_JDWP_TRANSPORT) \

vpath %.c      $(CVM_JDWP_SRCDIRS)
vpath %.S      $(CVM_JDWP_SRCDIRS)

CVM_JDWP_FLAGS += \
        CVM_SYMBOLS \
        CVM_OPTIMIZED \
        CVM_DEBUG \
        CVM_JVMDI \
        CVM_DYNAMIC_LINKING \

CVM_JDWP_FLAGS0 = $(foreach flag, $(CVM_JDWP_FLAGS), $(flag).$($(flag)))

CVM_JDWP_CLEANUP_ACTION = \
        rm -rf $(CVM_JDWP_BUILD_TOP)

CVM_JDWP_CLEANUP_OBJ_ACTION = \
        rm -rf $(CVM_JDWP_OBJDIR)

###############################################################################
# Make rules:

tools:: jdwp
tool-clean: jdwp-clean

jdwp-clean:
	$(CVM_JDWP_CLEANUP_ACTION)

ifeq ($(CVM_JVMDI), true)
    jdwp_build_list = jdwp_initbuild \
	    $(CVM_JDWP_BUILD_TOP)/JDWPCommands.h \
	    $(CVM_JDWP_LIBDIR)/$(CVM_JDWP_LIB) \
	    jdwp-dt
else
    jdwp_build_list =
endif

jdwp: $(jdwp_build_list)

jdwp_initbuild: jdwp_check_cvm jdwp_checkflags $(CVM_JDWP_BUILDDIRS)

# Make sure that CVM is built before building jdwp.  If not, the issue a
# warning and abort.
jdwp_check_cvm:
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM with before building JDWP."; \
	    exit 1; \
	else \
	    echo; echo "Building JDWP tool ..."; \
	fi

# Make sure all of the build flags files are up to date. If not, then do
# the requested cleanup action.
jdwp_checkflags: $(CVM_JDWP_FLAGSDIR)
	@for filename in $(CVM_JDWP_FLAGS0); do \
		if [ ! -f $(CVM_JDWP_FLAGSDIR)/$${filename} ]; then \
			echo "JDWP flag $${filename} changed. Cleaning up."; \
			rm -f $(CVM_JDWP_FLAGSDIR)/$${filename%.*}.*; \
			touch $(CVM_JDWP_FLAGSDIR)/$${filename}; \
			$(CVM_JDWP_CLEANUP_OBJ_ACTION); \
		fi \
	done

$(CVM_JDWP_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_JDWP_LIBDIR)/$(CVM_JDWP_LIB): $(CVM_JDWP_OBJECTS)
	@echo "Linking $@"
	$(SO_LINK_CMD) $(LINKLIBS)
	@echo "Done Linking $@"

# The following are used to build the .o files needed for $(CVM_JDWP_OBJECTS):

#####################################
# include all of the dependency files
#####################################
files := $(foreach file, $(wildcard $(CVM_JDWP_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

# jdwpgen

JDWPGENPKGDIR = com/sun/tools/jdwpgen
JDWPGENDIR = $(CVM_JDWP_SHAREROOT)/classes/$(JDWPGENPKGDIR)
JDWP_SPEC = $(JDWPGENDIR)/jdwp.spec
JDWPGEN = com.sun.tools.jdwpgen
JDWPGEN_CLASS = $(CVM_JDWP_CLASSES)/$(JDWPGENPKGDIR)/Main.class

$(JDWPGEN_CLASS) : $(CVM_JDWP_SHAREROOT)/classes/$(JDWPGENPKGDIR)/Main.java
	$(CVM_JAVAC) -d $(CVM_JDWP_CLASSES) \
		-sourcepath $(CVM_JDWP_SHAREROOT)/classes \
		$<

$(CVM_JDWP_BUILD_TOP)/JDWPCommands.h : $(JDWPGEN_CLASS)
	$(CVM_JAVA) -classpath $(CVM_JDWP_CLASSES) \
		$(JDWPGEN).Main $(JDWP_SPEC) \
                -jvmdi $(CVM_TOP)/src/share/javavm/export/jvmdi.h \
                -include $@

$(CVM_JDWP_OBJDIR)/%.o: %.c
	@echo "... $@"
	$(SO_CC_CMD)
ifeq ($(GENERATEMAKEFILES), true)
	@$(CC) $(CCDEPEND) $(CPPFLAGS) $< 2> /dev/null | \
	    sed 's!$*\.o!$(dir $@)&!g' > $(@:.o=.d)
endif

$(CVM_JDWP_OBJDIR)/%.o: %.S
	@echo "... $@"
	$(SO_ASM_CMD)

-include  ../$(TARGET_OS)/jdwp_transport.mk
-include  ../share/jdwp_transport.mk
