/*
 * @(#)typeid_impl.h	1.32 06/10/10
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
 * This file gives the interface to the type system.
 */

#ifndef _INCLUDED_TYPEID_IMPL_H
#define _INCLUDED_TYPEID_IMPL_H

/*
 * Internal data structures of the CVM type system.
 * These need only be exported for ROMizer support.
 * The external interface to the type system is defined in
 * typeid.h
 */

typedef CVMUint8	TypeidState;
#define TYPEID_STATE_INROM	1
#define INROM		TYPEID_STATE_INROM

#ifdef CVM_DEBUG
/*
 * For purposes of debugging, we tag many table entries
 * with a state: inRom, added, deleted.
 * These are manipulated only when the entry is added/deleted
 * from the table (not just another reference made), and
 * can be cleared by CVMtypeidPrintDiffs(), c.f.
 * It works best with NO_RECYCLE turned on as well.
 */
#define TYPEID_STATE_ADDED	2
#define TYPEID_STATE_DELETED	4
/* Note the ; and , at the end of the following! */
#define STATEFLAG(t)	TypeidState t;
#define ROMSTATE	TYPEID_STATE_INROM,

#else

#define STATEFLAG(t)
#define ROMSTATE

#endif

/*
 * This value is used to signify no-such-entry when an index
 * is required. Since 0 is, in some places, a valid index, we
 * use this value instead:
 */
#define TYPEID_NOENTRY ((CVMTypeIDTypePart)-1)

/*
 * This idiom is used in many places to build an extensable,
 * indexable data structure. This allows us to reference many
 * of these types as little indices, rather than as full pointers.
 * It only works if they are of uniform size (or can be shoe-horned into
 * a uniform-size scheme. Note that the next pointer is a segment **,
 * which makes for better ROMization of the table segment itself.
 * See usage examples below.
 */
#define SEGMENT_HEADER(T)	\
    int		firstIndex;	\
    int		nFree;		\
    int		nextFree;	\
    int		nInSegment;	\
    T **	next;

/*
 * This idiom is used for many of the table entries.
 * All the tables are adminstered in the same way: reference counted
 * entries, found by following a a hash bucket chain
 * that uses an index rather than a direct pointer (strictly space vs. time).
 * 
 * Beyond that they're different. The macro is included in the definitions
 * rather than the struct because of C's alignment rules: if the next entry
 * is byte-sized, there need not be any padding using the macro, but there probably
 * would be, using the struct.
 *
 * Entries are reference counted, for possible reuse. When a reference
 * count reaches 255, it sticks, and that entry becomes permenent. ROM
 * entries should be flagged with a count of 255. A reference count of 0
 * flags a free entry. See MAX_COUNT
 */

#define COMMON_TYPE_ENTRY_HEADER \
    CVMTypeIDPart	nextIndex;	/* hash bucket chain */ \
    CVMUint8		refCount; \
    STATEFLAG( state )

/*
 * The basic decoding of the scalarType cookie is described in
 * scalarType.h.  When that cookie includes an index to a larger table
 * entry, that entry is a struct scalarTableEntry, which is in a
 * scalarEntrySegment.
 * 
 * Scalar types are also used as classname cookies, since, for the vast
 * majority of classes, there is at least one scalar holding an instance
 * of the class type, (and thus a scalar type corresponding to the class)
 * it seemed economic to unify the two.
 * 
 * There are only two entry types: big arrays and classes. It seems
 * wasteful to use a whole byte for the tag, but alignment constraints are
 * probably going to cause it to be wasted on padding otherwise.
 * 
 */

#include "javavm/include/typeid.h"

struct pkg; /* see below */

struct scalarTableEntry {
    COMMON_TYPE_ENTRY_HEADER
    CVMUint8			tag;
    CVMUint8			nameLength; /* only used for classnameType */

    union valueUnion{
	struct classnameType {
	    struct pkg *	package;
	    const char *        classInPackage;
	} className;
	struct arrayType{
	    /* The following two fields must be the same size as those
	     * in the className struct above in order for the INIT_ARRAYTYPE
	     * macro below to work properly. That is why the CVMAddr
	     * type is used.
	     */
	    CVMAddr	depth;
	    CVMAddr	basetype;
	} bigArray;
    } value;
};

