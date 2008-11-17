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
# @(#)Makefile	1.10 06/10/25
#
#

# Top Level Makefile

CVM_OPTIMIZED=false
CVM_PRELOAD_LIB=false
CVM_DEBUG=false
CVM_JVMTI=false
CVM_JVMPI=false
CVM_JIT=false
J2ME_CLASSLIB=cdc

include Defs.mk

FILES=BarrierTest
CLASS_FILES=$(FILES:%=Class/%.class)

all : build run results

compile: $(CLASS_FILES) 

Class/%.class: Jsrc/%.java
	@mkdir -p ./Class
	javac -d Class Jsrc/*.java
	javah -d Include -classpath Class -jni $(FILES)
	@echo ""

build: compile
	@echo "..... Building CVM"
	(cd $(CVM_PREFIX)/build/$(CVM_TARGET); $(MAKE) JDK_HOME=$(JDK_HOME) CVM_TEST_GENERATION_GC=true CVM_DEBUG=$(CVM_DEBUG) CVM_OPTIMIZED=$(CVM_OPTIMIZED) CVM_PRELOAD_LIB=$(CVM_PRELOAD_LIB) CVM_JVMTI=$(CVM_JVMTI) CVM_JVMPI=$(CVM_JVMPI) CVM_JIT=$(CVM_JIT) J2ME_CLASSLIB=$(J2ME_CLASSLIB))

run:
	@cp Defs.mk RULES
	@chmod +x run.sh
	@./run.sh $(CVM_TARGET) 2>&1 | tee run.$(CVM_TARGET).log

results:
	@chmod +x results.sh
	@./results.sh $(CVM_TARGET) 2>&1 | tee results.$(CVM_TARGET).log

clean:
	@rm -f *.log
	@rm -rf Class Include
	@echo "..... Cleaning CVM"
	(cd $(CVM_PREFIX)/build/$(CVM_TARGET); $(MAKE) clean)
