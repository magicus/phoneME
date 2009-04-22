/*
* Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version
* 2 only, as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License version 2 for more details (a copy is
* included at /legal/license.txt).
*
* You should have received a copy of the GNU General Public License
* version 2 along with this work; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
* Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
* Clara, CA 95054 or visit www.sun.com if you need additional
* information or have any questions.
*/

#pragma once

typedef unsigned __int16 bits16;
typedef unsigned __int32 bits32;
typedef unsigned __int64 bits64;
typedef unsigned __int8  bits8 ;

#ifdef _NATIVE_WCHAR_T_DEFINED
    typedef wchar_t char16;
#else
    typedef unsigned __int16 char16;
#endif

typedef char char8;

typedef signed __int16 int16;
typedef signed __int32 int32;
typedef signed __int64 int64;
typedef signed __int8  int8 ;

typedef unsigned __int16 nat16; // Natural numbers include 0
typedef unsigned __int32 nat32;
typedef unsigned __int64 nat64;
typedef unsigned __int8  nat8 ;

typedef float  real32;
typedef double real64;
