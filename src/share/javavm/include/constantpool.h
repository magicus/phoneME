/*
 * @(#)constantpool.h	1.34 06/10/10
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
 * This file contains data structres and api's related to the constant pool.
 */

#ifndef _INCLUDED_CONSTANTPOOL_H
#define _INCLUDED_CONSTANTPOOL_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/typeid.h"

/*
 * The CVMConstantPool is made up of two parallel arrays. One contains type
 * information for each constant pool entry (cpTypes), and the other
 * contains the constant pool entry itself (entries). A entry may already
 * be resolved, in which case cpTypes for the entry will have the
 * CVM_CP_ENTRY_RESOLVED bit set.
 *
 * Romized classes don't require a cpTypes array so cpTypes will be
 * NULL in this case, unless class loading also isn't supported, in which
 * case the cpTypes field doesn't even exist.
 *
 * A constant pool entry can be in one of three states:
 * 1. resolved - the entry is fully resolved. It is ready to be used
 *    as-is by the interpreter.
 * 2. unresolved - the entry needs to be resolved before it is used by
 *    the interpreter. Entries can be in this state after class loading
 *    is finished, but some entries (numbers and Strings) are considered to
 *    be fully resolved after class loading is finished.
 * 3. intermediate - An entry can be in this state after class loading has
 *    has created the intial constant pool (this is called the first pass).
 *    On the second pass, all intermediate entries will either become
 *    unresolved, resolved, or no longer needed. It is called the
 *    "intermediate" state because entries are only in this state for a short
 *    period during class loading. No code outside of the class loader
 *    should ever access an entry in the intermediate state.
 *
 * The chart below helps explain the transformation constant pool entries
 * will go through, starting with the form they take in the class file.
 * As you read a row from left to right, the first non-empty column after
 * the "Class File" column is the initial state a constant pool entry
 * will be in after the first pass of class loading. For example, Integer
 * entries immediately become Resolved, FieldRefs become Unresolved, and
 * String entries are in the Intermediate state, which means more processing
 * will be done on them during the 2nd pass of class loading.
 *
 * After looking up the state a constant pool entry will be in after the first
 * pass of class loading, you can look to the next unempty column to the right
 * to find out which form and state the entry will take next. The "state" of
 * the column will indicate when the entry changes form. For example, a Class
 * entry in the class file starts off in the Intermediate state as a Class
 * entry. During the 2nd pass of class loading is moves to the unresolved
 * state as a ClassTypeID. During constant pool resolution, it moves to the
 * Resolved state as a ClassBlock. String entries don't have an Unresolved
 * form. They start off in the Intermediate state as String entries and 
 * during the 2nd pass of class loading become either a resolved StringObject
 * or a resolved StringICell.
 *
 * Utf8 entries are not used after class loading so they are put in the
 * Intermediate state for this reason, even though the Resolved state
 * might be more appropriate.
 *
 * FieldTypeID and MethodTypeID entries are put in the unresolved state
 * even though the resolved state might be more appropriate. This is done
 * for two reasons. The first is to be consistent with the handling of
 * ClassTypeID entries. The 2nd is so that there are no resolved entry
 * types that are not directly referenced by the intepreter.
 *
 * INVARIANT: Intermediate entry types are only accessed by the class
 * loader.
 *
 * INVARIANT: Unresolved entry types are only written by the class loader
 * and are only read by the constant pool resolver or while holding the
 * CVMcpLock().
 *
 * INVARIANT: All resolved entry types are accessed directly by the 
 * interpreter and the interpreter does not directly access any non-resolved
 * entry types.
 *
 * Class File     Intermediate   Unresolved            Resolved
 * --------------------------------------------------------------------
 * Utf8		  Utf8	
 * Integer					      Integer
 * Float					      Float
 * Long 					      Long
 * Double					      Double
 * Class	  Class          ClassTypeID	      ClassBlock
 * String	  String			      StringObj or StringICell
 * Fieldref			 Fieldref	      FieldBlock
 * Methodref			 Methodref	      MethodBlock
 * InterfaceMethodref		 InterfaceMethodref   MethodBlock
 * NameAndType    NameAndType    Field/MethodTypeID
 */

