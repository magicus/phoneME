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
# Alternative platform configuration section for Metrowerks.
#
# This file is included by these lines in jvm.make:
#
# ifdef ALT_PLATFORM
# include $(SHARE_DIR)/$(ALT_PLATFORM)/alt_config.make
# endif
#
# The symbol ALT_PLATFORM is defined by the Symbian OS build
# configuration files such as
# $(JVMWorkSpace)/build/win32_i386_cw/win32_i386_cw.cfg
#----------------------------------------------------------------------

MakeDepsMain_win32_cw = CWPlatform
Alt_MakeDepsOpts      = -forceWritePlatformSpecificFiles

CC_cw		= mwcc.exe
CPP_cw		= mwcc.exe
LINK_cw		= mwld.exe -application
LIBMGR_cw	= mwld.exe -library

ifeq ($(ENABLE_INLINEASM_INTERPRETER), true)
ASM_SUFFIX_cw	= .c
else
ASM_cw		= ml.exe
ASM_SUFFIX_cw	= .asm
endif

OBJ_SUFFIX_cw	= .obj
LIB_PREFIX_cw	=
LIB_SUFFIX_cw	= .lib
EXE_SUFFIX_cw	= .exe

EXTRA_CLEAN_cw	= *.asm *.lst

SAVE_TEMPS_cw	= -save-temps

ROMIZING_CFLAGS_cw	= -DROMIZING
