#
# @(#)testgc.mk	1.10 06/10/10
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

CVM_SRCDIRS += \
	$(CVM_SHAREROOT)/javavm/test/GCTest/Lib \
	$(CVM_SHAREROOT)/javavm/test/GCTest/DirectMem/Csrc \
	$(CVM_SHAREROOT)/javavm/test/GCTest/IndirectMem/Csrc \
	$(CVM_SHAREROOT)/javavm/test/GCTest/RootScans/Csrc

CVM_INCLUDE_DIRS  += \
	$(CVM_SHAREROOT)/javavm/test/GCTest/DirectMem/Include \
	$(CVM_SHAREROOT)/javavm/test/GCTest/IndirectMem/Include \
	$(CVM_SHAREROOT)/javavm/test/GCTest/RootScans/Include

CVM_SHAREOBJS += \
         libGC.o \
	 DMArrayCopyLongTest.o \
	 DMArrayCopyRefTest.o \
	 DMArrayCopyShortTest.o \
	 DMArrayCopyBooleanTest.o \
	 DMArrayCopyByteTest.o \
	 DMArrayCopyCharTest.o \
	 DMArrayCopyDoubleTest.o \
	 DMArrayCopyFloatTest.o \
	 DMArrayCopyIntTest.o \
	 DMArrayRWBodyBooleanTest.o \
	 DMArrayRWBodyByteTest.o \
	 DMArrayRWBodyCharTest.o \
	 DMArrayRWBodyDoubleTest.o \
	 DMArrayRWByteTest.o \
	 DMArrayRWBodyFloatTest.o \
	 DMArrayRWBodyIntTest.o \
	 DMArrayRWBodyLongTest.o \
	 DMArrayRWBodyRefTest.o \
	 DMArrayRWBodyShortTest.o \
	 DMArrayRWBooleanTest.o \
	 DMArrayRWCharTest.o \
	 DMArrayRWDoubleTest.o \
	 DMArrayRWFloatTest.o \
	 DMArrayRWIntTest.o \
	 DMArrayRWLongTest.o \
	 DMArrayRWRefTest.o \
	 DMArrayRWShortTest.o \
	 DMFieldRW32Test.o \
	 DMFieldRW64Test.o \
	 DMFieldRWDoubleTest.o \
	 DMFieldRWFloatTest.o \
	 DMFieldRWIntTest.o \
	 DMFieldRWLongTest.o \
	 DMFieldRWRefTest.o \
	 IDArrayRWBooleanTest.o \
	 IDArrayRWByteTest.o \
	 IDFieldRWLongTest.o \
	 IDArrayRWCharTest.o \
	 IDArrayRWDoubleTest.o \
	 IDArrayRWFloatTest.o \
	 IDArrayRWIntTest.o \
	 IDArrayRWLongTest.o \
	 IDArrayRWRefTest.o \
	 IDArrayRWShortTest.o \
	 IDFieldRW32Test.o \
	 IDFieldRW64Test.o \
	 IDFieldRWDoubleTest.o \
	 IDFieldRWFloatTest.o \
	 IDFieldRWIntTest.o \
	 IDFieldRWRefTest.o \
         RootScansTest.o
