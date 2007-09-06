#
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
######################################################################
#
# Top level shared makefile for all libraries.
#
# Following variables should have been set before including this file:
#  JWC_WORK_SPACE  - top of JWC workspace
#  JWC_BUILD_SPACE - directory where all results should be in
#  JWC_CONFIG      - name of JWC configuration
#  PCSL_DIST_DIR   - PCSL distribution for a particular platform
#  CLDC_DIST_DIR   - the VM distribution for a particular platform
#  JDK_DIR         - Path to the Java SDK
#  SIGTEST_JAR     - Path to sigtestdev.jar
#
# Additional variable is required until the decoupling is complete:
#  MIDP_OUTPUT_DIR - Path to existing JWC build
#
# This makefile performs sanity check for all the variables above,
# derives the name of the library from CURDIR, generates 
# the root makefile for this library and passes build targets to it.
#
######################################################################

.PHONY: all clean sanity

# 
# Derive the library name from CURDIR.
# Library name is a relative path to the library source root from $(JWC_WORK_SPACE)/src
# We use pwd to have the same representation of JWC_WORK_SPACE and current dir.
#
export LIBRARY=$(shell echo \
   $(wordlist \
        $(words $(shell cd $(JWC_WORK_SPACE); pwd | tr "/" " ") root src), \
        $(words $(shell pwd | tr "/" " ")), \
        $(shell pwd | tr "/" " ") \
    ) | tr " " "/")

#
# Destination path for all items built for this library
#
LIB_BUILD_SPACE=$(JWC_BUILD_SPACE)/$(LIBRARY)

#
# Root makefile for this library
#
LIB_ROOT_MAKEFILE=$(LIB_BUILD_SPACE)/root.make

#
# Main makefile for this library
#
LIB_MAKEFILE=$(LIB_BUILD_SPACE)/library.make

#
# Pass the targets to the root makefile for this library
#
all clean: sanity $(LIB_ROOT_MAKEFILE)
	@$(MAKE) -f $(LIB_ROOT_MAKEFILE) $@

#
# Rule to generate the root makefile for this library
#
$(LIB_ROOT_MAKEFILE):
	@mkdir -p $(shell dirname $@)
	@echo ... Making $@
	@echo ".PHONY: all clean" > $@
	@echo "" >> $@
	@echo "all clean: $(LIB_MAKEFILE)" >> $@
	@echo "	$(MAKE) -f $(LIB_MAKEFILE) $(LIBRARY)_\$$@" >> $@
	@echo "" >> $@
	@echo "include $(JWC_WORK_SPACE)/build/share/gendeps.make" >> $@
	@echo "" >> $@
	@echo '=========================================================='
	@cat $@
	@echo '=========================================================='

#
# Sanity check for all the required environment variables
#
sanity: 
	@if test ! -f $(PCSL_DIST_DIR)/inc/java_types.h; then \
	    echo '==========================================================';\
	    echo 'PCSL_DIST_DIR must be set, such that you can access';\
	    echo '$$(PCSL_DIST_DIR)/inc/java_types.h';\
	    echo 'See build documentation';\
	    echo '==========================================================';\
	    exit -1; \
	fi
	@if test ! -f $(CLDC_DIST_DIR)/lib/cldc_classes.zip; then \
	    echo '==========================================================';\
	    echo 'CLDC_DIST_DIR must be set, such that you can access';\
	    echo '$$(CLDC_DIST_DIR)/lib/cldc_classes.zip';\
	    echo 'See build documentation';\
	    echo '==========================================================';\
	    exit -1; \
	fi
	@if test ! -f $(MIDP_CLASSES_ZIP); then \
	    echo '==========================================================';\
	    echo 'MIDP_OUTPUT_DIR must be set, such that you can access';\
	    echo '$$(MIDP_CLASSES_ZIP)';\
	    echo 'See build documentation';\
	    echo '==========================================================';\
	    exit -1; \
	fi
	@if test ! -f $(JDK_DIR)/jre/lib/rt.jar; then \
	    echo '==========================================================';\
	    echo 'JDK_DIR must be set, such that you can access';\
	    echo 'javac as $$(JDK_DIR)/bin/javac'; \
	    echo '==========================================================';\
	    exit -1; \
	fi
	@if test ! -d $(JWC_WORK_SPACE)/src/core; then \
	    echo '==========================================================';\
	    echo 'JWC_WORK_SPACE must be set, such that you can access';\
	    echo '$$(JWC_WORK_SPACE)/src/core';\
	    echo 'See build documentation';\
	    echo '==========================================================';\
	    exit -1; \
	fi
	@if test ! -f $(JWC_WORK_SPACE)/build/$(JWC_CONFIG)/config.gmk; then \
	    echo '==========================================================';\
	    echo 'JWC_CONFIG must be set, such that you can access';\
	    echo '$$(JWC_WORK_SPACE)/build/$$(JWC_CONFIG)/config.gmk';\
	    echo 'See build documentation';\
	    echo '==========================================================';\
	    exit -1; \
	fi
	@if test -z $(JWC_BUILD_SPACE) ; then \
	    echo '==========================================================';\
	    echo 'JWC_BUILD_SPACE must be set. See build documentation';\
	    echo '==========================================================';\
	    exit -1; \
	fi
	@if test -z $(SIGTEST_JAR) || test ! -f $(SIGTEST_JAR); then \
	    echo '==========================================================';\
	    echo 'SIGTEST_JAR must be set. See build documentation';\
	    echo '==========================================================';\
	    exit -1; \
	fi
