/*
 * @(#)reflect.c	1.72 06/10/10
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
 * This entire file is conditionally compiled
 */

#ifdef CVM_REFLECT

#include "javavm/include/preloader.h"
#include "javavm/include/reflect.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/localroots.h"
#include "javavm/include/common_exceptions.h"
#ifdef CVM_CLASSLOADING
#include "javavm/include/constantpool.h"
#endif

#include "generated/offsets/java_lang_Class.h"
#include "generated/offsets/java_lang_reflect_AccessibleObject.h"
#include "generated/offsets/java_lang_reflect_Constructor.h"
#include "generated/offsets/java_lang_reflect_Field.h"
#include "generated/offsets/java_lang_reflect_Method.h"
#include "generated/jni/java_lang_reflect_Modifier.h"

/*********************************************************************************
 *
 * NOTE:
 * NOTE: documentation for all of these functions is in reflect.h.
 * NOTE:
 *
 **********************************************************************************/


/*
 * NOTE: These macros always walk from subclass towards superclass
 */

/*
 * Field traversal macros
 */

#define	WALK_DECLARED_FIELDS(cb, fb, body)				\
{									\
    int _i_, _list_count_ = CVMcbFieldCount(cb);			\
    /* Walk in declared order */					\
    for (_i_ = 0; _i_ < _list_count_; _i_++) {				\
	CVMFieldBlock* fb = CVMcbFieldSlot(cb, _i_);			\
	body;								\
    }									\
}

#define	COUNT_DECLARED_FIELDS(cb, count)				\
{									\
    int _i_, _list_count_ = CVMcbFieldCount(cb);			\
    /* Walk in reverse declared order */				\
    for (_i_ = _list_count_ - 1; _i_ >= 0; _i_--) {			\
        (count)++;							\
    }									\
}

#define	WALK_INSTANCE_FIELDS(cb, fb, body)				\
{									\
    CVMClassBlock* _cb_;   				                \
    for (_cb_ = cb; _cb_; _cb_ = CVMcbSuperclass(_cb_)) {		\
	WALK_DECLARED_FIELDS(_cb_, fb, body);				\
    }									\
}

/* Note that this does in fact traverse all static fields associated
   with an interface's classblock because interfaces show up in their
   own interface array. See classes.h for more details. */
#define	WALK_INTERFACE_FIELDS(cb, fb, body)				\
{									\
    int _j_, _icnt_ = CVMcbInterfaceCount(cb);				\
    int _declaredCnt_ = CVMcbImplementsCount(cb);			\
    CVMClassBlock* _cb_;						\
    /* Walk self interface first if this is an interface: */		\
    if (CVMcbIs(cb, INTERFACE)) {					\
        _cb_ = CVMcbInterfacecb(cb, _declaredCnt_);			\
        WALK_DECLARED_FIELDS(_cb_, fb, body);				\
    }									\
    /* Walk from subinterface towards superinterface */			\
    for (_j_ = 0; _j_ < _icnt_; _j_++) {				\
        if (CVMcbIs(cb, INTERFACE) && (_j_ == _declaredCnt_)) {		\
            continue; /* Skip the self interface. */          		\
        }								\
        _cb_ = CVMcbInterfacecb(cb,_j_);				\
        WALK_DECLARED_FIELDS(_cb_, fb, body);				\
    }									\
}

#define	COUNT_INTERFACE_FIELDS(cb, count)				\
{									\
    int _j_, _icnt_ = CVMcbInterfaceCount(cb);				\
    /* Walk from subinterface towards superinterface */			\
    for (_j_ = 0; _j_ < _icnt_; _j_++) {				\
        CVMClassBlock* _cb_ = CVMcbInterfacecb(cb,_j_);			\
        COUNT_DECLARED_FIELDS(_cb_, count);				\
    }									\
}

/*
 * Method traversal macros
 */

#define	WALK_DECLARED_METHODS(cb, mb, body)				\
{									\
    int _i_, _list_count_ = CVMcbMethodCount(cb);			\
    /* Walk in declared order */					\
    for (_i_ = 0; _i_ < _list_count_; _i_++) {				\
	CVMMethodBlock *mb = CVMcbMethodSlot(cb, _i_);			\
	body;								\
    }									\
}

#define	WALK_INSTANCE_METHODS(cb, mb, body)				    \
{									    \
    int _tcnt_ = CVMcbMethodTableCount(cb), _i_;			    \
    /* We don't have the strange invariant from JDK 1.2 that */		    \
    /* slot 0 of the methodtable is empty */				    \
    /* Walk from subtype towards supertype */				    \
    for (_i_ = _tcnt_ - 1; _i_ >= 0; _i_--) {				    \
	CVMMethodBlock* mb = CVMcbMethodTableSlot(cb,_i_);		    \
	/* See CVMclassPrepareInterfaces() in for miranda method details */ \
	if (CVMmbIsMiranda(mb)) {					    \
	    if (!CVMmbMirandaIsNonPublic(mb)) {				    \
		/* get the interface cb for miranda methods generated	    \
		 * for unimplemented interfaces.	       		    \
		 */							    \
		mb = CVMmbMirandaInterfaceMb(mb);			    \
	    } else {							    \
		/* Skip miranda methods generated for non-public methods */ \
		continue;						    \
	    }								    \
	}								    \
	body;								    \
    }									    \
}

#define	WALK_SUPER_METHODS(cb, mb, body)				\
{									\
    CVMClassBlock* _cb_;    						\
    for (_cb_ = (cb);  _cb_ != NULL; _cb_ = CVMcbSuperclass(_cb_)) {    \
	WALK_DECLARED_METHODS(_cb_, mb, body);				\
    }									\
}

/* NOTE that upon first examination this macro might appear not to
   work, but instead to miss the methods declared in this interface.
   However, it does in fact work as expected, for two reasons. First,
   each interface contains a reference to itself in its implemented
   interfaces table. Second, the implemented interfaces table always
   contains a flattened version of the interface hierarchy (for both
   classes and interfaces), so recursion is never necessary in this
   traversal. See classes.h (the interfaces member of CVMClassBlock)
   for more details. */
#define	WALK_INTERFACE_METHODS(cb, mb, body)				\
{									\
    int _icnt_ = CVMcbInterfaceCount(cb), _j_;				\
    int _declaredCnt_ = CVMcbImplementsCount(cb);			\
    CVMClassBlock* _cb_;						\
    CVMassert(CVMcbIs(cb, INTERFACE));                                  \
    /* Walk self interface first: */					\
    {									\
        _cb_ = CVMcbInterfacecb(cb,_declaredCnt_);			\
        WALK_DECLARED_METHODS(_cb_, mb, {				\
	    /* The interface method <clinit> need not be public */	\
    	    if (CVMmbIs(mb, PUBLIC)) {					\
		body;							\
	    }								\
	});								\
    }									\
    /* Walk from subinterface towards superinterface */			\
    for (_j_ = 0; _j_ < _icnt_; _j_++) {				\
        if (_j_ == _declaredCnt_) {					\
            continue; /* Skip the self interface. */          		\
        }								\
        _cb_ = CVMcbInterfacecb(cb,_j_);				\
        WALK_DECLARED_METHODS(_cb_, mb, {				\
	    /* The interface method <clinit> need not be public */	\
    	    if (CVMmbIs(mb, PUBLIC)) {					\
		body;							\
	    }								\
	});								\
    }									\
}

CVMBool 
CVMreflectEnsureInitialized(CVMExecEnv* ee, CVMClassBlock* cb) {
    CVMBool result;
    CVMD_gcSafeExec(ee, {
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	/* 
	 * CVMclassInit requires a JNI local frame so it can make method
	 * calls using JNI.
	 */
	if (CVMjniPushLocalFrame(env, 4) == JNI_OK) {
	    /* %comment c018 */
	    result = CVMclassInit(ee, cb);
	    CVMjniPopLocalFrame(env, NULL);
	} else 
	    result = CVM_FALSE;
    });
    return result;
}

