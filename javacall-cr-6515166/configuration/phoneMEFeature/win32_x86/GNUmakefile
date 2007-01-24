#
# Copyright 2006 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
#

ifeq ($(JAVACALL_DIR),)
$(error JAVACALL_DIR is not set.)
endif

#For each project specified environment shall be selected
ifeq ($(JAVACALL_PROJECT_DIR),)
JAVACALL_PROJECT_DIR=$(JAVACALL_DIR)
endif

#include $(JAVACALL_PROJECT_DIR)/environment.gmk
include environment.gmk

#For each project location for basic definitions shall be specified
#(location of the used javacall_platform_defs.h)
JAVACALL_PLATFORM_DEFS_DIR=$(JAVACALL_DIR)/implementation/win32

#For each project specified toolchain shall be selected
include $(JAVACALL_DIR)/build/vc/rules.gmk

#Specific targets can be achieved before and after achievement of common targets
#Specify these targets after including 'rules.gmk' file.
pre_target:
post_target: $(JAVACALL_OUTPUT_LIB_DIR)/emulator.res
