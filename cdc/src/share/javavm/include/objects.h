/*
 * @(#)objects.h	1.92 06/10/10
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
 * This file gives some core type definitions for Java objects in
 * CVM. The "bottom of the world" if you will.
 *
 * All forward declarations and typedef's are done in defs.h
 */

#ifndef _INCLUDED_OBJECTS_H
#define _INCLUDED_OBJECTS_H

#include "javavm/include/defs.h"
#include "javavm/include/signature.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/sync.h"
#ifdef CVM_JIT
#include "javavm/include/porting/jit/jit.h"
#endif

#ifndef CVM_GC_SPECIFIC_WORDS
#define CVM_GC_SPECIFIC_WORDS
#endif


/*
 * The following definitions make up a Java object header, to be found
 * in all arrays and objects.
 *
 * The first two words are common to all possible GC's. There is
 * enough "room" in these two words for supporting non-incremental
 * collectors, like generational collectors.
 *
 * More words may be added to the header as needed by a given GC
 * algorithm.
 *
 * The first word:
 *
 * This is a pointer to the ClassBlock of an object, 'clas', with the
 * lower two bits serving special purpose. 
 *
 * The lowest bit of the 'clas' word indicates the locking type for
 * this object. For registered real-time objects, locking must be
 * deterministic, performing priority inheritance or priority ceiling,
 * etc. For others, locking may use a fast average case user-level
 * scheme, which would work great for uncontended objects.
 *
 * The second lower bit of the 'clas' word indicates whether this
 * object is in the heap or in ROM. This indication is necessary for
 * GC to know that it is now looking at a non-GCable object. It still
 * has to scan such an object's fields for discovering reachability of
 * others, but it can't move the object around, nor reclaim its space.
 *
 * The second word:
 *
 * Includes a "VariousBits" word, used for synchronization, GC, and
 * object hashcodes. Depending on the synchronization state of this
 * object, the bits may be there, or may have been evacuated to some
 * sort of "lock record".
 */

/*
 * The widths of the bitfields of the "various" header word
 */
#define CVM_SYNC_BITS  2
#define CVM_HASH_BITS  24
#define CVM_GC_BITS (32 - CVM_SYNC_BITS - CVM_HASH_BITS)

#define CVM_GC_SHIFT	(CVM_SYNC_BITS + CVM_HASH_BITS)

#define CVM_SYNC_MASK	((1 << CVM_SYNC_BITS) - 1)
#define CVM_HASH_MASK	((1 << CVM_HASH_BITS) - 1)

#define CVMhdrBitsSync(h)	((h) & CVM_SYNC_MASK)
#define CVMhdrBitsPtr(h)	((h) & ~CVM_SYNC_MASK)
/* 
 * The argument to CVMhdrBitsHashCode() is a CVMAddr but
 * java.lang.Object.hashCode is a jint so cast it here.
 */
#define CVMhdrBitsHashCode(h)   ((CVMInt32)((h) >> CVM_SYNC_BITS) & CVM_HASH_MASK)
#define CVMhdrBitsGc(h)		((h) >> CVM_GC_SHIFT)

#define CVMhdrSetHashCode(hdr, hc)				\
    {								\
	CVMassert((hc & ~((1 << CVM_HASH_BITS) - 1)) == 0);	\
	(hdr) |= (hc << CVM_SYNC_BITS);			\
    }

#define CVMhdrSetSync(hdr, s)					\
    {								\
	CVMassert(((s) & ~3) == 0);				\
	(hdr) = CVMhdrBitsPtr((hdr)) | (s);			\
    }


/* 
 * various32 is casted to a pointer
 * (see CVMobjectVariousWord() in CVMobjGcBitsPlusPlusCompare())
 * therefore the type has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
struct CVMObjectHeader {
    volatile CVMClassBlock   *clas;
    volatile CVMAddr         various32;
    CVM_GC_SPECIFIC_WORDS
};

#define CVMobjectVariousWord(obj)  ((obj)->hdr.various32)

enum {
    CVM_LOCKSTATE_LOCKED = 0,
    CVM_LOCKSTATE_MONITOR = 1,
    CVM_LOCKSTATE_UNLOCKED = 2
};

/*
 * Hash for the object has not been computed yet
 */
