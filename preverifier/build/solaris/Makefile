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

TOP=../..
include $(TOP)/build/Makefile.inc

SRC_DIR = ../../src

OBJS = \
	check_class.o \
	main.o \
	utf.o \
	check_code.o \
        convert_md.o \
	util.o \
	jar.o \
        jar_support.o \
	classloader.o \
	file.o \
	classresolver.o \
	stubs.o \
	inlinejsr.o \
	sys_support.o

CFLAGS = \
	-I$(SRC_DIR) \
	-DUNIX -DSOLARIS2 \
	-DJAVAVERIFY \
	-DTRIMMED \
	-D$(ARCH) $(EXTRACFLAGS)

ifeq ($(GCC), true)
   CC = gcc
   LD = gcc
   CFLAGS +=  -Wall $(CPPFLAGS) $(OTHER_FLAGS)
   DEBUG_FLAG =  
   LDFLAGS =
   
ifeq ($(DEBUG), true)
   DEBUG_FLAG = -g 
endif   

   OPTIMIZE_FLAG = -O2

else 

   CC = cc
   LD = cc
   CFLAGS += -Xa $(CPPFLAGS) $(OTHER_FLAGS)
   DEBUG_FLAG =
   LDFLAGS =

ifeq ($(DEBUG), true)
   DEBUG_FLAG = -g -xsb 
endif   

   OPTIMIZE_FLAG = -xO2
endif


all: preverify

preverify: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@ $(LIBS)

%.o: $(SRC_DIR)/%.c
	$(CC) $(EXTRACFLAGS) $(CFLAGS) $(XWINFLAGS) $(OPTIMIZE_FLAG) $(DEBUG_FLAG) -c -o $@ $<

clean: 
	rm -rf core *.o preverify SunWS_cache .sb
