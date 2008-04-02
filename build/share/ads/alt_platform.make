#
#   
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

#----------------------------------------------------------------------
#
# Alternative platform configuration section for ADS
#
# This file is included by these lines in jvm.make:
#
# ifdef ALT_PLATFORM
# include $(SHARE_DIR)/$(ALT_PLATFORM)/alt_platform.make
# endif
#
# The symbol ALT_PLATFORM is defined by the ADS build
# configuration file $(JVMWorkSpace)/build/ads_arm/ads_adm.config
#----------------------------------------------------------------------

MakeDepsMain_ads     = ADSPlatform
MakeDepsOpts_ads     = -resolveVpath true
LOOP_GEN_ARG_ads     =

ASM_ads              = armasm

ifeq ($(ENABLE_THUMB_VM), true)
CPP_ads              = tcpp
CC_ads               = tcc
else
CPP_ads              = armcpp
CC_ads               = armcc
endif

LINK_ads             = armlink
LIBMGR_ads           = armar

SAVE_TEMPS_ads       =
OBJ_SUFFIX_ads       = .o
LIB_PREFIX_ads       =
LIB_SUFFIX_ads       = .lib
EXE_SUFFIX_ads       = .axf
ASM_SUFFIX_ads       = .asm

EXTRA_CLEAN_ads     = Interpreter_$(arch).asm
ROMIZING_CFLAGS_ads = -DROMIZING=1

MakeDepsSources_ads = \
	$(WorkSpace)/src/tools/MakeDeps/ADSPlatform.java
