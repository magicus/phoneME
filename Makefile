# 
# Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
#

all: sanity jarfile

JAVA          = $(JDK_DIR)/bin/java
JAVAC         = $(JDK_DIR)/bin/javac
JAR           = $(JDK_DIR)/bin/jar
JAVAFILES =  $(shell find src -name "*.java"|grep -v SCCS)
 
CLASSFILES = $(subst src,classes,$(JAVAFILES:java=class))

ifeq ($(DEBUG),true)
DEBUGFLAG=
else
DEBUGFLAG=":none"
endif

# $< is dependency
# $@ is target
#
$(CLASSFILES): classes/%.class : src/%.java
	@echo $< >> .filelist

eraselists:
	@rm -f .filelist 

compilefiles:
	@if [ '!' -d classes ]; then rm -rf classes; mkdir classes; fi;
	@if [ -f .filelist ]; then \
		echo $(JAVAC) -g$(DEBUGFLAG) `cat .filelist`; \
		$(JAVAC) -g$(DEBUGFLAG) -d classes -classpath classes \
	              `cat .filelist`; \
		fi

tools: eraselists $(CLASSFILES) compilefiles

jarfile: tools
	@rm -rf kdp.jar
	@$(JAR) cfM0 kdp.jar -C classes .

sanity:
	@if test ! -f $(JDK_DIR)/jre/lib/rt.jar; then \
	    echo '==========================================================';\
	    echo 'JDK_DIR must be set. I.e., set it such that you can access';\
	    echo 'javac as $$(JDK_DIR)/bin/javac'; \
	    echo Note: forward slash / must be used on Win32; \
	    echo '==========================================================';\
	    exit -1; \
	fi
clean:
	rm -rf *.zip
	rm -rf .filelist
	rm -rf classes
	rm -rf *~ */*~ */*/*~
	rm -rf *# */*# */*/*#

