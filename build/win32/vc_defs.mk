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
# @(#)vc_defs.mk	1.12 06/10/10
#
# defs for VC target
#

CVM_DEFINES += -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE \
	-D_WIN32_WINNT=0x0400 -DWIN32

WIN_LINKLIBS += ws2_32.lib user32.lib advapi32.lib kernel32.lib

LINK_SUBSYSTEM	= /subsystem:console
LINKEXE_ENTRY	= /entry:mainCRTStartup
