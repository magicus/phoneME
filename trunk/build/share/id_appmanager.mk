#
# Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 only,
# as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# version 2 for more details (a copy is included at /legal/license.txt).
# 
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
# CA 95054 or visit www.sun.com if you need additional information or have
# any questions.
#


include ../share/id_$(J2ME_CLASSLIB).mk

J2ME_PRODUCT_NAME       :=  CDC AMS 1.0
ifeq ($(J2ME_CLASSLIB), personal)
  J2ME_PRODUCT_NAME+= for PP
endif
ifeq ($(J2ME_CLASSLIB), basis)
  J2ME_PRODUCT_NAME+= for PBP
endif

J2ME_BUILD_VERSION	=
CVM_DONT_ADD_BUILD_ID   = true

J2ME_BUILD_STATUS	= beta
J2ME_BUILD_ID 		= $(CVM_BUILD_ID)

