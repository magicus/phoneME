/*
 * @(#)string.h	1.7 06/10/10
 *
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
 *
 */

#ifndef WINCE_STRING_H
#define WINCE_STRING_H

#include <string.h>

#if _WIN32_WCE < 300
#define strcat  strcatA
#define strchr  strchrA
#define strcmp  strcmpA
#define strncmp strncmpA
#define strcpy  strcpyA
#define strncpy strncpyA
#define strdup  strdupA
#else
#define stricmp	_stricmp
#define strdup  _strdup
#endif
#if _WIN32_WCE < 300
#define strlen  strlenA
#define strrchr strrchrA 
#define strstr  strstrA
#define strtol  strtolA
#endif

#endif