#define CVM_OBJECT_NO_HASH 0

/*
 * The default "empty" various header word
 */
#define CVM_OBJECT_DEFAULT_VARIOUS_WORD \
     (((CVM_OBJECT_NO_HASH) << CVM_SYNC_BITS) | CVM_LOCKSTATE_UNLOCKED)

/* For use only during GC. Is the hash state and lock state of the current
   header word "trivial" (GC bits don't matter here). */
#define CVMobjectTrivialHeaderWord(word)			\
    (((word) & ((1 << (CVM_SYNC_BITS + CVM_HASH_BITS)) - 1))	\
    == CVM_OBJECT_DEFAULT_VARIOUS_WORD)

/*
 * The mask for a computed hash to make it fit in the hashCode field
 * of CVMVariousBitsData 
 */
#define CVM_OBJECT_HASH_MASK ((1 << CVM_HASH_BITS) - 1)

/*
 * Get the hash value, setting it first if necessary
 */
extern CVMInt32
CVMobjectGetHashSafe(CVMExecEnv *ee, CVMObjectICell* indirectObj);

/*
 * Get the hash value, but only if it has already been set
 */
extern CVMInt32
CVMobjectGetHashNoSet(CVMExecEnv *ee, CVMObject* directObj);
    
#define CVMobjMonitorState(obj)				\
	CVMhdrBitsSync(CVMobjectVariousWord((obj)))

/* 
 * CVMobjMonitorSet()
 * f and m must be able to hold a native pointer because it is used
 * to used the variousWord value (struct CVMObjectHeader various32)
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMobjMonitorSet(obj, m, t)			\
    {							\
	CVMAddr f;					\
	f = (CVMAddr)(m);				\
	CVMassert(CVMhdrBitsSync(f) == 0);		\
	f |= t;						\
	CVMassert(CVMhdrBitsPtr(f) == (CVMAddr)m);	\
	CVMobjectVariousWord((obj)) = f;		\
    }

#define CVMobjMonitor(obj)				\
    ((CVMObjMonitor *)CVMhdrBitsPtr(CVMobjectVariousWord((obj))))

#define CVMobjectSyncKind(obj)   (CVMobjMonitorState((obj)) & 0x1)


/*
 * Use of the GC bits. The only operation supported for now:
 * increment and get value.  
 * %comment rt002
 */
#define CVMobjGcBitsPlusPlusCompare(obj, limit)			\
    (CVMobjMonitorState(obj) == CVM_LOCKSTATE_UNLOCKED ?	\
        ((CVMobjectVariousWord((obj)) += (1 << CVM_GC_SHIFT)) & ~((1<<CVM_GC_SHIFT)-1)) < ((limit) << CVM_GC_SHIFT) :	\
    (CVMobjMonitorState(obj) == CVM_LOCKSTATE_MONITOR ?		\
        ((CVMobjMonitor(obj)->bits += (1 << CVM_GC_SHIFT)) & ~((1<<CVM_GC_SHIFT)-1)) < ((limit) << CVM_GC_SHIFT) : 	\
        ((((CVMOwnedMonitor *)CVMhdrBitsPtr(CVMobjectVariousWord((obj))))->u.fast.bits \
	     += (1 << CVM_GC_SHIFT)) & ~((1<<CVM_GC_SHIFT)-1)) < ((limit) << CVM_GC_SHIFT)))

/*
 * The definition of an Object is somewhat circular. An Object is made
 * up of Object references. When accessed from C code, these slots are
 * defined to be of type ObjectICell for GC-safe access. So we give
 * the definition of an indirection cell, ICell, before we give the
 * definition of an Object.
 */
