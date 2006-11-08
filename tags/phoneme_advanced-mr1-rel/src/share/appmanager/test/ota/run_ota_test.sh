#!/bin/sh

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
# A script to invoke the OtaTest class.
#
${DBG_SH}

[ -z "$BUILDFLAVOR" ] && BUILDFLAVOR=linux-x86-suse
# Uncomment CDCAMS_HOME and set it appropriately
AM_HOME=${CDCAMS_HOME}/src/share/appmanager
# If you built in a subdir, specify it here
#SUBDIR=<subdir_value>

[ -z "$CDCAMS_HOME" ] && echo "CDCAMS_HOME not set!"

CP=${CDCAMS_HOME}/build/${BUILDFLAVOR}
[ -n "$SUBDIR" ] && CP=${CP}/${SUBDIR}

# Uncomment to set the discovery URL
#DISCOVERY_URL="-Dcom.sun.appmanager.ota.discoveryUrl=http://hostname:8000/ri-test/oma"
# Uncomment to set the output file
#DOWNLOAD_OUTPUT="-Dcom.sun.appmanager.ota.outputFile=/tmp/jarfile"

$CP/bin/cvm \
    -Djava.class.path=$CP:$CP/lib/appmanager.jar:$CP/lib/j2me_xml_cdc.jar:. \
    ${DISCOVERY_URL} \
    ${DOWNLOAD_OUTPUT} \
    OtaTest $*

exit $?