void
CVMreflectFields(CVMExecEnv* ee,
		 CVMClassBlock* cb,
		 int which,
		 CVMArrayOfRefICell* result)
{
    CVMBool mustAbort = CVM_FALSE;
    const CVMClassBlock* classJavaLangReflectField =
	CVMsystemClass(java_lang_reflect_Field);
    CVMInt32 j;
    CVMInt32 count = 0;

    if (CVMcbIs(cb, PRIMITIVE) ||
	CVMisArrayClass(cb)) {
	/* Allocate zero-length array of java.lang.reflect.Field objects */
	/* Casting away const is safe here (see classes.h, runtime flags) */
	CVMreflectNewArray(ee, (CVMClassBlock*) classJavaLangReflectField,
			   0, (CVMArrayOfAnyTypeICell*) result);
	return;
    }

    CVMreflectEnsureLinked(ee, cb);
    
    switch (which) {
    case REFLECT_MEMBER_PUBLIC: {
	/* NOTE: this discrepancy between the treatment of interfaces
	   and regular classes comes about because interfaces re-use
	   their interface array as a flattened hierarchy of their
	   superinterfaces and also contain themselves in that table.
	   The reason for this self-reference is to make construction
	   of a real class's interface array easier; it is then simply
	   the concatenation of the interface arrays of all its
	   implemented interfaces. See classes.h for more details. */
	
	CVMBool isInterface = CVMcbIs(cb, INTERFACE);
	if (!isInterface) {
	    /* Don't double-count interface fields since interfaces
	       are in their own interface arrays. */
	    WALK_INSTANCE_FIELDS(cb, fb, {
		/* count public class fields */
		if (CVMfbIs(fb, PUBLIC))
		    count++;
	    });
	}
	
	/* all interface fields are public and accessible */
	COUNT_INTERFACE_FIELDS(cb, count);
	
	/* Allocate array of java.lang.reflect.Field objects */
	/* Casting away const is safe here (see classes.h, runtime flags) */
	CVMreflectNewArray(ee, (CVMClassBlock*) classJavaLangReflectField,
			   count, (CVMArrayOfAnyTypeICell*) result);
	if (CVMID_icellIsNull(result)) /* exception occurred */
	    return;
	
	j = count;
	
	CVMID_localrootBegin(ee); {
	    CVMID_localrootDeclare(CVMObjectICell, fieldICell);
	    if (!isInterface) {
		WALK_INSTANCE_FIELDS(cb, fb, {
		    if (CVMfbIs(fb, PUBLIC)) {
			CVMreflectNewJavaLangReflectField(ee, fb, fieldICell);
			if (CVMID_icellIsNull(fieldICell)) { /* exception already
								thrown */
			    /* Must execute CVMID_localrootEnd() */
			    mustAbort = CVM_TRUE;
			    goto abort1;
			}
			j--;
			CVMID_arrayWriteRef(ee, result, j, fieldICell);
		    }
		});
	    }

	    CVMwithAssertsOnly(count = j);
	    j = 0;
	    WALK_INTERFACE_FIELDS(cb, fb, {
		CVMreflectNewJavaLangReflectField(ee, fb, fieldICell);
		if (CVMID_icellIsNull(fieldICell)) { /* exception already
							thrown */
		    /* Must execute CVMID_localrootEnd() */
		    mustAbort = CVM_TRUE;
		    goto abort1;
		}
		CVMID_arrayWriteRef(ee, result, j, fieldICell);
		j++;
	    });
	    CVMassert(j == count);
	} 
    abort1:
	CVMID_localrootEnd();
	if (mustAbort)
	    return;
	break;
	
    } /* case REFLECT_MEMBER_PUBLIC */
    
    case REFLECT_MEMBER_DECLARED: {
	COUNT_DECLARED_FIELDS(cb, count);
	
	/* Allocate array of java.lang.reflect.Field objects */
	/* Casting away const is safe here (see classes.h, runtime flags) */
	CVMreflectNewArray(ee, (CVMClassBlock*) classJavaLangReflectField,
			   count, (CVMArrayOfAnyTypeICell*) result);
	if (CVMID_icellIsNull(result)) /* exception occurred */
	    return;
	
	j = count;
	
	CVMID_localrootBegin(ee); {
	    CVMID_localrootDeclare(CVMObjectICell, fieldICell);
	    WALK_DECLARED_FIELDS(cb, fb, {
		CVMreflectNewJavaLangReflectField(ee, fb, fieldICell);
		if (CVMID_icellIsNull(fieldICell)) { /* exception already
							thrown */
		    /* Must execute CVMID_localrootEnd() */
		    mustAbort = CVM_TRUE;
		    goto abort2;
		}
		j--;
		CVMID_arrayWriteRef(ee, result, j, fieldICell);
	    });
	} 
    abort2:
	CVMID_localrootEnd();
	if (mustAbort)
	    return;
	
	CVMassert(j == 0);
	break;
	
    } /* case REFLECT_MEMBER_DECLARED */
    
    default:
	CVMassert(CVM_FALSE);
    }
}

void
CVMreflectField(CVMExecEnv* ee,
		CVMClassBlock* cb,
		const char* name,
		int which,
		CVMObjectICell* result)
{
    CVMFieldTypeID nameCookie;

    if (CVMcbIs(cb, PRIMITIVE) || CVMisArrayClass(cb)) {
	CVMthrowNoSuchFieldException(ee, "attempt to find field of "
				     "primitive or array class");
	return;
    }

    CVMreflectEnsureLinked(ee, cb);

    /*
     * We need to use the "new" function, because the "lookup" function
     * is only valid when we know that the type already exists.
     */
    nameCookie = CVMtypeidNewMembername(ee, name);
    if (nameCookie == CVM_TYPEID_ERROR) {
	/* Clear the exception. A new one will be thrown when the field
	 * is not found. */
	CVMclearLocalException(ee);
    }
    
    switch (which) {

    case REFLECT_MEMBER_PUBLIC: {
	CVMBool isInterface = CVMcbIs(cb, INTERFACE);
	if (!isInterface) {
	    WALK_INSTANCE_FIELDS(cb, fb, {
		if (CVMfbIs(fb, PUBLIC) &&
		    CVMtypeidIsSameName(nameCookie, CVMfbNameAndTypeID(fb))) {
		    CVMreflectNewJavaLangReflectField(ee, fb, result);
		    /* We don't need to check for throwing OutOfMemoryError
		       since CVMreflectNewJavaLangReflectField does it 
		       already */
		    goto finish;
		}
	    });
	}

	WALK_INTERFACE_FIELDS(cb, fb, {
	    if (CVMtypeidIsSameName(nameCookie, CVMfbNameAndTypeID(fb))) {
		CVMreflectNewJavaLangReflectField(ee, fb, result);
		/* We don't need to check for throwing OutOfMemoryError
		   since CVMreflectNewJavaLangReflectField does it already */
		goto finish;
	    }
	});
	break;
    }
    case REFLECT_MEMBER_DECLARED: {
	WALK_DECLARED_FIELDS(cb, fb, {
	    if (CVMtypeidIsSameName(nameCookie, CVMfbNameAndTypeID(fb))) {
		CVMreflectNewJavaLangReflectField(ee, fb, result);
		/* We don't need to check for throwing OutOfMemoryError
		   since CVMreflectNewJavaLangReflectField does it already */
		goto finish;
	    }
	});
	break;
    }

    default:
	CVMassert(CVM_FALSE);
    }
    
    CVMthrowNoSuchFieldException(ee, "%s", name);

 finish:
    if (nameCookie != CVM_TYPEID_ERROR) {
	CVMtypeidDisposeMembername(ee, nameCookie);
    }
}

void
CVMreflectNewJavaLangReflectField(CVMExecEnv* ee,
				  CVMFieldBlock* fb,
				  CVMObjectICell* result)
{
    const CVMClassBlock* classJavaLangReflectField =
	CVMsystemClass(java_lang_reflect_Field);
    CVMClassBlock* cb;
    /* Store raw fieldblock pointer as int */
    /* 
     * Modify the member variable slot of type 'int'
     * and cast it to CVMFieldBlock*.  The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    CVMAddr fbPtr;
    CVMFieldTypeID nameAndTypeID;
    char* fieldName;
    CVMClassBlock* fieldTypeClassBlock;
    CVMUint32 modifiers;

    /* Casting away const is safe here (see classes.h, runtime flags) */
    CVMID_allocNewInstance(ee, (CVMClassBlock*) classJavaLangReflectField,
			   result);
    if (CVMID_icellIsNull(result)) {
	CVMthrowOutOfMemoryError(ee, "CVMreflectNewJavaLangReflectField: "
				 "out of memory allocating Field object");
	return;
    }

    /* Fill in fields. */

    /* Store Java class in which this field is contained */
    cb = CVMfbClassBlock(fb);
    CVMID_fieldWriteRef(ee, result, CVMoffsetOfjava_lang_reflect_Field_clazz,
			CVMcbJavaInstance(cb));

    /* Store raw fieldblock pointer as int (NOTE: not 64-bit clean) */
    /* 
     * Modify the member variable slot of type 'int'
     * and cast it to CVMFieldBlock*.  The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    fbPtr = (CVMAddr) fb;
    CVMID_fieldWriteAddr(ee, result, CVMoffsetOfjava_lang_reflect_Field_slot, fbPtr);
  
    /* Store field name as Java String. */
    nameAndTypeID = CVMfbNameAndTypeID(fb);
    fieldName = CVMtypeidFieldNameToAllocatedCString(nameAndTypeID);
    if (fieldName == NULL) {
	/* %comment c019 */
	CVMthrowOutOfMemoryError(ee, "CVMreflectNewJavaLangReflectField: "
				 "out of memory getting field name");
	CVMID_icellSetNull(result);
	return;
    }
    {
	CVMBool success;
	CVMID_localrootBegin(ee); {
	    CVMID_localrootDeclare(CVMObjectICell, fieldNameICell);
	    CVMnewStringUTF(ee, fieldNameICell, fieldName);
	    success = !CVMID_icellIsNull(fieldNameICell);
	    CVMID_fieldWriteRef(ee, result,
				CVMoffsetOfjava_lang_reflect_Field_name,
				fieldNameICell);
	} CVMID_localrootEnd();
	free(fieldName);
	if (!success) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    CVMID_icellSetNull(result);
	    return;
	}
    }

    /* Store the Class object representing the type of this field. */

    fieldTypeClassBlock =
	CVMclassLookupByTypeFromClass(ee,
				      CVMtypeidGetType(nameAndTypeID),
				      CVM_FALSE, cb);
    /* It's possible that loading the field type's classblock threw an
       exception. */
    if (fieldTypeClassBlock == NULL) {
	CVMID_icellSetNull(result);
	return;
    }

    CVMID_fieldWriteRef(ee, result, CVMoffsetOfjava_lang_reflect_Field_type,
			CVMcbJavaInstance(fieldTypeClassBlock));
  
    /* Store this field's modifiers. */
    /* Make sure we use the constants defined by the JVM spec as opposed
       to any renumbered ones in the VM. We get these from
       java.lang.reflect.Modifier via JavaCodeCompact. */
    modifiers = 0;
    if (CVMfbIs(fb, PUBLIC))
	modifiers |= java_lang_reflect_Modifier_PUBLIC;
    if (CVMfbIs(fb, PRIVATE))
	modifiers |= java_lang_reflect_Modifier_PRIVATE;
    if (CVMfbIs(fb, PROTECTED))
	modifiers |= java_lang_reflect_Modifier_PROTECTED;
    if (CVMfbIs(fb, STATIC))
	modifiers |= java_lang_reflect_Modifier_STATIC;
    if (CVMfbIs(fb, FINAL))
	modifiers |= java_lang_reflect_Modifier_FINAL;
    if (CVMfbIs(fb, VOLATILE))
	modifiers |= java_lang_reflect_Modifier_VOLATILE;
    if (CVMfbIs(fb, TRANSIENT))
	modifiers |= java_lang_reflect_Modifier_TRANSIENT;
    CVMID_fieldWriteInt(ee, result,
			CVMoffsetOfjava_lang_reflect_Field_modifiers, modifiers);
}

