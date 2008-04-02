@REM   
@REM Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
@REM DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
@REM 
@REM This program is free software; you can redistribute it and/or
@REM modify it under the terms of the GNU General Public License version
@REM 2 only, as published by the Free Software Foundation.
@REM 
@REM This program is distributed in the hope that it will be useful, but
@REM WITHOUT ANY WARRANTY; without even the implied warranty of
@REM MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
@REM General Public License version 2 for more details (a copy is
@REM included at /legal/license.txt).
@REM 
@REM You should have received a copy of the GNU General Public License
@REM version 2 along with this work; if not, write to the Free Software
@REM Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
@REM 02110-1301 USA
@REM 
@REM Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
@REM Clara, CA 95054 or visit www.sun.com if you need additional
@REM information or have any questions.
@REM
@REM Sample batch file for setting the environment variables to build with
@REM Embedded Visual C++ 4.0
@REM
@REM You may need to change the pathnames according to your local
@REM settings. You should use DOS 8.3 pathnames to be compatible with
@REM Cygwin. Use the command "dir /x" to determine 8.3 pathnames.

set USE_VS2005=false
set EVC_COMMON_PATH=C:/PROGRA~1/MICROS~1.0/Common/EVC/bin

set EVC_I386_INCLUDE=C:/PROGRA~1/WIEB7A~1/wce420/POCKET~1/include/emulator
set EVC_I386_LIB=C:/PROGRA~1/WIEB7A~1/wce420/POCKET~1/lib/emulator
set EVC_I386_PATH=C:/PROGRA~1/MICROS~1.0/EVC/wce420/bin

set EVC_ARM_INCLUDE=C:/PROGRA~1/WIEB7A~1/wce420/POCKET~1/include/armv4
set EVC_ARM_LIB=C:/PROGRA~1/WIEB7A~1/wce420/POCKET~1/lib/armv4
set EVC_ARM_PATH=C:/PROGRA~1/MICROS~1.0/EVC/wce420/bin



