#
# @(#)defs_personal_motif.mk	1.6 06/10/10
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

# FIXME - The following requires that the host and target be the same.
# A $(CVM_TOOLS_DIR)/lib installation should be used if CVM_USE_NATIVE_TOOLS
# is false.
PROFILE_INCLUDE_DIRS += /usr/X11R6/include
AWT_LIB_LIBS += -L/usr/X11R6/lib -Xlinker -Bstatic -lXm -Xlinker -Bdynamic -lXp -lXt -lXmu -lXext -lX11 -lm
