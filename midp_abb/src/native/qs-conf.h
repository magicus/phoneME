/*
* Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#ifdef WIN32
// For Windows
#define ENV_GM_SOUNDS "/drivers/gm.dls"
typedef UINT (WINAPI * LP_GETSYSTEMDIRECTORY)(LPTSTR lpBuffer, UINT uSize);

#endif



// #define USE_MAP 1

#ifdef USE_MAP

#include "map/map_config.h"

#define ENV_CHANNELS    MMAPI_CHANNELS
#define ENV_RATE        MMAPI_SAMPLERATE
#define ENV_BITS        MMAPI_BITS

#define ENV_SAMPLETIME  MM_HI_MIX_BUFFER_SIZE_IN_MILLIS

#else

#define ENV_CHANNELS    2
#define ENV_RATE        44100
#define ENV_BITS        16

#define ENV_SAMPLETIME  20

#endif

#define ENV_SAMPLE_LATENCY  5
#define ENV_BLOCK (ENV_SAMPLETIME * (ENV_RATE * ENV_CHANNELS * (ENV_BITS/8)) / 1000)
