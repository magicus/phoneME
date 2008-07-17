#!/bin/bash
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

make \
  USE_VERBOSE_MAKE=true \
  USE_VS2005=true \
  CVM_COMPILER_INCOMPATIBLE=false \
  CVM_JIT=true \
  CVM_OPTIMIZED=true \
  CVM_SYMBOLS=true \
  CVM_PRELOAD_LIB=true \
  USE_JPEG=true \
    JPEG_DIR=$COMPONENTS_DIR/jpeg \
  USE_MIDP=true \
  CVM_INCLUDE_JAVACALL=false \
  PCSL_TARGET=wince_arm  \
  J2ME_CLASSLIB=foundation \
  CVM_BUILD_SUBDIR_NAME=opt_preload_jit \
  $@
