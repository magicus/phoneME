/*
 * @(#)string_impl.h	1.10 06/10/10
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

#ifndef INCLUDED_STRING_IMPL_H
#define INCLUDED_STRING_IMPL_H

/*
 * This is the struct declaration for the strings that
 * the ROMizer instantiates.
 */
/*
 * The following struct java_lang_String is accessed in 
 * CVMjniGetStringLength() (file jni_impl.c) with CVMID_fieldReadInt()
 * which is a macro that use coding like this:
 * CVMJavaInt * fieldLoc_ = ( CVMJavaInt *)((CVMJavaVal32*)(directObj)
 * + ((((sizeof(CVMObjectHeader)/sizeof(CVMAddr))+( 2 ))))) ;
 * This implies that the members of the struct must fit in
 * CVMJavaVal32 chunks. On 64 bit platforms it is required to
 * insert padding bytes to fullfill this requirement.
 */
struct java_lang_String {
	CVMObjectHeader		hdr;
	CVMArrayOfChar* 	value;
	CVMJavaInt		offset;
#ifdef CVM_64
        CVMJavaInt pad_to_slot_size_0;
#endif
	CVMJavaInt		count;
#ifdef CVM_64
        CVMJavaInt pad_to_slot_size_1;
#endif
#if JAVASE >= 16
        CVMJavaInt hash;
#ifdef CVM_64
        CVMJavaInt pad_to_slot_size_2;
#endif
#endif
};

/*
 * And this is the compressed version from which the rest can be
 * reconstructed
 */
struct java_lang_String_compressed {
	CVMJavaInt		offset;
	CVMJavaInt		count;
};

#define CVM_ROM_STRING_INIT_COMPRESSED(offset, count)		\
    {offset, count}

/*
 * String.intern internal data structure.
 * These are on a linked list, of which the first
 * is (or are) read-only.
 * The header is fixed, but the data following is of
 * variable size, of which "capacity" is the key.
 *
 * capacity is the total number of slots in this segment of
 *   the intern table (currently these are all the same, but need not
 *   be).
 *
 * load is the current number of occupied slots.
 *
 * maxLoad is:
 *  the maximum number of slots we want occupied: currently
 *	about 65% of the capacity for reasonably re-hash
 *	behavour: AND ALSO....
 *   an odd number for pre-loaded segments, or
 *   an even number for dynamically allocated segments
 *   (as a flag for CVMinternDestroy).
 * a refCount value of 255 is an unused entry
 * a refCount value of 254 is "sticky"
 * a refCount value of 253 is a deleted entry
 *
 * 
 * What these really look like in use is:
 * struct CVMInternSegment {
 * 	struct CVMInternSegment ** nextp;
 * 	CVMUint32	capacity;
 * 	CVMUint32	load;
 * 	CVMUint32	maxLoad;
 * 	CVMStringICell	data[capacity];
 * 	CVMUint8	refCount[capacity];
 *	struct CVMInternSegment * next;
 * } CVMInternSegment;
 * except that read-only segments, of course, have a next cell
 * separate from the data.
 *
 * Access to the variable-offset part is through macros.
 */

#define CVM_INTERN_SEGMENT_HEADER \
	struct CVMInternSegment ** nextp; \
	CVMUint32	capacity; \
	CVMUint32	load; \
	CVMUint32	maxLoad;

typedef struct CVMInternSegment {
    CVM_INTERN_SEGMENT_HEADER
    CVMStringICell	data[1];
} CVMInternSegment;

#define CVMInternRefCount(hp) ( (CVMUint8*)(&((hp)->data[(hp)->capacity])) )
#define CVMInternNext(hp) ( *(hp->nextp) )

#define CVMInternUnused		255
#define CVMInternSticky		254
#define CVMInternDeleted	253

#ifndef IN_ROMJAVA
extern const struct CVMInternSegment CVMInternTable;
#endif

#endif
