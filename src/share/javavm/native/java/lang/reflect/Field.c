/*
 * @(#)Field.c	1.31 06/10/10
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

#include "javavm/include/interpreter.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/reflect.h"
#include "javavm/include/preloader.h"
#include "javavm/export/jvm.h"

#ifdef CVM_REFLECT

#include "generated/offsets/java_lang_Class.h"
#include "generated/offsets/java_lang_reflect_AccessibleObject.h"
#include "generated/offsets/java_lang_reflect_Field.h"

#endif /* CVM_REFLECT */

/*
 * Native methods (CVM calling convention) for java.lang.reflect.Field.
 */

/* NOTE that we are gc-UNSAFE when we enter and exit these
   methods. This is very important. If the GC-safety policy for CVM
   native methods changes, much of this code will need to be rewritten
   to run in an CVMD_gcUnsafeExec block. */

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_get(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    CVMObject* fieldObj;
    CVMObject* obj = NULL;
    CVMObject* fieldTypeObj;
    CVMFieldBlock* declaringClassFb;
    CVMClassBlock* declaringClassCb;
    /* 
     * fieldTypeCbAddr holds a native pointer as an int value
     * therefore change the type to CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    CVMAddr fieldTypeCbAddr;
    CVMClassBlock* fieldTypeCb;
    CVMClassBlock* objectCb;
    CVMInt32 override;
    jvalue v;
    CVMObject* refFieldVal;
    CVMBasicType fromType;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    fieldObj = CVMID_icellDirect(ee, &arguments[0].j.r);
    /* Can this happen? */
    if (fieldObj == NULL) {
	CVMthrowNullPointerException(ee, "java.lang.reflect.Field.get(): "
				     "null Field object");
	return CNI_EXCEPTION;
    }

    /* Get the declaring class's fieldblock corresponding to the Field
       object. */
    declaringClassFb = CVMreflectGCUnsafeGetFieldBlock(fieldObj);
    CVMassert(declaringClassFb != NULL);

    /* Get the declaring class of this field (note that the "class"
       field in java.lang.reflect.Field is redundant) */
    declaringClassCb = CVMfbClassBlock(declaringClassFb);
    CVMassert(declaringClassCb != NULL);

    /*
     * We need to make sure the class is initialized.
     */
    if (!CVMcbInitializationDoneFlag(ee, declaringClassCb)) {
	if (!CVMreflectEnsureInitialized(ee, declaringClassCb)) {
	    return CNI_EXCEPTION;
	}
	/* recache fieldObj since we become gcsafe. */
	fieldObj = CVMID_icellDirect(ee, &arguments[0].j.r);
    }

    /* See whether the field is static. If so, we ignore the object
       argument. */
    if (CVMfbIs(declaringClassFb, STATIC)) {
	objectCb = declaringClassCb;
    } else {
	/* Check whether the object is a non-null instance of the
           declaring class. */
	obj = CVMID_icellDirect(ee, &arguments[1].j.r);
	if (obj == NULL) {
	    CVMthrowNullPointerException(ee, "java.lang.reflect.Field.get(): "
					 "null argument object for non-static "
					 "field reference");
	    return CNI_EXCEPTION;
	}
	objectCb = CVMobjectGetClass(obj);
	if (!(objectCb == declaringClassCb ||
	      CVMgcUnsafeIsInstanceOf(ee, obj, declaringClassCb))) {
	    CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field.get(): "
					     "argument object is of the wrong type");
	    return CNI_EXCEPTION;
	}
    }

    /*
     * Access checking (unless overridden by Field)
     */
    CVMD_fieldReadInt(fieldObj,
		      CVMoffsetOfjava_lang_reflect_AccessibleObject_override,
		      override);
    if (!override) {
	if (!(CVMcbIs(declaringClassCb, PUBLIC) &&
	      CVMfbIs(declaringClassFb, PUBLIC))) {
	    /* NOTE: we pass in CVM_TRUE for "forInvoke" because we
               used the CNI calling convention to enter this code,
               which doesn't push an intermediary frame which we would
               need to skip. See documentation for CVMreflectCheckAccess. */
	    if (!CVMreflectCheckAccess(ee, declaringClassCb,
				       CVMfbAccessFlags(declaringClassFb),
				       objectCb, CVM_TRUE, NULL))
		return CNI_EXCEPTION;		/* exception */
	}
    }

    CVMD_fieldReadRef(fieldObj,
		      CVMoffsetOfjava_lang_reflect_Field_type,
		      fieldTypeObj);
    /* Now get the classblock of the type of the field. */
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMD_fieldReadAddr(fieldTypeObj,
		       CVMoffsetOfjava_lang_Class_classBlockPointer,
		       fieldTypeCbAddr);
    fieldTypeCb = (CVMClassBlock*) fieldTypeCbAddr;
    CVMassert(fieldTypeCb != NULL);

    if (CVMcbIs(fieldTypeCb, PRIMITIVE)) {
	/* (This code is identical to code in CVMgetPrimitiveField().
	   There might be a way to share) */
	fromType = CVMcbBasicTypeCode(fieldTypeCb);
	if (!CVMfbIs(declaringClassFb, STATIC)) {
	    /* many of these cases could probably be folded
	       together since we're fundamentally dealing with either
	       32- or 64-bit values anyway */
	    switch (fromType) {
	    case CVM_T_BOOLEAN:
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.z);
		break;

	    case CVM_T_CHAR:
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.c);
		break;

	    case CVM_T_FLOAT:
		CVMD_fieldReadFloat(obj,
				    CVMfbOffset(declaringClassFb),
				    v.f);
		break;

	    case CVM_T_DOUBLE:
		CVMD_fieldReadDouble(obj,
				     CVMfbOffset(declaringClassFb),
				     v.d);
		break;
	    case CVM_T_BYTE: 
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.b);
		break;

	    case CVM_T_SHORT:
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.s);
		break;

	    case CVM_T_INT:
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.i);
		break;

	    case CVM_T_LONG:
		CVMD_fieldReadLong(obj,
				 CVMfbOffset(declaringClassFb),
				 v.j);
		break;
	    default:
		CVMthrowInternalError(ee, "java.lang.reflect.Field.get(): "
				      "getting Java value. "
				      "A non-primitive class showed up"
				      " as primitive.");
		return CNI_EXCEPTION;
	    }
	} else {
	    switch (fromType) {
	    case CVM_T_BOOLEAN:
		v.z = CVMfbStaticField(ee, declaringClassFb).i;
		break;
		
	    case CVM_T_CHAR:
		v.c = CVMfbStaticField(ee, declaringClassFb).i;
		break;

	    case CVM_T_FLOAT:
		v.f = CVMfbStaticField(ee, declaringClassFb).f;
		break;

	    case CVM_T_DOUBLE:
		v.d = CVMjvm2Double(&CVMfbStaticField(ee, 
		    declaringClassFb).raw);
		break;

	    case CVM_T_BYTE:
		v.b = CVMfbStaticField(ee, declaringClassFb).i;
		break;

	    case CVM_T_SHORT:
		v.s = CVMfbStaticField(ee, declaringClassFb).i;
		break;

	    case CVM_T_INT:
		v.i = CVMfbStaticField(ee, declaringClassFb).i;
		break;

	    case CVM_T_LONG:
		v.j = CVMjvm2Long(&CVMfbStaticField(ee, declaringClassFb).raw);
		break;

	    default:
		CVMthrowInternalError(ee, "java.lang.reflect.Field.get(): "
				      "getting Java value. "
				      "A non-primitive class showed up "
				      "as primitive.");
		return CNI_EXCEPTION;
	    }
	}
	
	/* Note we leave frame->topOfStack untouched, because it is
	   decached by the interpreter when we return, but it is very
	   important that we don't offer a GC-safe point after
	   mutating the stack slot(s) corresponding to the return
	   value because of possible changes of refness, which would
	   make the stackmap incorrect. */

	/* Wrap the primitive value. Note we become GC-safe here, so
	   all of the direct object references we hold could
	   potentially become invalid. */

	CVMD_gcSafeExec(ee, {
	    CVMgcSafeJavaWrap(ee, v, fromType,
			      &arguments[0].j.r);
	});
    } else {
	/* Note we leave frame->topOfStack untouched, because it is
	   decached by the interpreter when we return, but it is very
	   important that we don't offer a GC-safe point after
	   mutating the stack slot(s) corresponding to the return
	   value because of possible changes of refness, which would
	   make the stackmap incorrect. */

	if (!CVMfbIs(declaringClassFb, STATIC)) {
	    CVMD_fieldReadRef(obj,
			      CVMfbOffset(declaringClassFb),
			      refFieldVal);
	    CVMID_icellSetDirect(ee, &arguments[0].j.r, refFieldVal);
	} else {
	    CVMID_icellAssignDirect(ee,
				    &arguments[0].j.r,
				    &(CVMfbStaticField(ee, 
						       declaringClassFb).r));
	}
    }

    return CNI_SINGLE;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