union CVMConstantPoolEntry {
#ifdef CVM_CLASSLOADING
    /* Unresolved constant pool entries after first class loading pass. */
    union {
	struct { 		     /* CVM_CONSTANT_NameAndType */
	    CVMUint16     nameUtf8Idx;
	    CVMUint16     typeUtf8Idx;
	} nameAndType;
	struct {  		     /* CVM_CONSTANT_Class */
	    CVMUint16     utf8Idx;
	} clazz;
	struct {  		     /* CVM_CONSTANT_String */
	    CVMUint16     utf8Idx;
	} string;
	CVMUtf8*          utf8;      /* CVM_CONSTANT_Utf8 */
    } intermediate;
    /* Unresolved constant pool entries after class loading is done. */
    union {
	struct { 		     /* CVM_CONSTANT_Fieldref, Methodref
					and InterfaceMethodref */
	    CVMUint16     classIdx;
	    CVMUint16     typeIDIdx;
	} memberRef;
	CVMMethodTypeID   methodTypeID; /* CVM_CONSTANT_MethodTypeID */
	CVMFieldTypeID    fieldTypeID;  /* CVM_CONSTANT_FieldTypeID */
	CVMClassTypeID    classTypeID;  /* CVM_CONSTANT_ClassTypeID */
    } unresolved;
#endif
    /* resolved constant pool entries */
    union {
	CVMJavaVal32      val32;     /* Integer, Float, Long, and Double */
	CVMClassBlock*    cb;        /* CVM_CONSTANT_ClassBlock */
	CVMFieldBlock*    fb;        /* CVM_CONSTANT_FieldBlock */
	CVMMethodBlock*   mb;        /* CVM_CONSTANT_MethodBlock */
	CVMStringObject*  strObj;    /* CVM_CONSTANT_StringObj */
	CVMStringICell*   strICell;  /* CVM_CONSTANT_StringICell */
    } resolved;
};


typedef CVMUint8 CVMConstantPoolEntryType;

union CVMConstantPool {
    /* Array of type info for each entry. May be NULL for fully resolved
     * constant pools. We overlay with slot 0 of the entries[] array because
     * it is unused. */
#ifdef CVM_CLASSLOADING
    CVMConstantPoolEntryType* cpTypesX;
#endif
    /* Array of constant pool entries. Note that slot 0 is either unused
     * or is used as the cpTypesX field above. */
    CVMConstantPoolEntry      entriesX[1];
};

/*
 * Transition constant pools are always stack based, so we need to declare
 * one with a valid slot #1 entry (slot 0 if for the type table).
 */
struct CVMTransitionConstantPool {
    CVMConstantPool           cp;
    CVMConstantPoolEntry      entry1X;
};

/*
 * If a constant pool entry is resolved and the constant pool has a type
 * table, then the CVM_CP_ENTRY_RESOLVED bit will be set in the type table
 * for the constant pool entry.
 */
#define CVM_CP_ENTRY_RESOLVED 0x80
#ifdef CVM_DUAL_STACK
#define CVM_CP_ENTRY_ILLEGAL  0x40
#endif
#define CVM_CP_ENTRY_TYPEMASK 0x3F

/*
 * Constant pool entry types. If a constant pool has a type table, then
 * one of these flags will be set for in each entry to indicate the
 * type of the constant pool entry.
 *
 * See the constnat pool description above for details on the
 * meaning of these flags.
 */