#define CVM_ICELL_DECL(typename)                          \
struct CVM ## typename ## ICell {	                      \
    CVM ## typename * volatile ref_DONT_ACCESS_DIRECTLY;     \
}

CVM_ICELL_DECL(java_lang_Object);
CVM_ICELL_DECL(Object); 

/*
 * Size of object and offsets of fields.
 * These are used in preloaded code to initialize the 
 * ClassBlock, FieldBlock and MethodBlock structures.
 * They are also used in JCC-generated header files to define
 * field offsets, as used in native code as arguments to the direct
 * and indirect memory-access functions.
 */
/* 
 * For calculating the instance size it is required to
 * use sizeof(CVMAddr) instead of sizeof(CVMUint32) which
 * is only correct on 32 bit platforms.
 *
 * If we really want to get cleaner it's probably best to go all the way
 * and do object size calculations based on the real thing: CVMJavaVal32
 */
#define OBJ_SIZE(nwords) (sizeof(CVMObjectHeader)+(nwords)*sizeof(CVMJavaVal32))
#define CVM_FIELD_OFFSET(nword) ((sizeof(CVMObjectHeader)/sizeof(CVMJavaVal32))+(nword))
#define CVM_STATIC_OFFSET(nword) (nword)

/*
 * Object monitors
 */

enum {
    CVM_OBJMON_OWNED_NEXT,	/* per-thread owned list */
    CVM_OBJMON_SCAN_NEXT,	/* GC scan linked list */
    CVM_OBJMON_NUM_LISTS
};

#define CVM_OBJMON_MAGIC 0xaa55a55a

#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
#if defined(CVM_ADV_THREAD_BOOST) && !defined(CVM_ADV_MUTEX_SET_OWNER)
#define CVM_THREAD_BOOST
#endif
#endif

typedef enum {
    CVM_OBJMON_FREE = 0,	/* on free list */
    CVM_OBJMON_BOUND = 1,	/* on bound list */
    CVM_OBJMON_OWNED = 2,	/* and on owned list */
    CVM_OBJMON_BUSY = 0x4	/* is referenced by a ee->objLockCurrent */
} CVMObjMonitorState;

#ifdef __cplusplus
static inline CVMObjMonitorState
    operator|=(CVMObjMonitorState &lhs, const CVMObjMonitorState rhs)
{
    return lhs = (CVMObjMonitorState)(lhs | rhs);
}

static inline CVMObjMonitorState
    operator&=(CVMObjMonitorState &lhs, int rhs)
{
    return lhs = (CVMObjMonitorState)(lhs & rhs);
}
#endif

struct CVMObjMonitor {
    /* 
     * 'bits' is used as temporary storage for the member 'various32'
     * of the struct 'CVMObjectHeader'. Since 'various32' is a CVMAddr
     * 'bits' has to be a CVMAddr as well.
     */
    CVMAddr bits;
    CVMProfiledMonitor mon;
#ifdef CVM_DEBUG
    CVMUint32 magic;
    CVMOwnedMonitor *owner;
#endif
    CVMBool sticky;             /* keep around */
    CVMObject *obj;		/* direct pointer */
    CVMObjMonitorState state;
    CVMObjMonitor *next;
#ifdef CVM_THREAD_BOOST
    CVMBool boost;
#ifdef CVM_DEBUG
    CVMExecEnv *boostThread;
#endif
    CVMThreadBoostRecord boostInfo;
#endif
};

extern CVMBool
CVMobjMonitorInit(CVMExecEnv *ee, CVMObjMonitor *, CVMExecEnv *owner,
    CVMUint32 count);

extern void
CVMobjMonitorDestroy(CVMObjMonitor *m);

extern CVMObjMonitor *
CVMobjectInflatePermanently(CVMExecEnv *ee, CVMObjectICell* indirectObj);

#define CVMobjMonitorOwner(m) CVMprofiledMonitorGetOwner(&(m)->mon)
#define CVMobjMonitorCount(m) CVMprofiledMonitorGetCount(&(m)->mon)

