/*
 * @(#)classes.h	1.284 06/10/27
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
 * This file includes the definitions of interpeter's class file related
 * data structures: CVMClassBlock, CVMMethodBlock, and CVMFieldBlock.
 * Constant pool related data structures are in constantpool.h.
 */

#ifndef _INCLUDED_CLASSES_H
#define _INCLUDED_CLASSES_H

#include "javavm/export/jni.h"
#include "javavm/include/defs.h"
#include "javavm/include/typeid.h"
#include "javavm/include/constantpool.h"

#ifdef CVM_JIT
#include "javavm/include/jit/jitpcmap.h"
#include "javavm/include/jit/jitinlineinfo.h"
#include "javavm/include/porting/jit/jit.h"
#endif

#undef CVM_METHODBLOCK_HAS_CB

/*
 * CVMGCBitMap - this class is used to specify a bit map for a java
 * object. If a bit is set then the field at that offset is an object
 * reference which must be scanned. If the object has more than 30
 * fields then the CVMGCBitMap points to a bitmap. See gc_common.h for
 * details.
 */
/* 
 * Even if we don't use more than 32 bits in 'map' it necessary to
 * use the type CVMAddr since only this makes sure that the low bit
 * of 'map' is is the same bit as the low bit of 'bigmap' regardless
 * of the endianess.
 * This becomes an issue in a 64-bit port.
 */
union CVMGCBitMap {
    CVMAddr            map; /* low bit set indicates pointer to BigGCBitMap */
    CVMBigGCBitMap*    bigmap;
};

/* 
 * If the type CVMAddr is used for 'map' in 'CVMGCBitMap' it has to
 * be used here as well.
 */
struct CVMBigGCBitMap {
    CVMUint16         maplen;
    CVMAddr           map[1];
};

#ifdef CVM_JVMTI
typedef struct CVMClassBlockData CVMClassBlockData;
struct CVMClassBlockData {
    CVMClassBlockData *nextCBData;
    CVMClassBlock *currentCb;  /* Pointer to *old* class block
                                * It contains *new* methods and new
                                * constantpool.
                                */
};
#endif

/*
 * CVMClassBlock - The main data structure for storing information about
 * a java class. Unlike some other VM's, this data type is not an instance
 * of java/lang/Class. The java/lang/Class instance (stored in the
 * javaInstance field, is simply a 1 field java class that points
 * back to its CVMClassBlock.
 *
 * WARNING: if you change anything in this structure then you need to
 * fix up CVM_INIT_CLASSBLOCK.
 */
struct CVMClassBlock {
    /* bit map of object fields containing references */
    CVMGCBitMap     gcMapX;

    CVMClassTypeID  classNameX; /* see typeid.h */

    /* If the class has reached the SUPERCLASS_LOADED state, then
     * superclassX will contain an CVMClassBlock*. Otheriwse it contains
     * the typeID of the superclass.
     */
    union {
	CVMClassBlock*  superclassCb;     /* after loading superclass */
	CVMClassTypeID  superclassTypeID; /* before loading superclass */
	/*
	 * Number of mirnada methods for this class.
	 * Only valid after the class has been added to the list of
	 * classes to free. See staticsX.freeClassLink.
	 */
	CVMUint16       mirandaMethodCountX; /* during class unloading */
    } superclassX;

    union {
	CVMConstantPool* constantpoolX;
	CVMArrayInfo*    arrayInfoX;
    } cpX;

    CVMInterfaces*   interfacesX; /* null if no interfaces supported */
    CVMMethodArray*  methodsX;    /* array of class methods */
    CVMFieldArray*   fieldsX;     /* array of class fields */

    union {
	/* The statics area should have 64 bit statics grouped at the
	 * beginning so they'll all be 64 bit aligned 
	 */
	CVMJavaVal32*    statics;
	/*
	 * The freeClassLink field is used to link the class into
	 * CVMglobals.freeClassList when the class unloading code
	 * detects that the class is no longer referenced.
	 */
	CVMClassBlock*   freeClassLink;
    } staticsX;

    CVMUint16        constantpoolCountX;/* # entries in constantpool */
    CVMUint16        methodCountX;	/* # entries in methods array */
    CVMUint16        fieldCountX;       /* # entries in fields array */
    CVMUint16        methodTableCountX; /* # of entries in methodTable array */
					/* but see also CVMcbBasicTypeCode   */
    CVMUint16	     accessFlagsX;      /* access using CVMcbIs macro */
    volatile CVMUint8 runtimeFlagsX;    /* Runtime flags except INITIALIZED 
					 * and ERROR state. See notes below
					 * around the CVM_CLASS_* flag
					 * definitions for detail */
#ifdef CVM_LVM
    CVMUint16        lvmClassIndexX;    /* Reserved for future use. */
					/* %begin lvm
					 * Used by LVM. Non-zero if the class
					 * is subject to LVM isolation.
					 * This index is for retrieving 
					 * per-LVM class info (e.g. clinitEE)
					 * from a per-LVM info buffer in 
					 * LVM-enabled mode.
					 * %end lvm */
#endif
    CVMUint16        instanceSizeX;     /* instance size in bytes */
    CVMUint16        numStaticRefsX;    /* # of statics that are ref types,
					   but only for dynamic classes */
    CVMClassICell*   javaInstanceX;      /* java.lang.Class instance */

#ifdef CVM_DEBUG
    char*           theNameX; /* A DEBUG-ONLY string representation */
#endif

#ifdef CVM_CLASSLOADING
    CVMClassLoaderICell*  classLoaderX;  /* java.lang.ClassLoader    */
    CVMObjectICell*  protectionDomainX;  /* java.security.ProtectionDomain */
#endif

    union {
	/* If this class is currently having it's <clinit> method run, then
	 * clinitEE refers to the ee of the thread that is running the
	 * clinit method. We need to handle ROM classes differently since
	 * we can't modify the clinitEE of a ROM class at runtime.
	 * 
	 * For convenience, the least significant two bits of clinitEE.ee 
	 * (or *clinitEE.eePtr in ROMized case) contain the initialized 
	 * and error flags.
	 */
	CVMExecEnv** eePtr;    /* for ROM classes */
	CVMExecEnv*  ee;       /* for RAM classes */
	/*
	 * Pointer to the classTable slot that points to this class.
	 * Only valid after the class has been added to the list of
	 * classes to free. See staticsX.freeClassLink. Note that this
	 * never conflicts with the use of the ee or eePtr fields since
	 * they are only used while running the <clinit>, which can't
	 * possibly happen if the class is about to be unloaded.
	 */
	CVMClassBlock** classTableSlotPtr;
    } clinitEEX;

    /* All checked exceptions for all methods in the class are grouped
       into one array to save memory. Each method will have an offset
       into the checkedExceptions array. The first entry at the offset
       is the number of checked exceptions for the method, followed by
       all checked exceptions for the method. If two method have the
       same checked exceptions, they may both have the same offset into
       the checkedExceptions array (saving even more memory). Index 0
       of checkedExcpetions always contains 0. If a method specifies
       offset 0 then it has no checked exceptions. If no class methods
       have checked exceptions, then checkedExcpetions will be NULL.
       */
    CVMUint16*	     checkedExceptionsX;

#ifdef CVM_DEBUG_CLASSINFO
    char*           sourceFileNameX;  /* source file name */
#endif
    CVMMethodBlock** methodTablePtrX;

    /* Pointer to inner classes information. NULL if there is none. */
    /* %comment c */
    CVMInnerClassesInfo* innerClassesInfoX;

    /* class file version <major_version>.<minor_version> */
    CVMUint16        major_version;
    CVMUint16        minor_version;
#ifdef CVM_JVMTI
    CVMClassBlockData*   oldCBData;
#endif
};

#ifdef CVM_CLASSLOADING
#define CVMcbClassLoader(cb)	    ((cb)->classLoaderX)
#define CVMcbProtectionDomain(cb)   ((cb)->protectionDomainX)
#else
#define CVMcbClassLoader(cb)	    ((CVMClassLoaderICell*)NULL)
#define CVMcbProtectionDomain(cb)   ((CVMObjectICell*)NULL)
#endif

#define CVMcbAccessFlags(cb)        ((cb)->accessFlagsX)
#define CVMcbArrayInfo(cb)          ((cb)->cpX.arrayInfoX)
#define CVMcbCheckedExceptions(cb)  ((cb)->checkedExceptionsX)
#define CVMcbClassTableSlotPtr(cb)  ((cb)->clinitEEX.classTableSlotPtr)
#define CVMcbClassName(cb)	    ((cb)->classNameX)
#ifdef CVM_DEBUG
#define CVMcbClassNameString(cb)    ((cb)->theNameX)
#endif
#define CVMcbConstantPool(cb)	    ((cb)->cpX.constantpoolX)
#define CVMcbConstantPoolCount(cb)  ((cb)->constantpoolCountX)
#define CVMcbFieldCount(cb) 	    ((cb)->fieldCountX)
#define CVMcbFreeClassLink(cb)      ((cb)->staticsX.freeClassLink)
#define CVMcbGcMap(cb) 	            ((cb)->gcMapX)
#define CVMcbImplementsCountRaw(cb)					\
    (CVMcbInterfaces(cb) == NULL					\
     ? 0								\
     : CVMcbInterfaces(cb)->implementsCountX)

#define CVMcbImplementsCount(cb)					\
    (CVMcbInterfaces(cb) == NULL					\
     ? 0								\
     : (CVMcbInterfaces(cb) == CVMcbInterfaces(CVMcbSuperclass(cb))	\
	? 0 /* CVMcbInterfaces was inherited from the parent */		\
	: CVMcbInterfaces(cb)->implementsCountX))

#define CVMcbIsSystemClass(cb)      (CVMcbClassLoader(cb) == NULL)
#ifdef CVM_LVM /* %begin lvm */
#define CVMcbLVMClassIndex(cb)	    ((cb)->lvmClassIndexX)
#define CVMcbIsLVMSystemClass(cb)   (CVMcbIsSystemClass(cb))
#else /* %end lvm */
#define CVMcbIsLVMSystemClass(cb)   (CVM_FALSE)
#endif /* %begin,end lvm */

#define CVMcbInstanceSize(cb) 	    ((cb)->instanceSizeX)
#define CVMcbInterfacecb(cb,idx)    (CVMcbInterfaces(cb)->itable[idx]. \
				     interfaceCb)
#define CVMcbInterfaceItable(cbintf) ((cbintf)->itable)
#define CVMcbInterfacecbGivenItable(itable,idx) \
    ((itable)[idx].interfaceCb)
#define CVMcbInterfaceMethodTableIndices(cb, idx)			\
     				    (CVMcbInterfaces(cb)->itable[idx].	\
				     intfInfoX.methodTableIndicesX)
#define CVMcbInterfaceMethodTableIndex(cb, idx, methodIdx)		      \
     				    (CVMcbInterfaceMethodTableIndices(cb, idx)\
				     [methodIdx])
#define CVMcbInterfaceMethodTableIndicesGivenInterfaces(cbintf, idx)	\
     				    ((cbintf)->itable[idx].		\
                                     intfInfoX.methodTableIndicesX)
#define CVMcbInterfaceMethodTableIndexGivenInterfaces(cbintf, idx, methodIdx) \
    (CVMcbInterfaceMethodTableIndicesGivenInterfaces(cbintf, idx)[methodIdx])