enum CVMConstantPoolEntryTypeEnum {
    /* Constant pool types found in the class file. Some of these are
     * only used during class loading (Utf8, Class, and String), some
     * are immediately considered to be resolved (all numeric types),
     * and the rest are considered to be unresolved (Methodref,
     * InterfaceMethodref, and Fieldref).
     */
    CVM_CONSTANT_Utf8               = 1,
    CVM_CONSTANT_Unicode            = 2, /* unused */
    CVM_CONSTANT_Integer            = 3,
    CVM_CONSTANT_Float              = 4,
    CVM_CONSTANT_Long    	    = 5,
    CVM_CONSTANT_Double             = 6,
    CVM_CONSTANT_Class              = 7,
    CVM_CONSTANT_String             = 8,
    CVM_CONSTANT_Fieldref           = 9,
    CVM_CONSTANT_Methodref          = 10,
    CVM_CONSTANT_InterfaceMethodref = 11,
    CVM_CONSTANT_NameAndType        = 12,

    /* Some new constant pool types needed for unresolved entries. Note
     * that Fieldrefs, Methodrefs, and InterfaceMethodrefs are also
     * considered to be unresolved. 
     */
    CVM_CONSTANT_ClassTypeID  = 13,  /* unresolved CVM_CONSTANT_Class */
    CVM_CONSTANT_MethodTypeID = 14,  /* unresolved CVM_CONSTANT_Methodref */
    CVM_CONSTANT_FieldTypeID  = 15,  /* unresolved CVM_CONSTANT_Fieldref */

    /* Some new constant pool types needed for resolved entries. Note that
     * Integer, Float, Long, and Double are also considered to be resolved. 
     */
    CVM_CONSTANT_ClassBlock  = 19, /* resolved CVM_CONSTANT_Class */
    CVM_CONSTANT_FieldBlock  = 20, /* resolved CVM_CONSTANT_Fieldref */
    CVM_CONSTANT_MethodBlock = 21, /* resolved CVM_CONSTANT_Methodref or
				    * CVM_CONSTANT_InterfaceMethodref*/
    CVM_CONSTANT_StringObj   = 22, /* resolved romized CVM_CONSTANT_String */
    CVM_CONSTANT_StringICell = 23, /* resolved CVM_CONSTANT_String */

    CVM_CONSTANT_Invalid     = 24  /* marks an entry that should no longer
				    * be referenced. Used in Utf8 and String
				    * entries after class loading is done */
};

typedef enum CVMConstantPoolEntryTypeEnum CVMConstantPoolEntryTypeEnum;


/*
 * Macros for accessing constant pool entry type information.
 *
 * CVMcpTypes: Returns the constant pool type table. This will be NULL if
 * there is no class loading and will always be NULL for romized and
 * pre-resolved classes.
 *
 * CVMcpEntryType: Returns the type of the constant pool entry. Only callable
 * when class loading is supported.
 *
 * CVMcpIsResolved: Returns CVM_TRUE if the entry is resolved or if there
 * is no type table.
 *
 * CVMcpTypeIs: Checks the type of a constant pool entry. Cannot be called
 * with a constant pool that has no type table. This means it can't be called
 * with any romized or pre-resolved class.
 *
 * CVMcpSetResolvedEntryType and CVMcpSetUnresolvedEntryType: Sets a constant
 * pool entry's type in the type table.
 */
#ifdef CVM_CLASSLOADING
#define CVMcpTypes(cp)          ((cp)->cpTypesX)
#define CVMcpEntryType(cp, idx) (CVMcpTypes(cp)[idx] & CVM_CP_ENTRY_TYPEMASK)
#define CVMcpIsResolved(cp, idx)				\
    (CVMcpTypes(cp) == NULL 					\
     ? CVM_TRUE							\
     : (CVMcpTypes(cp)[idx] & CVM_CP_ENTRY_RESOLVED) != 0)
#ifdef CVM_DUAL_STACK
#define CVMcpIsIllegal(cp, idx)					\
    (CVMcpTypes(cp) == NULL 					\
     ? CVM_TRUE							\
     : (CVMcpTypes(cp)[idx] & CVM_CP_ENTRY_ILLEGAL) != 0)
