#
# Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

GNU_TOOLS_BINDIR  =

#
# PCSL defs
#
PCSL_TARGET      ?= $(WIN32_PLATFORM)_$(TARGET_CPU)
PCSL_PLATFORM     = $(PCSL_TARGET)_evc
NETWORK_MODULE    = winsock
PCSL_MAKE_OPTIONS = USE_CYGWIN=true

#
# MIDP defs
#

MIDP_PLATFORM     = $(WIN32_PLATFORM)
MIDP_MAKEFILE_DIR = build/$(WIN32_PLATFORM)

CONFIGURATION_OVERRIDE	= \
        $(MIDP_DIR)/src/configuration/wince/sp176x220.xml

MIDP_OBJECTS      = \
        $(MIDP_OUTPUT_DIR)/obj$(DEBUG_POSTFIX)/$(TARGET_CPU)/*.o \
        $(MIDP_OUTPUT_DIR)/obj$(DEBUG_POSTFIX)/$(TARGET_CPU)/resources.res

LIBPATH           += /libpath:$(call POSIX2HOST,$(PCSL_OUTPUT_DIR)/$(PCSL_TARGET)/lib)
MIDP_LIBS         = \
	libpcsl_file.lib libpcsl_memory.lib libpcsl_print.lib \
	libpcsl_string.lib libpcsl_network.lib Ws2.lib gx.lib aygshell.lib
WIN_LINKLIBS += $(MIDP_LIBS)