#define CVMcbInterfaceTypeID(cb,idx) \
                                    (CVMcbInterfaces(cb)->itable[idx]. \
				     intfInfoX.interfaceTypeIDX)
#define CVMcbInterfaceCount(cb)     (CVMcbInterfaces(cb) == NULL	     \
				     ? 0 				     \
				     : CVMcbInterfaces(cb)->interfaceCountX)
#define CVMcbInterfaceCountGivenInterfaces(cbintf) 			     \
                                     ((cbintf) == NULL			     \
				     ? 0 				     \
				     : (cbintf)->interfaceCountX)
#define CVMcbInterfaces(cb)         ((cb)->interfacesX)
#define CVMcbJavaInstance(cb)       ((cb)->javaInstanceX)
#define CVMcbMethodCount(cb) 	    ((cb)->methodCountX)
#define CVMcbMethodTableCount(cb)   ((cb)->methodTableCountX)
#define CVMcbMethodTablePtr(cb)     ((cb)->methodTablePtrX)
#define CVMcbMirandaMethodCount(cb) ((cb)->superclassX.mirandaMethodCountX)
#define CVMcbNumStaticRefs(cb) 	    ((cb)->numStaticRefsX)
#define CVMcbStatics(cb)            ((cb)->staticsX.statics)
#define CVMcbSuperclass(cb)  	    ((cb)->superclassX.superclassCb)
#define CVMcbSuperclassTypeID(cb)   ((cb)->superclassX.superclassTypeID)
#define CVMcbSourceFileName(cb)     ((cb)->sourceFileNameX)

#define CVMcbMethodSlot(cb, idx) \
    (&CVMcbMethods(cb)->ranges[(idx) >> 8].mb[(idx) & 0xff])
#define CVMcbFieldSlot(cb, idx)  \
    (&CVMcbFields(cb)->ranges[(idx) >> 8].fb[(idx) & 0xff])

#define CVMcbMethodTableSlot(cb, idx)    \
    (CVMcbMethodTablePtr(cb)[idx])
#define CVMobjMethodTableSlot(obj, idx)  \
    (CVMcbMethodTableSlot(CVMobjectGetClass(obj), idx))

/*
 * This is what CVMobjMethodTableSlot() used to look like before we made
 * the methodTable a ptr field rather than putting the entire methodTable
 * at the end of the cb (and leaving arrays without any methodTable).
 */
#if 0
#define CVMobjMethodTableSlot(obj, idx)					\
    (CVMisArrayObject(obj) ?						\
     CVMcbMethodTableSlot(CVMsystemClass(java_lang_Object), idx) :	\
     CVMcbMethodTableSlot(CVMobjectGetClass(obj), idx))
#endif

#ifdef CVM_JVMTI
#define CVMcbOldData(cb)            ((cb)->oldCBData)
#endif

/* Only the classloader and the above macros should use these macros. */
/* JVMTI RedefineClass API uses these as well */
#define CVMcbMethods(cb)            ((cb)->methodsX)
#define CVMcbFields(cb)             ((cb)->fieldsX)

/* We need "setter" macros for these 2 fields because the existing access
 * macros for them cannot be used as lvals.
 */
#define CVMcbSetInterfaceCount(cb, count)  \
    ((void)(CVMcbInterfaces(cb)->interfaceCountX = count))
#define CVMcbSetImplementsCount(cb, count) \
    ((void)(CVMcbInterfaces(cb)->implementsCountX = count))

