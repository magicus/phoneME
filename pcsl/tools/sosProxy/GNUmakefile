#####################################################################
# 	
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
#####################################################################

TOOLS_OUTPUT_DIR = $(PCSL_OUTPUT_DIR)/tools
JAR_OUTPUT_FILE = $(TOOLS_OUTPUT_DIR)/sosProxy.jar
TEMP_CLASS_DIR = /tmp/sosProxy

JAVA_FILES = CommPort.java \
             SoSserver.java

JAVAC_CMD    = $(JDK_DIR)/bin/javac$(EXE)
JAR_CMD    = $(JDK_DIR)/bin/jar$(EXE)

ifeq ($(USE_DEBUG), true)
    JAVAC_DEBUG = -g
else
    JAVAC_DEBUG =
endif

CLASS_FILES=$(JAVA_FILES:.java=.class)

all : $(JAR_OUTPUT_FILE)

$(JAR_OUTPUT_FILE) : $(JAVA_FILES)
	@mkdir -p $(TEMP_CLASS_DIR) $(LOG)
	$(JAVAC_CMD) $(JAVAC_DEBUG) -classpath $(CURDIR):$(JDK_DIR)/../commapi/comm.jar:$(JDK_DIR)/jre/lib/ext/jcl.jar -d $(TEMP_CLASS_DIR) $(JAVA_FILES)
	@mkdir -p $(TOOLS_OUTPUT_DIR) $(LOG)
	cd $(TEMP_CLASS_DIR); $(JAR_CMD) -cmf $(CURDIR)/Manifest.mf $(JAR_OUTPUT_FILE) $(CLASS_FILES)


clean::
	rm -rf $(TEMP_CLASS_DIR)
	rm -f $(JAR_OUTPUT_FILE)

