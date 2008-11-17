/*
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
 */

#ifndef _ppcError_h
#define _ppcError_h

#include <wceUtil.h>

#ifdef DEBUG
JAVAI_API void ppcPrintDebug(char *msg);
JAVAI_API void ppcPrintDebugW(unsigned short *msg);
#else
#define ppcPrintDebug(s)
#define ppcPrintDebugW(s)
#endif /* ! DEBUG */

JAVAI_API void ppcExit(int exitStatus);
JAVAI_API void ppcPrintError(char *msg);
JAVAI_API void ppcPrintErrorf(char *fmt, ...);

JAVAI_API void ppcHandleException(struct _EXCEPTION_POINTERS *lpe);

#endif /* _ppcError_H */
