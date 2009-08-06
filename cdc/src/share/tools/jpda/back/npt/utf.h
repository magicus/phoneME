/*
 * @(#)utf.h	1.6 06/10/26
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


/* Routines for various UTF conversions */

#ifndef  _UTF_H
#define _UTF_H

#include "utf_md.h"
#include "jni.h"

/* Error and assert macros */
#define UTF_ERROR(m) utfError(__FILE__, __LINE__,  m)
#define UTF_ASSERT(x) ( (x)==0 ? UTF_ERROR("ASSERT ERROR " #x) : (void)0 )

void utfError(char *file, int line, char *message);

struct UtfInst* JNICALL utfInitialize
			    (char *options);
void            JNICALL utfTerminate
			    (struct UtfInst *ui, char *options);
int             JNICALL utf8ToPlatform
			    (struct UtfInst *ui, jbyte *utf8, 
			     int len, char *output, int outputMaxLen);
int             JNICALL utf8FromPlatform
			    (struct UtfInst *ui, char *str, int len, 
			     jbyte *output, int outputMaxLen);
int             JNICALL utf8ToUtf16
			    (struct UtfInst *ui, jbyte *utf8, int len, 
			     jchar *output, int outputMaxLen);
int             JNICALL utf16ToUtf8m
			    (struct UtfInst *ui, jchar *utf16, int len, 
			     jbyte *output, int outputMaxLen);
int             JNICALL utf16ToUtf8s
			    (struct UtfInst *ui, jchar *utf16, int len, 
			     jbyte *output, int outputMaxLen);
int             JNICALL utf8sToUtf8mLength
			    (struct UtfInst *ui, jbyte *string, int length);
void            JNICALL utf8sToUtf8m
			    (struct UtfInst *ui, jbyte *string, int length, 
			     jbyte *new_string, int new_length);
int             JNICALL utf8mToUtf8sLength
			    (struct UtfInst *ui, jbyte *string, int length);
void            JNICALL utf8mToUtf8s
			    (struct UtfInst *ui, jbyte *string, int length, 
			     jbyte *new_string, int new_length);

#endif
