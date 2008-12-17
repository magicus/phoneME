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
# This makefile provide rules to trigger generation of main library makefiles 
# library.make and to generate the global list of dependencies of main library 
# makefiles on corresponding library destriptors libexport.gmk and 
# implementation descriptors libinfo.gmk.
#
# This makefile also does sanity check for the configuration file:
# if verifies that a valid implementation is specified for each library.
#
######################################################################

ifndef _gendeps_gmk_
_gendeps_gmk_ = defined

include $(JWC_WORK_SPACE)/build/share/libdefs.gmk

CONFIG_FILE=$(JWC_WORK_SPACE)/build/$(JWC_CONFIG)/config.gmk

include $(CONFIG_FILE)

#
# Environment variables passed to nested make
#
SUBMAKE_ENV = JWC_WORK_SPACE=$(JWC_WORK_SPACE) \
              JWC_BUILD_SPACE=$(JWC_BUILD_SPACE)

#
# The file with global dependencies list to be generated
#
GLOBAL_DEPS=$(JWC_BUILD_SPACE)/libdeps.make

#
# The rule to verify correctness of the selected configuration 
# and generate library.make
#
$(JWC_BUILD_SPACE)/%/library.make: $(GLOBAL_DEPS) $(CONFIG_FILE) 
	$(A)if test ! -d $(JWC_WORK_SPACE)/src/$* ; then  \
	    echo '======================================================'; \
	    echo 'Internal error. Unexpected target: $@'; \
	    echo '======================================================'; \
	    exit -1; \
	fi
	$(A)if test -z $($*) ; then  \
	    echo '======================================================'; \
	    echo 'Implementation not specified for library: $*.'; \
	    echo 'Problematic config file: $(CONFIG_FILE).'; \
	    echo 'See build documentation.'; \
	    echo '======================================================'; \
	    exit -1; \
	fi
	$(A)if test ! -f $(JWC_WORK_SPACE)/src/$*/libexport.gmk ; then  \
	    echo '======================================================'; \
	    echo 'Cannot find exported interface settings for library $*'; \
	    echo 'File not found: $(JWC_WORK_SPACE)/src/$*/libexport.gmk.'; \
	    echo 'See build documentation.'; \
	    echo '======================================================'; \
	    exit -1; \
	fi
	$(A)if test ! -f $(JWC_WORK_SPACE)/src/$*/$($*)/libinfo.gmk ; then  \
	    echo '======================================================'; \
	    echo 'Incorrect or missing implementation: $($*)'; \
	    echo 'specified for library: $*.'; \
	    echo 'File not found: $(JWC_WORK_SPACE)/src/$*/$($*)/libinfo.gmk.'; \
	    echo 'Problematic config file: $(CONFIG_FILE).'; \
	    echo 'See build documentation.'; \
	    echo '======================================================'; \
	    exit -1; \
	fi
	$(A)$(MAKE) $(SUBMAKE_ENV) LIBRARY=$* LIB_IMPL=$($*) \
              -f $(JWC_WORK_SPACE)/build/share/genlibrary.make

#
# List of dependencies for each library.make
# on libexport.gmk and libinfo.gmk from the corresponding library.
#
$(GLOBAL_DEPS): $(CONFIG_FILE)
	$(A)echo ... Generating global dependency list
	$(A)for i in `find $(JWC_WORK_SPACE)/src -name libexport.gmk`; do \
            export __JWC_SRC_SPACED="`echo $(JWC_WORK_SPACE)/src/ | tr "/" " "`"; \
            export __JWC_LIB=`echo $$i | tr "/" " " | \
               sed -e "s/$$__JWC_SRC_SPACED//g" | \
               sed -e "s/ libexport.gmk//g" | \
               sed -e "s/ /\//g"`; \
            unset __JWC_SRC_SPACED; \
            export __JWC_LIB_SRC=$(JWC_WORK_SPACE)/src/$$__JWC_LIB; \
            echo "" >> $(GLOBAL_DEPS); \
            echo "$(JWC_BUILD_SPACE)/$$__JWC_LIB/library.make: $(NL)" >> $(GLOBAL_DEPS); \
            echo "   $$__JWC_LIB_SRC/libexport.gmk $(NL)" >> $(GLOBAL_DEPS); \
            echo "   $$__JWC_LIB_SRC/\$$($$__JWC_LIB)/libinfo.gmk" >> $(GLOBAL_DEPS); \
            echo "" >> $(GLOBAL_DEPS); \
            echo "# Empty rule to avoid immediate failure if an " >> $(GLOBAL_DEPS); \
            echo "# incorrect implementation specified for this library" >> $(GLOBAL_DEPS); \
            echo "$$__JWC_LIB_SRC/\$$($$__JWC_LIB)/libinfo.gmk:" >> $(GLOBAL_DEPS); \
            echo "" >> $(GLOBAL_DEPS); \
            unset __JWC_LIB; \
            unset __JWC_LIB_SRC; \
        done
ifdef VERBOSE_BUILD
	$(A)echo '=========================================================='
	$(A)cat $@
	$(A)echo '=========================================================='
endif

-include $(GLOBAL_DEPS)

endif
