#
# @(#)defs_zoneinfo.mk	1.8 06/10/10
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

#
# Makefile for building javazic utility & TimeZone resource files
#

ZONEINFO_CLASSES = \
	sun/tools/MyClassPath.java \
	sun/tools/javazic/BackEnd.java \
	sun/tools/javazic/Calendar.java \
	sun/tools/javazic/Checksum.java \
	sun/tools/javazic/GenDoc.java \
	sun/tools/javazic/Gen.java \
	sun/tools/javazic/GenSrc.java \
	sun/tools/javazic/Main.java \
	sun/tools/javazic/Mappings.java \
	sun/tools/javazic/Month.java \
	sun/tools/javazic/RuleDay.java \
	sun/tools/javazic/Rule.java \
	sun/tools/javazic/RuleRec.java \
	sun/tools/javazic/Simple.java \
	sun/tools/javazic/Time.java \
	sun/tools/javazic/Timezone.java \
	sun/tools/javazic/Zoneinfo.java \
	sun/tools/javazic/Zone.java \
	sun/tools/javazic/ZoneRec.java \
	sun/util/calendar/CalendarDate.java \
	sun/util/calendar/CalendarSystem.java \
	sun/util/calendar/Gregorian.java \
	sun/util/calendar/ZoneInfoFile.java \
	sun/util/calendar/ZoneInfo.java \

ZONEINFO_CLASSES_DIR = $(CVM_BUILD_TOP)/zic_classes
TZDATA = $(CVM_SHAREROOT)/tools/javazic/tzdata/
TZDATA_VER = `cat $(TZDATA)VERSION`

# This is for full timezone support all timezones.  This isn't
# ideal for J2ME as this is about 240k.
#TZFILE = \
#    africa antarctica asia australasia europe northamerica \
#    pacificnew southamerica systemv backward \
#    etcetera solar87 solar88 solar89 systemv

# We are restricting timezone support to just certain timezones
# as samples for J2ME CDC/Foundation Profile reference, 
#  America/Los_Angeles, Asia/Calcutta and Asia/Novosibirsk.  
#  This is to save space for J2ME builds.
TZFILE = j2meref
JDKTZDATA = $(CVM_SHAREROOT)/tools/javazic/tzdata_jdk/
JDKTZFILES = gmt jdk11_backward
TZFILES = \
    $(addprefix $(TZDATA),$(TZFILE)) \
    $(addprefix $(JDKTZDATA),$(JDKTZFILES))

FILES_class = $(ZONEINFO_CLASSES:%.java=$(ZONEINFO_CLASSES_DIR)/%.class)
ZONEINFO_WORKDIR = $(ZONEINFO_CLASSES_DIR)/zi
ZONEINFO_INSTALLDIR = $(CVM_LIBDIR)/zi
MAPFILE = ZoneInfoMappings

