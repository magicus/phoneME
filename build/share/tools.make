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

JAVA		     = $(JDK_DIR)/bin/java
JAVAC                = $(JDK_DIR)/bin/javac
JAVAC_DEBUG          = :none
JAR                  = $(JDK_DIR)/bin/jar

all: jcc.jar buildtool.jar 

#----------------------------------------------------------------------
# jcc.jar -- creates native function table for the VM
#----------------------------------------------------------------------

JCC_DIR = $(WorkSpace)/src/tools/jcc
JCC_MF  = $(JCC_DIR)/MANIFEST.MF
JCC_SRC = \
    $(JCC_DIR)/JavaCodeCompact.java \
    $(JCC_DIR)/components/Attribute.java \
    $(JCC_DIR)/components/AttributeFactory.java \
    $(JCC_DIR)/components/ClassComponent.java \
    $(JCC_DIR)/components/ClassConstant.java \
    $(JCC_DIR)/components/ClassInfo.java \
    $(JCC_DIR)/components/ClassMemberInfo.java \
    $(JCC_DIR)/components/CodeAttribute.java \
    $(JCC_DIR)/components/CodeAttributeFactory.java \
    $(JCC_DIR)/components/ConstantObject.java \
    $(JCC_DIR)/components/ConstantPool.java \
    $(JCC_DIR)/components/ConstantValueAttribute.java \
    $(JCC_DIR)/components/ConstantValueAttributeFactory.java \
    $(JCC_DIR)/components/DoubleValueConstant.java \
    $(JCC_DIR)/components/ExceptionEntry.java \
    $(JCC_DIR)/components/ExceptionsAttribute.java \
    $(JCC_DIR)/components/ExceptionsAttributeFactory.java \
    $(JCC_DIR)/components/FMIrefConstant.java \
    $(JCC_DIR)/components/FieldConstant.java \
    $(JCC_DIR)/components/FieldInfo.java \
    $(JCC_DIR)/components/InterfaceConstant.java \
    $(JCC_DIR)/components/LineNumberTableAttribute.java \
    $(JCC_DIR)/components/LineNumberTableAttributeFactory.java \
    $(JCC_DIR)/components/LineNumberTableEntry.java \
    $(JCC_DIR)/components/LocalVariableTableAttribute.java \
    $(JCC_DIR)/components/LocalVariableTableAttributeFactory.java \
    $(JCC_DIR)/components/LocalVariableTableEntry.java \
    $(JCC_DIR)/components/MethodConstant.java \
    $(JCC_DIR)/components/MethodInfo.java \
    $(JCC_DIR)/components/NameAndTypeConstant.java \
    $(JCC_DIR)/components/SingleValueConstant.java \
    $(JCC_DIR)/components/SourceFileAttribute.java \
    $(JCC_DIR)/components/StackMapAttribute.java \
    $(JCC_DIR)/components/StackMapAttributeFactory.java \
    $(JCC_DIR)/components/StackMapFrame.java \
    $(JCC_DIR)/components/StringConstant.java \
    $(JCC_DIR)/components/UnicodeConstant.java \
    $(JCC_DIR)/components/UninterpretedAttribute.java \
    $(JCC_DIR)/components/UninterpretedAttributeFactory.java \
    $(JCC_DIR)/jcc/Const.java \
    $(JCC_DIR)/jcc/EVMConst.java \
    $(JCC_DIR)/jcc/MemberLoader.java \
    $(JCC_DIR)/jcc/OpcodeConst.java \
    $(JCC_DIR)/jcc/Str2ID.java \
    $(JCC_DIR)/jcc/Util.java \
    $(JCC_DIR)/runtime/CCodeWriter.java \
    $(JCC_DIR)/runtime/CLDC_HI_NativesWriter.java \
    $(JCC_DIR)/runtime/CoreImageWriter.java \
    $(JCC_DIR)/runtime/HeaderDump.java \
    $(JCC_DIR)/text/PI18n.java \
    $(JCC_DIR)/util/BufferedPrintStream.java \
    $(JCC_DIR)/util/ClassFile.java \
    $(JCC_DIR)/util/ClassFileConst.java \
    $(JCC_DIR)/util/ClassFileFinder.java \
    $(JCC_DIR)/util/ClassReader.java \
    $(JCC_DIR)/util/ClassnameFilter.java \
    $(JCC_DIR)/util/ClassnameFilterList.java \
    $(JCC_DIR)/util/DataFormatException.java \
    $(JCC_DIR)/util/EmptyEnumeration.java \
    $(JCC_DIR)/util/FileCompare.java \
    $(JCC_DIR)/util/LinkerUtil.java \
    $(JCC_DIR)/util/Localizer.java \
    $(JCC_DIR)/util/Set.java \
    $(JCC_DIR)/util/SignatureIterator.java \
    $(JCC_DIR)/vm/ArrayClassInfo.java \
    $(JCC_DIR)/vm/ClassClass.java \
    $(JCC_DIR)/vm/EVMClass.java \
    $(JCC_DIR)/vm/EVMClassFactory.java \
    $(JCC_DIR)/vm/EVMMethodInfo.java \
    $(JCC_DIR)/vm/InterfaceMethodFactory.java \
    $(JCC_DIR)/vm/InterfaceMethodTable.java \
    $(JCC_DIR)/vm/InterfaceVector.java \
    $(JCC_DIR)/vm/PrimitiveClassInfo.java \
    $(JCC_DIR)/vm/StringTable.java \
    $(JCC_DIR)/vm/VMClassFactory.java \
    $(JCC_DIR)/vm/VMClassMemberInfo.java \
    $(JCC_DIR)/vm/VMConst.java \
    $(JCC_DIR)/vm/VMMethodInfo.java

