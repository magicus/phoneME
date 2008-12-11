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
# @(#)rules.mk	1.16 06/10/10
#
# rules for VxWorks target
#

#
# Rules for building bsp, boot disk image and downloadable image.
#

download_image: $(TORNADO2DIR)/default/vxWorks

download_image_clean:
	@echo ">>>Making "$@" ..." ;
	(rm -rf $(TORNADO2DIR)/default)
	(rm -f $(TORNADO2DIR)/*.lst)
	@echo "<<<Finished "$@" ..." ;

boot_disk:
	@echo ">>>Making "$@" ..." ;
	-volcheck
	cp $(WIND_BASE)/FLOPPY-BOOT/boot-disk.14 $(DISK_DEVICE)
	eject
	@echo "<<<Finished "$@" ..." ;

boot_rom: bsp_file
	@echo ">>>Making "$@" ..." ;
	(cd $(TORNADO2DIR)-boot; \
	$(MAKE) bootrom; \
	objcopy386 -O binary bootrom bootrom.sys; \
	volcheck; \
	cp bootrom.sys $(MOUNTED_DISK); \
	eject)
	@echo "<<<Finished "$@" ..." ;



bsp_file:: $(TORNADO2DIR)-boot/vxWorks

bsp_clean:: download_image_clean
	@echo ">>>Making "$@" ..." ;
	(cd $(TORNADO2DIR)-boot; $(MAKE) clean)
	@echo "<<<Finished "$@" ..." ;

default_dir:: $(TORNADO2DIR)/default



$(TORNADO2DIR)-boot/vxWorks::
	@echo ">>>Making "$@" ..." ;
	(cd $(TORNADO2DIR)-boot; $(MAKE))
	@echo "<<<Finished "$@" ..." ;

$(TORNADO2DIR)/default::
	mkdir -p $(TORNADO2DIR)/default

$(TORNADO2DIR)/default/vxWorks:: all bsp_file default_dir
	@echo ">>>Making "$@" ..." ;
	(cd $(TORNADO2DIR)/default; \
	$(MAKE) -f ../Makefile BUILD_SPEC=default DEFAULT_RULE=vxWorks-x86 CVM_TARGET=$(CVM_TARGET) vxWorks)
	@echo "<<<Finished "$@" ..." ;
