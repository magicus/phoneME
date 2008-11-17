/*
 * @(#)verify_impl.h	1.4 06/10/10
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
#ifndef _VERIFY_IMPL_
#define _VERIFY_IMPL_

#include "javavm/include/defs.h"

/*
 * This is the representation of datum on the stack and in local variables.
 * It is used during operation of the verifier(s).
 */


/*
 * The tags for data object types, on the stack and in the "registers".
 * They are often augmented with additional, specific, type information.
 * The result is a fullinfo_type. See the MAKE_FULLINFO macro below.
 */
typedef enum {
    ITEM_Bogus,
    ITEM_Void,			/* only as a function return value */
    ITEM_Integer,
    ITEM_Float,
    ITEM_Double,
    ITEM_Double_2,		/* 2nd word of double in register */
    ITEM_Long,
    ITEM_Long_2,		/* 2nd word of long in register */
    ITEM_Array,
    ITEM_Object,		/* Extra info field gives name. */
    ITEM_NewObject,		/* Like object, but uninitialized. */
    ITEM_InitObject,		/* "this" is init method, before call 
				    to super() */
    ITEM_ReturnAddress,		/* Extra info gives instr # of start pc */
    /* The following three are only used within array types. 
     * Normally, we use ITEM_Integer, instead. */
    ITEM_Byte,
    ITEM_Short,
    ITEM_Char,
    /* The following used by split verifier as a wild card for "any ref" */
    ITEM_Reference
}ITEM_Tag;

/*
 * fullinfo_type is a 32-bit quantity into which several fields are
 * packed.  in addition to the ITEM_ tag, above, we can pack an array
 * depth (indirection) and another 11-bit field.  
 *
 * For ITEM_Object (and ITEM_InitObject) this is the
 * index of the object type.  (Thus for arrays of these, the extra
 * information is that of the underlying base type.)
 *
 * For ITEM_NewObject this is a reference back to the 'new' instruction
 * originating the object.
 *
 * For ITEM_ReturnAddress it is the instruction number of the first
 * PC of the current subroutine.  (Remember that we enforce
 * one-entry-per-ret.)
 */

typedef CVMUint32 fullinfo_type;

#define GET_ITEM_TYPE(thing) ((thing) & 0x1F)
#define GET_INDIRECTION(thing) (((thing) & 0xFFFF) >> 5)
#define GET_EXTRA_INFO(thing) ((thing) >> 16)
#define WITH_ZERO_INDIRECTION(thing) ((thing) & ~(0xFFE0))
#define WITH_ZERO_EXTRA_INFO(thing) ((thing) & 0xFFFF)

#define MAKE_FULLINFO(type, indirect, extra) \
	((type) + ((indirect) << 5) + ((extra) << 16))

#define MAKE_Object_ARRAY(indirect) \
	(context->object_info + ((indirect) << 5)) 
#define NULL_FULLINFO MAKE_FULLINFO(ITEM_Object, 0, 0)
/* The following somewhat misleading, but I'd have to fix many places...*/
#define ITEM_Null NULL_FULLINFO
#endif
