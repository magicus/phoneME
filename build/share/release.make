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

#----------------------------------------------------------------------
#
# source bundle creation
#
#----------------------------------------------------------------------

# Default filter rule for source release :
# Copy files from specified sub directories that are under svn controlled.
# Usage: re_src_filter(src_dir, output_dir, sub_dir_list)
define re_src_filter
	if [ -d "$(1)/.svn" ]; then \
		(for i in $(3); do mkdir -p $(2)/$$i; svn export --force -q $(1)/$$i $(2)/$$i; done); \
	else \
		mkdir -p $(2); \
		(for i in $(3); do cp -rp $(1)/$$i $(2)/; done); \
	fi
endef


# Verify source output directory is set
source_bundle_verify:
	$(A)if test ! -d $(JVMWorkSpace)/src; then \
	    echo '==========================================================';\
	    echo 'JVMWorkSpace must be set. See build documentation';\
	    echo '==========================================================';\
	    exit -1; \
	fi;
	$(A)if [ -z "$(SOURCE_OUTPUT_DIR)" ]; then \
	    echo '==========================================================';\
	    echo 'SOURCE_OUTPUT_DIR must be set.';\
	    echo '==========================================================';\
	    exit -1; \
	fi;

CLDC_SOURCE_OUTPUT_DIR=$(SOURCE_OUTPUT_DIR)/cldc

# Copy and filter JPEG source in JPEG_SOURCE_OUTPUT_DIR
source_bundle: source_bundle_verify
	@echo " ... CLDC VM source bundle" $(LOG)
	@$(call re_src_filter,$(JVMWorkSpace),$(CLDC_SOURCE_OUTPUT_DIR),src build)