CVMFieldBlock*
CVMreflectGCSafeGetFieldBlock(CVMExecEnv* ee, CVMObjectICell* obj)
{
    CVMFieldBlock* fb;

    CVMD_gcUnsafeExec(ee, {
	fb = CVMreflectGCUnsafeGetFieldBlock(CVMID_icellDirect(ee, obj));
    });

    return fb;
}

CVMFieldBlock*
CVMreflectGCUnsafeGetFieldBlock(CVMObject* obj)
{
    /* 
     * Modify the member variable slot of type 'int'
     * and cast it to CVMFieldBlock*.  The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    CVMAddr val;
    CVMFieldBlock* fb;
    CVMD_fieldReadAddr(obj, CVMoffsetOfjava_lang_reflect_Field_slot, val);
    fb = (CVMFieldBlock*) val;
    CVMassert(fb != NULL);
    return fb;
}

/*
 * Code supporting reflection: used by Method.c
 *
 * NOTE (from JDK 1.2, might not be applicable): Some of this should
 * be replaced by direct JNI calls, as the JNI already provides much
 * of the necessary functionality.
 */

#define	GET_ARRAY_LENGTH(ee,object,result) \
	if ((object == NULL) || CVMID_icellIsNull((object))) { \
	    (result) = 0;		   \
	} else {			   \
	    CVMID_arrayGetLength((ee), (object), (result)); \
	}

/* NOTE: unused "CVMreflectInvocationTargetException" removed. */

/* Elided GET_CLASS_METHODBLOCK and GET_METHODBLOCK_SLOT macros since
   they're designed to make methodblock accesses 64-bit clean, but so
   much of the code is already unclean, it's not worth it.

   Actually, it probably wouldn't be THAT much work to make the field
   accesses (for example) 64-bit clean, but you'd have to add another
   field to java.lang.reflect.Field for a pointer to the FieldRange
   and use the slot as an index into it. There are issues about
   whether C pointers and Java references are guaranteed to be the
   same size. */

CVMMethodBlock*
CVMreflectGetObjectMethodblock(CVMExecEnv* ee,
			       CVMClassBlock* ocb,
			       CVMMethodBlock* mb)
{
    /* Get declaring class of given methodblock */
    CVMClassBlock* dcb = CVMmbClassBlock(mb);
    CVMMethodBlock* omb;

    CVMassert(!CVMmbIs(mb, STATIC));
    /* OCB should have been linked by the time we get in here; see
       javavm/native/java/lang/reflect/Method.c. The class is ensured
       to have been initialized, which means it has been linked. */
    CVMassert(CVMcbCheckRuntimeFlag(ocb, LINKED));
    CVMassert(!CVMcbIs(ocb, INTERFACE));

    if (CVMcbIs(dcb, INTERFACE)) {
	/*
	 * Interface method lookup
	 * An interface method's offset is its index in the interface
	 */
	CVMInt32 icount = CVMcbInterfaceCount(ocb), i;
	for (i = 0; i < icount; i++) {
	    if (dcb == CVMcbInterfacecb(ocb, i)) {
		/* This class implements the requested interface.
		   Use the precomputed offsets to get the method block.
		   See executejava.c for better comments on this process. */
		CVMInt32 methodIndex =
		    CVMcbInterfaceMethodTableIndex(ocb, i, 
						   CVMmbMethodSlotIndex(mb));
		omb = CVMcbMethodTableSlot(ocb, methodIndex);
		goto found;
	    }
	}
	goto nosuchmethod;
    } /* CVMcbIs(dcb, INTERFACE) */
    else if (CVMmbIs(mb, PRIVATE)) {
	/* Private methods are not in the dispatch table */
	omb = mb;
    } /* CVMmbIs(mb, PRIVATE) */
    else {
	/*
	 * Dynamic method lookup.
	 */

	/* NOTE: what if the method is public but final, and declared
	   final in the base class? It seems (romjava.c) that such
	   methods show up in the method table even though they don't
	   need to. Why is this? */
	omb = CVMcbMethodTableSlot(ocb, CVMmbMethodTableIndex(mb));
    } /* !CVMcbIs(cb, INTERFACE) && !CVMmbIs(mb, PRIVATE) */

found:
    CVMassert(CVMtypeidIsSame(CVMmbNameAndTypeID(mb), CVMmbNameAndTypeID(omb)));
    return omb;

nosuchmethod:
    CVMsignalError(ee,
		   CVMsystemClass(java_lang_reflect_Method_ArgumentException),
		   "%C.%M", CVMmbClassBlock(mb), mb);
    return NULL;
}

int
CVMreflectGetParameterCount(CVMMethodTypeID sig)
{
    CVMterseSig tsig;
    CVMtypeidGetTerseSignature( sig, &tsig );
    CVMassert(CVM_TERSE_PARAMCOUNT(tsig) >= 0 );
    return CVM_TERSE_PARAMCOUNT(tsig);
}

void
CVMreflectGetParameterTypes(CVMExecEnv* ee,
			    CVMSigIterator *sigp,
			    CVMClassBlock* cb,
			    CVMArrayOfRefICell* result)
{
    CVMInt32 cnt, i = 0;
    CVMClassBlock* pcb;
    CVMClassTypeID p;

    /*
     * Compute number of parameters
     */
    cnt = CVM_SIGNATURE_ITER_PARAMCOUNT(*sigp);

    /*
     * Allocate class array
     */

    CVMreflectNewClassArray(ee, cnt, result);
    if (CVMID_icellIsNull(result)) /* error */
	return;

    /*
     * Resolve parameter types to classblocks
     */
    while ((p=CVM_SIGNATURE_ITER_NEXT(*sigp)) != CVM_TYPEID_ENDFUNC) {
	pcb = CVMclassLookupByTypeFromClass(ee, p, CVM_FALSE, cb);
	if (pcb == NULL) {
	    CVMID_icellSetNull(result);
	    return;
	}
	CVMID_arrayWriteRef(ee, result, i, CVMcbJavaInstance(pcb));
	i++;
    }

    CVMassert(i == cnt);

}

void
CVMreflectGetExceptionTypes(CVMExecEnv* ee,
			    CVMClassBlock* cb,
			    CVMMethodBlock* mb,
			    CVMArrayOfRefICell* result)
{
    CVMCheckedExceptions* checkedExceptions;
    CVMUint16 cnt = 0;    

    checkedExceptions = CVMmbCheckedExceptions(mb);
    if (checkedExceptions != NULL)
	cnt = checkedExceptions->numExceptions;

    CVMreflectNewClassArray(ee, cnt, result);
    if (CVMID_icellIsNull(result)) /* error */
	return;

    if (cnt != 0) {
	CVMUint16 i;
	for (i = 0; i < cnt; i++) {
	    CVMClassBlock *exceptioncb;

#ifdef CVM_CLASSLOADING
	    /* Must ensure these constant pool entries are resolved to
               classblocks */
	    if (!CVMcpResolveEntryFromClass(ee, cb, CVMcbConstantPool(cb),
					    checkedExceptions->exceptions[i])) {
		/* Exception already thrown */
		CVMID_icellSetNull(result);
		return;
	    }
#endif
	    exceptioncb = CVMcpGetCb(CVMcbConstantPool(cb),
				     checkedExceptions->exceptions[i]);
	    CVMID_arrayWriteRef(ee, result, i, CVMcbJavaInstance(exceptioncb));
	}
    }
}

CVMBool
CVMreflectGCUnsafePushArg(CVMExecEnv* ee,
			  CVMFrame* currentJavaFrame,
			  CVMObject* arg,
			  CVMClassBlock* argType,
			  CVMClassBlock* exceptionCb)
{
    if (CVMcbIs(argType, PRIMITIVE)) {
	jvalue v;
	CVMBasicType declaredTypeCode = CVMcbBasicTypeCode(argType);
	CVMBasicType valueTypeCode;
	CVMUint32 numBytes;

	if (CVMgcUnsafeJavaUnwrap(ee, arg, &v, &valueTypeCode,
				  exceptionCb) == CVM_FALSE)
	    return CVM_FALSE;			/* exception */

	if (valueTypeCode != declaredTypeCode)
	    REFLECT_WIDEN(v, valueTypeCode, declaredTypeCode, bad);

	REFLECT_SET(currentJavaFrame->topOfStack, declaredTypeCode, v);
	numBytes = CVMbasicTypeSizes[declaredTypeCode];
	/* 
	 * For any 64-bit ports that (unnecessarily) allocate two "Slots"
	 * for Longs and Doubles we have to accept 16 Bytes as well.
	 * For "single slot" Longs and Doubles
	 * this should be reverted to the original state.
	 */
#ifdef CVM_64
	CVMassert((numBytes == 1) ||
		  (numBytes == 2) ||
		  (numBytes == 4) ||
		  (numBytes == 8) ||
		  (numBytes == 16));
#else
	CVMassert((numBytes == 1) ||
		  (numBytes == 2) ||
		  (numBytes == 4) ||
		  (numBytes == 8));
#endif
	currentJavaFrame->topOfStack += (numBytes > sizeof(CVMJavaVal32)) ? 2 : 1;

	return CVM_TRUE;
    } else {
	CVMBool retVal;
	retVal = CVMgcUnsafeIsInstanceOf(ee, arg, argType);
      
	if (retVal == CVM_TRUE) {
	    /* arg is an instance of argType */
	    CVMID_icellSetDirect(ee,
				 (CVMObjectICell *)
				 currentJavaFrame->topOfStack,
				 arg);
	    (currentJavaFrame->topOfStack)++;
	    return CVM_TRUE;
	}
    }

bad:
    if (exceptionCb == NULL)
	CVMthrowIllegalArgumentException(ee, "argument type mismatch");
    else
	CVMsignalError(ee, exceptionCb, "argument type mismatch");
    return CVM_FALSE;
}

/* NOTE: unused "CVMreflectPopResult" removed. */

/* NOTE: invoke() functionality moved to
   javavm/native/java/lang/reflect/Method.c and Constructor.c. */

/*
 * Support code for class Class native code
 */


void
CVMreflectNewJavaLangReflectMethod(CVMExecEnv* ee,
				   CVMMethodBlock* mb,
				   CVMObjectICell* result)
{
    CVMClassBlock* returnType;
    CVMClassBlock* cb;
    /* 
     * Modify the member variable slot of type 'int'
     * and cast it to CVMMethodBlock*.  The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    CVMAddr mbPtr;
    CVMMethodTypeID nameAndTypeID;
    CVMSigIterator parameters;
    char* methodName;
    CVMUint32 modifiers;

    const CVMClassBlock* classJavaLangReflectMethod =
	CVMsystemClass(java_lang_reflect_Method);

    /* Casting away const is safe here (see classes.h, runtime flags) */
    CVMID_allocNewInstance(ee, (CVMClassBlock*) classJavaLangReflectMethod,
			   result);
    if (CVMID_icellIsNull(result)) {
	CVMthrowOutOfMemoryError(ee, "CVMreflectNewJavaLangReflectMethod: "
				 "out of memory allocating Method object");
	return;
    }

    /* Fill in fields. */

    /* Store Java class in which this field is contained */
    cb = CVMmbClassBlock(mb);
    CVMID_fieldWriteRef(ee, result, CVMoffsetOfjava_lang_reflect_Method_clazz,
			CVMcbJavaInstance(cb));

    /* 
     * Modify the member variable slot of type 'int'
     * and cast it to CVMMethodBlock*.  The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    mbPtr = (CVMAddr) mb;
    CVMID_fieldWriteAddr(ee, result, CVMoffsetOfjava_lang_reflect_Method_slot, mbPtr);

    nameAndTypeID =  CVMmbNameAndTypeID(mb);
    CVMtypeidGetSignatureIterator( nameAndTypeID, &parameters );

    CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMArrayOfRefICell, parameterTypes);
	CVMID_localrootDeclare(CVMArrayOfRefICell, checkedExceptions);

	/* Parameter types */
	CVMreflectGetParameterTypes(ee, &parameters, cb, parameterTypes);
	if (CVMID_icellIsNull(parameterTypes)) /* exception occurred */ {
	    CVMID_icellSetNull(result);
	    goto abort;
	}
	CVMID_fieldWriteRef(ee, result,
			    CVMoffsetOfjava_lang_reflect_Method_parameterTypes,
			    (CVMObjectICell*) parameterTypes);
	
	/* Return type */
	returnType = CVMclassLookupByTypeFromClass(
            ee, CVM_SIGNATURE_ITER_RETURNTYPE(parameters), CVM_FALSE, cb);
	if (returnType == NULL) {
	    /* exception occurred */	    
	    CVMID_icellSetNull(result);
	    goto abort;
	}
	CVMID_fieldWriteRef(ee, result,
			    CVMoffsetOfjava_lang_reflect_Method_returnType,
			    CVMcbJavaInstance(returnType));
	
	/* Checked exception types */
	CVMreflectGetExceptionTypes(ee, cb, mb, checkedExceptions);
	if (CVMID_icellIsNull(checkedExceptions)) /* exception occurred */ {
	    CVMID_icellSetNull(result);
	    goto abort;
	}
	CVMID_fieldWriteRef(ee, result,
			    CVMoffsetOfjava_lang_reflect_Method_exceptionTypes,
			    (CVMObjectICell*) checkedExceptions);

	/* Store method name as Java String. */
	methodName = CVMtypeidMethodNameToAllocatedCString(nameAndTypeID);
	if (methodName == NULL) {
	    CVMID_icellSetNull(result);
	    CVMthrowOutOfMemoryError(ee, "CVMreflectNewJavaLangReflectMethod: "
				     "out of memory allocating method name");
	    goto abort;
	}
	{
	    CVMBool success;
	    CVMID_localrootDeclare(CVMObjectICell, methodNameICell);
	    CVMnewStringUTF(ee, methodNameICell, methodName);
	    success = !CVMID_icellIsNull(methodNameICell);
	    CVMID_fieldWriteRef(ee, result, 
				CVMoffsetOfjava_lang_reflect_Method_name,
				methodNameICell);
	    free(methodName);
	    if (!success) {
		CVMID_icellSetNull(result);
		CVMthrowOutOfMemoryError(ee, NULL);
		goto abort;
	    }
	}

	/* Modifiers */
	/* Make sure we use the constants defined by the JVM spec as
	   opposed to any renumbered ones in the VM. We get these from
	   java.lang.reflect.Modifier via JavaCodeCompact. */
	modifiers = 0;
	if (CVMmbIs(mb, PUBLIC))
	    modifiers |= java_lang_reflect_Modifier_PUBLIC;
	if (CVMmbIs(mb, PRIVATE))
	    modifiers |= java_lang_reflect_Modifier_PRIVATE;
	if (CVMmbIs(mb, PROTECTED))
	    modifiers |= java_lang_reflect_Modifier_PROTECTED;
	if (CVMmbIs(mb, STATIC))
	    modifiers |= java_lang_reflect_Modifier_STATIC;
	if (CVMmbIs(mb, FINAL))
	    modifiers |= java_lang_reflect_Modifier_FINAL;
	if (CVMmbIs(mb, SYNCHRONIZED))
	    modifiers |= java_lang_reflect_Modifier_SYNCHRONIZED;
	if (CVMmbIs(mb, NATIVE))
	    modifiers |= java_lang_reflect_Modifier_NATIVE;
	if (CVMmbIs(mb, ABSTRACT))
	    modifiers |= java_lang_reflect_Modifier_ABSTRACT;
        if (CVMmbIsJava(mb) && CVMjmdIs(CVMmbJmd(mb), STRICT))
	    modifiers |= java_lang_reflect_Modifier_STRICT;
	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_reflect_Method_modifiers, modifiers);

	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_reflect_AccessibleObject_override,
			    CVM_FALSE);
    }
