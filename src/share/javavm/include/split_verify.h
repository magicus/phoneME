/*
 * @(#)split_verify.h	1.3 06/10/10
 */

#ifndef SPLIT_VERIFY_H
/*
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
 * CDC split verifier. Derived from KVM.
 * These are the manifest constant and functions needed by the rest of
 * the VM, such as class creation, to create the internal data structures
 * for split verification from the class file. The class file constants
 * are given in
 * 	../jdk-export/jvm.h
 * and the definition of fullinfo_type, and all its tags is given in
 *	../include/verify_impl.h
 */

/*=========================================================================
 * Include files
 *=======================================================================*/

#include "verify_impl.h"
#include "jvm.h"

/*=========================================================================
 * Data
 *=======================================================================*/

/*
 * These structures represent StackMap attribute of a method's Code
 * as read in during class creation.
 * They are used by the split verifier then discarded.
 * (Future project: convert to a more compact form for use by
 * garbage collection.)
 */

/*
 * A single map consists of a pc, and an array of elements
 * representing the contents of the locals and the evaluation
 * stack. These elements are fullinfo_type, which is also
 * used internally by the full verifier.
 */
typedef fullinfo_type CVMsplitVerifyMapElement;

/*
 * Object elements also contain the object typeid.
 * Uninitialized objects contain the pc offset of the corresponding 'new'.
 */
extern fullinfo_type CVMsplitVerifyMakeVerifierType(CVMClassTypeID key);

#define VERIFY_MAKE_UNINIT(_offset)  MAKE_FULLINFO(ITEM_NewObject,0,_offset)
#define VERIFY_MAKE_OBJECT(_typeid)  \
    (CVMassert(CVMtypeidFieldIsRef(_typeid)), \
    (CVMtypeidGetArrayDepth(_typeid)==0) ? \
	MAKE_FULLINFO(ITEM_Object,0,_typeid) : \
	CVMsplitVerifyMakeVerifierType(_typeid))

typedef struct CVMsplitVerifyMap{
    CVMUint32			pc;
    CVMUint16			nLocals;
    CVMUint16			sp;
    /* map contains nLocals+sp used elements */
    CVMsplitVerifyMapElement	mapElements[1];
} CVMsplitVerifyMap;

/*
 * This is the map for each method.
 * For ease of access, each CVMsplitVerifyMap will be the same
 * size, having a map of maxLocals+maxStack elements, though
 * they may not all be used.
 */
typedef struct CVMsplitVerifyStackMaps{
    CVMUint16	   nEntries;
    CVMUint16	   entrySize; /* bytes, for your convenience */
    CVMsplitVerifyMap maps[1];
} CVMsplitVerifyStackMaps;

/*
 * How to index into a collection of split verifier maps.
 * Parameters: struct CVMsplitVerifyStackMaps* _svsmp
 *	       int i
 * Returns:    (struct CVMsplitVerifyMap*)
 */
#define CVM_SPLIT_VERIFY_MAPS_INDEX(_svsmp, _i)\
    ((struct CVMsplitVerifyMap*)((char*)(_svsmp->maps)+(_i*_svsmp->entrySize)))


/*=========================================================================
 * Functions
 *=======================================================================*/


/*
 * We just read in a collection of stack maps for the indicated method,
 * formatted as a CVMsplitVerifyStackMaps and subsidiary structures.
 * Cache the result until we're ready to use it.
 * CVM_TRUE: all ok
 * CVM_FALSE: something went wrong. Usually means that this method already
 * 		had a stackmap. Exception will have been thrown.
 */
CVMBool CVMsplitVerifyAddMaps(
    CVMExecEnv*, CVMClassBlock*, CVMMethodBlock *, CVMsplitVerifyStackMaps*);

/*
 * Find out if we have any split verifier maps saved for any methods
 * of the indicated class.
 */
CVMBool CVMsplitVerifyClassHasMaps(CVMExecEnv*, CVMClassBlock*);

/*
 * Verify all the methods of the class using split verifier.
 * Deletes stack maps.
 * (Future work: rewrite as pointerstackmaps for GC.)
 */
int CVMsplitVerifyClass(CVMExecEnv*, CVMClassBlock*, CVMBool isRedefine);

/*
 * Delete all stackmaps when done.
 */
void CVMsplitVerifyClassDeleteMaps(CVMClassBlock* cb);


#endif /*SPLIT_VERIFY_H */