#define CVMcbSetAccessFlag(cb, flag) \
    ((void)(CVMcbAccessFlags(cb) |= CVM_CLASS_ACC_##flag))

/*
 * Runtime flags for classes:
 *
 * Most of runtime flags are stored in the cb->runtimeFlagsX field.
 * Since this field of romized classes cannot be modified at runtime, 
 * and some romized classes need to record the initialized and the error
 * state, these two flags are rather stored in the clinitEE.ee 
 * (or *clinitEE.eePtr in ROMized case) field using the least significant 
 * two bits for convenience.
 *
 * In a number of places we cast away the constness of a romized
 * CVMClassBlock in order to store a pointer to the CVMClassBlock
 * in a field that is also used to point to non-const CVMClassBlocks.
 * This is safe because an invariant of the VM is that we won't write
 * into any CVMClassBlock that is const. This is easy to ensure because
 * the only time we ever write into an CVMClassBlock (after it is
 * initialized) is to update the rutime flags.
 */

/* class is ROMized. */
#define CVM_CLASS_IN_ROM                0x01
/* class or one of its super classes has static field(s) or <clinit>. */
#define CVM_CLASS_HAS_STATICS_OR_CLINIT 0x02
/* superclass and interfaces have  been loaded */
#define CVM_CLASS_SUPERCLASS_LOADED     0x04
/* fields have been prepared. */
#define CVM_CLASS_FIELDS_PREPARED       0x08
/* dynamically loaded class did not declare at least one interface, so
 * its CVMcbInterfaces belongs to its superclass and should not be
 * freed when the class is unloaded */
#define CVM_CLASS_INHERITED_INTERFACES  0x10
/* interfaces, methods and fields  have ben preprared. */
#define CVM_CLASS_LINKED                0x20
/* class is verified. */
#define CVM_CLASS_VERIFIED              0x40
/* class has been scanned by GC */
#define CVM_CLASS_GC_SCANNED            0x80

/* Flag setting for a ROMized class that doesn't have <clinit> nor
 * static fields. All the flags set except HAS_STATICS_OR_CLINIT. */
#define CVM_ROMCLASS_INIT         ((CVMUint8)~CVM_CLASS_HAS_STATICS_OR_CLINIT)
 /* Flag setting for a ROMized class with <clinit> and/or static field(s). */
#define CVM_ROMCLASS_WCI_INIT         ((CVMUint8)~0)

#define CVMcbRuntimeFlags(cb)         ((cb)->runtimeFlagsX)

#define CVMcbCheckRuntimeFlag(cb, flag)               \
    ((CVMcbRuntimeFlags(cb) & CVM_CLASS_##flag) != 0)

/* 
 * We must become unsafe to avoid a race condition with gc setting or
 * clearing the SCANNED bit. Note that because of the use of gcUnsafeExec,
 * runtimeFlagsX must be declared volatile or accesses to it could be moved
 * outside of the gcUnsafeExec block.
 */
#define CVMcbSetRuntimeFlag(cb, ee_, flag) {		\
    CVMD_gcUnsafeExec(ee_, {				\
        CVMcbRuntimeFlags(cb) |= CVM_CLASS_##flag;	\
    });							\
}
#define CVMcbClearRuntimeFlag(cb, ee_, flag) {		\
    CVMD_gcUnsafeExec(ee_, {				\
        CVMcbRuntimeFlags(cb) &= ~CVM_CLASS_##flag;	\
    });							\
}

#define CVMcbGcScanned(cb) \
    ((((cb)->runtimeFlagsX) & CVM_CLASS_GC_SCANNED) != 0)

#define CVMcbSetGcScanned(cb) \
    ((void)(((cb)->runtimeFlagsX) |= CVM_CLASS_GC_SCANNED))

#define CVMcbClearGcScanned(cb) \
    (((cb)->runtimeFlagsX) &= ~CVM_CLASS_GC_SCANNED)

#define CVMcbIsInROM(cb)          (CVMcbCheckRuntimeFlag(cb, IN_ROM))
#define CVMcbHasStaticsOrClinit(cb) \
    CVMcbCheckRuntimeFlag(cb, HAS_STATICS_OR_CLINIT)
#define CVMcbSetHasStaticsOrClinit(cb, ee) \
    CVMcbSetRuntimeFlag(cb, ee, HAS_STATICS_OR_CLINIT)

/*
 * Store the error and the initialized flags using the least significant
 * two bits of clinitEE.ee (or *clinitEE.eePtr in ROMized case). 
 * This is needed for romized classes. See also the notes above around 
 * the definition of the CVM_CLASS_* flags.
 */
#define CVM_CLINITEE_CLASS_ERROR_FLAG  0x1 /* CVMclassLoadSuperclasses() or
					    * <clinit> caused an error. Look
					    * at SUPERCLASS_LOADED flag to
					    * determine which. */
#define CVM_CLINITEE_CLASS_INIT_FLAG   0x2 /* static initializers have been
					    * run. */
#define CVM_CLINITEE_FLAGS_MASK \
    (CVM_CLINITEE_CLASS_ERROR_FLAG | CVM_CLINITEE_CLASS_INIT_FLAG)

/* 
 * clinitEE is cast to a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMclinitEEPurifyX(clinitEE) \
    ((CVMExecEnv*)((CVMAddr)(clinitEE) & ~CVM_CLINITEE_FLAGS_MASK))
/* 
 * clinitEE is casted to a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMclinitEEGetFlagsX(clinitEE) \
    ((CVMAddr)(clinitEE) & CVM_CLINITEE_FLAGS_MASK)

#ifdef CVM_LVM /* %begin lvm */

/* In LVM-enabled mode, we replicate clinitEE field per LVM,
 * which is accessed via 'ee'. */
#define CVMcbLVMROMClassClinitEERawX(ee, cb)	\
    (((CVMExecEnv**)CVMLVMeeStatics(ee))[CVMcbLVMClassIndex(cb)])

/*
 * CVMcbLVMROMClassClinitEERawX(cb) is casted to a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMcbSetROMClassInitializationFlag(ee_, cb)		\
    ((void)(CVMassert(CVMcbLVMClassIndex(cb) > 0),		\
	    CVMcbLVMROMClassClinitEERawX((ee_), (cb))		\
	 = (CVMExecEnv*)((CVMAddr)CVMcbLVMROMClassClinitEERawX((ee_), (cb)) \
			 | CVM_CLINITEE_CLASS_INIT_FLAG)))

#define CVMcbClinitEERawX(ee_, cb)			\
    (*((CVMcbLVMClassIndex(cb) > 0) ?			\
       &(CVMcbLVMROMClassClinitEERawX((ee_), (cb)))	\
       : &((cb)->clinitEEX.ee)))

#else /* %end lvm */

#define CVMcbROMClassClinitEERawX(cb) \
    (*(cb)->clinitEEX.eePtr)
#define CVMcbGetROMClassClinitEE(cb) \
    CVMclinitEEPurifyX(CVMcbROMClassClinitEERawX(cb))
/*
 * CVMcbROMClassClinitEERawX(cb) returns a native pointer of type
 * CVMExecEnv*
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMcbROMClassInitializationDoneFlag(cb)			\
    (((CVMAddr)CVMcbROMClassClinitEERawX(cb)			\
			& CVM_CLINITEE_CLASS_INIT_FLAG) != 0)
/*
 * CVMcbROMClassClinitEERawX(cb) is casted to a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMcbSetROMClassInitializationFlag(ee_, cb)		\
    ((void)((void)(ee_),					\
	    CVMcbROMClassClinitEERawX(cb)			\
	        = (CVMExecEnv*)((CVMAddr)CVMcbROMClassClinitEERawX(cb) \
				| CVM_CLINITEE_CLASS_INIT_FLAG)))

#define CVMcbClinitEERawX(ee_, cb) \
    (*((void)(ee_), (CVMcbIsInROM(cb) ? &(CVMcbROMClassClinitEERawX(cb)) \
	: &((cb)->clinitEEX.ee))))

#endif /* %begin,end lvm */

#define CVMcbGetClinitEE(ee, cb) \
    CVMclinitEEPurifyX(CVMcbClinitEERawX(ee, cb))

/*
 * clinitEE is casted to a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMcbSetClinitEE(ee, cb, clinitEE) {				\
    CVMcbClinitEERawX(ee, cb) = (CVMExecEnv*)((CVMAddr)clinitEE	\
			| CVMclinitEEGetFlagsX(CVMcbClinitEERawX(ee, cb))); \
    CVMassert(CVMcbGetClinitEE(ee, cb) == clinitEE);			\
}

/*
 * CVMcbClinitEERawX(ee, cb) is casted to a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMcbSetInitializationDoneFlag(ee, cb)				\
    ((void)(CVMcbClinitEERawX(ee, cb)					\
		= (CVMExecEnv*)((CVMAddr)CVMcbClinitEERawX(ee, cb)	\
			| CVM_CLINITEE_CLASS_INIT_FLAG)))

/*
 * CVMcbClinitEERawX(ee, cb) returns the member ee in
 * struct CVMClassBlock which is a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMcbInitializationDoneFlag(ee, cb)				\
    (((CVMAddr)CVMcbClinitEERawX(ee, cb)				\
	& CVM_CLINITEE_CLASS_INIT_FLAG) != 0)

/*
 * CVMcbClinitEERawX(ee, cb) is casted to a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMcbSetErrorFlag(ee, cb)					\
    ((void)(CVMcbClinitEERawX(ee, cb)					\
		= (CVMExecEnv*)((CVMAddr)CVMcbClinitEERawX(ee, cb)	\
			| CVM_CLINITEE_CLASS_ERROR_FLAG)))

/*
 * CVMcbClinitEERawX(ee, cb) returns the member ee in
 * struct CVMClassBlock which is a native pointer
 * therefore change the cast to CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVMcbCheckErrorFlag(ee, cb)	   \
    (((CVMAddr)CVMcbClinitEERawX(ee, cb) \
	& CVM_CLINITEE_CLASS_ERROR_FLAG) != 0)

/*
 * CVMcbCheckInitNeeded() - Macro for checking if checkinit_quick 
 * version of opcode should be used. 
 *
 * %begin lvm
 * If Logical VM is activated, we need to use checkinit_quick opcodes
 * for those classes that are subject to LVM isolation (system classes
 * that have a <clinit> or statics).
 *
 * If LVM is not supported or this is not a class that requires LVM
 * isolation, then we only need CVMcbCheckInitNeeded to handle a rare
 * corner case.
 * %end lvm
 * If the current thread is currently running the <clinit> of
 * the class, then we need to quicken to the checkinit opcode in case
 * another thread comes in and executes the quickened opcode before
 * the <clinit> thread is done.
 */
#define CVMcbCheckInitNeeded(cb, ee)					 \
    (CVMcbIsLVMSystemClass(cb) ?					 \
     CVMcbHasStaticsOrClinit(cb) : !CVMcbInitializationDoneFlag(ee, cb))

/*
 * Initialization is needed if it has not been done and is not
 * being done by the current thread.
 */
#define CVMcbInitializationNeeded(cb, ee)				\
    (!CVMcbInitializationDoneFlag(ee, cb) && CVMcbGetClinitEE(ee, cb) != ee)

/*
 * There are exactly nine primitive classes.
 * For each of them, the methodTableCount field contains a type code
 * that is used to table-lookup more detailed type information. See
 * the data structures declared in basictypes.h.
 */
#define CVMcbBasicTypeCode(cb)	((CVMBasicType) CVMcbMethodTableCount(cb))

/*
 * CVMInterfaces - Contains information about all interfaces that a class
 * or interface implements.
 *
 * For cb's corresponding to Java classes, it contains entries for all of
 * the interfaces the class implements, directly or indirectly; it is a
 * flattened version of the interface hierarchy. 
 *
 * For class blocks corresponding to Java interfaces, the interface array 
 * contains the same information, as well as an entry for this interface.
 * The fact that interface cb's contain entries in the interface table 
 * for themselves as well as a flattened version of the interface
 * hierarchy, makes class preparation easier because all that needs to be
 * done to create a class's interface list is concatenate those of all of 
 * the interfaces it implements. This does pose some problems for certain
 * reflection-related traversals. See reflect.c for more details.
 * 
 * INVARIANT: the first "implementsCount" entries in the itable
 * are those which this class declares it implements, in
 * left-to-right, order as written in the Java file (according to
 * the documentation for java.lang.Class.getInterfaces().) The remaining
 * entries are for super classes.
 *
 * The methodTableIndices field points to an array of indices that converts
 * the index of the method in the interface's methods array to the index of
 * the method in the class' methodTable array. It is not setup until the
 * class reaches the LINKED state.
 *
 * When a class is first loaded, the "interfaceTypeID" field contains the
 * typeID for the interface. When the class has its super clases loaded
 * (and moves to the SUPERCLASS_LOADED state), the interfaceTypeID is used
 * to lookup the cb for the "interfaceCb" field. When the class moves to
 * the LINKED state, the interfaceTypeID is overwritten with the
 * the methodTableIndices field.
 *
 * If a class does not declare any interfaces, but does inherit one or
 * more from its superclass, then it will simply use its parents's
 * CVMcbInterfaces structure. The class' INHERITED_INTERFACES flag will
 * be set in this case so we know not to free the CVMcbInterfaces when
 * the class is unloaded. Since this flag is not set for romized classes,
 * the CVMcbImplementsCount() macro has been modified to check the
 * if the CVMcbInterfaces belongs to the parent class and return 0 if it does.
 */

struct CVMInterfaceTable {
    CVMClassBlock* interfaceCb;        /* ClassBlock of the interface */
    union {
	CVMUint16*     methodTableIndicesX; /* see comment above for details */
	CVMClassTypeID interfaceTypeIDX;    /* class typeID's of interface */
    } intfInfoX;
};

struct CVMInterfaces {
    CVMUint16      interfaceCountX;  /* # entries in itable */
    CVMUint16      implementsCountX; /* # of entries the class implements */
    CVMInterfaceTable itable[1];  /* info for all of the class' interfaces. */
};

/*
 * The immutable part of a method block. Once initialized (by JCC or
 * by the class loader), these fields are never written into.  
 */
struct CVMMethodBlockImmutable {
#ifdef CVM_METHODBLOCK_HAS_CB
    CVMClassBlock*   cbX; /* An extra pointer to a methodblock's class */
#endif
    CVMMethodTypeID  nameAndTypeIDX;/* name and type */
    CVMUint16  methodTableIndexX;/* index of this method into cb.methodTable */
    CVMUint8   argsSizeX;     /* calculated based on signature and if static */
    CVMUint8   methodIndexX;  /* index into CB.methods[].mb[] of this method */

    /* Top 4 bits for the invoker type. Bottom 12 bits for the access flags. */
    CVMUint16  invokerAndAccessFlagsX;

    CVMUint16  checkedExceptionsOffsetX; /* index into CB.checkedExceptions */
    union {   /* code specific method information */
	CVMJavaMethodDescriptor* jmd;        /* more info for java methods */
	CVMUint8*  nativeCode;       /* code for native methods */
	/* 
	 * Even if we don't use more than 16 bits in 'methodSlotIndex'
	 * it necessary to use the type CVMAddr since only this makes sure
	 * that the low bit of 'methodSlotIndex' is is the same bit as the
	 * low bit of 'jmd' regardless of the endianess.
	 */
	CVMAddr    methodSlotIndex;  /* for interface methods, the real 16-bit
				      * index of the method. This could be a
				      * CVMUint16, but it's easier for JCC if
				      * it's an CVMUint32.
				      */
	CVMMethodBlock* interfaceMb; /* for miranda methods, the interface mb
				      * the miranda methods was created for.
				      * Sometimes NULL. 
				      * See CVMclassPrepareInterfaces.
				      */
    } codeX;
};

/*
 * MethodBlock - Java, native, and interface methods all require some sort
 * of per method data structure, but they also each have their own unique 
 * requirements. For this reason MethodBlock only contains the information
 * needed by all three and a separate descriptor record is used to contain
 * information specific to the type of method. This saves us about 16 bytes
 * per interface method and 12 bytes per native method.
 *
 * Another optimization is to move the ClassBlock* out of the MethodBlock
 * and only save one copy. Since the MethodBlocks are stored in an array,
 * we just put the ClassBlock* in the 4 bytes that precede the array.
 * The MethodBlock.methodIndex field contains the method's index into this
 * array so it can then locate its ClassBlock*. This is similar to what
 * we do with FieldBlocks.
 */

struct CVMMethodBlock {
#ifdef CVM_JIT
    void*      jitInvokerX; /* Invoker for invocations from compiled code */
    CVMUint8*  startPCX;    /* where to start execution of compiled method */
    CVMUint16  jitFlagsX;
#define CVMJIT_NOT_INLINABLE	0x1	/* Method cannot be inlined */
#define CVMJIT_IS_INTRINSIC	0x10	/* Method is intrinsic. */
#define CVMJIT_ALWAYS_INLINABLE 0x40    /* agressively inline. */
#define CVMJIT_NEEDS_TO_INLINE  0x80    /* must inline called methods. */
/* The following flags are only set after an attempt to compile the method */
#define CVMJIT_NOT_COMPILABLE	0x2	/* Method cannot be compiled */
#define CVMJIT_HAS_BACKBRANCH	0x4	/* Method has a backward branch */
#define CVMJIT_COMPILE_ONCALL   0x8     /* Method to be compiled on 1st call */
#define CVMJIT_HAS_RET          0x20    /* Method has ret opcode. */
#define CVMJIT_IGNORE_THRESHOLD 0x100   /* ignore threshold when inlining. */
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
#define CVMJIT_IS_OVERRIDDEN 	0x200   /* Method is overridden by subclass. */
#endif
/* Notes for MTASK:
 * 
 * Normally, the 16-bit invokeCostX field contains the actual invokeCost
 * value of the method.
 *
 * When CVM_MTASK is #defined, the content of the invokeCostX field is
 * determined by whether if the method is from a romized class or a
 * dynamically loaded class. For a method from a dynamically loaded class,
 * invokeCostX contains the actual invokeCost value as usual. For a method
 * from a romized class, an array is used to store the invokeCost value,
 * and the invokeCostX field contains the array index. The purpose of
 * using an array for romized methods is to avoid "write" to the romized
 * method block when we need to update the invokeCost.
 */
    CVMUint16 invokeCostX;
#define CVMJIT_MAX_INVOKE_COST  65535   /* The maximum for invokeCost */
#endif
    CVMMethodBlockImmutable immutX;
};

#define CVMmbIsMiranda(mb)						\
    (CVMmbInvokerIdx(mb) == CVM_INVOKE_NONPUBLIC_MIRANDA_METHOD ||	\
     CVMmbInvokerIdx(mb) == CVM_INVOKE_MISSINGINTERFACE_MIRANDA_METHOD)
#define CVMmbMirandaIsNonPublic(mb) \
    (CVMmbInvokerIdx(mb) == CVM_INVOKE_NONPUBLIC_MIRANDA_METHOD)

#define CVMmbIsJava(mb)    (!CVMmbIs(mb, NATIVE) && !CVMmbIs(mb, ABSTRACT))

struct CVMMethodRange {
    CVMClassBlock*     cb; /* ClassBlock for all methods in this MethodRange */
    CVMMethodBlock     mb[256];
};

struct CVMMethodArray {
    CVMMethodRange ranges[1];
};

/*
 * Make sure we get the right offset of the classblock from the
 * method blocks. This is needed, if any padding occurs between
 * cb and mb in CVMMethodRange. This macro returns that offset
 * and is used in CVMmbMethodRange() macro below.
 * original was:
 * define CVMmbMethodRange(mb)       \
 * ((CVMMethodRange*) ((CVMClassBlock**)((mb) - CVMmbMethodIndex(mb)) - 1))
 */
#define CVM_CLASSBLOCK_OFFSET_IN_METHODRANGE (CVMoffsetof(CVMMethodRange, mb[0]) - CVMoffsetof(CVMMethodRange, cb))
#define CVMmbMethodRange(mb)       \
    ((CVMMethodRange*) ((CVMUint8 *)((mb) - CVMmbMethodIndex(mb)) - CVM_CLASSBLOCK_OFFSET_IN_METHODRANGE))

#ifdef CVM_DEBUG_ASSERTS
#define CVMmbJmd(mb)	      (*(CVMassert(CVMmbIsJava(mb)),		\
				 &(mb)->immutX.codeX.jmd))
