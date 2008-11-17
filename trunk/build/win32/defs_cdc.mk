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
# @(#)defs_cdc.mk	1.15 06/10/10
#

#
# win32 is currently not supported, but include FileURLMapper
# in CVM_BUILDTIME_CLASSES to parallel identical changes in
# the solaris & linux builds. This is to prevent a redundant
# presence of sun.misc.FileURLMapper in the $(J2ME_CLASSLIB) jarfile.
#
CVM_BUILDTIME_CLASSES += \
        sun.misc.FileURLMapper

#
# CDC library platform classes
#
CLASSLIB_CLASSES += \
	java.lang.Terminator \

#
# Things to build for CDC
#
CVM_TARGETOBJS_SPACE += \
	Runtime_md.o \
	FileSystem_md.o \
	InetAddressImplFactory.o \
	Inet6AddressImpl.o \
	Inet4AddressImpl.o \
	timezone_md.o \
	net_util_md.o \
	io_util_md.o \
	dirent_md.o \
	NetworkInterface.o \
	PlainDatagramSocketImpl_md.o \

include ../win32/defs_cdc_$(WIN32_PLATFORM).mk
