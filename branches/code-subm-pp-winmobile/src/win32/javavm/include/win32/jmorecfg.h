/*
 * @(#)jmorecfg.h	1.1 07/1/24
 *
 * Copyright  2007-2007 Davy Preuveneers. All Rights Reserved.  
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
 */

#ifndef _WIN32_JMORECFG_H
#define _WIN32_JMORECFG_H

#ifdef WINCE

/*
 * For WinCE devices, "boolean" is already declared as "unsigned char" in
 * rpcndr.h. Therefore, the redefinition of "boolean" must remain the same.
 */
 
#ifndef HAVE_BOOLEAN

typedef unsigned char boolean;

#define HAVE_BOOLEAN

#pragma include_alias ("jmorecfg.h", "../share/basis/native/image/jpeg/lib/jmorecfg.h")
#include "jmorecfg.h"

#endif

#endif /* WINCE */

#endif /* _WIN32_JMORECFG_H */