#define CVMmbNativeCode(mb)   (*(CVMassert(CVMmbIs((mb), NATIVE)),	\
				 &(mb)->immutX.codeX.nativeCode))
#define CVMmbMethodSlotIndex(mb)  					\
    (*(CVMassert(CVMmbIs(mb, ABSTRACT) && 				\
		 CVMcbIs(CVMmbClassBlock(mb), INTERFACE)),		\
       &(mb)->immutX.codeX.methodSlotIndex))
#define CVMmbMirandaInterfaceMb(mb)   (*(CVMassert(CVMmbIsMiranda(mb)),	\
					 &(mb)->immutX.codeX.interfaceMb))
#else
#define CVMmbJmd(mb)                   ((mb)->immutX.codeX.jmd)
#define CVMmbNativeCode(mb)            ((mb)->immutX.codeX.nativeCode)
#define CVMmbMethodSlotIndex(mb)       ((mb)->immutX.codeX.methodSlotIndex)
#define CVMmbMirandaInterfaceMb(mb)    ((mb)->immutX.codeX.interfaceMb)
#endif

#define CVMmbAccessFlags(mb) \
    ((mb)->immutX.invokerAndAccessFlagsX & CVM_METHOD_ACC_FLAGS_MASK)
#define CVMmbSetAccessFlags(mb, flags_) { \
	CVMUint16 invokerAndAccessFlags = (mb)->immutX.invokerAndAccessFlagsX; \
	CVMUint16 invoker = invokerAndAccessFlags & ~CVM_METHOD_ACC_FLAGS_MASK; \
	CVMassert((flags_) <= CVM_METHOD_ACC_FLAGS_MASK); \
	(mb)->immutX.invokerAndAccessFlagsX = \
	    invoker | ((flags_) & CVM_METHOD_ACC_FLAGS_MASK); \
    }

/* Sets the invokerIdx and access flags in the given CVMMethodBlockImmutable
   record.  This is only currently used by the preloader. */
#define CVMprivateMbImmSetInvokerAndAccessFlags(mbImm, invokerIdx_, flags_) { \
	CVMUint16 invokerIdx;					 \
	CVMUint16 accessFlags;					 \
	invokerIdx = (invokerIdx_);				 \
	accessFlags = (flags_);					 \
	CVMassert(invokerIdx <= CVM_METHOD_INVOKER_MASK);	 \
	CVMassert(accessFlags <= CVM_METHOD_ACC_FLAGS_MASK);	 \
	mbImm->invokerAndAccessFlagsX =				 \
	    (invokerIdx << CVM_METHOD_INVOKER_SHIFT) | accessFlags; \
    }

#define CVMmbArgsSize(mb)              ((mb)->immutX.argsSizeX)
#define CVMmbCapacity(mb)	       (CVMjmdMaxCapacity(CVMmbJmd(mb)))
/* This macro returns either NULL or an CVMCheckedExceptions* which is
   guaranteed to have a non-zero numExceptions field. */
#define CVMmbCheckedExceptions(mb)     ((CVMCheckedExceptions*)	\
    ((CVMmbCheckedExceptionsOffset(mb) == 0) ? NULL :		\
     &CVMcbCheckedExceptions(CVMmbClassBlock(mb))		\
			     [CVMmbCheckedExceptionsOffset(mb)]))
#define CVMmbCheckedExceptionsOffset(mb)  \
                                       ((mb)->immutX.checkedExceptionsOffsetX) 
#define CVMmbClassBlockInRange(mb)     (CVMmbMethodRange(mb)->cb)
#ifdef CVM_METHODBLOCK_HAS_CB
#define CVMmbClassBlock(mb)            ((mb)->immutX.cbX)
#else
#define CVMmbClassBlock(mb)	       (CVMmbMethodRange(mb)->cb)
#endif
#define CVMmbCodeLength(mb)	       (CVMjmdCodeLength(CVMmbJmd(mb)))
#define CVMmbExceptionTable(mb)        (CVMjmdExceptionTable(CVMmbJmd(mb)))
#define CVMmbExceptionTableLength(mb)  \
     (CVMjmdExceptionTableLength(CVMmbJmd(mb)))

#define CVMmbInvokerIdx(mb) \
    ((mb)->immutX.invokerAndAccessFlagsX >> CVM_METHOD_INVOKER_SHIFT)
#define CVMmbSetInvokerIdx(mb, invokerType) {					\
	CVMUint16 invokerAndAccessFlags = (mb)->immutX.invokerAndAccessFlagsX; \
	CVMUint16 flags = invokerAndAccessFlags & CVM_METHOD_ACC_FLAGS_MASK; \
	(mb)->immutX.invokerAndAccessFlagsX = \
	    ((invokerType) << CVM_METHOD_INVOKER_SHIFT) | flags; \
    }

#define CVMmbJavaCode(mb)              (CVMjmdCode(CVMmbJmd(mb)))
#define CVMmbLineNumberTable(mb)       (CVMjmdLineNumberTable(CVMmbJmd(mb)))
#define CVMmbLineNumberTableLength(mb) \
     (CVMjmdLineNumberTableLength(CVMmbJmd(mb)))
#define CVMmbLocalVariableTable(mb)    (CVMjmdLocalVariableTable(CVMmbJmd(mb)))
#define CVMmbLocalVariableTableLength(mb) \
     (CVMjmdLocalVariableTableLength(CVMmbJmd(mb)))
#define CVMmbMaxLocals(mb)    	       (CVMjmdMaxLocals(CVMmbJmd(mb)))
#define CVMmbMethodIndex(mb)           ((mb)->immutX.methodIndexX)
#define CVMmbMethodTableIndex(mb)      ((mb)->immutX.methodTableIndexX)
#define CVMmbNameAndTypeID(mb)         ((mb)->immutX.nameAndTypeIDX)
#ifdef CVM_JIT
#define CVMmbJitInvoker(mb)            ((mb)->jitInvokerX)
#endif

/* Similar to CVMmbMethodIndex, except it also takes into account the 
   CVMMethodRange the method is in. It can produce an index that is
   greater than 255 if needed, and used with CVMcbMethodSlot.
*/
#define CVMmbFullMethodIndex(mb)                                              \
    (((CVMmbMethodRange(mb) - CVMcbMethods(CVMmbClassBlock(mb))->ranges) << 8)\
     + CVMmbMethodIndex(mb))

typedef CVMUint16 CVMCheckedException;

struct CVMCheckedExceptions {
    CVMUint16             numExceptions;
    CVMCheckedException   exceptions[1];
};

#ifdef CVM_JIT

/*
 * Context for a compiled method.
 */
struct CVMCompiledMethodDescriptor {
    CVMUint16       capacityX;   /* stack frame size for compiled method */
    CVMUint8        spillSizeX;  /* size of register spill area. */
    CVMUint8        entryCountX; /* method entry counter */
    CVMMethodBlock* mbX;         /* pointer back to the method's mb */
    CVMUint16       maxLocalsX;  /* number local words for compiled method */

    /* start of code buffer */
    CVMInt16	codeBufOffsetX;
    /* Entry point from interpreted code */
    CVMInt16	startPCFromInterpretedOffsetX;
    CVMInt16	pcMapTableOffsetX;
    CVMInt16	stackMapsOffsetX;
    CVMInt16	inliningInfoOffsetX;
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    /* 
     * This is the offset into the CMD where the table of callees
     * resides for this compiled method. The first entry is the number
     * of entries. What follows are addresses in the code cache of all
     * direct method calls to this method.
     */
    CVMInt16	calleesOffsetX;
#endif
#ifdef CVMCPU_HAS_CP_REG
    /* base register for the constant pool */
    CVMInt16	cpBaseRegOffsetX;
#endif
#ifdef CVMJIT_PATCH_BASED_GC_CHECKS
    CVMInt16	gcCheckPCsOffsetX;
#endif
#ifdef CVM_DEBUG_ASSERTS
#define CVMJIT_CMD_MAGIC 0x1007
    CVMUint16	magic;
#endif
};

#define CVMmbStartPC(mb)       ((mb)->startPCX)
#define CVMmbIsCompiled(mb)    (CVMmbIsJava(mb) && (CVMmbStartPC(mb) != NULL))
#define CVMmbCmd(mb)	       \
    (CVMassert(CVMmbIsCompiled((mb))),	\
    ((CVMCompiledMethodDescriptor *)CVMmbStartPC(mb)) - 1)

#define CVMcmdCapacity(cmd)      ((cmd)->capacityX)
#define CVMcmdMaxLocals(cmd)      ((cmd)->maxLocalsX)
#define CVMcmdCodeBufAddr(cmd)	\
    ((CVMUint8 *)((CVMUint16 *)(cmd) + (cmd)->codeBufOffsetX))
#define CVMcmdEntryCount(cmd)    ((cmd)->entryCountX)
#define CVMcmdSpillSize(cmd)     ((cmd)->spillSizeX)
#define CVMcmdCodeBufSize(cmd)   (CVMJITcbufSize(CVMcmdCodeBufAddr(cmd)))

#define CVMcmdCompiledPcMapTable(cmd)	\
    ((CVMCompiledPcMapTable*)((CVMUint16 *)(cmd) + (cmd)->pcMapTableOffsetX))
#define CVMcmdNumPcMaps(cmd)	\
     ((cmd)->pcMapTableOffsetX != 0 \
      ? CVMcmdCompiledPcMapTable((cmd))->numEntries : 0)
#define CVMcmdCompiledPcMaps(cmd) (CVMassert(CVMcmdNumPcMaps(cmd) != 0), \
				   CVMcmdCompiledPcMapTable((cmd))->maps)
#define CVMcmdStackMaps(cmd)					\
    ((cmd)->stackMapsOffsetX == 0 ? NULL : 			\
    ((CVMCompiledStackMaps*)					\
	((CVMUint16 *)(cmd) + (cmd)->stackMapsOffsetX)))
#define CVMcmdGCCheckPCs(cmd)					\
    ((cmd)->gcCheckPCsOffsetX == 0 ? NULL :			\
    ((CVMCompiledGCCheckPCs*)					\
	((CVMUint16 *)(cmd) + (cmd)->gcCheckPCsOffsetX)))
#define CVMcmdInliningInfo(cmd)					\
    ((cmd)->inliningInfoOffsetX == 0 ? NULL :			\
    ((CVMCompiledInliningInfo*)					\
	((CVMUint16 *)(cmd) + (cmd)->inliningInfoOffsetX)))
