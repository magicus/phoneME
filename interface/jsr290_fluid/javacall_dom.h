/*
* Copyright  2000-2008 Sun Microsystems, Inc. All Rights
* Reserved.  Use is subject to license terms.
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

#ifndef __JAVACALL_DOM_H_
#define __JAVACALL_DOM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"

/* Maximum size of exception message */
#define JAVACALL_DOM_BUFFER_SIZE 128
    
/** 
 * Types of Exceptions
 */
typedef enum JAVACALL_DOM_EXCEPTIONS_ENUM {
    JAVACALL_DOM_INDEX_SIZE_ERR              =  0x1,
    JAVACALL_DOM_DOMSTRING_SIZE_ERR          =  0x2,
    JAVACALL_DOM_HIERARCHY_REQUEST_ERR       =  0x3,
    JAVACALL_DOM_WRONG_DOCUMENT_ERR          =  0x4,
    JAVACALL_DOM_INVALID_CHARACTER_ERR       =  0x5,
    JAVACALL_DOM_NO_DATA_ALLOWED_ERR         =  0x6,
    JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR =  0x7,
    JAVACALL_DOM_NOT_FOUND_ERR               =  0x8,
    JAVACALL_DOM_NOT_SUPPORTED_ERR           =  0x9,
    JAVACALL_DOM_INUSE_ATTRIBUTE_ERR         =  0xA,
    JAVACALL_DOM_INVALID_STATE_ERR           =  0xB,
    JAVACALL_DOM_SYNTAX_ERR                  =  0xC,
    JAVACALL_DOM_INVALID_MODIFICATION_ERR    =  0xD,
    JAVACALL_DOM_NAMESPACE_ERR               =  0xE,
    JAVACALL_DOM_INVALID_ACCESS_ERR          =  0xF,
    JAVACALL_DOM_TYPE_MISMATCH_ERR           = 0x10
} JAVACALL_DOM_EXCEPTIONS;

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_H_ */