jcc.jar: $(JCC_SRC)
	@if test ! -d jcc; then \
		mkdir jcc; \
	fi
	@$(JAVAC) -classpath jcc -d jcc $?
	@cp -f $(JCC_DIR)/JCCMessage.properties jcc
	@rm -f $@
	@$(JAR) cfm $@ $(JCC_MF) -C jcc .
	@echo created $@

#----------------------------------------------------------------------
# buildtoo.jar -- misc build script that does the following
#                 [1] Generating jvmconfig.h
#                 [2] Generating FSImage.c for platforms that need a 
#                     simulated file system.
#----------------------------------------------------------------------

BUILDTOOL_DIR = $(WorkSpace)/src/tools/buildtool
BUILDTOOL_MF  = $(BUILDTOOL_DIR)/MANIFEST.MF
BUILDTOOL_SRC = $(wildcard $(BUILDTOOL_DIR)/*.java \
                           $(BUILDTOOL_DIR)/config/*.java \
                           $(BUILDTOOL_DIR)/mjpp/*.java \
                           $(BUILDTOOL_DIR)/makedep/*.java \
                           $(BUILDTOOL_DIR)/tests/*.java \
                           $(BUILDTOOL_DIR)/util/*.java)

buildtool.jar: $(BUILDTOOL_SRC)
	@if test ! -d buildtool; then \
		mkdir buildtool; \
	fi
	@$(JAVAC) -classpath buildtool -d buildtool $?
	@cp -f $(BUILDTOOL_DIR)/config/config_prolog.txt buildtool/config
	@rm -f $@
	@$(JAR) cfm $@ $(BUILDTOOL_MF) -C buildtool .
	@echo created $@

clean:
	-rm -rf makedeps jcc buildtool *.jar

#----------------------------------------------------------------------
#
# These built-in GNU-make rules for SCCS are troublesome, especially
# we don't have functioning SCCS on Windows. By declaring them here
# without a command, these built-in rules are cancelled.
#
#----------------------------------------------------------------------

%:: s.%

%:: SCCS/s.%

#----------------------------------------------------------------------
#
# Remove all these bogus default implicit rules that make building VERY
# slow over Samba.
#
#----------------------------------------------------------------------

%: %.o
%: %.c
%: %.cc
%: %.C
%: %.cpp
%: %.p
%: %.f
%: %.F
%: %.r
%: %.s
%: %.S
%: %.mod
%: %.sh
%:: %,v
%:: RCS/%,v
%:: RCS/%