#ifdef CVM_REFLECT

static CNIResultCode
CVMgetPrimitiveField(CVMExecEnv* ee, CVMStackVal32 *arguments,
		     CVMMethodBlock **p_mb,
		     CVMBasicType destinationType)
{
    CVMObject* fieldObj;
    CVMObject* obj = NULL;
    CVMObject* fieldTypeObj;
    CVMFieldBlock* declaringClassFb;
    CVMClassBlock* declaringClassCb;
    /* 
     * fieldTypeCbAddr holds a native pointer as an int value
     * therefore change the type to CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    CVMAddr fieldTypeCbAddr;
    CVMClassBlock* fieldTypeCb;
    CVMClassBlock* objectCb;
    CVMInt32 override;
    jvalue v;
    CVMBasicType fromType;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    fieldObj = CVMID_icellDirect(ee, &arguments[0].j.r);
    /* Can this happen? */
    if (fieldObj == NULL) {
	CVMthrowNullPointerException(ee, "java.lang.reflect.Field."
				     "getPrimitiveField(): "
				     "null Field object");
    }

    /* Get the declaring class's fieldblock corresponding to the Field
       object. */
    declaringClassFb = CVMreflectGCUnsafeGetFieldBlock(fieldObj);
    CVMassert(declaringClassFb != NULL);

    /* Get the declaring class of this field (note that the "class"
       field in java.lang.reflect.Field is redundant) */
    declaringClassCb = CVMfbClassBlock(declaringClassFb);
    CVMassert(declaringClassCb != NULL);

    /*
     * We need to make sure the class is initialized.
     */
    if (!CVMcbInitializationDoneFlag(ee, declaringClassCb)) {
	if (!CVMreflectEnsureInitialized(ee, declaringClassCb)) {
	    return CNI_EXCEPTION;
	}
	/* recache fieldObj since we become gcsafe. */
	fieldObj = CVMID_icellDirect(ee, &arguments[0].j.r);
    }

    /* See whether the field is static. If so, we ignore the object
       argument. */
    if (CVMfbIs(declaringClassFb, STATIC)) {
	objectCb = declaringClassCb;
    } else {
	/* Check whether the object is a non-null instance of the
           declaring class. */
	obj = CVMID_icellDirect(ee, &arguments[1].j.r);
	if (obj == NULL) {
	    CVMthrowNullPointerException(ee, "java.lang.reflect.Field."
					 "getPrimitiveField(): "
					 "null argument object for non-static "
					 "field reference");
	    return CNI_EXCEPTION;
	}
	objectCb = CVMobjectGetClass(obj);
	if (!(objectCb == declaringClassCb ||
	      CVMgcUnsafeIsInstanceOf(ee, obj, declaringClassCb))) {
	    CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field."
					     "getPrimitiveField(): "
					     "argument object is of "
					     "the wrong type");
	    return CNI_EXCEPTION;
	}
    }

    /*
     * Access checking (unless overridden by Field)
     */
    CVMD_fieldReadInt(fieldObj,
		      CVMoffsetOfjava_lang_reflect_AccessibleObject_override,
		      override);
    if (!override) {
	if (!(CVMcbIs(declaringClassCb, PUBLIC) &&
	      CVMfbIs(declaringClassFb, PUBLIC))) {
	    /* NOTE: we pass in CVM_TRUE for "forInvoke" because we
               used the CNI calling convention to enter this code,
               which doesn't push an intermediary frame which we would
               need to skip. See documentation for CVMreflectCheckAccess. */
	    if (!CVMreflectCheckAccess(ee, declaringClassCb,
				       CVMfbAccessFlags(declaringClassFb),
				       objectCb, CVM_TRUE, NULL))
		return CNI_EXCEPTION;		/* exception */
	}
    }

    CVMD_fieldReadRef(fieldObj,
		      CVMoffsetOfjava_lang_reflect_Field_type,
		      fieldTypeObj);
    /* Now get the classblock of the type of the field. */
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMD_fieldReadAddr(fieldTypeObj,
		       CVMoffsetOfjava_lang_Class_classBlockPointer,
		       fieldTypeCbAddr);
    fieldTypeCb = (CVMClassBlock*) fieldTypeCbAddr;
    CVMassert(fieldTypeCb != NULL);

    /* Now ready to acquire the field's value based on type. */
    if (CVMcbIs(fieldTypeCb, PRIMITIVE)) {
	fromType = CVMcbBasicTypeCode(fieldTypeCb);
	if (!CVMfbIs(declaringClassFb, STATIC)) {
	    /* many of these cases could probably be folded
	       together since we're fundamentally dealing with either
	       32- or 64-bit values anyway */
	    switch (fromType) {
	    case CVM_T_BOOLEAN:
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.z);
		break;

	    case CVM_T_CHAR:
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.c);
		break;

	    case CVM_T_FLOAT:
		CVMD_fieldReadFloat(obj,
				    CVMfbOffset(declaringClassFb),
				    v.f);
		break;

	    case CVM_T_DOUBLE:
		CVMD_fieldReadDouble(obj,
				 CVMfbOffset(declaringClassFb),
				 v.d);
		break;

	    case CVM_T_BYTE: 
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.b);
		break;

	    case CVM_T_SHORT:
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.s);
		break;

	    case CVM_T_INT:
		CVMD_fieldReadInt(obj,
				  CVMfbOffset(declaringClassFb),
				  v.i);
		break;

	    case CVM_T_LONG:
		CVMD_fieldReadLong(obj,
				 CVMfbOffset(declaringClassFb),
				 v.j);
		break;

	    default:
		CVMthrowInternalError(ee, "java.lang.reflect.Field."
				      "getPrimitiveField(): "
				      "getting Java value. "
				      "A non-primitive class "
				      "showed up as primitive.");
		return CNI_EXCEPTION;
	    }
	} else {
	    switch (fromType) {
	    case CVM_T_BOOLEAN:
		v.z = CVMfbStaticField(ee, declaringClassFb).i;
		break;
		
	    case CVM_T_CHAR:
		v.c = CVMfbStaticField(ee, declaringClassFb).i;
		break;

	    case CVM_T_FLOAT:
		v.f = CVMfbStaticField(ee, declaringClassFb).f;
		break;

	    case CVM_T_DOUBLE:
		v.d = CVMjvm2Double(&CVMfbStaticField(ee, 
						      declaringClassFb).raw);
		break;

	    case CVM_T_BYTE:
		v.b = CVMfbStaticField(ee, declaringClassFb).i;
		break;

	    case CVM_T_SHORT:
		v.s = CVMfbStaticField(ee, declaringClassFb).i;
		break;

	    case CVM_T_INT:
		v.i = CVMfbStaticField(ee, declaringClassFb).i;
		break;

	    case CVM_T_LONG:
		v.j = CVMjvm2Long(&CVMfbStaticField(ee, declaringClassFb).raw);
		break;

	    default:
		CVMthrowInternalError(ee, "java.lang.reflect.Field."
				      "getPrimitiveField(): "
				      "getting Java value. "
				      "A non-primitive class showed up "
				      "as primitive.");
		return CNI_EXCEPTION;
	    }
	}

	/* Note we leave frame->topOfStack untouched, because it is
	   decached by the interpreter when we return, but it is very
	   important that we don't offer a GC-safe point after
	   mutating the stack slot(s) corresponding to the return
	   value because of possible changes of refness, which would
	   make the stackmap incorrect. */

	/* Attempt to widen the primitive value. If this fails
	   then we throw an IllegalArgumentException. */
	if (fromType != destinationType)
	    REFLECT_WIDEN(v, fromType,
			  destinationType,
			  fail);

	REFLECT_SET(&arguments[0].j, destinationType, v);
	    
	if (destinationType == CVM_T_DOUBLE ||
	    destinationType == CVM_T_LONG)
	    return CNI_DOUBLE;
	else
	    return CNI_SINGLE;

    fail:
	CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field."
					 "getPrimitiveField(): "
					 "widening primitive value");
	return CNI_EXCEPTION;
    } else {
	CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field."
					 "getPrimitiveField(): "
					 "attempt to get object "
					 "field as primitive");
	return CNI_EXCEPTION;
    }
}