/* Purpose: Gets the CVMObject that this monitor corresponds to. */
#define CVMobjMonitorDirectObject(m) ((m)->obj)

/* Purpose: Casts a CVMProfiledMonitor into a CVMObjMonitor. */
/* NOTE: The specified CVMProfiledMonitor must have already been determined to
         be an instance of CVMObjMonitor.  This is possible because
         CVMObjMonitor is actually a subclass of CVMProfiledMonitor. */
#define CVMcastProfiledMonitor2ObjMonitor(self)    \
    ((CVMObjMonitor *)(((CVMUint8 *)(self)) -      \
                       CVMoffsetof(CVMObjMonitor, mon)))

/* Purpose: Casts a CVMObjMonitor into a CVMProfiledMonitor. */
/* NOTE: This is possible because CVMObjMonitor is a subclass of
         CVMProfiledMonitor. */
#define CVMcastObjMonitor2ProfiledMonitor(self) \
    ((CVMProfiledMonitor *)(&(self)->mon))

#define CVM_OWNEDMON_MAGIC 0xbaddabff

/* special values for CVMOwnedMonitor.count */
#define CVM_INVALID_REENTRY_COUNT ((CVMAddr)-1)
#define CVM_MAX_REENTRY_COUNT	  ((CVMAddr)-256)

typedef enum {
    CVM_OWNEDMON_FAST = 0,	/* CVMProfiledMonitor not initialized */
    CVM_OWNEDMON_HEAVY = 1	/* CVMProfiledMonitor initialized */
} CVMOwnedMonitorType;

#ifdef CVM_DEBUG
typedef enum {
    CVM_OWNEDMON_FREE = 0,	/* on free list */
    CVM_OWNEDMON_OWNED = 1	/* on owned list */
} CVMOwnedMonitorState;
#endif

struct CVMOwnedMonitor {
    CVMExecEnv *owner;
    CVMOwnedMonitorType type;
    CVMObject *object;          /* direct pointer */
    union {
	/* for fast locks */
	struct {
	    CVMAddr bits;
	} fast;
	/* for heavy-weight monitor */
	struct {
	    CVMObjMonitor *mon;
	} heavy;
    } u;
    CVMOwnedMonitor *next;
#ifdef CVM_DEBUG
    CVMUint32 magic;
    CVMOwnedMonitorState state;
#endif
#if CVM_FASTLOCK_TYPE != CVM_FASTLOCK_NONE
    /* 
     * The address of count is used as argument in 
     * CVMatomicCompareAndSwap() which expects 
     * the type volatile CVMAddr *
     * (see objsync.c, CVMfastReentryTryLock())
     * therefore the type has to be CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    volatile CVMAddr count;   /* Lock re-entry count. */
#endif
};

/* Purpose: Run all monitor scavengers. */
/* NOTE: This function should only be called after all threads are consistent
         i.e. GC safe. */
/* NOTE: ee->objLocksOwned, and ee->objLocksFreeOwned may be changed by
         CVMsyncGCSafeAllMonitorScavenge().  Do not cache these values across
         a call to this function. */
void
CVMsyncGCSafeAllMonitorScavenge(CVMExecEnv *ee);

/*
 * So an object field is either an untyped 32-bit "slot", or a GC-safe
 * object reference 
 */
/*
 * Generic 32-bit wide "Java slot" definition. This type occurs
 * in operand stacks, Java locals, object fields, constant pools.
 */
/*
 * The field 'raw' is used to access the complete content of
 * union CVMJavaVal32 (for example for copying the whole union)
 * Because CVMObjectICell is a native pointer, the datatype of
 * raw requires to be CVMAddr which is 8 byte on 64 bit platforms
 * and 4 byte on 32 bit platforms.
 */
union CVMJavaVal32 {
    CVMJavaInt     i;
    CVMJavaFloat   f;
    CVMObjectICell r;
    CVMAddr	   raw;
};

