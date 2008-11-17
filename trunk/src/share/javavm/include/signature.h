/*
 * @(#)signature.h	1.9 06/10/10
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

/*
 * The keyletters used in type signatures
 */

#ifndef _SIGNATURE_H_
#define _SIGNATURE_H_

#define CVM_SIGNATURE_ANY		'A'
#define CVM_SIGNATURE_ARRAY		'['
#define CVM_SIGNATURE_BYTE		'B'
#define CVM_SIGNATURE_CHAR		'C'
#define CVM_SIGNATURE_CLASS		'L'
#define CVM_SIGNATURE_ENDCLASS		';'
#define CVM_SIGNATURE_ENUM		'E'
#define CVM_SIGNATURE_FLOAT		'F'
#define CVM_SIGNATURE_DOUBLE        	'D'
#define CVM_SIGNATURE_FUNC		'('
#define CVM_SIGNATURE_ENDFUNC		')'
#define CVM_SIGNATURE_INT		'I'
#define CVM_SIGNATURE_LONG		'J'
#define CVM_SIGNATURE_SHORT		'S'
#define CVM_SIGNATURE_VOID		'V'
#define CVM_SIGNATURE_BOOLEAN		'Z'

#define CVM_SIGNATURE_ANY_STRING	"A"
#define CVM_SIGNATURE_ARRAY_STRING	"["
#define CVM_SIGNATURE_BYTE_STRING	"B"
#define CVM_SIGNATURE_CHAR_STRING	"C"
#define CVM_SIGNATURE_CLASS_STRING	"L"
#define CVM_SIGNATURE_ENDCLASS_STRING	";"
#define CVM_SIGNATURE_ENUM_STRING	"E"
#define CVM_SIGNATURE_FLOAT_STRING	"F"
#define CVM_SIGNATURE_DOUBLE_STRING    	"D"
#define CVM_SIGNATURE_FUNC_STRING	"("
#define CVM_SIGNATURE_ENDFUNC_STRING	")"
#define CVM_SIGNATURE_INT_STRING	"I"
#define CVM_SIGNATURE_LONG_STRING	"J"
#define CVM_SIGNATURE_SHORT_STRING	"S"
#define CVM_SIGNATURE_VOID_STRING	"V"
#define CVM_SIGNATURE_BOOLEAN_STRING	"Z"



#endif /* !_SIGNATURE_H_ */