#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
#define CVMcmdCallees(cmd)					\
    ((cmd)->calleesOffsetX == 0 ? NULL :			\
    ((CVMMethodBlock**)						\
	((CVMUint32 *)(cmd) + (cmd)->calleesOffsetX)))
#endif
#define CVMcmdStartPC(cmd)	  ((CVMUint8*)((cmd) + 1))
#define CVMcmdEndPC(cmd)				\
    ((cmd)->stackMapsOffsetX != 0			\
     ? (CVMUint8*)CVMcmdStackMaps(cmd)			\
     : CVMcmdCodeBufAddr(cmd) + CVMcmdCodeBufSize(cmd))
#define CVMcmdCodeSize(cmd)       \
    (CVMcmdEndPC(cmd) - CVMcmdStartPC(cmd))
#define CVMcmdMb(cmd)             ((cmd)->mbX)
#define CVMcmdStartPCFromInterpreted(cmd)	\
    ((CVMUint8 *)(cmd) + (cmd)->startPCFromInterpretedOffsetX)
#ifdef CVMCPU_HAS_CP_REG
#define CVMcmdCPBaseReg(cmd)	\
    ((void*)((CVMUint16 *)(cmd) + (cmd)->cpBaseRegOffsetX))
#endif /* CVMCPU_HAS_CP_REG */ 
#endif /* CVM_JIT */

#ifdef CVM_JIT
#ifdef CVM_MTASK
#define CVMmbInvokeCostSet(mb, val)				\
{								\
    if (CVMcbIsInROM(CVMmbClassBlock(mb))) {			\
        CVMROMMethodInvokeCosts[(mb)->invokeCostX] = (val);	\
    } else {							\
        (mb)->invokeCostX = (val);				\
    }								\
}	
#define CVMmbInvokeCost(mb) \
	(CVMcbIsInROM(CVMmbClassBlock(mb)) ? \
		CVMROMMethodInvokeCosts[(mb)->invokeCostX] : \
		((mb)->invokeCostX))
#else /* CVM_MTASK */
#define CVMmbInvokeCostSet(mb, val) \
	(mb)->invokeCostX = (val)
#define CVMmbInvokeCost(mb) \
        ((mb)->invokeCostX)
#endif /* CVM_MTASK */
#else  /* CVM_JIT */
#define CVMmbInvokeCostSet(mb, val)
#define CVMmbInvokeCost(mb)
#endif /* CVM_JIT */

struct CVMJavaMethodDescriptor {
    /* An optimization is made to speed up method calls.
       The total size of the frame is saved in "capacity"
       rather than just saving max_stack. This
       saves us some calcuating when a frame is pushed.
       */
    CVMUint16      maxLocalsX;    /* including arguments */
    CVMUint16	   flagsX;	  /* see CVM_JMD_xxx     */
    /* %comment f */  
    CVMUint32      capacityX;     /* maxLocals + sizeof(Frame) + max_stack */
    CVMUint16      exceptionTableLengthX;
    CVMUint16      codeLengthX; 
#if 0
    CVMUint32      inliningX;         /* possible inlining code */
#endif

#ifdef CVM_DEBUG_CLASSINFO
    CVMUint16      lineNumberTableLengthX;
    CVMUint16      localVariableTableLengthX;
#endif
    /* The byte codes, exception handlers, line number table, and
       local variable table all immediately follow the
       JavaMethodDescriptor data structure. Their offsets need to
       be calculated.

       CVMUint8 code[codeLength];
       CVMExceptionHandler exceptionTable[exceptionTableLength];
       CVMLineNumberEntry lineNumberTable[lineNumberTableLength];
       CVMLocalVariableEntry localVariableTable[localVariableTableLength];
       */
};

/*
 * A stackmap entry for an interpreted Java method
 */
struct CVMStackMapEntry {
    CVMUint16 pc;
    CVMUint16 state[1];
};

/*
 * The stackmaps for the interpreted method
 */
struct CVMStackMaps {
    CVMStackMaps *    next;
    CVMStackMaps *    prev;
    CVMUint32         size;
    CVMMethodBlock *  mb;
    CVMUint32         noGcPoints;
    CVMStackMapEntry  smEntries[1];
};

#ifdef CVM_JIT
/*
 * A stackmap entry for a compiled Java method
 */
struct CVMCompiledStackMapEntry {
    CVMUint16		pc;	       /* offset from start of method's code */
    CVMUint8		totalSize;     /* total # of bits to examine */
    CVMUint8		paramSize;     /* outgoing parameters */
    CVMUint16		state;	       /* state bits or offset to them. */
};

struct CVMCompiledStackMapLargeEntry{
    CVMUint16		totalSize;
    CVMUint16		paramSize;
    CVMUint16		state[1];
};

struct CVMCompiledStackMaps {
    CVMUint32		     noGCPoints;
    CVMCompiledStackMapEntry smEntries[1];
};

#define CVMmbCompileFlags(mb)		    ((mb)->jitFlagsX)
#endif

#define CVMjmdFlags(jmd)                    ((jmd)->flagsX)
#define CVMjmdCapacity(jmd)                 ((jmd)->capacityX)
#define CVMjmdCodeLength(jmd)               ((jmd)->codeLengthX)
#define CVMjmdMutable(jmd)		    ((jmd)->mutableX)
#define CVMjmdExceptionTableLength(jmd)     ((jmd)->exceptionTableLengthX)
#define CVMjmdLineNumberTableLength(jmd)    ((jmd)->lineNumberTableLengthX)
#define CVMjmdLocalVariableTableLength(jmd) ((jmd)->localVariableTableLengthX)
#define CVMjmdMaxLocals(jmd)                ((jmd)->maxLocalsX)

/*
 * In case somebody outside of the interpreter wants to know.
 */
#define CVMjmdMaxStack(jmd) \
    ((jmd)->capacityX - (jmd)->maxLocalsX - \
     CVM_JAVAFRAME_SIZE / sizeof(CVMStackVal32))

#define CVMjmdCode(jmd) \
    ((CVMUint8*)jmd + CVMjmdCodeOffset(jmd))
#define CVMjmdExceptionTable(jmd) \
    ((CVMExceptionHandler*) \
     ((CVMUint8*)jmd + CVMjmdExceptionTableOffset(jmd)))
#define CVMjmdLineNumberTable(jmd) \
    ((CVMLineNumberEntry*) \
     ((CVMUint8*)jmd + CVMjmdLineNumberTableOffset(jmd)))
#define CVMjmdLocalVariableTable(jmd) \
    ((CVMLocalVariableEntry*) \
     ((CVMUint8*)jmd + CVMjmdLocalVariableTableOffset(jmd)))

#define CVMjmdCodeOffset(jmd)			\
    (sizeof(CVMJavaMethodDescriptor))
#define CVMjmdExceptionTableOffset(jmd)		\
    (CVMjmdCodeOffset(jmd) + ((CVMjmdCodeLength(jmd) + 3) & ~3))
#define CVMjmdLineNumberTableOffset(jmd)			\
    (CVMjmdExceptionTableOffset(jmd) + 				\
     (CVMjmdExceptionTableLength(jmd) * sizeof(CVMExceptionHandler)))
#define CVMjmdLocalVariableTableOffset(jmd)			\
    (CVMjmdLineNumberTableOffset(jmd) + 			\
     (CVMjmdLineNumberTableLength(jmd) * sizeof(CVMLineNumberEntry)))

/*
 * The total size of a JavaMethodDescriptor and all its trailing data
 * structures
 */
#ifdef CVM_DEBUG_CLASSINFO
#define CVMjmdTotalSize(jmd) \
     (CVMjmdLocalVariableTableOffset(jmd) + \
      CVMjmdLocalVariableTableLength(jmd) * sizeof(CVMLocalVariableEntry))
#else
#define CVMjmdTotalSize(jmd) \
     (CVMjmdExceptionTableOffset(jmd) + \
      CVMjmdExceptionTableLength(jmd) * sizeof(CVMExceptionHandler))
#endif

/*
 * CVMjmdEndPtr() is used to get the start
 * address of 'extension' (struct CVMJavaMethodExtension)
 * so it must have correct alignment.
 */
#define CVMjmdEndPtr(jmd) \
    ((CVMUint8*)CVMalignAddrUp((CVMUint8*)(jmd) + CVMjmdTotalSize(jmd)))

/*
 * CVMJavaMethodDescriptor-related flags.
 * For tagging methods that the preloader believes might need to be
 * rewritten, and those that have been rewritten by the stackmap
 * disambiguator.
 * Also, an indication that the method needs
 * 4-byte alignment (because it contains a switch).
 */
#define CVM_JMD_MAY_NEED_REWRITE	1
#define CVM_JMD_NEEDS_ALIGNMENT		2
#define CVM_JMD_DID_REWRITE		4
#define CVM_JMD_STRICT			8

#ifdef CVM_JVMTI
#define CVM_JMD_OBSOLETE               16
#endif

/*
 * When the stackmap disambiguator rewrites a Java method, it replaces
 * the entire CVMJavaMethodDescriptor with a newly allocated one. In
 * this case, it must make room for the address of the original method
 * descriptor.
 *
 * In truth, this is only needed very occasionally, when rewriting the
 * first Java method of a class so that the start of the original
 * allocation can be found!
 *
 * This will be placed at the end of the new allocation.  
 */
typedef struct CVMJavaMethodExtension {
    CVMJavaMethodDescriptor* originalJmd; 
} CVMJavaMethodExtension;

struct CVMExceptionHandler {
    CVMUint16          startpc;
    CVMUint16          endpc;
    CVMUint16          handlerpc;
    CVMUint16          catchtype;
};

#ifdef CVM_DEBUG_CLASSINFO

struct CVMLineNumberEntry {
    CVMUint16          startpc;
    CVMUint16          lineNumber;
};

struct CVMLocalVariableEntry {
    CVMUint16          startpc;
    CVMUint16          length;
    CVMUint16          index;
    /* 
     * Instead of a field typeid, we now use the name and type part of the
     * typeid separately to relax the alignment restrictions of this structure.
     * This saves 2 bytes per structure for 32 bit typeid's.
     */
    CVMTypeIDNamePart  nameID;
    CVMTypeIDTypePart  typeID;
};

#endif


/*
 * FieldBlocks have been optimized to only occupy 8 bytes. They are all located
 * in an array with the address just before the array containing the
 * ClassBlock* for all of the fields in the array. That data structure
 * that contains the ClassBlock* and the array is the CVMFieldRange.
 *
 * A FieldBlock locates its ClassBlock* by looking backwards FieldBlock.fbIndex
 * entries to find the start of the array and then another sizeof(ptr) to 
 * locate the ClassBlock*. Since cbInfo is only 8 bits, the
 * CVMFieldRange must be repeated every 256 enties.
 */

struct CVMFieldBlock {
    CVMFieldTypeID nameAndTypeIDX; /* name and type */
    CVMUint8       accessFlagsX;
    CVMUint8       fbIndexX;   /* fields index into FieldRange.fb */
    CVMUint16      offsetX;    /* offset in words into the object for instance
				* variables or offset in words into
				* ClassBlock.statics for static variables
				*/
};

struct CVMFieldRange {
    CVMClassBlock*     cb; /* ClassBlock for all fields in this FieldRange */
    CVMFieldBlock      fb[256];
};

struct CVMFieldArray {
    CVMFieldRange ranges[1];
};


