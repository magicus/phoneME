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
# @(#)Makefile	1.8 06/10/10
#
#

include ../Defs.mk

FILES=RootScansTest
CLASS_FILES=$(FILES:%=Class/%.class)

Class/%.class: Jsrc/%.java
	$(JDK_HOME)/bin/javac -d Class -classpath Class $<
	$(JDK_HOME)/bin/javah -d Include -classpath Class -jni $(FILES)
	@echo ""

all: run results

compile: $(CLASS_FILES)

build:
	@chmod +x build.sh
	@./build.sh 2>&1 | tee build.log

run:
	@chmod +x run.sh
	@./run.sh 2>&1 | tee run.$(CVM_TARGET).log

results:
	@chmod +x results.sh
	@./results.sh $(CVM_TARGET) 2>&1 | tee results.$(CVM_TARGET).log

clean:
	rm -f Class/* Include/* *.log