/*
 * Generic 64-bit Java value definition
 */
/* 
 * CVMmemCopy64Stub is used for copying the 'raw'
 * field of struct JavaVal32 into v.
 * Because the raw field has been changed into type CVMAddr,
 * the type has to be changed accordingly.
*/
union CVMJavaVal64 {
    CVMJavaLong   l;
    CVMJavaDouble d;
    CVMAddr	  v[2];
};

/*
 * The following two may appear in arrays
 */
union CVMJavaVal16 {
    CVMJavaShort   s;
    CVMJavaChar    c;
};

union CVMJavaVal8 {
    CVMJavaByte    b;
    CVMJavaBoolean z;
};


/*
 * The "C superclass" of all Java objects. Arrays and all types of
 * Java objects may be cast to java_lang_Object* for uniform access to
 * the header. Also, all object fields can be accessed in a type-less
 * manner using this typedef.  
 */
struct CVMjava_lang_Object {
    volatile CVMObjectHeader         hdr;
    volatile CVMJavaVal32            fields[1];
};

/*
 * Getting the first word of the object as an integer
 */
/* 
 * CVMobjectGetClassWord()
 * - obj is CVMObject*, 
 * - hdr is CVMObjectHeader (preloader_impl.h)
 * - clas is CVMClassBlock*  (object.h)
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMobjectGetClassWord(obj)  ((CVMAddr)((obj)->hdr.clas))
#define CVMobjectSetClassWord(obj, word)  \
    ((obj)->hdr.clas = (CVMClassBlock*)(word))

/*
 * Getting the class of an object
 */
#define CVM_OBJECT_CLASS_MASK               ~3
#define CVMobjectGetClass(obj) \
    ((CVMClassBlock*)(CVMobjectGetClassWord(obj) & CVM_OBJECT_CLASS_MASK))
#define CVMobjectGetClassFromClassWord(word) \
    ((CVMClassBlock*)((word) & CVM_OBJECT_CLASS_MASK))

/*
 * This is how you check for the ROM and sync properties of an object
 */
#define CVM_OBJECT_MARKED_KIND_SHIFT	0
#define CVM_OBJECT_IN_ROM_SHIFT 	1
#define CVM_OBJECT_MARKED_KIND_MASK	(1 << CVM_OBJECT_MARKED_KIND_SHIFT)
#define CVM_OBJECT_IN_ROM_MASK       	(1 << CVM_OBJECT_IN_ROM_SHIFT)

#define CVMobjectHeaderIsInROM(obj) \
    ((CVMobjectGetClassWord(obj) & CVM_OBJECT_IN_ROM_MASK) != 0)

#ifdef CVM_DEBUG_ASSERTS
#define CVMobjectIsInROM(obj) \
    (CVMobjectHeaderIsInROM(obj) ? \
     (CVMassert(CVMpreloaderReallyInROM(obj)), CVM_TRUE) : \
     (CVMassert(!CVMpreloaderReallyInROM(obj)), CVM_FALSE))
#else
#define CVMobjectIsInROM CVMobjectHeaderIsInROM
#endif

/*
 * Set, reset and check the marked bit on an object
 */
#define CVMobjectMarked(obj) \
    ((CVMBool)((CVMobjectGetClassWord(obj) & CVM_OBJECT_MARKED_KIND_MASK) >> CVM_OBJECT_MARKED_KIND_SHIFT))

#define CVMobjectSetMarked(obj) \
    (CVMobjectSetClassWord(obj, \
        CVMobjectGetClassWord(obj) | CVM_OBJECT_MARKED_KIND_MASK))

#define CVMobjectClearMarked(obj) \
    (CVMobjectSetClassWord(obj, \
        CVMobjectGetClassWord(obj) & ~CVM_OBJECT_MARKED_KIND_MASK))

#define CVMobjectMarkedOnClassWord(word) \
    (((word) & CVM_OBJECT_MARKED_KIND_MASK) >> CVM_OBJECT_MARKED_KIND_SHIFT)