/*
 * Make sure we get the right offset of the classblock from the
 * field blocks. This is needed, if any padding occurs between
 * cb and fb in CVMFieldRange. This macro returns that offset
 * and is used in CVMmbFieldRange() macro below.
 * original was:
 * define CVMfbFieldRange(fb)       \
 * ((CVMFieldRange*) ((CVMClassBlock**)((fb) - CVMfbIndex(fb)) - 1))
 */
#define CVM_CLASSBLOCK_OFFSET_IN_FIELDRANGE (CVMoffsetof(CVMFieldRange, fb[0]) - CVMoffsetof(CVMFieldRange, cb))
#define CVMfbFieldRange(fb)       \
    ((CVMFieldRange*) ((CVMUint8 *)((fb) - CVMfbIndex(fb)) - CVM_CLASSBLOCK_OFFSET_IN_FIELDRANGE))

#define CVMfbAccessFlags(fb)      ((fb)->accessFlagsX)
#define CVMfbClassBlock(fb)       (CVMfbFieldRange(fb)->cb)
#define CVMfbIndex(fb)            ((fb)->fbIndexX)
#define CVMfbNameAndTypeID(fb)    ((fb)->nameAndTypeIDX)
#define CVMfbOffset(fb)           ((fb)->offsetX)

#ifdef CVM_LVM /* %begin lvm */
/* In LVM-enabled mode, we replicate static fields per LVM,
 * which are accessed via 'ee'. */
#define CVMfbStaticField(ee, fb)				\
    (*((CVMcbLVMClassIndex(CVMfbClassBlock(fb)) > 0)		\
	? &(CVMLVMeeStatics(ee)[CVMfbOffset(fb)])		\
	: &(CVMcbStatics(CVMfbClassBlock(fb))[CVMfbOffset(fb)])))
#else /* %end lvm */
#define CVMfbStaticField(ee, fb)  \
    (*((void)(ee), &(CVMcbStatics(CVMfbClassBlock(fb))[CVMfbOffset(fb)])))
#endif /* %begin,end lvm */

/*
 * This indicates whether this field occupies one word or two,
 * or if it is a reference type. For use by {getfield,putfield}_quick_w.
 */
#define CVMfbIsDoubleWord(fb) \
    CVMtypeidFieldIsDoubleword(CVMfbNameAndTypeID(fb))
#define CVMfbIsRef(fb) \
    CVMtypeidFieldIsRef(CVMfbNameAndTypeID(fb))

/*
 * CVMInnerClassesInfo for storing information about inner classes.
 */

#define CVMcbInnerClassesInfo(cb)      ((cb)->innerClassesInfoX)
#define CVMcbInnerClassesInfoCount(cb) (CVMcbInnerClassesInfo(cb) == NULL    \
					? 0				    \
					: CVMcbInnerClassesInfo(cb)->count)
#define CVMcbInnerClassInfo(cb, idx)   (&CVMcbInnerClassesInfo(cb)->info[idx])

struct CVMInnerClassInfo {
    CVMUint16 innerClassIndex;
    CVMUint16 outerClassIndex;
    CVMUint16 unused; /* innerNameIndex is not supported */
    CVMUint16 innerClassAccessFlags;
};

struct CVMInnerClassesInfo {
    CVMUint32 count;
    CVMInnerClassInfo info[1];
};

/*
 * Method invoker indices. Stored in CVMMethodBlock.invokerIdx field.
 * Used to determine what code will handle a method's invocation.
 *
 * WARNING: The exact order and values of these macros may be important.
 * [ see executejava.c]
 */
enum {
    CVM_INVOKE_JAVA_METHOD,
    CVM_INVOKE_JAVA_SYNC_METHOD,
    CVM_INVOKE_CNI_METHOD,
    CVM_INVOKE_JNI_METHOD,
    CVM_INVOKE_JNI_SYNC_METHOD,
    CVM_INVOKE_ABSTRACT_METHOD,
    CVM_INVOKE_NONPUBLIC_MIRANDA_METHOD,
    CVM_INVOKE_MISSINGINTERFACE_MIRANDA_METHOD,
#ifdef CVM_CLASSLOADING
    CVM_INVOKE_LAZY_JNI_METHOD,
#endif
    CVM_NUM_INVOKER_TYPES
};

#define CVM_METHOD_INVOKER_SHIFT   12  /* top 4 bits in the CVMUint16 */
#define CVM_METHOD_INVOKER_MASK    0xf /* 4 bits only */

/* 
 * Class, field, and method access and modifier flags. Some of these
 * values are different from the red book so they'll fit in one byte.
 * CVMreflectCheckAccess (reflect.c) and CVMverifyMemberAccess2 
 * (interpreter.c) require the following invariants:
 *   CVM_METHOD_ACC_PUBLIC == CVM_FIELD_ACC_PUBLIC
 *   CVM_METHOD_ACC_PROTECTED == CVM_FIELD_ACC_PROTECTED
 *   CVM_METHOD_ACC_PRIVATE == CVM_FIELD_ACC_PRIVATE
 * If these are not met, some of the code for reflection and object
 * serialization will have to be reworked.)
 *
 * NOTE: ACC_PUBLIC, ACC_PRIVATE, and ACC_PROTECTED are encoded differently
 *       from the other flags.  These 3 are encoded in 2 bits shared between
 *       them instead of being assigned 1 bit each.
 */
#define CVM_CLASS_ACC_PUBLIC        0x01 /* visible to everyone */
#define CVM_CLASS_ACC_PRIMITIVE     0x02 /* primitive type class */
#define CVM_CLASS_ACC_FINALIZABLE   0x04 /* has non-trivial finalizer */
#define CVM_CLASS_ACC_REFERENCE     0x08 /* is a flavor of weak reference */
#define CVM_CLASS_ACC_FINAL         0x10 /* no further subclassing */
#define CVM_CLASS_ACC_SUPER         0x20 /* funky handling of invokespecial */
#define CVM_CLASS_ACC_INTERFACE     0x200  /* class is an interface */
#define CVM_CLASS_ACC_ABSTRACT      0x400  /* may not be instantiated */
#define CVM_CLASS_ACC_SYNTHETIC     0x1000 /* Not in source. */
#define CVM_CLASS_ACC_ANNOTATION    0x2000 /* Declared as an annotation type.*/
#define CVM_CLASS_ACC_ENUM          0x4000 /* Declared as enum type. */

#define CVM_FIELD_ACC_PUBLIC        0x01 /* visible to everyone */
#define CVM_FIELD_ACC_PRIVATE       0x03 /* visible only to defining class */
#define CVM_FIELD_ACC_PROTECTED     0x02 /* visible to subclasses */
#define CVM_FIELD_ACC_STATIC        0x08 /* instance variable is static */
#define CVM_FIELD_ACC_FINAL         0x10 /* no subclassing/overriding */
#define CVM_FIELD_ACC_VOLATILE      0x40 /* cannot cache in registers */
#define CVM_FIELD_ACC_TRANSIENT     0x80 /* not persistent */
#define CVM_FIELD_ACC_SYNTHETIC     0x04 /* Not in source. */
#define CVM_FIELD_ACC_ENUM          0x20 /* Declared as enum type. */

#define CVM_METHOD_ACC_PUBLIC       CVM_FIELD_ACC_PUBLIC     /* invariant */
#define CVM_METHOD_ACC_PRIVATE      CVM_FIELD_ACC_PRIVATE    /* invariant */
#define CVM_METHOD_ACC_PROTECTED    CVM_FIELD_ACC_PROTECTED  /* invariant */
#define CVM_METHOD_ACC_STATIC       0x08 /* method is static */
#define CVM_METHOD_ACC_FINAL        0x10 /* no further overriding */
#define CVM_METHOD_ACC_SYNCHRONIZED 0x20 /* wrap method call in monitor lock */
#define CVM_METHOD_ACC_BRIDGE       0x40 /* Bridge generated by javac. */
#define CVM_METHOD_ACC_VARARGS      0x80 /* Declared with varargs. */
#define CVM_METHOD_ACC_NATIVE      0x100 /* implemented in C */
#define CVM_METHOD_ACC_ABSTRACT    0x400 /* no definition provided */
#define CVM_METHOD_ACC_STRICT      0x200 /* floating point is strict. */
#define CVM_METHOD_ACC_SYNTHETIC    0x04 /* Not in source. */

/* CVM_MEMBER_PPP_MASK is used for masking the 3 flags: ACC_PUBLIC,
   ACC_PRIVATE, and ACC_PROTECTED.  The "PPP" refers to the 3 Ps of the
   encoded flags that it masks.
*/
#define CVM_MEMBER_PPP_MASK \
    (CVM_FIELD_ACC_PUBLIC | CVM_FIELD_ACC_PRIVATE | CVM_FIELD_ACC_PROTECTED)

/* CVM_METHOD_ACC_FLAGS_MASK is the mask of all method ACC flags. */
#define CVM_METHOD_ACC_FLAGS_MASK \
    (CVM_MEMBER_PPP_MASK   | CVM_METHOD_ACC_STATIC | \
     CVM_METHOD_ACC_FINAL  | CVM_METHOD_ACC_SYNCHRONIZED  | \
     CVM_METHOD_ACC_BRIDGE | CVM_METHOD_ACC_VARARGS | \
     CVM_METHOD_ACC_NATIVE | CVM_METHOD_ACC_ABSTRACT | \
     CVM_METHOD_ACC_STRICT | CVM_METHOD_ACC_SYNTHETIC)

/*
 * Test for specific access flags 
 */