abort:
    CVMassert(CVMID_icellIsNull(result) == CVMlocalExceptionOccurred(ee));
    CVMID_localrootEnd();
}

/** Tell us whether this methodblock is <init> or <clinit>; returns
    CVMBool. */

#define CVMmbIsSpecial(mb) \
	(CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb)) || \
	 CVMtypeidIsStaticInitializer(CVMmbNameAndTypeID(mb)))
    
void
CVMreflectMethods(CVMExecEnv* ee,
		  CVMClassBlock* cb,
		  int which,
		  CVMArrayOfRefICell* result)
{
    CVMBool mustAbort = CVM_FALSE;

    const CVMClassBlock* classJavaLangReflectMethod =
	CVMsystemClass(java_lang_reflect_Method);

    if (CVMcbIs(cb, PRIMITIVE)) {
	/* Allocate zero-length array of java.lang.reflect.Method objects */
	/* Casting away const is safe here (see classes.h, runtime flags) */
	CVMreflectNewArray(ee, (CVMClassBlock*) classJavaLangReflectMethod,
			   0, (CVMArrayOfAnyTypeICell*) result);
	return;
    }

    CVMreflectEnsureLinked(ee, cb);

    switch (which) {

    case REFLECT_MEMBER_PUBLIC: {
	/* NOTE: this discrepancy between the treatment of interfaces
	   and regular classes comes about because interfaces re-use
	   their interface array as a flattened hierarchy of their
	   superinterfaces and also contain themselves in that table.
	   The reason for this self-reference is to make construction
	   of a real class's interface array easier; it is then simply
	   the concatenation of the interface arrays of all its
	   implemented interfaces. See classes.h for more details. */
	CVMBool isInterface = CVMcbIs(cb, INTERFACE);
	int cnt = 0, j;
        CVMMethodBlock **interfaceMBs = NULL;

	if (isInterface) {
            int newCnt = 0;

	    /* All interface methods are public */
	    WALK_INTERFACE_METHODS(cb, mb, cnt++);

            if (cnt == 0) {
                goto done;
            }

            interfaceMBs = malloc(cnt * sizeof(CVMMethodBlock *));
            if (interfaceMBs == NULL) {
	        CVMthrowOutOfMemoryError(ee, NULL);
	        return;
	    }

	    WALK_INTERFACE_METHODS(cb, mb, {
                int i;
                for (i = 0; i < newCnt; i++) {
		    CVMMethodTypeID nameAndTypeID = CVMmbNameAndTypeID(mb);
		    if (nameAndTypeID == CVMmbNameAndTypeID(interfaceMBs[i])) {
		        break;
		    }
		}
                if (i == newCnt) {
                    interfaceMBs[newCnt] = mb;
		    newCnt++;
		}
            });
            cnt = newCnt;
        done:
            ;

	} else {
	    /* Count public instance methods */
	    /* NOTE: CVMmbIsSpecial #defined above, in this file, not
               in classes.h */
	    WALK_INSTANCE_METHODS(cb, mb, {
		if (CVMmbIs(mb, PUBLIC)
		    && !CVMmbIsSpecial(mb))
		    cnt++;
	    });
	    
	    /* Count public static methods */
	    /* NOTE: CVMmbIsSpecial #defined above, in this file, not
               in classes.h */
	    WALK_SUPER_METHODS(cb, mb, {
		if (CVMmbIs(mb, PUBLIC)
		    && CVMmbIs(mb, STATIC)
		    && !CVMmbIsSpecial(mb))
		    cnt++;
	    });
	}

	/* Allocate array of java.lang.reflect.Method objects */
	/* Casting away const is safe here (see classes.h, runtime flags) */
	CVMreflectNewArray(ee, (CVMClassBlock*) classJavaLangReflectMethod,
			   cnt, (CVMArrayOfAnyTypeICell*) result);
	if (CVMID_icellIsNull(result)) { /* exception occurred */
	    if (interfaceMBs != NULL) {
	        free(interfaceMBs);
	    }
	    return;
	}	
	/* Fill array in declaration order */
	j = cnt;

	CVMID_localrootBegin(ee); {
	    CVMID_localrootDeclare(CVMObjectICell, methodICell);

	    if (isInterface) {
                int i;
		CVMassert(interfaceMBs != NULL || cnt == 0);
                for(i =0; i < cnt; i++) {
		    CVMMethodBlock *mb = interfaceMBs[i];
                    CVMassert(mb != NULL);
		    CVMreflectNewJavaLangReflectMethod(ee, mb, methodICell);
		    if (CVMID_icellIsNull(methodICell)) { /* exception occurred */
			CVMID_icellSetNull(result);
			/* Must execute CVMID_localrootEnd() */
			mustAbort = CVM_TRUE;
			goto abort1;
		    }
		    CVMID_arrayWriteRef(ee, result, i, methodICell);
                }
		CVMwithAssertsOnly(j -= i);

	    } else {

		/* Gather public instance methods */
		/* NOTE: CVMmbIsSpecial #defined above, in this file, not
		   in classes.h */
		WALK_INSTANCE_METHODS(cb, mb, {
		    if (CVMmbIs(mb, PUBLIC)
			&& !CVMmbIsSpecial(mb)) {
			CVMreflectNewJavaLangReflectMethod(ee, mb, methodICell);
			if (CVMID_icellIsNull(methodICell)) { /* exception occurred */
			    CVMID_icellSetNull(result);
			    /* Must execute CVMID_localrootEnd() */
			    mustAbort = CVM_TRUE;
			    goto abort1;
			}
			j--;
			CVMID_arrayWriteRef(ee, result, j, methodICell);
		    }
		});

		/* Gather public static methods */
		/* NOTE: CVMmbIsSpecial #defined above, in this file, not
		   in classes.h */
		WALK_SUPER_METHODS(cb, mb, {
		    if (CVMmbIs(mb, PUBLIC)
			&& CVMmbIs(mb, STATIC)
			&& !CVMmbIsSpecial(mb)) {
			CVMreflectNewJavaLangReflectMethod(ee, mb, methodICell);
			if (CVMID_icellIsNull(methodICell)) { /* exception occurred */
			    CVMID_icellSetNull(result);
			    /* Must execute CVMID_localrootEnd() */
			    mustAbort = CVM_TRUE;
			    goto abort1;
			}
			j--;
			CVMID_arrayWriteRef(ee, result, j, methodICell);
		    }
		});
	    }

	}
	abort1:
	CVMID_localrootEnd();
	if (interfaceMBs != NULL) {
	    free(interfaceMBs);
	}
	if (mustAbort)
	    return;

	CVMassert(j == 0);
	break;
    }

    case REFLECT_MEMBER_DECLARED:
    {
	CVMInt32 cnt = 0, j;

	/* Filter out static initializers and constructors */
	/* NOTE: CVMmbIsSpecial #defined above, in this file, not
	   in classes.h */
	WALK_DECLARED_METHODS(cb, mb, {
	    if (!CVMmbIsSpecial(mb))
		cnt++;
	});

	/* Allocate array of java.lang.reflect.Method objects */
	/* Casting away const is safe here (see classes.h, runtime flags) */
	CVMreflectNewArray(ee, (CVMClassBlock*) classJavaLangReflectMethod,
			   cnt, (CVMArrayOfAnyTypeICell*) result);
	if (CVMID_icellIsNull(result)) /* exception occurred */
	    return;

	/* Fill array in declaration order */
	j = cnt;

	CVMID_localrootBegin(ee); {
	    CVMID_localrootDeclare(CVMObjectICell, methodICell);

	    /* Filter out static initializers and constructors */
	    WALK_DECLARED_METHODS(cb, mb, {
		if (!CVMmbIsSpecial(mb)) {
		    CVMreflectNewJavaLangReflectMethod(ee, mb, methodICell);
		    if (CVMID_icellIsNull(methodICell)) { /* exception occurred */
			CVMID_icellSetNull(result);
			/* Must execute CVMID_localrootEnd() */
			mustAbort = CVM_TRUE;
			goto abort2;
		    }
		    j--;
		    CVMID_arrayWriteRef(ee, result, j, methodICell);
		}
	    });
	} 
    abort2:
	CVMID_localrootEnd();
	if (mustAbort)
	    return;

	CVMassert(j == 0);
	break;
    }

    default:
	CVMassert(CVM_FALSE);
    }
}