#define CVMobjectSetMarkedOnClassWord(word) \
    ((word) |= CVM_OBJECT_MARKED_KIND_MASK)
#define CVMobjectClearMarkedOnClassWord(word) \
    ((word) &= ~CVM_OBJECT_MARKED_KIND_MASK)

/*
 * The default value of the marked bit is this
 */
#define CVM_DEFAULT_MARKED_TYPE	0

/*
 * The following is a "sync functions" type. There are initially two
 * of these that I can think of -- a deterministic locker, and a fast
 * locker. The first one does priority inheritance in the way an
 * RT-spec would require. The second one does fast sync a la
 * Hotspot/ExactVM, with a user-level fast lock for the common,
 * uncontended case. The global syncKinds array lists those.
 *
 * Each of the locking functions returns TRUE on success and FALSE on
 * failure.  
 */
struct CVMSyncVector {
    CVMTryLockFunc   *tryLock;
    CVMLockFunc      *lock;
    CVMTryLockFunc   *tryUnlock;
    CVMLockFunc      *unlock;
    CVMWaitFunc      *wait;
    CVMNotifyFunc    *notify;
    CVMNotifyAllFunc *notifyAll;
    void             *dummyWord;  /* to make the size of this structure
				     a multiple of 8 */
};
    
/*
 * An array of sync CV-tables. Currently with a fast entry and a
 * deterministic entry.
 */
extern const CVMSyncVector CVMsyncKinds[];

/*
 * Macros for accessing an objects sync functions.
 */
#define CVMobjectSyncVector(obj)   CVMsyncKinds[CVMobjectSyncKind(obj)]
#define CVMicellSyncVector(ee, ic)  \
    CVMobjectSyncVector(CVMID_icellDirect(ee, ic))

/*
 * CVM_TIMEOUT_INFINITY is used to set the timeout for an objectWait
 * to an infinite value.
 */
#define CVM_TIMEOUT_INFINITY 0

/*
 * WARNING: All of these macros require you to be gcunsafe, even the
 * ones that take an CVMObjectICell*. The interpreter must also have
 * its state flushed before calling any of the ones that take an
 * CVMObjectICell*.
 */
#define CVMobjectTryLock(ee, obj)   \
	CVMobjectSyncVector(obj).tryLock(ee, obj)

#define CVMobjectTryUnlock(ee, obj) \
	CVMobjectSyncVector(obj).tryUnlock(ee, obj)

#define CVMobjectLock(ee, ic)	    \
	CVMicellSyncVector(ee, ic).lock(ee, ic)

#define CVMobjectUnlock(ee, ic)	    \
	CVMicellSyncVector(ee, ic).unlock(ee, ic)

#define CVMobjectWait(ee, ic, ms)   \
	CVMicellSyncVector(ee, ic).wait(ee, ic, ms)

#define CVMobjectNotify(ee, ic)     \
	CVMicellSyncVector(ee, ic).notify(ee, ic)   

#define CVMobjectNotifyAll(ee, ic)  \
	CVMicellSyncVector(ee, ic).notifyAll(ee, ic)

/*
 * Direct calls to the fast lock and unlock routines
 */
extern CVMTryLockFunc CVMfastTryLock, CVMfastTryUnlock;

/* Checks to see if the current thread owns a lock on the specified object.
   This function can become GC safe. */
extern CVMBool CVMobjectLockedByCurrentThread(CVMExecEnv *ee,
                                              CVMObjectICell *objICell);

extern CVMBool CVMgcSafeObjectLock(CVMExecEnv *ee, CVMObjectICell *o);
extern CVMBool CVMgcSafeObjectUnlock(CVMExecEnv *ee, CVMObjectICell *o);
extern CVMBool CVMgcSafeObjectNotify(CVMExecEnv *ee, CVMObjectICell *o);
extern CVMBool CVMgcSafeObjectNotifyAll(CVMExecEnv *ee, CVMObjectICell *o);
extern CVMBool CVMgcSafeObjectWait(CVMExecEnv *ee, CVMObjectICell *o,
				   CVMInt64 millis);

