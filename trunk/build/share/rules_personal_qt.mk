#
# @(#)rules_personal_qt.mk	1.12 06/10/25
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
	@echo "MOC                    =" `which $(word 1,$(MOC))`
	@echo "QTEMBEDDED             = $(QTEMBEDDED)"
	@echo "QTOPIA                 = $(QTOPIA)"
	@echo "QT_NEED_THREAD_SUPPORT = $(QT_NEED_THREAD_SUPPORT)"
	@echo "QT_KEYPAD_MODE         = $(QT_KEYPAD_MODE)"
	@echo "QT_STATIC_LINK         = $(QT_STATIC_LINK)"

#
# Generate moc files.
#
.PRECIOUS: $(MOC_OUTPUT_DIR)/%_moc.cc
$(MOC_OUTPUT_DIR)/%_moc.cc: $(CVM_SHAREROOT)/personal/native/awt/$(AWT_PEERSET_NAME)/%.h
	@echo "moc $@"
	$(AT)$(MOC) $< -o $@

#
# Compile moc generated files.
#
# Unfortunately we can't use the %.cc rule defined it defs.mk. For some
# reason vpath doesn't enable pattern matching on intermediate files.
# We must provide a rule that gives an explicit path for the intermediate
# dependencies.
#
$(CVM_OBJDIR)/%_moc.o: $(MOC_OUTPUT_DIR)/%_moc.cc
	@echo "c++ $@"
	$(CCC_CMD_SPEED)
	$(GENERATEMAKEFILES_CMD)
	$(CSTACKANALYSIS_CMD)