CVMBool
CVMreflectMatchParameterTypes(CVMExecEnv *ee,
			      CVMMethodBlock* mb,
			      CVMArrayOfRefICell* classes,
			      CVMInt32 cnt)
{
    CVMBool mustAbort = CVM_FALSE;
    CVMMethodTypeID nameAndTypeID = CVMmbNameAndTypeID(mb);
    CVMSigIterator signature;
    CVMClassTypeID p;
    CVMClassBlock* cb;
    CVMClassBlock* cbFromSig;
    CVMClassBlock* cbFromArray;
    CVMInt32 i = 0;
    /* 
     * Modify the member variable slot of type 'int'
     * and cast it to CVMClassBlock*.  The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    CVMAddr cbAsInt;

    CVMtypeidGetSignatureIterator( nameAndTypeID, &signature);

    cb = CVMmbClassBlock(mb);

    CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMObjectICell, classICell);
	
	while ( (p = CVM_SIGNATURE_ITER_NEXT( signature ) ) != CVM_TYPEID_ENDFUNC ){
	    /*
	     * The match must be exact, not by name.  This can be slow...
	     */
	    cbFromSig = CVMclassLookupByTypeFromClass(ee, p, CVM_FALSE, cb );
	    if (cbFromSig == NULL) {
		/* exception: Must execute CVMID_localrootEnd() */
		mustAbort = CVM_TRUE;
		goto abort;
	    }
	    CVMID_arrayReadRef(ee, classes, i, classICell);
	    i++;
	    /* This can be an assertion because the class can't be
               NULL at this point (see
               CVMreflectNewJavaLangReflectMethod) */
	    CVMassert(!CVMID_icellIsNull(classICell));

	    /* Get classblock out of java.lang.Class object */
	    /* 
	     * Modify the member variable slot of type 'int' and cast
	     * it to CVMClassBlock*.  The java type 'int' only
	     * guarantees 32 bit, but because one slot is used as
	     * storage space and a slot is 64 bit on 64 bit platforms,
	     * it is possible to store a native pointer without
	     * modification of java source code. This assumes that all
	     * places in the C-code which set/get this member are
	     * caught.  
	     */
	    CVMID_fieldReadAddr(ee, classICell,
				CVMoffsetOfjava_lang_Class_classBlockPointer,
				cbAsInt);
	    cbFromArray = (CVMClassBlock*) cbAsInt;
	    if (cbFromSig != cbFromArray) {
		/* Must execute CVMID_localrootEnd() */
		mustAbort = CVM_TRUE;
		goto abort;
	    }
	}

    }
abort:
    CVMID_localrootEnd();
    if (mustAbort)
	return CVM_FALSE;

    return CVM_TRUE;
}

