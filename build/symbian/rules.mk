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
# @(#)rules.mk	1.19 06/10/10
#

ifeq ($(CVM_PROVIDE_TARGET_RULES), true)

ifeq ($(CVM_BUILD_SUBDIR_NAME),.)
ABLD_BAT = ABLD.BAT
BLD_INF = bld.inf
CVM_MMP_NAME = cvm
ifeq ($(CVM_DLL),true)
CVMLAUNCHER_MMP_NAME = cvmlauncher
endif
else
ABLD_BAT = ABLD-$(CVM_BUILD_SUBDIR_NAME).BAT
BLD_INF = $(CVM_BUILD_SUBDIR_NAME)/bld.inf
CVM_MMP_NAME = cvm-$(CVM_BUILD_SUBDIR_NAME)
ifeq ($(CVM_DLL),true)
CVMLAUNCHER_MMP_NAME = cvmlauncher-$(CVM_BUILD_SUBDIR_NAME)
endif
endif
CVM_MMP = $(CVM_MMP_NAME).mmp
ifeq ($(CVM_DLL),true)
CVMLAUNCHER_MMP = $(CVMLAUNCHER_MMP_NAME).mmp
endif

SYMBIAN_BUILD_DIR = $(subst \,/,$(call POSIX2WIN,$(shell pwd)))

$(CVM_BINDIR)/$(CVM) :: $(CVM_MMP) $(CVMLAUNCHER_MMP) $(CVM_OBJECTS) $(CVM_OBJDIR)/$(CVM_ROMJAVA_O) $(CVM_FDLIB_FILES) $(CVMEXT_LIB)

$(CVM_BINDIR)/$(CVM) :: $(ABLD_BAT)
ifeq ($(CVM_DLL),true)
	$(AT)mkdir -p ../$(SYMBIAN_DEFDIR)
	$(AT)rm -f ../$(SYMBIAN_DEFDIR)/cvmu.def
	$(AT)cp $(CVM_TOP)/src/symbian/lib/$(CVM_DEFFILE) ../$(SYMBIAN_DEFDIR)/cvmu.def
endif
	$(AT)$(call SYMBIAN_DEV_CMD,$^,makefile $(SYMBIAN_PLATFORM))
	$(AT)perl ../symbian/fix_project.pl $(SYMBIAN_ROOT_U) $(SYMBIAN_BUILD_DIR) $(SYMBIAN_PLATFORM) $(CVM_MMP_NAME)
	$(AT)$(call SYMBIAN_DEV_CMD,$^,target $(SYMBIAN_PLATFORM) $(SYMBIAN_BLD))

#
# Make sure we add Symbian gcc to the PATH before we start.
#
$(ABLD_BAT) : $(BLD_INF) FORCE
ifneq ($(BLD_INF),bld.inf)
	$(AT)cp $(BLD_INF) bld.inf
endif
	$(AT)$(call SYMBIAN_DEV_CMD,bldmake,bldfiles $(SYMBIAN_PLATFORM))
ifneq ($(ABLD_BAT),ABLD.BAT)
	$(AT)mv ABLD.BAT $@
endif

$(BLD_INF) : FORCE
	@echo ... generating $@
	$(AT)rm -f $@
	$(AT)echo "PRJ_MMPFILES" > $@
	$(AT)echo $(CVM_MMP) >> $@
	$(AT)echo $(CVMLAUNCHER_MMP) >> $@

# Project file for the launcher when building CVM as a DLL
ifeq ($(CVM_DLL),true)
$(CVMLAUNCHER_MMP) : FORCE
	$(AT)rm -f $@
	$(AT)echo "TARGET CVMLAUNCHER.EXE" > $@
	$(AT)echo "TARGETTYPE EXE" >> $@
	$(AT)echo "UID 0" >> $@
	$(AT)echo "SYSTEMINCLUDE \epoc32\include \epoc32\include\libc" >> $@
	$(AT)echo "LIBRARY $(SYMBIAN_LINK_LIBS)" >> $@
	$(AT)echo "EPOCSTACKSIZE 32768" >> $@
	$(AT)echo $(SYMBIAN_USER_OPTION) >> $@
	$(AT)for s in $(CVM_DEFINES); do \
		printf "MACRO %s\n" $$s | sed -e 's/-D//g'; \
	done >> $@
	$(AT)for s in $(ALL_INCLUDE_DIRS); do \
		printf "USERINCLUDE %s\n" $$s; \
	done >> $@
	$(AT)echo SOURCEPATH ../../src/symbian/bin>> $@
	$(AT)echo SOURCE java_md.cpp >> $@