/* CVMMicroLock APIs for object locking */

extern void CVMobjectMicroLock(CVMExecEnv *ee, CVMObject *obj);
extern void CVMobjectMicroUnlock(CVMExecEnv *ee, CVMObject *obj);

#ifndef CVM_DEBUG
#define CVMobjectMicroLock(ee, obj)     CVMobjectMicroLockImpl(obj)
#define CVMobjectMicroUnlock(ee, obj)   CVMobjectMicroUnlockImpl(obj)
#endif

#define CVMobjectMicroLockImpl(obj) \
    CVMmicrolockLock(&CVMglobals.objGlobalMicroLock)

#define CVMobjectMicroUnlockImpl(obj) \
    CVMmicrolockUnlock(&CVMglobals.objGlobalMicroLock)

#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
/* 
 * Simple Sync helper function for unlocking in Simple Sync methods when
 * there is contention for the lock after locking. It is only needed for
 * CVM_FASTLOCK_ATOMICOPS since once locked, CVM_FASTLOCK_MICROLOCK
 * will never allow for contention of Simple Sync methods.
 */
extern void
CVMsimpleSyncUnlock(CVMExecEnv *ee, CVMObject* obj);
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

/*
 * Array declarations. They look exactly like objects, (they can be
 * cast to an Object* for uniform access to the header when needed)
 * except they have an additional length field.  
 */
/* 
 * The member elems in all CVMArrayOf<type> structs 
 * must have the same offset. This is the case on 32 bit platforms 
 * but not automatically on 64 bit platforms.
 */
#ifdef CVM_64
#define CVM_ARRAY_DECL(name, elem_type)		\
struct CVMArrayOf##name {			\
    volatile CVMObjectHeader hdr;		\
    volatile CVMJavaInt      length;		\
    volatile CVMUint32       pad;               \
    volatile elem_type       elems[1];		\
}
#else
#define CVM_ARRAY_DECL(name, elem_type)		\
struct CVMArrayOf##name {			\
    CVMObjectHeader hdr;			\
    CVMJavaInt      length;			\
    elem_type       elems[1];			\
}
#endif

/*
 * The CVMArrayInfo struct stores information about arrays that is
 * not needed for non array types.
 */
struct CVMArrayInfo {
    CVMUint16      depth;
    CVMBasicType   baseType;
    CVMClassBlock* baseCb;
    CVMClassBlock* elementCb;
};

#define CVMarrayDepth(arrayCb)        \
  (*(CVMassert(CVMisArrayClass(arrayCb)), &CVMcbArrayInfo(arrayCb)->depth))
#define CVMarrayBaseType(arrayCb)     \
  (*(CVMassert(CVMisArrayClass(arrayCb)), &CVMcbArrayInfo(arrayCb)->baseType))
#define CVMarrayBaseCb(arrayCb)       \
  (*(CVMassert(CVMisArrayClass(arrayCb)), &CVMcbArrayInfo(arrayCb)->baseCb))
#define CVMarrayElementCb(arrayCb)    \
  (*(CVMassert(CVMisArrayClass(arrayCb)), &CVMcbArrayInfo(arrayCb)->elementCb))

/*
 * Given an array CB, return element size in that array
 */
#define CVMarrayElemSize(arrayCb)			\
    (CVMisArrayOfArrayClass(arrayCb) ?			\
	sizeof(CVMObject*) :				\
        CVMbasicTypeSizes[CVMarrayBaseType(arrayCb)])

#define CVMarrayElemTypeCode(arrayCb)		\
    (CVMisArrayOfArrayClass(arrayCb) ?		\
	CVM_T_CLASS :				\
        CVMarrayBaseType(arrayCb))