void
CVMreflectMethod(CVMExecEnv* ee,
		 CVMClassBlock* cb,
		 const char* name,
		 CVMArrayOfRefICell* types,
		 int which,
		 CVMObjectICell* result)
{
    CVMInt32 tcnt, pcnt;
    CVMMethodTypeID nameCookie;

    GET_ARRAY_LENGTH(ee, types, tcnt); /* handles NULL */

    if (CVMcbIs(cb, PRIMITIVE)) {
	nameCookie = CVM_TYPEID_ERROR;
	goto nosuchmethod;
    }

    CVMreflectEnsureLinked(ee, cb);

    /*
     * We need to use the "new" function, because the "lookup" function
     * is only valid when we know that the type already exists.
     */
    nameCookie = CVMtypeidNewMembername(ee, name);
    if (nameCookie == CVM_TYPEID_ERROR) {
	/* Clear the exception. */
	CVMclearLocalException(ee);
	goto nosuchmethod;
    }

    switch (which) {

    case REFLECT_MEMBER_PUBLIC: {

	if (CVMcbIs(cb, INTERFACE)) {

	    WALK_INTERFACE_METHODS(cb, mb, {
		CVMMethodTypeID nameAndTypeID = CVMmbNameAndTypeID(mb);
		if ((CVMtypeidIsSameName(nameCookie, nameAndTypeID)) &&
		    (tcnt == 
		     (pcnt = CVMreflectGetParameterCount(nameAndTypeID))) &&
		    (pcnt == 0 ||
		     CVMreflectMatchParameterTypes(ee, mb, types, pcnt))) {
		    CVMreflectNewJavaLangReflectMethod(ee, mb, result);
		    goto finish;
		}
		/* CVMreflectMatchParameterTypes can throw an exception */
		if (CVMexceptionOccurred(ee)) {
		    goto finish;
		}
	    });

	    goto nosuchmethod;
	}

	WALK_INSTANCE_METHODS(cb, mb, {
	    CVMMethodTypeID nameAndTypeID = CVMmbNameAndTypeID(mb);
	    if (CVMmbIs(mb, PUBLIC) &&
		!CVMmbIsSpecial(mb) &&
		(CVMtypeidIsSameName(nameCookie, nameAndTypeID)) &&
		(tcnt ==
		 (pcnt = CVMreflectGetParameterCount(nameAndTypeID))) &&
		(pcnt == 0 ||
		 CVMreflectMatchParameterTypes(ee, mb, types, pcnt))) {
		CVMreflectNewJavaLangReflectMethod(ee, mb, result);
		goto finish;
	    }
	    /* CVMreflectMatchParameterTypes can throw an exception */
	    if (CVMexceptionOccurred(ee)) {
		goto finish;
	    }
	});

	WALK_SUPER_METHODS(cb, mb, {
	    CVMMethodTypeID nameAndTypeID = CVMmbNameAndTypeID(mb);
	    if (CVMmbIs(mb, PUBLIC) &&
		CVMmbIs(mb, STATIC) &&
		!CVMmbIsSpecial(mb) &&
		(CVMtypeidIsSameName(nameCookie, nameAndTypeID )) &&
		(tcnt ==
		 (pcnt = CVMreflectGetParameterCount(nameAndTypeID))) &&
		(pcnt == 0 ||
		 CVMreflectMatchParameterTypes(ee, mb, types, pcnt))) {
		CVMreflectNewJavaLangReflectMethod(ee, mb, result);
		goto finish;
	    }
	    /* CVMreflectMatchParameterTypes can throw an exception */
	    if (CVMexceptionOccurred(ee)) {
		goto finish;
	    }
	});

	break;
    }

    case REFLECT_MEMBER_DECLARED: {
        CVMMethodBlock *foundMB = NULL;
	int foundMBClassDepth = 0;

	WALK_DECLARED_METHODS(cb, mb, {
	    CVMMethodTypeID nameAndTypeID = CVMmbNameAndTypeID(mb);
	    if (!CVMmbIsSpecial(mb) &&
		(CVMtypeidIsSameName(nameCookie, nameAndTypeID )) &&
		(tcnt ==
		 (pcnt = CVMreflectGetParameterCount(nameAndTypeID))) &&
		(pcnt == 0 ||
		 CVMreflectMatchParameterTypes(ee, mb, types, pcnt))) {

	        /* In case of ambiguous method overloading, we need to
		   walk all the methods and look for the most "specific"
		   method: */
	        CVMSigIterator signature;
		CVMClassTypeID returnTypeID;

		CVMtypeidGetSignatureIterator(nameAndTypeID, &signature);
		returnTypeID = CVM_SIGNATURE_ITER_RETURNTYPE(signature);

		if (CVMtypeidIsPrimitive(returnTypeID)) {
		    /* If the first method we encounter is a primitive type,
		       then its as "specific" as it gets.  We need not compare
		       primitive types with class types even if some exists.
		       The 2 are considered equally "specific". */
		    if (foundMB == NULL) {
		        foundMB = mb;
		        goto found;
		    }

		} else {
	            CVMClassBlock *returnTypeCB;
		    CVMClassBlock *superCB;
	            int classDepth = 0;

		    /* Get the return type CB: */
		    returnTypeCB = CVMclassLookupByTypeFromClass(ee,
				       returnTypeID, CVM_FALSE,
				       CVMmbClassBlock(mb));
		    if (CVMexceptionOccurred(ee)) {
		        goto finish;
		    }

		    /* Scan the superclass tree and count the inheritance
		       depth: */
		    superCB = returnTypeCB;
		    while (superCB != NULL) {
		        superCB = CVMcbSuperclass(superCB);
			classDepth++;
		    }

	            if (foundMB == NULL || (classDepth > foundMBClassDepth)) {
		        foundMB = mb;
		        foundMBClassDepth =  classDepth;
	            }
		}
	    }
	    /* CVMreflectMatchParameterTypes can throw an exception */
	    if (CVMexceptionOccurred(ee)) {
		goto finish;
	    }
	});

    found:
	if (foundMB != NULL) {
	    CVMreflectNewJavaLangReflectMethod(ee, foundMB, result);
	    goto finish;
	}

	break;
    }

    default:
	CVMassert(CVM_FALSE);
    }

  nosuchmethod:
    CVMthrowNoSuchMethodException(ee, "%s", name);

 finish:
    if (nameCookie != CVM_TYPEID_ERROR) {
	CVMtypeidDisposeMembername(ee, nameCookie);
    }
}

/*
 *
 */


void
CVMreflectNewJavaLangReflectConstructor(CVMExecEnv* ee,
					CVMMethodBlock* mb,
					CVMObjectICell* result)
{
    CVMClassBlock *cb;

    const CVMClassBlock* classJavaLangReflectConstructor =
	CVMsystemClass(java_lang_reflect_Constructor);

    /* 
     * Modify the member variable slot of type 'int'
     * and cast it to CVMMethodBlock*.  The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    CVMAddr mbPtr;
    CVMMethodTypeID nameAndTypeID;
    CVMSigIterator parameters;
    CVMUint32 modifiers;

    /* Casting away const is safe here (see classes.h, runtime flags) */
    CVMID_allocNewInstance(ee,
			   (CVMClassBlock*) classJavaLangReflectConstructor,
			   result);
    if (CVMID_icellIsNull(result)) {
	CVMthrowOutOfMemoryError(ee, "out of memory allocating Constructor object");
	return;
    }

    CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMArrayOfRefICell, parameterTypes);
	CVMID_localrootDeclare(CVMArrayOfRefICell, exceptionTypes);

	/* declaring class */
	cb = CVMmbClassBlock(mb);
	CVMID_fieldWriteRef(ee, result,
			    CVMoffsetOfjava_lang_reflect_Constructor_clazz,
			    CVMcbJavaInstance(cb));

	/* Store raw methodblock pointer as int (NOTE: not 64-bit clean) */
	/* 
	 * Modify the member variable slot of type 'int'
	 * and cast it to CVMMethodBlock*.  The java type 'int' only guarantees
	 * 32 bit, but because one slot is used as storage space and
	 * a slot is 64 bit on 64 bit platforms, it is possible 
	 * to store a native pointer without modification of
	 * java source code. This assumes that all places in the C-code
	 * which set/get this member are caught.
	 */
	mbPtr = (CVMAddr) mb;
	CVMID_fieldWriteAddr(ee, result,
			     CVMoffsetOfjava_lang_reflect_Constructor_slot,
			     mbPtr);

	/* parameter types from signature */
	nameAndTypeID =  CVMmbNameAndTypeID(mb);
	CVMtypeidGetSignatureIterator( nameAndTypeID, &parameters );

	CVMreflectGetParameterTypes(ee, &parameters, cb, parameterTypes);
	if (CVMID_icellIsNull(parameterTypes)) {
	    CVMID_icellSetNull(result);
	    /* Must execute CVMID_localrootEnd() */
	    goto abort; /* exception */
	}
	CVMID_fieldWriteRef(ee, result,
			    CVMoffsetOfjava_lang_reflect_Constructor_parameterTypes,
			    (CVMObjectICell*) parameterTypes);

	CVMreflectGetExceptionTypes(ee, cb, mb, exceptionTypes);
	if (CVMID_icellIsNull(exceptionTypes)) {
	    CVMID_icellSetNull(result);
	    /* Must execute CVMID_localrootEnd() */
	    goto abort; /* exception */
	}
	CVMID_fieldWriteRef(ee, result,
			    CVMoffsetOfjava_lang_reflect_Constructor_exceptionTypes,
			    (CVMObjectICell*) exceptionTypes);

	/* Modifiers */
	/* Make sure we use the constants defined by the JVM spec as
	   opposed to any renumbered ones in the VM. We get these from
	   java.lang.reflect.Modifier via JavaCodeCompact. */
	modifiers = 0;
	if (CVMmbIs(mb, PUBLIC))
	    modifiers |= java_lang_reflect_Modifier_PUBLIC;
	if (CVMmbIs(mb, PRIVATE))
	    modifiers |= java_lang_reflect_Modifier_PRIVATE;
	if (CVMmbIs(mb, PROTECTED))
	    modifiers |= java_lang_reflect_Modifier_PROTECTED;
	if (CVMmbIs(mb, STATIC))
	    modifiers |= java_lang_reflect_Modifier_STATIC;
	if (CVMmbIs(mb, FINAL))
	    modifiers |= java_lang_reflect_Modifier_FINAL;
	if (CVMmbIs(mb, SYNCHRONIZED))
	    modifiers |= java_lang_reflect_Modifier_SYNCHRONIZED;
	if (CVMmbIs(mb, NATIVE))
	    modifiers |= java_lang_reflect_Modifier_NATIVE;
	if (CVMmbIs(mb, ABSTRACT))
	    modifiers |= java_lang_reflect_Modifier_ABSTRACT;
        if (CVMmbIsJava(mb) && CVMjmdIs(CVMmbJmd(mb), STRICT))
            modifiers |= java_lang_reflect_Modifier_STRICT;
	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_reflect_Constructor_modifiers,
			    modifiers);

	CVMID_fieldWriteInt(ee, result,
			    CVMoffsetOfjava_lang_reflect_AccessibleObject_override,
			    CVM_FALSE);
    }