/* type tags for the union */
#define CVM_TYPE_ENTRY_FREE	0
#define CVM_TYPE_ENTRY_OBJ	1
#define CVM_TYPE_ENTRY_ARRAY	2

#define INIT_CLASSTYPE( hashnext, pkgname, myname, length ) \
    { hashnext, MAX_COUNT, ROMSTATE CVM_TYPE_ENTRY_OBJ, length, {{ pkgname, myname }}}

#define INIT_ARRAYTYPE( hashnext, depth, base ) \
    { hashnext, MAX_COUNT, ROMSTATE CVM_TYPE_ENTRY_ARRAY, 0, \
      {{ (struct pkg*)depth, (const char*)base }} }


/****************************************************************************
 * type structures are allocated in arrays, called segments,
 * which are linked together.
 * They are allocated, reference counted, freed, and re-used.
 * Since the segments are of variable size, the ROMizer needs only to
 * produce one big one of these.
 * >>Note the extra indirection required for the "next" pointer. This allows
 * the segment to be ReadOnly, even though the next pointer won't be!
 */

#define SCALAR_TABLE_SEGMENT_HEADER SEGMENT_HEADER(struct scalarTableSegment)

struct scalarTableSegment {
    SCALAR_TABLE_SEGMENT_HEADER
    struct scalarTableEntry
		data[1]; /* variable size */
};

/*
 * CVMtypeTable points to the first scalarTableSegment.
 */
#ifndef IN_ROMJAVA
extern struct scalarTableSegment CVMFieldTypeTable;
#endif

#define FIRST_SCALAR_TABLE_INDEX CVMtypeLastScalar+1

/****************************************************************************
 * The basis of type-structure lookup is the package.
 * This introduces another layer of data-structure management.
 * Packages are looked up using an initial hash of the name into
 * a table. Then each package itself contains a mini-hash table for
 * looking up the scalarTableEntry's.
 * These packages cannot be ReadOnly, as they contain hash tables, which are
 * administred by add-at-beginning-of-chain, so the entries can be ReadOnly.
 * These are reference counted.
 */


#define NCLASSHASH	11
#define NPACKAGEHASH	17

struct pkg {
    const char *	pkgname;
    struct pkg *	next;	/* hash bucket chain */
    TypeidState		state; /* to hold INROM flag if no other */
    CVMUint8	 	refCount;
    CVMTypeIDTypePart	typeData[NCLASSHASH];
};

extern struct pkg ** const CVM_pkgHashtable;

extern struct pkg * const CVMnullPackage; /* classes without packages go here. */

/*************************************************************
 * A Form, a.k.a. Terse Signature, has two uses: 
 * (a) as part of method signatures, it increases sharing of these types.
 * (b) for JNI parameter passing.
 * 
 * A form is a variable-length array of type code 'syllables', one for the
 * return value and one for each parameter. The encoding we use is the
 * scalar type encoding, with CVM_TYPEID_OBJ standing for all heap types:
 * objects and all arrays.
 * 
 * The lowest-order syllable of the first word is that of the return
 * type.  The syllable ajacent to that is for the first parameter.  The
 * syllable ajacent to that is for the second parameter.  And so on.
 * 
 * Since most signature types have fewer than 7 parameters, most
 * form encodings will fit in one word. These are included in the
 * method signature directly. All that remains are of the bigger
 * type.
 *
 * There are very few forms. They come and go very slowly.
 * There is no point in keeping them in an indexed array.
 */
 

#define DECLARE_SIG_FORM_N( NAME, N ) \
    struct NAME{ \
	struct sigForm * next;	/* hash bucket chain */ \
	CVMUint8	 refCount; \
	STATEFLAG(state)	\
	CVMUint8	 nParameters; \
	/* may be 2 bytes wasted here, depending on target alignment requirements */ \
	CVMUint32	 data[N]; \
    }

DECLARE_SIG_FORM_N( sigForm, 1 );

/* #define FORM_SIZE_INCREMENT (sizeof(struct sigForm) / sizeof(unsigned int)) */
#define FORM_SYLLABLESPERDATUM (2*sizeof(CVMUint32))
#define FORM_DATAWORDS(n) (((n)+FORM_SYLLABLESPERDATUM-1)/FORM_SYLLABLESPERDATUM)