#endif
#define CVMcpTypeIs(cp, idx, type)			\
    (CVMassert(CVMcpTypes(cp) != NULL),			\
     (CVMcpTypes(cp)[idx] & CVM_CP_ENTRY_TYPEMASK) == CVM_CONSTANT_##type)
#define CVMcpSetResolvedEntryType(cp, idx, type)			     \
    if (CVMcpTypes(cp) != NULL) {					     \
        CVMcpTypes(cp)[idx] = (CVM_CP_ENTRY_RESOLVED | CVM_CONSTANT_##type); \
    }
#ifdef CVM_DUAL_STACK
#define CVMcpSetIllegalEntryType(cp, idx, type)			     \
    if (CVMcpTypes(cp) != NULL) {					     \
        CVMcpTypes(cp)[idx] = (CVM_CP_ENTRY_ILLEGAL | CVM_CONSTANT_##type); \
    }
#endif
#define CVMcpSetUnresolvedEntryType(cp, idx, type)	\
    if (CVMcpTypes(cp) != NULL) {		       	\
	CVMcpTypes(cp)[idx] = CVM_CONSTANT_##type;	\
    }
#else
#define CVMcpTypes(cp)             (NULL)
#define CVMcpIsResolved(cp, idx)   (CVM_TRUE)
#define CVMcpTypeIs(cp, idx, type) (CVMassert(CVM_FALSE), CVM_FALSE)
#endif

/* 
 * Macros for accessing constant pool entries 
 */

/* intermediate entries */
#ifdef CVM_CLASSLOADING
#define CVMcpGetNameAndTypeNameUtf8Idx(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, NameAndType)),		\
     (cp)->entriesX[idx].intermediate.nameAndType.nameUtf8Idx)
#define CVMcpGetNameAndTypeTypeUtf8Idx(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, NameAndType)),		\
     (cp)->entriesX[idx].intermediate.nameAndType.typeUtf8Idx)
#define CVMcpGetClassUtf8Idx(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, Class)),		\
     (cp)->entriesX[idx].intermediate.clazz.utf8Idx)
#define CVMcpGetStringUtf8Idx(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, String)),		\
     (cp)->entriesX[idx].intermediate.string.utf8Idx)
#define CVMcpGetUtf8(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, Utf8)),	\
     (cp)->entriesX[idx].intermediate.utf8)
#endif /* CVM_CLASSLOADING */

/* unresolved entries */
#ifdef CVM_CLASSLOADING
#define CVMcpGetMemberRefClassIdx(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, Fieldref)			\
	       || CVMcpTypeIs(cp, idx, Methodref)		\
	       || CVMcpTypeIs(cp, idx, InterfaceMethodref)),	\
     (cp)->entriesX[idx].unresolved.memberRef.classIdx)
#define CVMcpGetMemberRefTypeIDIdx(cp, idx)		\
    (CVMassert(CVMcpTypeIs(cp, idx, Fieldref)			\
	       || CVMcpTypeIs(cp, idx, Methodref)		\
	       || CVMcpTypeIs(cp, idx, InterfaceMethodref)),	\
     (cp)->entriesX[idx].unresolved.memberRef.typeIDIdx)
#define CVMcpGetFieldTypeID(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, FieldTypeID)),	\
     (cp)->entriesX[idx].unresolved.fieldTypeID)
#define CVMcpGetMethodTypeID(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, MethodTypeID)),	\
     (cp)->entriesX[idx].unresolved.methodTypeID)
#define CVMcpGetClassTypeID(cp, idx)			\
    (CVMassert(CVMcpTypeIs(cp, idx, ClassTypeID)),	\
     (cp)->entriesX[idx].unresolved.classTypeID)
#endif /* CVM_CLASSLOADING */