#endif /* CVM_REFLECT */

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_getBoolean(CVMExecEnv* ee, CVMStackVal32 *arguments,
				      CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMgetPrimitiveField(ee, arguments, p_mb, CVM_T_BOOLEAN);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_getByte(CVMExecEnv* ee, CVMStackVal32 *arguments,
				   CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMgetPrimitiveField(ee, arguments, p_mb, CVM_T_BYTE);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_getChar(CVMExecEnv* ee, CVMStackVal32 *arguments,
				   CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMgetPrimitiveField(ee, arguments, p_mb, CVM_T_CHAR);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_getShort(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMgetPrimitiveField(ee, arguments, p_mb, CVM_T_SHORT);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_getInt(CVMExecEnv* ee, CVMStackVal32 *arguments,
				  CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMgetPrimitiveField(ee, arguments, p_mb, CVM_T_INT);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_getLong(CVMExecEnv* ee, CVMStackVal32 *arguments,
				   CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMgetPrimitiveField(ee, arguments, p_mb, CVM_T_LONG);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_getFloat(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMgetPrimitiveField(ee, arguments, p_mb, CVM_T_FLOAT);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_getDouble(CVMExecEnv* ee, CVMStackVal32 *arguments,
				     CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMgetPrimitiveField(ee, arguments, p_mb, CVM_T_DOUBLE);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_set(CVMExecEnv* ee, CVMStackVal32 *arguments,
			       CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    CVMObject* fieldObj;
    CVMObject* obj = NULL;
    CVMObject* argObj;
    CVMObject* fieldTypeObj;
    CVMFieldBlock* declaringClassFb;
    CVMClassBlock* declaringClassCb;
    /* 
     * fieldTypeCbAddr holds a native pointer as an int value
     * therefore change the type to CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    CVMAddr fieldTypeCbAddr;
    CVMClassBlock* fieldTypeCb;
    CVMClassBlock* objectCb;
    CVMInt32 override;
    jvalue v;
    CVMBasicType fromType, toType;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    fieldObj = CVMID_icellDirect(ee, &arguments[0].j.r);
    /* Can this happen? */
    if (fieldObj == NULL) {
	CVMthrowNullPointerException(ee, "java.lang.reflect.Field.set(): "
				     "null Field object");
	return CNI_EXCEPTION;
    }

    /* Get the declaring class's fieldblock corresponding to the Field
       object. */
    declaringClassFb = CVMreflectGCUnsafeGetFieldBlock(fieldObj);
    CVMassert(declaringClassFb != NULL);

    /* Get the declaring class of this field (note that the "class"
       field in java.lang.reflect.Field is redundant) */
    declaringClassCb = CVMfbClassBlock(declaringClassFb);
    CVMassert(declaringClassCb != NULL);

    /* See whether the field is static. If so, we ignore the object
       argument. */
    if (CVMfbIs(declaringClassFb, STATIC)) {
	objectCb = declaringClassCb;
    } else {
	/* Check whether the object is a non-null instance of the
           declaring class. */
	obj = CVMID_icellDirect(ee, &arguments[1].j.r);
	if (obj == NULL) {
	    CVMthrowNullPointerException(ee, "java.lang.reflect.Field.set(): "
					 "null argument object for non-static "
					 "field reference");
	    return CNI_EXCEPTION;
	}
	objectCb = CVMobjectGetClass(obj);
	if (!(objectCb == declaringClassCb ||
	      CVMgcUnsafeIsInstanceOf(ee, obj, declaringClassCb))) {
	    CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field.set(): "
					     "argument object is of the wrong type");
	    return CNI_EXCEPTION;
	}
    }

    /*
     * Access checking (unless overridden by Field)
     */
    CVMD_fieldReadInt(fieldObj,
		      CVMoffsetOfjava_lang_reflect_AccessibleObject_override,
		      override);
    if (!override) {
	if (!(CVMcbIs(declaringClassCb, PUBLIC) &&
	      CVMfbIs(declaringClassFb, PUBLIC))) {
	    /* NOTE: we pass in CVM_TRUE for "forInvoke" because we
               used the CNI calling convention to enter this code,
               which doesn't push an intermediary frame which we would
               need to skip. See documentation for CVMreflectCheckAccess. */
	    if (!CVMreflectCheckAccess(ee, declaringClassCb,
				       CVMfbAccessFlags(declaringClassFb),
				       objectCb, CVM_TRUE, NULL))
		return CNI_EXCEPTION;		/* exception */
	}
    }

    if (CVMfbIs(declaringClassFb, FINAL)) {
	CVMthrowIllegalAccessException(ee, "field is final");
	return CNI_EXCEPTION;		/* exception */
    }

    CVMD_fieldReadRef(fieldObj,
		      CVMoffsetOfjava_lang_reflect_Field_type,
		      fieldTypeObj);
    /* Now get the classblock of the type of the field. */
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMD_fieldReadAddr(fieldTypeObj,
		       CVMoffsetOfjava_lang_Class_classBlockPointer,
		       fieldTypeCbAddr);
    fieldTypeCb = (CVMClassBlock*) fieldTypeCbAddr;
    CVMassert(fieldTypeCb != NULL);

    /* Get the argument object */
    argObj = CVMID_icellDirect(ee, &arguments[2].j.r);
    /* NOTE this is allowed to be NULL for non-primitive types */

    if (CVMcbIs(fieldTypeCb, PRIMITIVE)) {
	if (argObj == NULL) {
	    CVMthrowIllegalArgumentException(ee,
					     "java.lang.reflect.Field.set(): "
					     "null argument object for "
					     "primitive field");
	    return CNI_EXCEPTION;
	}
	toType = CVMcbBasicTypeCode(fieldTypeCb);
	if (CVMgcUnsafeJavaUnwrap(ee, argObj, &v, &fromType, NULL) ==
	    CVM_FALSE) {
	    return CNI_EXCEPTION;
	}
	if (fromType != toType)
	    REFLECT_WIDEN(v, fromType,
			  toType,
			  fail);
	/* Now we have the value we want to set.
	   Have to handle static and non-static fields differently. */
	if (!CVMfbIs(declaringClassFb, STATIC)) {
	    /* many of these cases could probably be folded
	       together. I think the
	       only issue is properly handling the values < 32
	       bits. */
	    switch (toType) {
	    case CVM_T_BOOLEAN:
		CVMD_fieldWriteInt(obj, CVMfbOffset(declaringClassFb),
				   v.z);
		break;
	    case CVM_T_CHAR:
		CVMD_fieldWriteInt(obj, CVMfbOffset(declaringClassFb),
				   v.c);
		break;
	    case CVM_T_FLOAT:
		CVMD_fieldWriteFloat(obj, CVMfbOffset(declaringClassFb),
				     v.f);
		break;
	    case CVM_T_DOUBLE:
		CVMD_fieldWriteDouble(obj, CVMfbOffset(declaringClassFb),
				      v.d);
		break;
	    case CVM_T_BYTE:
		CVMD_fieldWriteInt(obj, CVMfbOffset(declaringClassFb),
				   v.b);
		break;
	    case CVM_T_SHORT:
		CVMD_fieldWriteInt(obj,
				   CVMfbOffset(declaringClassFb),
				   v.s);
		break;
	    case CVM_T_INT:
		CVMD_fieldWriteInt(obj,
				   CVMfbOffset(declaringClassFb),
				   v.i);
		break;
	    case CVM_T_LONG:
		CVMD_fieldWriteLong(obj,
				    CVMfbOffset(declaringClassFb),
				    v.j);
		break;
	    default:
		CVMthrowInternalError(ee, "illegal primitive type");
		return CNI_EXCEPTION;
	    }
	} else {
	    /* many of these cases could probably be folded
	       together. I think the
	       only issue is properly handling the values < 32
	       bits. */
	    switch (toType) {
	    case CVM_T_BOOLEAN:
		CVMfbStaticField(ee, declaringClassFb).i = v.z;
		break;
	    case CVM_T_CHAR:	
		CVMfbStaticField(ee, declaringClassFb).i = v.c;
		break;
	    case CVM_T_FLOAT:
		CVMfbStaticField(ee, declaringClassFb).f = v.f;
		break;
	    case CVM_T_DOUBLE:
		CVMdouble2Jvm(&CVMfbStaticField(ee, 
						declaringClassFb).raw, v.d);
		break;
	    case CVM_T_BYTE:
		CVMfbStaticField(ee, declaringClassFb).i = v.b;
		break;
	    case CVM_T_SHORT:
		CVMfbStaticField(ee, declaringClassFb).i = v.s;
		break;
	    case CVM_T_INT:
		CVMfbStaticField(ee, declaringClassFb).i = v.i;
		break;
	    case CVM_T_LONG:
		CVMlong2Jvm(&CVMfbStaticField(ee, declaringClassFb).raw, v.j);
		break;
	    default:
		CVMthrowInternalError(ee, "illegal primitive type");
		return CNI_EXCEPTION;
	    }
	}
    } else {
	/* Check argument object against field's type */
	CVMBool retVal;
	/* NOTE this works for null objects as well */
	retVal = CVMgcUnsafeIsInstanceOf(ee, argObj, fieldTypeCb);
	if (retVal == CVM_TRUE) {
	    if (!CVMfbIs(declaringClassFb, STATIC)) {
		CVMD_fieldWriteRef(obj, CVMfbOffset(declaringClassFb),
				   argObj);
	    } else {
		CVMID_icellSetDirect(ee,
				     &(CVMfbStaticField(ee, 
							declaringClassFb).r),
				     argObj);
	    }
	} else {
	    CVMthrowIllegalArgumentException(ee,
					     "java.lang.reflect.Field.set(): "
					     "argument object is of "
					     "wrong type");
	    return CNI_EXCEPTION;
	}
	
    }

    return CNI_VOID;

fail:
    CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field.set():"
				     "widening primitive value");
    return CNI_EXCEPTION;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

#ifdef CVM_REFLECT

CNIResultCode
CVMsetPrimitiveField(CVMExecEnv* ee, CVMStackVal32 *arguments,
		     CVMMethodBlock **p_mb,
		     CVMBasicType argType)
{
    CVMObject* fieldObj;
    CVMObject* obj = NULL;
    CVMObject* fieldTypeObj;
    CVMFieldBlock* declaringClassFb;
    CVMClassBlock* declaringClassCb;
    /* 
     * fieldTypeCbAddr holds a native pointer as an int value
     * therefore change the type to CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    CVMAddr fieldTypeCbAddr;
    CVMClassBlock* fieldTypeCb;
    CVMClassBlock* objectCb;
    CVMInt32 override;
    CVMJavaVal32 val32;
    CVMJavaVal64 val64;		/* kept in "stack" order, not native order */
    CVMBasicType toType;
    jvalue v;
    CVMBool argIs64Bit;
    CVMBool fieldIs64Bit;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    fieldObj = CVMID_icellDirect(ee, &arguments[0].j.r);
    /* Can this happen? */
    if (fieldObj == NULL) {
	CVMthrowNullPointerException(ee, "java.lang.reflect.Field."
				     "setPrimitiveField(): "
				     "null Field object");
	return CNI_EXCEPTION;
    }

    /* Get the declaring class's fieldblock corresponding to the Field
       object. */
    declaringClassFb = CVMreflectGCUnsafeGetFieldBlock(fieldObj);
    CVMassert(declaringClassFb != NULL);

    /* Get the declaring class of this field (note that the "class"
       field in java.lang.reflect.Field is redundant) */
    declaringClassCb = CVMfbClassBlock(declaringClassFb);
    CVMassert(declaringClassCb != NULL);

    /* See whether the field is static. If so, we ignore the object
       argument. */
    if (CVMfbIs(declaringClassFb, STATIC)) {
        /* According to Field.set() API spec, if the underlying field
           is static and the class that declares the field is uninitialized,
           we need to initializ the class. */
        if (!CVMcbInitializationDoneFlag(ee, declaringClassCb)) {
            if (!CVMreflectEnsureInitialized(ee, declaringClassCb)) {
                return CNI_EXCEPTION;
            }
        }
	objectCb = declaringClassCb;
    } else {
	/* Check whether the object is a non-null instance of the
           declaring class. */
	obj = CVMID_icellDirect(ee, &arguments[1].j.r);
	if (obj == NULL) {
	    CVMthrowNullPointerException(ee, "java.lang.reflect.Field.set(): "
					 "null argument object for non-static "
					 "field reference");
	    return CNI_EXCEPTION;
	}
	objectCb = CVMobjectGetClass(obj);
	if (!(objectCb == declaringClassCb ||
	      CVMgcUnsafeIsInstanceOf(ee, obj, declaringClassCb))) {
	    CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field.set(): "
					     "argument object is of the wrong type");
	    return CNI_EXCEPTION;
	}
    }

    /*
     * Access checking (unless overridden by Field)
     */
    CVMD_fieldReadInt(fieldObj,
		      CVMoffsetOfjava_lang_reflect_AccessibleObject_override,
		      override);
    if (!override) {
	if (!(CVMcbIs(declaringClassCb, PUBLIC) &&
	      CVMfbIs(declaringClassFb, PUBLIC))) {
	    /* NOTE: we pass in CVM_TRUE for "forInvoke" because we
               used the CNI calling convention to enter this code,
               which doesn't push an intermediary frame which we would
               need to skip. See documentation for CVMreflectCheckAccess. */
	    if (!CVMreflectCheckAccess(ee, declaringClassCb,
				       CVMfbAccessFlags(declaringClassFb),
				       objectCb, CVM_TRUE, NULL))
		return CNI_EXCEPTION;		/* exception */
	}
    }

    if (CVMfbIs(declaringClassFb, FINAL)) {
	CVMthrowIllegalAccessException(ee, "field is final");
	return CNI_EXCEPTION;		/* exception */
    }

    CVMD_fieldReadRef(fieldObj,
		      CVMoffsetOfjava_lang_reflect_Field_type,
		      fieldTypeObj);
    /* Now get the classblock of the type of the field. */
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMD_fieldReadAddr(fieldTypeObj,
		       CVMoffsetOfjava_lang_Class_classBlockPointer,
		       fieldTypeCbAddr);
    fieldTypeCb = (CVMClassBlock*) fieldTypeCbAddr;
    CVMassert(fieldTypeCb != NULL);

    if (!CVMcbIs(fieldTypeCb, PRIMITIVE)) {
	/* Wording in the spec may lead the unwary to believe
	 * that this should work. Unfortunately.
	 */
	CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field."
					 "setPrimitiveField(): "
					 "attempt to set object field "
					 "as primitive" );
	return CNI_EXCEPTION;
    }

    /* Read the argument off the stack. How we do so depends on
       whether we're expecting a 64-bit argument or not. */
    argIs64Bit = (argType == CVM_T_LONG ||
		  argType == CVM_T_DOUBLE);
    if (argIs64Bit) {
	CVMmemCopy64(val64.v, &arguments[2].j.raw);
        val32.i = 0; /* fix compiler warning */
    } else {
	val32 = arguments[2].j;
        val64.l = CVMlongConstZero(); /* fix compiler warning */
    }
    
    toType = CVMcbBasicTypeCode(fieldTypeCb);
    fieldIs64Bit = (toType == CVM_T_LONG ||
		    toType == CVM_T_DOUBLE);
    if (argType != toType) {
	/* Put it in the appropriate slot of jvalue so we can use the
	   REFLECT_WIDEN macro */
	switch (argType) {
	case CVM_T_BOOLEAN:
	    v.z = val32.i;
	    break;
	case CVM_T_CHAR:
	    v.c = val32.i;
	    break;
	case CVM_T_FLOAT:
	    v.f = val32.f;
	    break;
	case CVM_T_DOUBLE:
	    /* "stack" order to native order */
	    v.d = CVMjvm2Double(val64.v);
	    break;
	case CVM_T_BYTE:
	    v.b = val32.i;
	    break;
	case CVM_T_SHORT:
	    v.s = val32.i;
	    break;
	case CVM_T_INT:
	    v.i = val32.i;
	    break;
	case CVM_T_LONG:
	    /* "stack" order to native order */
	    v.j = CVMjvm2Long(val64.v);
	    break;
	default:
	    CVMthrowInternalError(ee, "illegal primitive type");
	    return CNI_EXCEPTION;
	}
	REFLECT_WIDEN(v, argType, toType, fail);
	/* Put it back in the appropriate CVMJavaVal */
	switch (toType) {
	case CVM_T_BOOLEAN:
	    val32.i = v.z;
	    break;
	case CVM_T_CHAR:
	    val32.i = v.c;
	    break;
	case CVM_T_FLOAT:
	    val32.f = v.f;
	    break;
	case CVM_T_DOUBLE:
	    /* native order to "stack" order */
	    CVMdouble2Jvm(val64.v, v.d);
	    break;
	case CVM_T_BYTE:
	    val32.i = v.b;
	    break;
	case CVM_T_SHORT:
	    val32.i = v.s;
	    break;
	case CVM_T_INT:
	    val32.i = v.i;
	    break;
	case CVM_T_LONG:
	    /* native order to "stack" order */
	    CVMlong2Jvm(val64.v, v.j);
	    break;
	default:
	    CVMthrowInternalError(ee, "illegal primitive type");
	    return CNI_EXCEPTION;
	}
    }

    if (!CVMfbIs(declaringClassFb, STATIC)) {
	if (fieldIs64Bit) {
	    CVMD_fieldWrite64(obj,
			      CVMfbOffset(declaringClassFb),
			      (CVMJavaVal32*) val64.v);
	} else {
	    CVMD_fieldWrite32(obj,
			      CVMfbOffset(declaringClassFb),
			      val32);
	}
    } else {
	if (fieldIs64Bit) {
	    CVMmemCopy64(&CVMfbStaticField(ee, declaringClassFb).raw,
			 val64.v);
	} else {
	    CVMfbStaticField(ee, declaringClassFb) = val32;
	}
    }

    /* Note we leave frame->topOfStack untouched, because it is
       decached by the interpreter when we return, but it is very
       important that we don't offer a GC-safe point after
       mutating the stack slot(s) corresponding to the return
       value because of possible changes of refness, which would
       make the stackmap incorrect. */

    return CNI_VOID;

fail:
    CVMthrowIllegalArgumentException(ee, "java.lang.reflect.Field.set():"
				     "widening primitive value");
    return CNI_EXCEPTION;
}

#endif

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_setBoolean(CVMExecEnv* ee, CVMStackVal32 *arguments,
				      CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMsetPrimitiveField(ee, arguments, p_mb, CVM_T_BOOLEAN);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_setByte(CVMExecEnv* ee, CVMStackVal32 *arguments,
				   CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMsetPrimitiveField(ee, arguments, p_mb, CVM_T_BYTE);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_setChar(CVMExecEnv* ee, CVMStackVal32 *arguments,
				   CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMsetPrimitiveField(ee, arguments, p_mb, CVM_T_CHAR);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_setShort(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMsetPrimitiveField(ee, arguments, p_mb, CVM_T_SHORT);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_setInt(CVMExecEnv* ee, CVMStackVal32 *arguments,
				  CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMsetPrimitiveField(ee, arguments, p_mb, CVM_T_INT);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_setLong(CVMExecEnv* ee, CVMStackVal32 *arguments,
				   CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMsetPrimitiveField(ee, arguments, p_mb, CVM_T_LONG);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_setFloat(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMsetPrimitiveField(ee, arguments, p_mb, CVM_T_FLOAT);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Field_setDouble(CVMExecEnv* ee, CVMStackVal32 *arguments,
				     CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMsetPrimitiveField(ee, arguments, p_mb, CVM_T_DOUBLE);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}