/*
 * If most forms are small enough to fit within the signatures,
 * then managing the few that don't isn't very important.
 * They are variable sized structurs, that I link together.
 * There as so very few (a dozen?) that I don't even bother
 * with a hash table. Reference counting is good, though.
 * When adding new ones, add at the front of the list.
 * This way, though the CVMformTable cell has to be writable,
 * the forms themselves can be in ROM.
 */

/*
 * CVMformTable points to the first sigForm
 */
extern struct sigForm ** const CVMformTablePtr;

/*******************************************************************
 * A method signature is a form reference plus an array of data types,
 * which correspond to the "Object" items in the form. Of course, they
 * may be either class types or array types, as both are objects.
 * These are intrinsically variable-sized data, which I arrange as follows:
 * there is a fixed-sized part which is kept in segmented arrays, similar
 * to DataTypes. If there are 0<=n<=2 items in the "details" array,
 * they are kept in a small array directly in the object. For n>2,
 * there is a pointer to the array, which is union'ed with the in-line
 * array. This results in no savings in the ROMized case (where we should
 * use sharing of sequence prefixes of detail arrays anyway), but
 * should result in savings for dynamically-created signatures (for which
 * sharing of details is probably too much bother).
 * If the signature is short (nSyllables<=8, i.e. 7 or fewer parameters)
 * then the form is included directly in the signature structure.
 * Otherwise, there is a pointer to one of the above sigForm things.
 * The length of the details array is not given here: it can be
 * derived by inspecting the form.
 */

#define METHOD_TYPE_HEADER \
    COMMON_TYPE_ENTRY_HEADER \
    CVMUint8		nParameters; /* needed for inline form */

#define N_METHOD_INLINE_DETAILS	2

struct methodTypeTableEntry {
    METHOD_TYPE_HEADER

    union methodFormUnion {
	struct sigForm *	formp;
	CVMUint32		formdata;
    } form;
    union methodDetailUnion {
	CVMTypeIDTypePart	data[N_METHOD_INLINE_DETAILS];
	CVMTypeIDTypePart *	datap;
    } details;
};

/********************************************************************
 * The indexable table of these things.
 */

#define METHOD_TABLE_SEGMENT_HEADER SEGMENT_HEADER(struct methodTypeTableSegment)

struct methodTypeTableSegment {
    METHOD_TABLE_SEGMENT_HEADER
    struct methodTypeTableEntry
		data[1]; /* variable size */
};

#ifndef IN_ROMJAVA
extern struct methodTypeTableSegment CVMMethodTypeTable;
#endif

/*
 * And the hash table that starts our search.
 * NOTE that its a table of indices, not a table of pointers.
 */

/* 
 * IMPORTANT:
 * When you change this number also change it in CVMMethodType in jcc.
 */
#define NMETHODTYPEHASH	(13*37) /* arbitrary odd number */

extern CVMTypeIDPart * const CVMMethodTypeHash;

/********************************************************************
 * The member name table. Hashed, reference counted,
 * indexed. Just like all the above.
 */
struct memberName {
    COMMON_TYPE_ENTRY_HEADER
    const char *	name;
};

#define MEMBER_NAME_TABLE_SEGMENT_HEADER SEGMENT_HEADER(struct memberNameTableSegment)

struct memberNameTableSegment{
    MEMBER_NAME_TABLE_SEGMENT_HEADER
    struct memberName	data[1]; /* of variable size */
};

#define NMEMBERNAMEHASH	(41*13)	/* pretty arbitrary odd number */

extern CVMTypeIDPart * const CVMMemberNameHash;

#ifndef IN_ROMJAVA
extern struct memberNameTableSegment CVMMemberNames;
#endif

/********************************************************************
 * Here for the convenience of several of the C functions. 
 * Many entries are reference counted, for possible reuse.
 * When a reference count reaches 255, it sticks, and that entry
 * becomes permenent.
 * ROM entries should be flagged with a count of 255.
 */

#define MAX_COUNT ((CVMUint8)0xff)

#define conditionalIncRef( ep ) \
    {if ( (ep)->refCount < MAX_COUNT ) \
	(ep)->refCount += 1; }

#endif
