@echo off
@rem ======================================================================
@rem SCCS ID   
@rem
@rem Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
@rem DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
@rem 
@rem This program is free software; you can redistribute it and/or
@rem modify it under the terms of the GNU General Public License version
@rem 2 only, as published by the Free Software Foundation.
@rem 
@rem This program is distributed in the hope that it will be useful, but
@rem WITHOUT ANY WARRANTY; without even the implied warranty of
@rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
@rem General Public License version 2 for more details (a copy is
@rem included at /legal/license.txt).
@rem 
@rem You should have received a copy of the GNU General Public License
@rem version 2 along with this work; if not, write to the Free Software
@rem Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
@rem 02110-1301 USA
@rem 
@rem Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
@rem Clara, CA 95054 or visit www.sun.com if you need additional
@rem information or have any questions.
@rem
@rem idetool.bat - Tool for creating IDE build files for the win32_i386 
@rem               platforms.
@rem
@rem               Runs on Windows NT/2000/XP only. Windows 9x/ME are NOT
@rem               supported.
@rem
@rem               JDK 1.3 or later is required. The programs java.exe
@rem               and javac.exe must be in your PATH.
@rem
@rem Usage:        idetool create          [create IDE files in current dir]
@rem               idetool create <out>    [create IDE files in <out> dir]
@rem
@rem               idetool clean           [clean IDE files in current dir]
@rem               idetool clean <out>     [clean IDE files in <out> dir]
@rem
@rem               [Examples]
@rem
@rem               idetool create
@rem               idetool clean
@rem
@rem               idetool create C:\build_dir
@rem               idetool clean  C:\build_dir
@rem
@rem Notes:        To debug this file, remove the 'echo off' line at the
@rem               top of this file.
@rem ======================================================================

if %OS%. == Windows_NT. goto :nt

echo Error: Windows NT/2000/XP command processor required
goto end

:nt

@rem ======================================================================
@rem Make all variables definitions local in this file, so it won't
@rem affect the user's environment
@rem ======================================================================
SETLOCAL

set JVM_IDETOOL_MAIN=makedep.Win32IDETool
set JVM_OS=win32
set JVM_ARCH=i386
set JVM_COMPILER=visCPP

@rem ======================================================================
@rem jvm_ide.bat contains the shared code for creating IDE project files
@rem for different IDE environments.
@rem ======================================================================

call ..\share\jvm_ide.bat %*
ENDLOCAL

:end
