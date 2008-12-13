/*
 *   
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

/** \file StackMapDefinitions.hpp
    This contains definitions used by the StackMap, StackMapsList and Verifier classes.
*/

/** Enumerated StackMap types 
 */
enum StackMapKind {
    ITEM_Bogus = 0,       // Unused
    ITEM_Integer = 1,
    ITEM_Float = 2,
    ITEM_Double = 3,
    ITEM_Long = 4,
    ITEM_Null = 5,        // Result of aconst_null
    ITEM_InitObject = 6,  // "this" is in <init> method, before call to super()
    ITEM_Object = 7,      // Extra info field gives name
    ITEM_NewObject = 8,   // Like object, but uninitialized
                            // Following codes do not exist within classfiles 
    ITEM_Long_2 = 9,      // 2nd word of long in register 
    ITEM_Double_2 = 10,   // 2nd word of double in register
    ITEM_Category1 = 11,
    ITEM_Category2 = 12,
    ITEM_DoubleWord = 13,
    ITEM_Reference = 14,
    forcestackmapkind = 0x10000000  // force int-size enum 
};

/* ------------------------------------------------------------------------ *\
 *                          "NEW" object data types                         *
\* ------------------------------------------------------------------------ */

#define ITEM_NewObject_Flag 0x1000

#define ENCODE_NEWOBJECT(pc) ((((pc) & 0xFFFF) << 16) | ITEM_NewObject_Flag)
#define DECODE_NEWOBJECT(no) (((no) >> 16) & 0xFFFF)

// Tests a tag type to ensure if it was created using create_tag_type_for_new_object()
#define IS_TAG_TYPE_FOR_NEW_OBJECT(tag_type) (tag_type & ITEM_NewObject_Flag)