/* resolved entries */
#define CVMcpGetVal32(cp, idx)			\
    ((cp)->entriesX[idx].resolved.val32)
#define CVMcpGetVal64(cp, idx, dresult)        	\
    (CVMmemCopy64(dresult.v, &((cp)->entriesX[idx].resolved.val32.raw)))
#define CVMcpGetCb(cp, idx)						     \
    (CVMassert(CVMcpTypes(cp) == NULL || CVMcpTypeIs(cp, idx, ClassBlock)),  \
     (cp)->entriesX[idx].resolved.cb)
#define CVMcpGetFb(cp, idx)						     \
    (CVMassert(CVMcpTypes(cp) == NULL || CVMcpTypeIs(cp, idx, FieldBlock)),  \
     (cp)->entriesX[idx].resolved.fb)
#define CVMcpGetMb(cp, idx)						     \
    (CVMassert(CVMcpTypes(cp) == NULL || CVMcpTypeIs(cp, idx, MethodBlock)), \
     (cp)->entriesX[idx].resolved.mb)
#define CVMcpGetStringObj(cp, idx)					     \
    (CVMassert(CVMcpTypes(cp) == NULL || CVMcpTypeIs(cp, idx, StringObj)),   \
     (cp)->entriesX[idx].resolved.strObj)
#define CVMcpGetStringICell(cp, idx)					     \
    (CVMassert(CVMcpTypes(cp) == NULL || CVMcpTypeIs(cp, idx, StringICell)), \
     (cp)->entriesX[idx].resolved.strICell)

/* 
 * Macros for setting constant pool entries 
 */

/* intermediate entries */
#ifdef CVM_CLASSLOADING
#define CVMcpSetNameAndTypeNameUtf8Idx(cp, cpIdx, utf8idx)		  \
    (cp)->entriesX[cpIdx].intermediate.nameAndType.nameUtf8Idx = utf8idx; \
    CVMcpSetUnresolvedEntryType(cp, cpIdx, NameAndType);
#define CVMcpSetNameAndTypeTypeUtf8Idx(cp, cpIdx, utf8idx)		  \
    (cp)->entriesX[cpIdx].intermediate.nameAndType.typeUtf8Idx = utf8idx; \
    CVMcpSetUnresolvedEntryType(cp, cpIdx, NameAndType);
#define CVMcpSetClassUtf8Idx(cp, cpIdx, utf8idx)		\
    (cp)->entriesX[cpIdx].intermediate.clazz.utf8Idx = utf8idx;	\
    CVMcpSetUnresolvedEntryType(cp, cpIdx, Class);
#define CVMcpSetStringUtf8Idx(cp, cpIdx, utf8idx)			\
    (cp)->entriesX[cpIdx].intermediate.string.utf8Idx = utf8idx;	\
    CVMcpSetUnresolvedEntryType(cp, cpIdx, String);
#define CVMcpSetUtf8(cp, cpIdx, utf8_)			\
    (cp)->entriesX[cpIdx].intermediate.utf8 = utf8_;	\
    CVMcpSetUnresolvedEntryType(cp, cpIdx, Utf8);
#endif /* CVM_CLASSLOADING */

/* unresolved entries */
#ifdef CVM_CLASSLOADING
#define CVMcpSetMemberRefClassIdx(cp, cpIdx, classIdx_, refType)        \
    (cp)->entriesX[cpIdx].unresolved.memberRef.classIdx = classIdx_;	\
    CVMcpSetUnresolvedEntryType(cp, cpIdx, refType);
#define CVMcpSetMemberRefTypeIDIdx(cp, cpIdx, typeIDIdx_, refType)      \
    (cp)->entriesX[cpIdx].unresolved.memberRef.typeIDIdx = typeIDIdx_;	\
    CVMcpSetUnresolvedEntryType(cp, cpIdx, refType);
