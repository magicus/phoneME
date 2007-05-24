@rem 	
@rem
@rem Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

@echo off
setlocal

%~d0
set CURR_DIR=%~p0bin
goto :start_run

:go_inside
set CURR_DIR=%CURR_DIR%\i386
chdir %CURR_DIR% > nul
if ERRORLEVEL 1 goto :go_err
goto :run_it

:go_err
echo '%CURR_DIR%' does not exist.
goto :EOF

:start_run

chdir %CURR_DIR% > nul
if ERRORLEVEL 1 goto :go_err

rem
rem The following code will be generated during the build.
rem

rem dir usertest_g.bat > nul
rem echo %ERRORLEVEL%
rem if ERRORLEVEL 1 goto :go_inside
rem goto :run_it

rem :run_it
rem usertest_g.bat
rem goto :EOF

rem endlocal