abort:
    CVMID_localrootEnd();
}

void
CVMreflectConstructors(CVMExecEnv* ee,
		       CVMClassBlock* cb,
		       int which,
		       CVMArrayOfRefICell* result)
{
    CVMBool mustAbort = CVM_FALSE;
    CVMBool pub = (which == REFLECT_MEMBER_PUBLIC);
    CVMInt32 cnt = 0, j;

    const CVMClassBlock* classJavaLangReflectConstructor =
	CVMsystemClass(java_lang_reflect_Constructor);

    /* Exclude primitive, interface and array types */
    if (CVMcbIs(cb, PRIMITIVE) ||
	CVMcbIs(cb, INTERFACE) ||
	CVMisArrayClass(cb)) {
	/* Allocate zero-length array of java.lang.reflect.Constructor objects */
	/* Casting away const is safe here (see classes.h, runtime flags) */
	CVMreflectNewArray(ee,
			   (CVMClassBlock*) classJavaLangReflectConstructor,
			   0, (CVMArrayOfAnyTypeICell*) result);
	return;
    }

    CVMreflectEnsureLinked(ee, cb);
    
    WALK_DECLARED_METHODS(cb, mb, {
	if (CVMmbIsSpecial(mb)
	    && (!pub || CVMmbIs(mb, PUBLIC))
	    && (CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb)))) {
	    cnt++;
	}
    });

    /* Allocate result array */
    /* Casting away const is safe here (see classes.h, runtime flags) */
    CVMreflectNewArray(ee, (CVMClassBlock*) classJavaLangReflectConstructor,
		       cnt, (CVMArrayOfAnyTypeICell*) result);
    if (CVMID_icellIsNull(result)) /* exception occurred */
	return;

    /* Fill in declared order */
    j = cnt;

    CVMID_localrootBegin(ee); {
	CVMID_localrootDeclare(CVMObjectICell, constructorICell);

	WALK_DECLARED_METHODS(cb, mb, {
	    if (CVMmbIsSpecial(mb)
		&& (!pub || CVMmbIs(mb, PUBLIC))
		&& (CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb)))) {
		CVMreflectNewJavaLangReflectConstructor(ee, mb,
							constructorICell);
		if (CVMID_icellIsNull(constructorICell)) {
		    /* exception occurred */
		    CVMID_icellSetNull(result);
		    /* Must execute CVMID_localrootEnd() */
		    mustAbort = CVM_TRUE;
		    goto abort;
		}
		j--;
		CVMID_arrayWriteRef(ee, result, j, constructorICell);
	    }
	});
    }
abort:
    CVMID_localrootEnd();
    if (mustAbort)
	return;

    CVMassert(j == 0);
}

void
CVMreflectConstructor(CVMExecEnv* ee,
		      CVMClassBlock* cb,
		      CVMArrayOfRefICell* types,
		      int which,
		      CVMObjectICell* result)
{
    CVMInt32 tcnt, pcnt;
    CVMBool pub = (which == REFLECT_MEMBER_PUBLIC);

    /* Exclude primitive, interface and array types */
    if (CVMcbIs(cb, PRIMITIVE) ||
	CVMcbIs(cb, INTERFACE) ||
	CVMisArrayClass(cb))
	goto nosuchmethod;

    CVMreflectEnsureLinked(ee, cb);

    GET_ARRAY_LENGTH(ee, types, tcnt); /* handles NULL */

    WALK_DECLARED_METHODS(cb, mb, {
	CVMMethodTypeID nameAndTypeID = CVMmbNameAndTypeID(mb);
	CVMSigIterator parameters;
	CVMtypeidGetSignatureIterator( nameAndTypeID, &parameters );
	if (CVMmbIsSpecial(mb) &&
	    (!pub || CVMmbIs(mb, PUBLIC)) &&
	    (CVMtypeidIsConstructor(nameAndTypeID)) &&
	    (tcnt == (pcnt = CVM_SIGNATURE_ITER_PARAMCOUNT(parameters))) &&
	    (pcnt == 0 ||
	     CVMreflectMatchParameterTypes(ee, mb, types, pcnt))) {
	    CVMreflectNewJavaLangReflectConstructor(ee, mb, result);
	    return;
	}
	/* CVMreflectMatchParameterTypes can throw an exception */
	if (CVMexceptionOccurred(ee)) {
	    return;
	}
    });
    
  nosuchmethod:
    CVMthrowNoSuchMethodException(ee, "<init>");
}

void
CVMreflectMethodBlockToNewJavaMethod(CVMExecEnv* ee,
				     CVMMethodBlock* mb,
				     CVMObjectICell* result)
{
    /* %comment k003 */
    if (CVMmbIsSpecial(mb))
	CVMreflectNewJavaLangReflectConstructor(ee, mb, result);
    else
	CVMreflectNewJavaLangReflectMethod(ee, mb, result);
}

CVMMethodBlock*
CVMreflectGCSafeGetMethodBlock(CVMExecEnv* ee,
			       CVMObjectICell* method,
			       CVMClassBlock* exceptionCb)
{
    CVMMethodBlock* mb;
    
    CVMD_gcUnsafeExec(ee, {
	mb = CVMreflectGCUnsafeGetMethodBlock(ee,
					      CVMID_icellDirect(ee, method),
					      exceptionCb);
    });

    return mb;
}

CVMMethodBlock*
CVMreflectGCUnsafeGetMethodBlock(CVMExecEnv* ee,
				 CVMObject* method,
				 CVMClassBlock* exceptionCb)
{
    const CVMClassBlock* classJavaLangReflectConstructor;
    const CVMClassBlock* classJavaLangReflectMethod;
    CVMClassBlock* cb;
    /* 
     * Modify the member variable slot of type 'int'
     * and cast it to CVMMethodBlock*.  The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    CVMAddr mbAsInt;
    CVMMethodBlock* mb;

    if (method == NULL)
	return NULL;

    classJavaLangReflectConstructor =
	CVMsystemClass(java_lang_reflect_Constructor);
    classJavaLangReflectMethod =
	CVMsystemClass(java_lang_reflect_Method);

    /* Test incoming object. Note this is not redundant because
       calling code is native code. */
    cb = CVMobjectGetClass(method);
    CVMassert(cb != NULL);
    if ((cb != classJavaLangReflectConstructor) &&
	(cb != classJavaLangReflectMethod)) {
	if (exceptionCb == NULL) {
	    CVMthrowIllegalArgumentException(ee, NULL);
	} else {
	    CVMsignalError(ee, exceptionCb, NULL);
	}
	return NULL;
    }

    /* NOTE: not 64-bit clean */
    if (cb == classJavaLangReflectConstructor) {
	CVMD_fieldReadAddr(method,
			   CVMoffsetOfjava_lang_reflect_Constructor_slot,
			   mbAsInt);
    } else {
	CVMD_fieldReadAddr(method,
			   CVMoffsetOfjava_lang_reflect_Method_slot,
			   mbAsInt);
    }

    mb = (CVMMethodBlock*) mbAsInt;
    CVMassert(mb != NULL);
    return mb;
}

void
CVMreflectInterfaces(CVMExecEnv* ee,
		     CVMClassBlock* cb,
		     CVMArrayOfRefICell* result)
{
    int i;

    if (CVMisArrayClass(cb)) {
	/* All arrays implement java.lang.Cloneable and java.io.Serializable */
	CVMreflectNewClassArray(ee, 2, result);
	if (CVMID_icellIsNull(result))
	    return; /* exception already thrown */
	CVMID_arrayWriteRef(
            ee, result, 0,
	    CVMcbJavaInstance(CVMsystemClass(java_lang_Cloneable)));
	CVMID_arrayWriteRef(
            ee, result, 1,
	    CVMcbJavaInstance(CVMsystemClass(java_io_Serializable)));
    } else {
	CVMreflectNewClassArray(ee, CVMcbImplementsCount(cb), result);
	if (CVMID_icellIsNull(result))
	    return; /* exception already thrown */
	for (i = 0; i < CVMcbImplementsCount(cb); i++) {
	    /* NOTE that the JDK performs some constant pool
               resolution here, but it's unnecessary for us since we
               resolve all implemented interfaces' classblocks at link
               time. */
	    CVMID_arrayWriteRef(ee, result, i,
				CVMcbJavaInstance(CVMcbInterfacecb(cb, i)));
	}
    }
}