#define CVMcpSetFieldTypeID(cp, cpIdx, fieldTypeID_)			\
    CVMassert(CVMcpTypeIs(cp, cpIdx, NameAndType));			\
    (cp)->entriesX[cpIdx].unresolved.fieldTypeID = fieldTypeID_;	\
    CVMcpSetUnresolvedEntryType(cp, cpIdx, FieldTypeID);
#define CVMcpSetMethodTypeID(cp, cpIdx, methodTypeID_)			\
    CVMassert(CVMcpTypeIs(cp, cpIdx, NameAndType));			\
    (cp)->entriesX[cpIdx].unresolved.methodTypeID = methodTypeID_;	\
    CVMcpSetUnresolvedEntryType(cp, cpIdx, MethodTypeID);
#define CVMcpSetClassTypeID(cp, cpIdx, classTypeID_)			\
    CVMassert(CVMcpTypeIs(cp, cpIdx, Class));				\
    (cp)->entriesX[cpIdx].unresolved.classTypeID = classTypeID_;	\
    CVMcpSetUnresolvedEntryType(cp, cpIdx, ClassTypeID);
#endif /* CVM_CLASSLOADING */

/* resolved entries */
/* WARNING: must set entry value before setting the type flag in order
 * to avoid readers thinking the entry is resolved when it is not. 
 * Readers don't grab the lock, but it's ok if they think the entry
 * is unresolved when it's really resolved, because readers will always
 * attempt to resolve unresolved entries first. Writers always grab
 * the lock first if there's a chance of a race condition with a reader.
 */
#ifdef CVM_CLASSLOADING
#define CVMcpSetVal64(cp, idx, val64_)        	\
    (CVMmemCopy64(&((cp)->entriesX[idx].resolved.val32.raw), &(val64_)->raw))
#define CVMcpSetInteger(cp, idx, i_)       	\
    (cp)->entriesX[idx].resolved.val32.i = i_;  \
    CVMcpSetResolvedEntryType(cp, idx, Integer);
#define CVMcpSetFloat(cp, idx, f_)		\
    (cp)->entriesX[idx].resolved.val32.f = f_;  \
    CVMcpSetResolvedEntryType(cp, idx, Float);
#define CVMcpSetLong(cp, idx, lval64_)					\
    CVMlong2Jvm(&((cp)->entriesX[idx].resolved.val32.raw), (lval64_));	\
    CVMcpSetResolvedEntryType(cp, idx, Long);
#define CVMcpSetDouble(cp, idx, dval64_)				 \
    CVMdouble2Jvm(&((cp)->entriesX[idx].resolved.val32.raw), (dval64_)); \
    CVMcpSetResolvedEntryType(cp, idx, Double);
#define CVMcpSetCb(cp, idx, cb_)		       	\
    CVMassert(CVMcpTypeIs(cp, idx, ClassTypeID));	\
    (cp)->entriesX[idx].resolved.cb = cb_;		\
    CVMcpSetResolvedEntryType(cp, idx, ClassBlock);
#define CVMcpSetFb(cp, idx, fb_)			\
    CVMassert(CVMcpTypeIs(cp, idx, Fieldref));		\
    (cp)->entriesX[idx].resolved.fb = fb_;		\
    CVMcpSetResolvedEntryType(cp, idx, FieldBlock);
#define CVMcpSetMb(cp, idx, mb_)				\
    CVMassert(CVMcpTypeIs(cp, idx, Methodref) ||		\
	      CVMcpTypeIs(cp, idx, InterfaceMethodref));	\
    (cp)->entriesX[idx].resolved.mb = mb_;			\
    CVMcpSetResolvedEntryType(cp, idx, MethodBlock);
#ifdef CVM_JVMTI
/* This is used by JVMTI to insert a resolved, redefined method reference
 * into this constantpool entry.  Any references via this constantpool
 * entry will now return the new redefined method.
 */
