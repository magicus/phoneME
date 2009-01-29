#
# @(#)rules_basis_qt.mk	1.10 06/10/25
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


printconfig::
	@echo "QT_TARGET_DIR          = $(QT_TARGET_DIR)"
	@echo "QT_TARGET_INCLUDE_DIR  = $(QT_TARGET_INCLUDE_DIR)"
	@echo "QT_TARGET_LIB_DIR      = $(QT_TARGET_LIB_DIR)"
	@echo "QTEMBEDDED             = $(QTEMBEDDED)"
	@echo "QTOPIA                 = $(QTOPIA)"
	@echo "QT_NEED_THREAD_SUPPORT = $(QT_NEED_THREAD_SUPPORT)"
	@echo "QT_KEYPAD_MODE         = $(QT_KEYPAD_MODE)"
	@echo "QT_STATIC_LINK         = $(QT_STATIC_LINK)"
