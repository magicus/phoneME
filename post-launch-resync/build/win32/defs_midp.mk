#
# @(#)defs_midp.mk	1.0 06/11/02
#
# Copyright © 2006 Sun Microsystems, Inc. All rights reserved.  
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
#
#
GNU_TOOLS_BINDIR  =

#
# PCSL defs
#
PCSL_TARGET       = $(WIN32_PLATFORM)_$(TARGET_CPU)
PCSL_PLATFORM     = $(PCSL_TARGET)_evc
NETWORK_MODULE    = winsock
PCSL_MAKE_OPTIONS = USE_CYGWIN=true

#
# MIDP defs
#
MIDP_MAKEFILE_DIR = build/$(WIN32_PLATFORM)

CONFIGURATION_OVERRIDE	= \
        c:/ws/cyclops/midp_cdc/src/configuration/wince/sp176x220.xml

MIDP_OBJECTS      = \
        $(MIDP_OUTPUT_DIR)/obj$(DEBUG_POSTFIX)/$(TARGET_CPU)/*.o \
        $(MIDP_OUTPUT_DIR)/obj$(DEBUG_POSTFIX)/$(TARGET_CPU)/resources.res
                             
MIDP_LIBS         = \
        /libpath:$(PCSL_OUTPUT_DIR)/$(PCSL_TARGET)/lib \
         libpcsl_file.lib libpcsl_memory.lib libpcsl_print.lib \
         libpcsl_string.lib libpcsl_network.lib Ws2.lib gx.lib aygshell.lib
                             