#define CVMcpResetMb(cp, idx, mb_)				\
    (cp)->entriesX[idx].resolved.mb = mb_;
#endif
#define CVMcpSetStringObj(cp, idx, stringObj_)		\
    CVMassert(CVM_FALSE); /* should never be needed */	\
    (cp)->entriesX[idx].resolved.strObj = stringObj_;	\
    CVMcpSetResolvedEntryType(cp, idx, StringObj);
#define CVMcpSetStringICell(cp, idx, stringICell_)		\
    CVMassert(CVMcpTypeIs(cp, idx, String));			\
    (cp)->entriesX[idx].resolved.strICell = stringICell_;	\
    CVMcpSetResolvedEntryType(cp, idx, StringICell);
#endif /* CVM_CLASSLOADING */

/*
 * CVMcpResolveEntryFromClass - resolve a constant pool entry.
 *   currentCb: the "current class" for security and class loading purposes.
 *   cp:        the constant pool
 *   cpIndex:   index of entry in cp to resolve
 */
#define CVMcpResolveEntryFromClass(ee, currentCb, cp, cpIndex)		\
  (CVMcpIsResolved(cp, cpIndex)						\
   ? CVM_TRUE								\
   : CVMprivate_cpResolveEntryFromClass(ee, currentCb, cp, cpIndex))

/*
 * CVMcpResolveEntry - resolve a constant pool entry. Same as 
 * CVMcpResolveEntryFromClass, except the current class is fetched for
 * you from the current frame.
 */
#define CVMcpResolveEntry(ee, cp, cpIndex)		     		\
    CVMcpResolveEntryFromClass(ee, CVMeeGetCurrentFrameCb(ee), cp, cpIndex)

/*
 * CVMcpCheckResolvedAndGetTID - Returns TRUE if entry is resolved.
 * Returns FALSE if not resolved and returns the typeid of the entry
 * in *p_typeid.
 */
#define CVMcpCheckResolvedAndGetTID(ee, currentCb, cp, cpIndex, p_typeid)\
  (CVMcpIsResolved(cp, cpIndex)						 \
   ? CVM_TRUE								 \
   : CVMprivate_cpExtractTypeIDFromUnresolvedEntry(ee, currentCb, cp,	 \
						   cpIndex, p_typeid))
extern CVMBool
CVMprivate_cpExtractTypeIDFromUnresolvedEntry(CVMExecEnv* ee,
					      CVMClassBlock* currentCb, 
					      CVMConstantPool* cp, 
					      CVMUint16 cpIndex,
					      CVMTypeID* p_typeid);

/* Iterate the class' constant pool and resolve unresolved entries. */
extern void 
CVMcpResolveCbEntriesWithoutClassLoading(CVMExecEnv* ee, 
                                         CVMClassBlock* cb);

/*
 * CVMprivate_cpResolveEntryFromClass - used by the macros above, but only
 * if class loading is supported. Otherwise the compiler dead strips the call.
 */
#ifdef CVM_CLASSLOADING
CVMBool
CVMprivate_cpResolveEntryFromClass(CVMExecEnv* ee,
				   CVMClassBlock* currentCb, 
				   CVMConstantPool* cp,
				   CVMUint16 cpIndex);
/*
 * CVMcpResolveEntryWithoutClassLoading - resolves the specified cp entry,
 * but only if it can be done without causing any class loading. Returns
 * TRUE on success. If false is returned, then "p_typeid" will contain the
 * CVMMethodTypeID or CVMFieldTypeID of the entry if the entry is a MemberRef.
 * This is done because the typeID must be fetched while holding a lock.
 */

#define CVMcpResolveEntryWithoutClassLoading(ee, cb, cp, cpIndex, p_typeid) \
  (CVMcpIsResolved(cp, cpIndex)						\
   ? CVM_TRUE								\
   : CVMprivate_cpResolveEntryWithoutClassLoading(ee, cb, cp, cpIndex, p_typeid))