#define CVMprivateThingIs(thing_, what_, flag_) \
    ((((thing_)->accessFlagsX) & CVM_##what_##_ACC_##flag_) != 0)

#define CVMmemberPPPAccessIs(accessFlags_, what_, flag_) \
    (((accessFlags_) & CVM_MEMBER_PPP_MASK) == CVM_##what_##_ACC_##flag_)

#define CVMfbIs(fb_, flag_) \
    ((CVM_FIELD_ACC_##flag_ & CVM_MEMBER_PPP_MASK) ? \
     CVMmemberPPPAccessIs((fb_)->accessFlagsX, FIELD, flag_) : \
     CVMprivateThingIs(fb_, FIELD, flag_))

#define CVMmbIs(mb_, flag_) \
    ((CVM_METHOD_ACC_##flag_ & CVM_MEMBER_PPP_MASK) ? \
     CVMmemberPPPAccessIs((mb_)->immutX.invokerAndAccessFlagsX, METHOD, flag_) : \
     ((((mb_)->immutX.invokerAndAccessFlagsX) & CVM_METHOD_ACC_##flag_) != 0))

#define CVMjmdIs(jmd_, flag_) \
    ((CVMjmdFlags(jmd_) & CVM_JMD_##flag_) != 0)

#define CVMcbIs(cb_, flag_) \
    CVMprivateThingIs(cb_, CLASS, flag_)

/*
 * Macros for initializing, and in some cases declaring, CVMClassBlocks,
 * CVMMethodBlocks, and CVMFieldBlocks.
 */

#ifdef CVM_DEBUG_CLASSINFO
#define CVM_INIT_SOURCEFILENAME(sourceFileName) \
    sourceFileName,
#define CVM_INIT_LINENUMERTABLELENGTH(lineNumberTableLength) \
    lineNumberTableLength,
#define CVM_INIT_LOCALVARIABLETABLELENGTH(localVariableTableLength) \
    localVariableTableLength,
#else
#define CVM_INIT_SOURCEFILENAME(sourceFileName) 
#define CVM_INIT_LINENUMERTABLELENGTH(lineNumberTableLength)
#define CVM_INIT_LOCALVARIABLETABLELENGTH(localVariableTableLength)
#endif

#ifdef CVM_DEBUG
#define CVM_INIT_NAMESTRING(name) \
    name,
#else
#define CVM_INIT_NAMESTRING(name)
#endif

#ifdef CVM_CLASSLOADING
#define CVM_INIT_CLASSLOADING_FIELDS(cl, pd) \
    cl, pd,    /* classLoader and protectionDomain */
#else
#define CVM_INIT_CLASSLOADING_FIELDS(cl, pd)
#endif

#ifdef CVM_JVMTI
#define CVM_OLD_DATA NULL,
#else
#define CVM_OLD_DATA
#endif

/*
 * Put the string representation of the class in
 */
#ifdef CVM_LVM
#define CVM_PRIVATE_LVM_CLASS_INDEX(index) (index),
#else
#define CVM_PRIVATE_LVM_CLASS_INDEX(index) 
#endif
#define CVM_INIT_CLASSBLOCK(gcMap, classname, classnameString,		\
                        superclass, constantpool,			\
                        interfaces, methods, fields, statics,		\
			constantpool_count, method_count,		\
			field_count, method_table_count,		\
			access_flags, runtime_flags, lvm_class_index,	\
			instance_size, javaInstance, clinitEE,		\
			checkedExceptions, sourceFileName,		\
			methodTablePtr,					\
			classLoader, protectionDomain,			\
			innerClassesInfo)				\
			{{gcMap}, classname,                 		\
			  {(CVMClassBlock*)superclass},			\
			  {(CVMConstantPool*)constantpool},		\
			  (CVMInterfaces*)interfaces,			\
			  (CVMMethodArray *)methods,			\
			  (CVMFieldArray *)fields, {statics},		\
			  constantpool_count, method_count,		\
			  field_count, method_table_count,		\
			  access_flags, runtime_flags,                  \
			  CVM_PRIVATE_LVM_CLASS_INDEX(lvm_class_index)	\
			  instance_size, 0,				\
			  (CVMClassICell*)javaInstance,			\
			  CVM_INIT_NAMESTRING(classnameString)		\
			  CVM_INIT_CLASSLOADING_FIELDS(classLoader,	\
			    protectionDomain)				\
			  {clinitEE}, (CVMUint16*)checkedExceptions,	\
			  CVM_INIT_SOURCEFILENAME(sourceFileName)	\
			  (CVMMethodBlock**)methodTablePtr,		\
                          (CVMInnerClassesInfo*)innerClassesInfo,	\
                          0,0,                                          \
                          CVM_OLD_DATA}

#ifdef CVM_METHODBLOCK_HAS_CB
#define CVM_INIT_METHODBLOCK_CB_FIELD(cb) (CVMClassBlock*)cb,
#else
#define CVM_INIT_METHODBLOCK_CB_FIELD(cb)
#endif

#ifdef CVM_JIT
#define CVM_INIT_METHODBLOCK_JIT_FIELDS(inv) (void*)inv,0,0,0
#else
#define CVM_INIT_METHODBLOCK_JIT_FIELDS(inv)
#endif

#define CVM_INIT_METHODBLOCK(cb, inv, nameAndTypeID, methodTableIndex,	\
			     argsSize, invokerIdx, accessFlags,		\
			     methodIndex, checkedExceptionsOffset,	\
			     code)					\
    {									\
        CVM_INIT_METHODBLOCK_JIT_FIELDS(inv)				\
        0,								\
	{ CVM_INIT_METHODBLOCK_CB_FIELD(cb)    				\
        nameAndTypeID, methodTableIndex,				\
	argsSize, invokerIdx,						\
	accessFlags, methodIndex,					\
	checkedExceptionsOffset, {(CVMJavaMethodDescriptor*)code} }	\
    }

#define CVM_INIT_METHODBLOCK_IMMUTABLE(cb, 				\
                             nameAndTypeID, methodTableIndex,		\
			     argsSize, invokerIdx, accessFlags,		\
			     methodIndex, checkedExceptionsOffset,	\
			     code)					\
    {									\
        CVM_INIT_METHODBLOCK_CB_FIELD(cb)     				\
	nameAndTypeID, methodTableIndex,				\
	argsSize, methodIndex,						\
	(invokerIdx << CVM_METHOD_INVOKER_SHIFT) | accessFlags,		\
	checkedExceptionsOffset, {(CVMJavaMethodDescriptor*)code}	\
    }

#define CVM_INIT_JAVA_METHOD_DESCRIPTOR(				\
                max_stack, maxLocals, flags,				\
                exceptionTableLength, codeLength,			\
		lineNumberTableLength, localVariableTableLength)	\
    {									\
	maxLocals, flags,						\
        CVM_JAVAFRAME_SIZE / sizeof(CVMStackVal32) +			\
	    maxLocals + max_stack,					\
        exceptionTableLength, codeLength,				\
	CVM_INIT_LINENUMERTABLELENGTH(lineNumberTableLength)		\
	CVM_INIT_LOCALVARIABLETABLELENGTH(localVariableTableLength)	\
    }

#define CVM_INIT_FIELDBLOCK(nameAndTypeID, accessFlags, fbIndex, offset) \
    {									 \
	  nameAndTypeID, accessFlags, fbIndex, offset			 \
    }

/***********************
 * API's
 ***********************/

/*
 * Do a lookup without class loading. Checks if class is preloaded
 * (only if loader == NULL) and checks if the class is in the
 * loaderCache.
 *
 * NOTE: It is possible for this function to return NULL even if the
 * class is loaded. This can happen if the loader specified is neither
 * the intiating or defining ClassLoader.
 */
CVMClassBlock*
CVMclassLookupClassWithoutLoading(CVMExecEnv* ee, CVMClassTypeID typeID,
				  CVMClassLoaderICell* loader);

/*
 * Class lookup by name or typeID. 
 *   -The "init" flag indicates if static initializers should be run.
 *   -The "throwError" flag will cause NoClassDefFoundError to be thrown
 *    instead of ClassNotFoundException.
 *   -An exception os thrown if a NULL result is returned.
 */

extern CVMClassBlock*
CVMclassLookupByNameFromClass(CVMExecEnv* ee, const char* name,
			      CVMBool init, CVMClassBlock* fromClass);

extern CVMClassBlock*
CVMclassLookupByTypeFromClass(CVMExecEnv* ee, CVMClassTypeID typeID, 
			      CVMBool init, CVMClassBlock* fromClass);

extern CVMClassBlock*
CVMclassLookupByNameFromClassLoader(CVMExecEnv* ee, const char* name, 
				    CVMBool init,
				    CVMClassLoaderICell* loader, 
				    CVMObjectICell* pd,
				    CVMBool throwError);

extern CVMClassBlock*
CVMclassLookupByTypeFromClassLoader(CVMExecEnv* ee, CVMClassTypeID typeID, 
				    CVMBool init,
				    CVMClassLoaderICell* loader, 
				    CVMObjectICell* pd,
				    CVMBool throwError);

/*
 * Get an array class with elements of type elemCb, but only if it
 * is already created.
 */
CVMClassBlock*
CVMclassGetArrayOfWithNoClassCreation(CVMExecEnv* ee, CVMClassBlock* elemCb);

/*
 * Get an array class with elements of type elemCb
 */
extern CVMClassBlock*
CVMclassGetArrayOf(CVMExecEnv* ee, CVMClassBlock* elemCb);

/*
 * Look for public or declared method of type 'tid'
 */
extern CVMMethodBlock*
CVMclassGetMethodBlock(const CVMClassBlock* cb, const CVMMethodTypeID tid,
		       CVMBool isStatic);

#define CVMclassGetNonstaticMethodBlock(cb, tid)	\
	CVMclassGetMethodBlock(cb, tid, CVM_FALSE)
#define CVMclassGetStaticMethodBlock(cb, tid)		\
	CVMclassGetMethodBlock(cb, tid, CVM_TRUE)

/*
 * Look for declared method of type 'tid'
 */
CVMMethodBlock*
CVMclassGetDeclaredMethodBlockFromTID(const CVMClassBlock* cb,
				      CVMMethodTypeID tid);

/*
 * Look for declared method of given name and signature
 */
extern CVMMethodBlock*
CVMclassGetDeclaredMethodBlock(CVMExecEnv *ee,
			       const CVMClassBlock* cb,
			       const char *name,
			       const char *sig);
/*
 * Look for public or declared field of type 'tid'
 */
extern CVMFieldBlock*
CVMclassGetFieldBlock(const CVMClassBlock* cb, const CVMFieldTypeID tid, 
		      CVMBool isStatic);

#define CVMclassGetNonstaticFieldBlock(cb, tid)		\
	CVMclassGetFieldBlock(cb, tid, CVM_FALSE)
#define CVMclassGetStaticFieldBlock(cb, tid)		\
	CVMclassGetFieldBlock(cb, tid, CVM_TRUE)

/*
 * Link a class' hierarchy by resolving symbolic links to its superclass
 * and interfaces. All classes in the hierarcy are guarnateed to be loaded
 * before calling.
 */
#ifdef CVM_CLASSLOADING
extern CVMBool
CVMclassLinkSuperClasses(CVMExecEnv* ee, CVMClassBlock* cb);
#endif

/*
 * Load a class using the specified class loader.
 */
#ifdef CVM_CLASSLOADING
extern CVMClassBlock*
CVMclassLoadClass(CVMExecEnv* ee, CVMClassLoaderICell* loader, 
		  const char* classname, CVMClassTypeID classTypeID);
#endif

/*
 * Load a class using the specified class loader.
 */
CVMClassICell*
CVMclassLoadBootClass(CVMExecEnv* ee, const char* classname);

/*
 * Given an array of bytes, create an CVMClassBlock.
 */
#ifdef CVM_CLASSLOADING
extern CVMClassICell*
CVMclassCreateInternalClass(CVMExecEnv* ee,
			    const CVMUint8* externalClass, 
			    CVMUint32 classSize, 
			    CVMClassLoaderICell* loader, 
			    const char* classname,
			    const char* dirNameOrZipFileName,
			    CVMBool isRedefine);

#endif

/*
 * Free a Class' memory. Called by the gc when it detects that there
 * are no references to the class.
 */
extern void
CVMclassFree(CVMExecEnv* ee, CVMClassBlock* cb);

/*
 * Free up a Class' JavaMethodDescriptors and associated stack maps.
 * For dynamic classes this is called by CVMclassFree, above,
 * but the preloader needs it to deallocate dynamic memory associated
 * with preloaded classes, such as the stackmaps.
 */
extern void
CVMclassFreeJavaMethods(CVMExecEnv* ee, CVMClassBlock* cb,
			CVMBool isPreloaded);

/*
 * Similar to CVMclassFreeJavaMethods(), but only frees parts related
 * to the method being compiled.
 */
#ifdef CVM_JIT
extern void
CVMclassFreeCompiledMethods(CVMExecEnv* ee, CVMClassBlock* cb);
#endif

/*
 * Frees up the CVMFieldTypeID's in the LocalVariableTable.
 */
extern void
CVMclassFreeLocalVariableTableFieldIDs(CVMExecEnv* ee, CVMMethodBlock* mb);

/*
 * Do all the class unloading. Called after gc has completed its scan.
 */
#ifdef CVM_CLASSLOADING
extern void
CVMclassDoClassUnloadingPass1(CVMExecEnv* ee,
			      CVMRefLivenessQueryFunc isLive,
			      void* isLiveData,
			      CVMRefCallbackFunc transitiveScanner,
			      void* transitiveScannerData,
			      CVMGCOptions* gcOpts);

extern void
CVMclassDoClassUnloadingPass2(CVMExecEnv* ee);
#endif

/*
 * CVMclassVerify - Verify a class, including its bytecodes.
 */
#ifdef CVM_CLASSLOADING

extern CVMBool
CVMclassVerify(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isRedefine);

enum { CVM_VERIFY_NONE = 0, CVM_VERIFY_REMOTE, CVM_VERIFY_ALL, CVM_VERIFY_UNRECOGNIZED };

/*
 * Returns one of the above options given a verification spec from a
 * command line option
 */
extern CVMInt32
CVMclassVerificationSpecToEncoding(char* verifySpec);

/*
 * CVMloaderNeedsVerify - returns TRUE if the classloader requires
 * verification of its loaded classes.
 *
 * verifyTrusted should be set true if verification must be done even
 * for trusted class loaders when the verification level is REMOTE.
 */
CVMBool
CVMloaderNeedsVerify(CVMExecEnv* ee, CVMClassLoaderICell* loader,
                     CVMBool verifyTrusted);

#endif /* CVM_CLASSLOADING */

/*
 * Link an already loaded class. This mainly involves preparing the
 * fields, methodtable, and interface table. Throws and exception and 
 * returns CVM_FALSE is unsuccessful.
 */
#ifdef CVM_CLASSLOADING
extern CVMBool
CVMclassLink(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isRedefine);
#endif

/*
 * CVMclassInit - run static initializers for entire class hierarchy.
 */
extern CVMBool
CVMclassInit(CVMExecEnv* ee, CVMClassBlock* cb);

/*
 * CVMclassInitNoCRecursion - setup interpeter state so the interpreter
 * can run the static intializers.
 */
extern int
CVMclassInitNoCRecursion(CVMExecEnv* ee, CVMClassBlock* cb,
			 CVMMethodBlock **p_mb);

/*** End functions in classinitialize.c ***/

/*
 * CVMdefineClass - Used to define a new class. Used by CVMjniDefineClass()
 * and JVM_DefineClass().
 */
extern CVMClassICell*
CVMdefineClass(CVMExecEnv* ee, const char *name, CVMClassLoaderICell* loader,
	       const CVMUint8* buf, CVMUint32 bufLen, CVMObjectICell* pd,
	       CVMBool isRedefine);

/*
 * CVMclassCreateMultiArrayClass - creates the specified array class by
 * iteratively (i.e. non-recursively) creating all the layers of the array
 * class from the inner most to the outer most.  Needed for any array class
 * type that wasn't reference by romized code and we make a reference to it
 * at runtime.
 */
extern CVMClassBlock*
CVMclassCreateMultiArrayClass(CVMExecEnv* ee, CVMClassTypeID arrayTypeId,
			      CVMClassLoaderICell* loader, CVMObjectICell* pd);

/*
 * A 'class callback' called on each class that is iterated over.
 */
typedef void (*CVMClassCallbackFunc)(CVMExecEnv* ee,
				     CVMClassBlock* cb,
				     void* data);

typedef void (*CVMStackCallbackFunc)( CVMObject**, void*);

/*
 * Iterate over all classes, both romized and dynamically loaded,
 * and call 'callback' on each class.
 */
extern void
CVMclassIterateAllClasses(CVMExecEnv* ee, 
			  CVMClassCallbackFunc callback,
			  void* data);

/*
 * Iterate over the loaded classes table, and call 'callback'
 * on each class.
 */
extern void
CVMclassIterateDynamicallyLoadedClasses(CVMExecEnv* ee, 
					CVMClassCallbackFunc callback,
					void* data);
/*
 * Free all classes in the class table. This is used during vm shutdown.
 */
extern void
CVMclassTableFreeAllClasses(CVMExecEnv* ee);

/*
 * Class table functions. The class table is a simple database of all
 * dynamically loaded classes. It is used by gc to scan dynamically loaded
 * classes and is also needed by jvmti to get a list of all loaded classes.
 * In both cases CVMclassIterateDynamicallyLoadedClasses() is used to
 * iterate over the class table.
 */

extern CVMClassBlock**
CVMclassTableAllocateSlot(CVMExecEnv* ee);

void
CVMclassTableFreeSlot(CVMExecEnv* ee, CVMClassBlock** cbPtr);

extern void
CVMclassTableMarkUnscannedClasses(CVMExecEnv* ee,
				  CVMRefLivenessQueryFunc isLive,
				  void* isLiveData);

extern void
CVMclassTableIterate(CVMExecEnv* ee,
		     CVMClassCallbackFunc callback,
		     void* data);

#ifdef CVM_DEBUG
extern void
CVMclassTableDump(CVMExecEnv* ee);
#endif /* CVM_DEBUG */

/*
 * Loader cache functions. The loader cache is a database of <loader,cb>
 * pairs. The loader is the initiating class loader. It is used to speed
 * up class lookups and assist in implementing loader constraints.
 */

extern CVMClassLoaderICell*
CVMloaderCacheGetGlobalRootFromLoader(CVMExecEnv* ee,
				      CVMClassLoaderICell* loader);

#ifdef CVM_CLASSLOADING
extern CVMBool
CVMloaderCacheCheckPackageAccess(CVMExecEnv *ee, CVMClassLoaderICell* loader, 
				 CVMClassBlock* cb, CVMObjectICell* pd);
#endif

#define CVMloaderCacheLookup(ee, classID, loader) \
    CVMloaderCacheLookupWithProtectionDomain(ee, classID, loader, NULL)

extern CVMClassBlock*
CVMloaderCacheLookupWithProtectionDomain(CVMExecEnv* ee,
					 CVMClassTypeID classID,
					 CVMClassLoaderICell* loader,
					 CVMObjectICell* pd);

extern CVMBool
CVMloaderCacheAdd(CVMExecEnv* ee, CVMClassBlock* cb,
		  CVMClassLoaderICell* loader);

#ifdef CVM_CLASSLOADING
extern void
CVMloaderCacheMarkUnscannedClassesAndLoaders(CVMExecEnv* ee,
					      CVMRefLivenessQueryFunc isLive,
					      void* isLiveData);
extern void
CVMloaderCachePurgeUnscannedClassesAndLoaders(CVMExecEnv* ee);
#endif /* CVM_CLASSLOADING */

extern CVMBool
CVMloaderCacheInit();

extern void
CVMloaderCacheDestroy(CVMExecEnv* ee);

#ifdef CVM_DEBUG
extern void
CVMloaderCacheDump(CVMExecEnv* ee);
#endif /* CVM_DEBUG */

#ifdef CVM_JVMTI

typedef struct {
    int index;
    void *entry;
} CVMLoaderCacheIterator;

void
CVMloaderCacheIterate(CVMExecEnv* ee, CVMLoaderCacheIterator *iter);

CVMBool
CVMloaderCacheIterateNext(CVMExecEnv* ee, CVMLoaderCacheIterator *iter);

CVMObjectICell *
CVMloaderCacheIterateGetLoader(CVMExecEnv* ee, CVMLoaderCacheIterator *iter);

CVMClassBlock *
CVMloaderCacheIterateGetCB(CVMExecEnv* ee, CVMLoaderCacheIterator *iter);

#endif

#ifdef CVM_CLASSLOADING
#define CVM_NULL_CLASSLOADER_LOCK(ee)   \
    CVMsysMutexLock(ee, &CVMglobals.nullClassLoaderLock)
#define CVM_NULL_CLASSLOADER_UNLOCK(ee) \
    CVMsysMutexUnlock(ee, &CVMglobals.nullClassLoaderLock)
#else
#define CVM_NULL_CLASSLOADER_LOCK(ee)
#define CVM_NULL_CLASSLOADER_UNLOCK(ee)
#endif

/*
 * Loader constraint functions.
 */

#ifdef CVM_CLASSLOADING
CVMBool 
CVMloaderConstraintsCheckMethodSignatureLoaders(CVMExecEnv* ee,
						CVMMethodTypeID methodID, 
						CVMClassLoaderICell* loader1,
						CVMClassLoaderICell* loader2);
CVMBool 
CVMloaderConstraintsCheckFieldSignatureLoaders(CVMExecEnv* ee,
					       CVMFieldTypeID fieldID, 
					       CVMClassLoaderICell* loader1,
					       CVMClassLoaderICell* loader2);
extern void 
CVMloaderConstraintsMarkUnscannedClassesAndLoaders(
    CVMExecEnv* ee,
    CVMRefLivenessQueryFunc isLive,
    void* isLiveData);

extern void 
CVMloaderConstraintsPurgeUnscannedClassesAndLoaders(CVMExecEnv* ee);

#ifdef CVM_DEBUG
extern void
CVMloaderConstraintsDump(CVMExecEnv* ee);
#endif

#endif /* CVM_CLASSLOADING */

/*
 * Traverse the scannable state of one class, and call 'callback' on
 * each reference in it. The scannable state of the class is its
 * statics, and also the ClassLoader and javaInstance fields.  
 */
extern void
CVMclassScan(CVMExecEnv* ee, CVMClassBlock* cb,
	     CVMRefCallbackFunc callback, void* data);

/* 
 * Initialization and destruction of the classes module
 */
extern CVMBool
CVMclassModuleInit(CVMExecEnv* ee);

extern void
CVMclassModuleDestroy(CVMExecEnv* ee);

extern CVMBool
CVMclassBootClassPathInit(JNIEnv *env);

extern CVMBool
CVMclassClassPathInit(JNIEnv *env);

extern void
CVMclassBootClassPathDestroy(CVMExecEnv* ee);

extern void
CVMclassClassPathDestroy(CVMExecEnv* ee);

/*
 * Find file that contains the class bytes
 */
JNIEXPORT jobject JNICALL
CVMclassFindContainer(JNIEnv *env, jobject tthis, jstring name);

typedef struct {
    CVMClassPathEntry*   entries;
    CVMUint16            numEntries;
    char*                pathString;
    CVMBool              initialized;
} CVMClassPath;

extern CVMBool
CVMclassPathInit(JNIEnv* env, CVMClassPath* path, char* additionalPathString,
	      CVMBool doNotFailWhenPathNotFound, CVMBool initJavaSide);

/*
 * Obtain the system class loader (initialize the cache if not yet done)
 */
CVMClassLoaderICell* 
CVMclassGetSystemClassLoader(CVMExecEnv* ee);

/*
 * Set the system class loader.
 */
void
CVMclassSetSystemClassLoader(CVMExecEnv* ee, jobject loader);

#ifdef CVM_INSPECTOR

/* Purpose: Checks to see if the specified pointer is a valid classblock. */
CVMBool CVMclassIsValidClassBlock(CVMExecEnv *ee, CVMClassBlock *cb);

#endif

#ifdef CVM_CLASSLIB_JCOV
/*
 * Load a classfile from the specified path without converting it into a
 * classblock.
 */
CVMUint8 *
CVMclassLoadSystemClassFileFromPath(CVMExecEnv *ee, const char *classname,
                                    CVMClassPath *path,
                                    CVMUint32 *classfileSize);

/*
 * Releases a previously loaded classfile.
 */
void
CVMclassReleaseSystemClassFile(CVMExecEnv *ee, CVMUint8 *classfile);
#endif

#endif /* _INCLUDED_CLASSES_H */
