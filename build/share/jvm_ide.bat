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
@rem ======================================================================

echo ----------------------------------------------------------------------

if .%2 == .  (
    set BuildSpace=..
) ELSE (
    set BuildSpace=%2
)

echo BuildSpace=%BuildSpace%
set BuildRoot=%BuildSpace%\%JVM_OS%_%JVM_ARCH%_ide

if .%1 == .create (
    goto create
)
if .%1 == .clean (
    goto clean
)

:invalid
echo # Invalid arguments: "%*"
echo # Usage:        idetool create          create IDE files in current dir
echo #               idetool create [out]    create IDE files in [out] dir
echo # 
echo #               idetool clean           clean IDE files in current dir
echo #               idetool clean [out]     clean IDE files in [out] dir
echo # 
echo #               [Examples]
echo # 
echo #               idetool create
echo #               idetool clean
echo # 
echo #               idetool create C:\build_dir
echo #               idetool clean  C:\build_dir
goto jvmend

:clean
echo Removing all temporary and output files
rmdir /Q /S %BuildRoot%\buildtool      > NUL 2> NUL
rmdir /Q /S %BuildRoot%\javaapi        > NUL 2> NUL
rmdir /Q /S %BuildRoot%\loopgen        > NUL 2> NUL
rmdir /Q /S %BuildRoot%\loop           > NUL 2> NUL
rmdir /Q /S %BuildRoot%\romgen         > NUL 2> NUL
rmdir /Q /S %BuildRoot%\romimage       > NUL 2> NUL
rmdir /Q /S %BuildRoot%\cldc_vm        > NUL 2> NUL
rmdir /Q /S %BuildRoot%\Debug          > NUL 2> NUL
rmdir /Q /S %BuildRoot%\Release        > NUL 2> NUL
rmdir /Q /S %BuildRoot%\Product        > NUL 2> NUL
del /Q %BuildRoot%\platform.cfg        > NUL 2> NUL
del /Q %BuildRoot%\*.dsp               > NUL 2> NUL
del /Q %BuildRoot%\*.dsw               > NUL 2> NUL
del /Q %BuildRoot%\*.ncb               > NUL 2> NUL
del /Q %BuildRoot%\*.opt               > NUL 2> NUL
del /Q %BuildRoot%\*~                  > NUL 2> NUL
goto jvmend

:create
set WorkSpace=..\..
set BuildToolSrcDir=%WorkSpace%\src\tools\buildtool

echo Creating project files in with BuildSpace=%BuildSpace%

@rem ======================================================================
@rem Create the output directories if necessary
@rem ======================================================================
if NOT EXIST %BuildSpace% (
   echo mkdir %BuildSpace%
   mkdir %BuildSpace%
)

if NOT EXIST %BuildRoot% (
   echo mkdir %BuildRoot%
   mkdir %BuildRoot%
)

set BuildToolDir=%BuildRoot%\buildtool

if NOT EXIST %BuildToolDir% (
  echo mkdir %BuildToolDir%
  mkdir %BuildToolDir%
)

@rem ======================================================================
@rem Delete old output files if they exist -- they would confuse VC++ in
@rem running the C compiler in batch mode.
@rem ======================================================================

rmdir /S /Q %BuildRoot%\i386loop\Debug       > NUL 2> NUL
rmdir /S /Q %BuildRoot%\i386loop\Release     > NUL 2> NUL
rmdir /S /Q %BuildRoot%\romgen\Debug         > NUL 2> NUL
rmdir /S /Q %BuildRoot%\romgen\Release       > NUL 2> NUL
rmdir /S /Q %BuildRoot%\cldc_vm\Debug        > NUL 2> NUL
rmdir /S /Q %BuildRoot%\cldc_vm\Release      > NUL 2> NUL

@rem ======================================================================
@rem Compile the BuildTool files
@rem ======================================================================
set BuildToolSrcs=%BuildToolSrcDir%\*.java
set BuildToolSrcs=%BuildToolSrcs% %BuildToolSrcDir%\config\*.java
set BuildToolSrcs=%BuildToolSrcs% %BuildToolSrcDir%\makedep\*.java
set BuildToolSrcs=%BuildToolSrcs% %BuildToolSrcDir%\mjpp\*.java
set BuildToolSrcs=%BuildToolSrcs% %BuildToolSrcDir%\util\*.java
set BuildToolSrcs=%BuildToolSrcs% %BuildToolSrcDir%\tests\*.java

echo javac -d %BuildToolDir% %BuildToolSrcs%
javac -d %BuildToolDir% %BuildToolSrcs%
IF %ERRORLEVEL% NEQ 0 goto jvmend

copy %BuildToolSrcDir%\config\config_prolog.txt %BuildToolDir%\config

@rem ======================================================================
@rem create platform.cfg - platform configuration file
@rem ======================================================================
echo os_family= %JVM_OS%             > %BuildRoot%\platform.cfg
echo arch     = %JVM_ARCH%          >> %BuildRoot%\platform.cfg
echo os_arch  = %JVM_OS%_%JVM_ARCH% >> %BuildRoot%\platform.cfg
echo compiler = %JVM_COMPILER%      >> %BuildRoot%\platform.cfg

@rem ======================================================================
@rem create product.cfg - product configuration file
@rem ======================================================================
set ProductFile=%WorkSpace%/build/share/product.make

if exist %ProductFile% goto product
set ProductFile=%WorkSpace%/build/share/jvm.make
:product

find "PRODUCT_NAME " < %ProductFile% | find "=" > %BuildRoot%\product.cfg
find "RELEASE_VERSION " < %ProductFile% | find "=" >> %BuildRoot%\product.cfg

@rem ======================================================================
@rem Execute MakeDeps
@rem ======================================================================
set CMDS=java -classpath %BuildToolDir% %JVM_IDETOOL_MAIN%
set CMDS=%CMDS% -product %BuildRoot%\product.cfg
set CMDS=%CMDS% -platform %BuildRoot%\platform.cfg
set CMDS=%CMDS% -workspace %WorkSpace%
set CMDS=%CMDS% -buildspace %BuildSpace%
set CMDS=%CMDS% -database %WorkSpace%\src\vm\includeDB

echo %CMDS%
%CMDS%
IF %ERRORLEVEL% NEQ 0 goto jvmend

echo project file created as %BuildRoot%\cldc_vm.dsw
echo type start %BuildRoot%\cldc_vm.dsw to open the project file.

:jvmend
echo ----------------------------------------------------------------------