void
CVMreflectInnerClasses(CVMExecEnv* ee,
		       CVMClassBlock* cb,
		       CVMArrayOfRefICell* result)
{
    CVMConstantPool* cp = CVMcbConstantPool(cb);
    CVMClassBlock*   icb;
    CVMConstantPool* icp;
    CVMClassBlock**  member_cbs;
    int              member_count = 0;
    CVMUint32        icount = CVMcbInnerClassesInfoCount(cb);
    CVMUint32        i, j;
    
    if (icount == 0) {
	/* Skip the work */
	CVMreflectNewClassArray(ee, 0, result);
	return;
    }

    member_cbs = (CVMClassBlock**)malloc(icount * sizeof(CVMClassBlock*));
    if (member_cbs == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return;
    }

    /* Find any "member" classes. */
    for (i = 0; i < icount; i++) {
	CVMUint16 outerIdx = CVMcbInnerClassInfo(cb, i)->outerClassIndex;
	CVMUint16 innerIdx = CVMcbInnerClassInfo(cb, i)->innerClassIndex;
	CVMBool agree = CVM_FALSE;

	if (outerIdx == 0) { /* not a member */
	    continue;
	}
#ifdef CVM_CLASSLOADING
	if (!CVMcpResolveEntryFromClass(ee, cb, cp, outerIdx)) {
	    goto exception;
	}
#endif
	if (CVMcpGetCb(cp, outerIdx) != cb) {
	    continue;
	}

#ifdef CVM_CLASSLOADING
	/* "cb" is the outer class, so this is a member */
	if (!CVMcpResolveEntryFromClass(ee, cb, cp, innerIdx)) {
	    goto exception;
	}
#endif

	/* does member agree that "cb" is indeed its outer class? */
	icb = CVMcpGetCb(cp, innerIdx);
	icp = CVMcbConstantPool(icb);
	for (j = 0; j < CVMcbInnerClassesInfoCount(icb); j++) {
	    CVMUint16 i_outerIdx =
		CVMcbInnerClassInfo(icb, j)->outerClassIndex;
	    CVMUint16 i_innerIdx =
		CVMcbInnerClassInfo(icb, j)->innerClassIndex;

#ifdef CVM_CLASSLOADING
	    if (!CVMcpResolveEntryFromClass(ee, icb, icp, i_innerIdx)) {
		goto exception;
	    }
#endif
	    if (CVMcpGetCb(icp, i_innerIdx) != icb) {
		continue;
	    }

	    if (i_outerIdx == 0) {
		break; /* member says it is not a member; agree is FALSE. */
	    }
#ifdef CVM_CLASSLOADING
	    if (!CVMcpResolveEntryFromClass(ee, icb, icp, i_outerIdx)) {
		goto exception;
	    }
#endif
	    if (CVMcpGetCb(icp, i_outerIdx) == cb) {
		member_cbs[member_count++] = CVMcpGetCb(cp, innerIdx);
		agree = CVM_TRUE;
		break;
	    }
	}

	if (!agree) {
	    CVMthrowIncompatibleClassChangeError(
                ee, "%C and %C disagree on InnerClasses attribute", cb, icb);
	    goto exception;
	}
    }

    /* Allocate the result array */
    CVMreflectNewClassArray(ee, member_count, result);
    if (CVMID_icellIsNull(result)) {  /* exception */
	goto exception;
    }
    for (i = 0; i < member_count; i++) {
	CVMID_arrayWriteRef(ee, result, i, CVMcbJavaInstance(member_cbs[i]));
    }
    
 exception:
    free(member_cbs);
}

CVMClassBlock*
CVMreflectGetDeclaringClass(CVMExecEnv* ee,
			    CVMClassBlock* cb)
{
    CVMConstantPool* cp = CVMcbConstantPool(cb);
    CVMClassBlock*   ocb;
    CVMConstantPool* ocp;
    CVMUint32 icount = CVMcbInnerClassesInfoCount(cb);
    CVMUint32 i, j;

    if (icount == 0) {  /* neither an inner nor an outer class. */
	return NULL;
    }

    for (i = 0; i < icount; i++) {
	CVMUint16 outerIdx = CVMcbInnerClassInfo(cb, i)->outerClassIndex;
	CVMUint16 innerIdx = CVMcbInnerClassInfo(cb, i)->innerClassIndex;

#ifdef CVM_CLASSLOADING
	if (!CVMcpResolveEntryFromClass(ee, cb, cp, innerIdx)) {
	    return NULL;
	}
#endif
	if (CVMcpGetCb(cp, innerIdx) != cb) {
	    continue;
	}

	/* "cb" is the inner class, so this must be the right record */
	if (outerIdx == 0) {
	    return NULL;  /* not a member */
	}
#ifdef CVM_CLASSLOADING
	if (!CVMcpResolveEntryFromClass(ee, cb, cp, outerIdx)) {
	    return NULL;
	}
#endif

	/* does out class agree that "cb" is its member? */
	ocb = CVMcpGetCb(cp, outerIdx);
	ocp = CVMcbConstantPool(ocb);
	for (j = 0; j < CVMcbInnerClassesInfoCount(ocb); j++) {
	    CVMUint16 o_outerIdx =
		CVMcbInnerClassInfo(ocb, j)->outerClassIndex;
	    CVMUint16 o_innerIdx =
		CVMcbInnerClassInfo(ocb, j)->innerClassIndex;

	    if (o_outerIdx == 0) {
		continue;
	    }
#ifdef CVM_CLASSLOADING
	    if (!CVMcpResolveEntryFromClass(ee, ocb, ocp, o_outerIdx)) {
		return NULL;
	    }
#endif
	    if (CVMcpGetCb(ocp, o_outerIdx) != ocb) {
		continue;
	    }

#ifdef CVM_CLASSLOADING
	    if (!CVMcpResolveEntryFromClass(ee, ocb, ocp, o_innerIdx)) {
		return NULL;
	    }
#endif
	    if (CVMcpGetCb(ocp, o_innerIdx) == cb) {
		/* We verified both parties are in agreement. */
		return CVMcpGetCb(cp, outerIdx);
	    }
	}

	/* We found entry in "cb", but it doesn't agree with outer class. */
	{
	    CVMthrowIncompatibleClassChangeError(
                ee, "%C and %C disagree on InnerClasses attribute", cb, ocb);
	    return NULL;
	}
    }
    return NULL;
}

/********************************************************************
 * Utility routines from JDK 1.2's reflect_util.c (and others added) */

CVMBool
CVMreflectCheckAccess(CVMExecEnv* ee,
		      CVMClassBlock* memberClass, /* declaring class */
		      CVMUint32 acc,		  /* declared field or
						     method access */
		      CVMClassBlock* targetClass, /* for protected */
		      CVMBool forInvoke,          /* Is this being called 
						     from implementation of
						     Method.invoke()? */
		      CVMClassBlock* exceptionCb)
{
    CVMClassBlock* clientClass = NULL;

    /* Since we're going to be doing some private stuff here,
       let's make sure some invariants are met */
    CVMassert(CVM_METHOD_ACC_PROTECTED == CVM_FIELD_ACC_PROTECTED);

#define IsProtected(x)	CVMmemberPPPAccessIs((x), FIELD, PROTECTED)

    CVMassert(ee != NULL);

    /*
     * The "client" is the class associated with the nearest frame
     * which has a "real" methodblock, i.e., is not a
     * reflection-related method (Method.invoke(), and on Dean's
     * suggestion, Constructor.newInstance() or Class.newInstance() as
     * well) or transition frame. getCallerClass already skips
     * Method.invoke frames, so pass 0 in that case.
     */
    clientClass = CVMgetCallerClass(ee, (forInvoke) ? 0 : 1);

    if (clientClass != memberClass) {
	if (!CVMverifyClassAccess(ee, clientClass, memberClass, CVM_FALSE))
	    goto bad;
	if (!CVMverifyMemberAccess(ee, clientClass, memberClass,
				   acc, CVM_FALSE))
	    goto bad;
    }

    /*
     * Additional test for protected members: JLS 6.6.2
     * This could go faster if we had package objects
     */
    if (IsProtected(acc))
	if (targetClass != clientClass)
	    if (!CVMisSameClassPackage(ee, clientClass, memberClass))
		if (!CVMisSubclassOf(ee, targetClass, clientClass))
		    goto bad;

    /* Passed all tests */
    return CVM_TRUE;

  bad:
    /* Already throw C stack overflow exception */
    if (CVMlocalExceptionOccurred(ee)) {
	return CVM_FALSE;
    }
    else if (exceptionCb == NULL) {
	CVMthrowIllegalAccessException(ee, "%C", memberClass);
    } else {
	CVMsignalError(ee, exceptionCb, "%C", memberClass);
    }
    return CVM_FALSE;
}

void
CVMreflectNewArray(CVMExecEnv* ee,
		   CVMClassBlock* cb,
		   CVMInt32 length,
		   CVMArrayOfAnyTypeICell* result)
{
    /* Patterned after CVMjniNewObjectArray in jni_impl.c. */
    CVMClassBlock* arrayCb = CVMclassGetArrayOf(ee, cb);
    if (arrayCb == NULL) {
	return;
    }

    if (CVMcbIs(cb, PRIMITIVE)) {
	CVMID_allocNewArray(ee, CVMcbBasicTypeCode(cb),
			    arrayCb, length, (CVMObjectICell*) result);
    } else {
	CVMID_allocNewArray(ee, CVM_T_CLASS, arrayCb, length,
			    (CVMObjectICell*) result);
    }

    if (CVMID_icellIsNull(result))
	CVMthrowOutOfMemoryError(ee, "reflection: allocating new array");
}

void
CVMreflectNewClassArray(CVMExecEnv* ee, int length,
			CVMArrayOfRefICell* result)
{
    const CVMClassBlock* classJavaLangClass = CVMsystemClass(java_lang_Class);

    /* Casting away const is safe here (see classes.h, runtime flags) */
    CVMreflectNewArray(ee, (CVMClassBlock*) classJavaLangClass,
		       length, (CVMArrayOfAnyTypeICell*) result);
}

#endif /* CVM_REFLECT */