endif

# CVM project file.
$(CVM_MMP) : FORCE
	@echo ... generating $@
	$(AT)rm -f $@
	$(AT)rm -rf $(CVM_OBJDIR)/*
	$(AT)echo "TARGET CVM.$(SYMBIAN_TARGET)" > $@
	$(AT)echo "TARGETTYPE $(SYMBIAN_TARGET)" >> $@
	$(AT)echo "UID 0" >> $@
	$(AT)echo $(SYMBIAN_BUILD_AS_ARM) >> $@
ifeq ($(CVM_DLL),true)
	$(AT)echo "#if !defined(WINS)" >> $@
	$(AT)echo "EPOCALLOWDLLDATA" >> $@
	$(AT)echo "EPOCDATALINKADDRESS	0x33000000" >> $@
	$(AT)echo "EPOCCALLDLLENTRYPOINTS" >> $@
	$(AT)echo "#endif" >> $@
endif
	$(AT)echo "SYSTEMINCLUDE \epoc32\include \epoc32\include\libc" >> $@
	$(AT)echo "LIBRARY $(SYMBIAN_LINK_LIBS)" >> $@
	$(AT)echo "EPOCSTACKSIZE 32768" >> $@
	$(AT)echo $(SYMBIAN_USER_OPTION) >> $@
ifeq ($(CVM_SYMBOLS),true)
	$(AT)echo SRCDBG >> $@
endif
	$(AT)for s in $(CVM_DEFINES); do \
		printf "MACRO %s\n" $$s | sed -e 's/-D//g'; \
	done >> $@
	$(AT)for s in $(ALL_INCLUDE_DIRS); do \
		printf "USERINCLUDE %s\n" $$s; \
	done >> $@

$(CVM_OBJDIR)/%.o: %.cc
	$(AT)echo SOURCEPATH $(dir $^) >> $(CVM_MMP)
	$(AT)echo SOURCE $(notdir $^) >> $(CVM_MMP)

$(CVM_OBJDIR)/%.o: %.cpp
	$(AT)echo SOURCEPATH $(dir $^) >> $(CVM_MMP)
	$(AT)echo SOURCE $(notdir $^) >> $(CVM_MMP)

$(CVM_OBJDIR)/%.o: %.c
	$(AT)echo SOURCEPATH $(dir $^) >> $(CVM_MMP)
	$(AT)echo SOURCE $(notdir $^) >> $(CVM_MMP)

$(CVM_OBJDIR)/%.o: %.S
	$(AT)echo SOURCEPATH $(dir $^) >> $(CVM_MMP)
	$(AT)echo SOURCE $(notdir $^) >> $(CVM_MMP)

$(CVM_FDLIB_FILES): $(CVM_OBJDIR)/%.o: $(CVM_FDLIBM_SRCDIR)/%.c
	$(AT)echo SOURCEPATH $(dir $^) >> $(CVM_MMP)
	$(AT)echo SOURCE $(notdir $^) >> $(CVM_MMP)

FORCE:

endif

ifneq ($(CVM_PROVIDE_TARGET_RULES), true)
$(TLS_LIB) : CPPFLAGS += -DCVM_HPI_DLL_IMPL
$(TLS_LIB) : COMPILE_FLAVOR = SPEED
$(TLS_LIB) : LINKLIBS = EDLL.LIB EUSER.LIB
$(TLS_LIB) : SO_LINKFLAGS += /nodefaultlib
$(TLS_LIB) : SO_LINKFLAGS += /entry:_E32Dll /include:'?_E32Dll@@YGHPAXI0@Z'

$(TLS_LIB) : $(CVM_OBJDIR)/threads_md_dll.o
	$(AT)$(call SO_LINK_CMD, $^,)

$(CVM_OBJDIR)/threads_md_dll.o : threads_md.cpp
	@echo "c++ $@"
	$(CCC_CMD_$(COMPILE_FLAVOR))
	$(GENERATEMAKEFILES_CMD)
	$(CSTACKANALYSIS_CMD)
endif

#-include epoc.make

clean::
	$(CVM_SYMBIAN_CLEANUP_ACTION)
