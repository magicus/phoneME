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
# @(#)top.mk	1.97 06/10/10
#
# Topmost makefile shared by all Solaris platforms
#
######################################################
# Definitions of supported Solaris specific options.
# Shared options are described in build/share/top.mk
######################################################
#
# USE_SUNCC: if true, then use cc instead of gcc. Defaults to false
#     unless CC=cc. This option is now deprecated. Use
#     CVM_USE_NATIVE_TOOLS instead.
#
# HAVE_64_BIT_IO: if defined, use 64-bit versions of certain POSIX
#     I/O routines in src/share/porting/posix/posix_io_md.c. Should
#     default to being defined on Solaris, but unfortunately gcc picks
#     up its includes out of its distribution rather than /usr/include, so
#     we don't get the 64-bit prototypes.

#
# USE_SUNCC is the same as CVM_USE_NATIVE_TOOLS
#
ifneq ($(USE_SUNCC), )
CVM_USE_NATIVE_TOOLS = $(USE_SUNCC)
else
USE_SUNCC = $(CVM_USE_NATIVE_TOOLS)
endif