CVMBool
CVMprivate_cpResolveEntryWithoutClassLoading(CVMExecEnv* ee,
					     CVMClassBlock* currentCb, 
					     CVMConstantPool* cp, 
					     CVMUint16 cpIndex,
					     CVMTypeID* p_typeid);
#endif /* CVM_CLASSLOADING */

/*
 * CVMallocConstantPool: Allocates a constant pool, including the type table
 */
#ifdef CVM_CLASSLOADING
#define CVMallocConstantPool(cp, numEntries)				    \
{									    \
    (cp) = (CVMConstantPool*)malloc(				    \
	CVMoffsetof(CVMConstantPool, entriesX) + (numEntries) *		    \
	(sizeof(CVMConstantPoolEntry) + sizeof(CVMConstantPoolEntryType))); \
    if ((cp) != NULL) {							    \
	(cp)->cpTypesX = (CVMConstantPoolEntryType*)			    \
	    (offsetof(CVMConstantPool, entriesX) +			    \
	    (numEntries) * sizeof(CVMConstantPoolEntryType));		    \
    }									    \
}
#endif /* CVM_CLASSLOADING */

/*
 * CVMinitTransitionConstantPool: Initializes a stack based constant pool
 * for a transition frame.
 */
#ifdef CVM_CLASSLOADING
#define CVMinitTransitionConstantPool(cp_, mb_)	\
    (cp_)->cp.cpTypesX = NULL;			\
    (cp_)->entry1X.resolved.mb = mb_;
#else
#define CVMinitTransitionConstantPool(cp_, mb_) \
    (cp_)->entry1X.resolved.mb = mb_;
#endif /* CVM_CLASSLOADING */

/*
 * Constant Pool access lock.
 *
 * Sometimes setting or getting a constant pool entry must be done
 * within a lock because often you need to verify an entry's type before
 * you can get its value, but it's possible that the type may change may
 * change between the check and the fetch of the entry. Fortunately this
 * isn't always a problem, so there are opportunities to limit locking.
 * The following 3 situations outline when locking is necessary:
 * 
 * (1) The Interpreter Loop: The interpreter loop never needs to acquire
 * the lock because it only deals with fully resolved entries. Their
 * types and values do not change.
 *
 * (2) Classloader: The classloader never needs to acquire the lock because
 * it is not possible for another thread to access a constant pool that
 * the classloader is creating or modifying.
 *
 * (3) Quickening and Constant Pool Resolution: This is where all the
 * difficult locking work is done. There are a number a race conditions
 * that can happen here. Here are some rules that are followed to avoid
 * them:
 *
 *    Constant pool entries are always be resolved before the quickened opcode
 *    is written out. This prevents the interpeter loop from seeing a quick
 *    opcode while the constant pool entry is still unresolved.
 *
 *    Writing an entry must always be done within the lock (except during
 *    class loading). Writing an entry involves 2 writes: one for the type
 *    and one for the value. This must always be done within a lock to make
 *    sure other threads don't think an entry is of a certain type, but end
 *    up getting an invalid value because the type changed.
 *
 *    If you first check an entry's type and then read it based on the
 *    discovered type, this must all be done within the lock, unless you
 *    are dealing with a fully resolved type. The reason for this is that
 *    if the entry is unresolved, its type and value may end up changing
 *    after you verified the type, but before you read the value.
 *
 * Contention for this lock is very low, so a global lock is appropriate.
 */

#ifdef CVM_CLASSLOADING
/* Use a micro-lock. It is forbidden to block while holding this lock */
#define CVMcpLock(ee)   CVMsysMicroLock(ee, CVM_CONSTANTPOOL_MICROLOCK)
#define CVMcpUnlock(ee) CVMsysMicroUnlock(ee, CVM_CONSTANTPOOL_MICROLOCK)
#endif

#endif /* _INCLUDED_CONSTANTPOOL_H */