/* 
 * Keep the correct alignment for the next object/array by using 
 * CVMalignAddrUp() instead of CVMalignWordUp().
 */
#define CVMobjectSizeGivenClass(obj, cb)				 \
    (CVMisArrayClass(cb) ?						 \
    CVMalignAddrUp(CVMoffsetof(CVMArrayOfAnyType, elems) +		 \
        CVMarrayElemSize(cb) * 			 			 \
        ((CVMArrayOfAnyType*)(obj))->length) 				 \
    :				 					 \
    CVMcbInstanceSize(cb))
     
#define CVMobjectSize(obj) \
    CVMobjectSizeGivenClass(obj, CVMobjectGetClass(obj))

#define CVMisArrayObject(obj)  (CVMisArrayClass(CVMobjectGetClass((obj))))
#define CVMisArrayClass(cb)    CVMtypeidIsArray(CVMcbClassName(cb))

#define CVMisArrayClassOfBasicType(cb, basicTypeSig)			     \
     (CVMisArrayClass(cb) && (CVMtypeidGetArrayDepth(CVMcbClassName(cb))==1) \
      && (CVMtypeidGetArrayBasetype(CVMcbClassName(cb)) == basicTypeSig))

#define CVMisArrayOfArrayClass(cb)  				\
     (CVMisArrayClass(cb) && (CVMtypeidGetArrayDepth(CVMcbClassName(cb))>1) )

#define CVMisArrayOfClass(cb)  					             \
     (CVMisArrayClass(cb) && (CVMtypeidGetArrayDepth(CVMcbClassName(cb))==1) \
      && (CVMtypeidFieldIsRef(CVMtypeidGetArrayBasetype(CVMcbClassName(cb)))))

/*
 */
#define CVMisArrayOfAnyBasicType(cb)			            	     \
     (CVMisArrayClass(cb) && (CVMtypeidGetArrayDepth(CVMcbClassName(cb))==1) \
      && (!CVMtypeidFieldIsRef(CVMtypeidGetArrayBasetype(CVMcbClassName(cb)))))

/*
 * The following type to make array declarations for double and long
 * arrays work.
 */
typedef CVMJavaVal32 CVMTwoJavaWords[2];

CVM_ARRAY_DECL(Byte,    CVMJavaByte);      /* CVMArrayOfByte    */
CVM_ARRAY_DECL(Short,   CVMJavaShort);     /* CVMArrayOfShort   */
CVM_ARRAY_DECL(Char,    CVMJavaChar);      /* CVMArrayOfChar    */
CVM_ARRAY_DECL(Boolean, CVMJavaBoolean);   /* CVMArrayOfBoolean */
CVM_ARRAY_DECL(Int,     CVMJavaInt);       /* CVMArrayOfInt     */
CVM_ARRAY_DECL(Ref,     CVMObjectICell);   /* CVMArrayOfRef     */
CVM_ARRAY_DECL(Float,   CVMJavaFloat);     /* CVMArrayOfFloat   */
CVM_ARRAY_DECL(Long,    CVMTwoJavaWords);  /* CVMArrayOfLong    */
CVM_ARRAY_DECL(Double,  CVMTwoJavaWords);  /* CVMArrayOfDouble  */
CVM_ARRAY_DECL(AnyType, CVMJavaVal32);     /* CVMArrayOfAnyType */

CVM_ICELL_DECL(ArrayOfByte);
CVM_ICELL_DECL(ArrayOfShort);
CVM_ICELL_DECL(ArrayOfChar);
CVM_ICELL_DECL(ArrayOfBoolean);
CVM_ICELL_DECL(ArrayOfInt);
CVM_ICELL_DECL(ArrayOfRef);
CVM_ICELL_DECL(ArrayOfFloat);
CVM_ICELL_DECL(ArrayOfLong);
CVM_ICELL_DECL(ArrayOfDouble);
CVM_ICELL_DECL(ArrayOfAnyType);

#endif /* _INCLUDED_OBJECTS_H */
