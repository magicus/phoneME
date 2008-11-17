/*
 * @(#)reflect.h	1.25 06/10/10
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

#ifndef _INCLUDED_REFLECT_H
#define _INCLUDED_REFLECT_H

#include "javavm/include/defs.h"
#include "javavm/include/stacks.h"
#include "javavm/export/jni.h"
#include "generated/jni/java_lang_reflect_Member.h"

/* Reflection support functions. These are in the order they are
   implemented in reflect.c. NOTE: should probably be ordered into
   public/private functions and declared so in this header; however,
   these are largely used by Method.c, Field.c, etc. and not elsewhere. */

enum { 
    REFLECT_MEMBER_PUBLIC   = java_lang_reflect_Member_PUBLIC, 
    REFLECT_MEMBER_DECLARED = java_lang_reflect_Member_DECLARED
};

extern CVMBool 
CVMreflectEnsureInitialized(CVMExecEnv* ee, CVMClassBlock* cb);

#ifdef CVM_CLASSLOADING
#define	CVMreflectEnsureLinked(ee, cb)			\
if (!CVMcbCheckRuntimeFlag(cb, LINKED)) {		\
    if (!CVMclassLink(ee, cb, CVM_FALSE)) {		\
	return;						\
    }							\
    CVMassert(CVMcbCheckRuntimeFlag(cb, LINKED));	\
}
#else
#define CVMreflectEnsureLinked(ee, cb)
#endif

/* NOTE: removed unused "REFLECT_GET" macro. */

/*
 * WARNING: This macro, unlike in JDK 1.2's sources, should ONLY be
 * used to manipulate the top of stack, rather than being used to copy
 * values out of the middle of Java objects.
 *
 * void
 * REFLECT_SET(void *p, CVMBasicType pCode, jvalue v);
 *
 * Set the value pointed to by P with basic type PCODE from the
 * appropriate slot of the jvalue V. PCODE *MUST* correspond to a
 * basic type, not a reference type; NOTE that this is DIFFERENT
 * BEHAVIOR THAN JDK 1.2.
 */

/* 
 * The following jvm2long, long2jvm, jvm2double and double2jvm are
 * used in conjunction with the 'raw' field of struct JavaVal32.
 * Because the raw field is of type CVMAddr, the type of location has
 * to be changed accordingly.
 *
 * CVMAddr is 8 byte on 64 bit platforms and 4 byte on 32 bit
 * platforms.
 */
#define	REFLECT_SET(p, pCode, v)					\
{                                                                       \
    switch (pCode) {                                                    \
    case CVM_T_BOOLEAN:                                                 \
	*(jint *) (p) = (jint) v.z;					\
        break;                                                          \
    case CVM_T_BYTE:							\
	*(jint *) (p) = (jint) v.b;					\
        break;                                                          \
    case CVM_T_CHAR:							\
	*(jint *) (p) = (jint) v.c;					\
        break;                                                          \
    case CVM_T_SHORT:							\
	*(jint *) (p) = (jint) v.s;					\
        break;                                                          \
    case CVM_T_INT:						        \
        *(jint *) (p) = v.i; break; 				        \
    case CVM_T_FLOAT:							\
        *(jfloat *) (p) = v.f; break;				        \
    case CVM_T_LONG:							\
        CVMlong2Jvm((CVMAddr*) p, v.j); break;			\
    case CVM_T_DOUBLE:							\
        CVMdouble2Jvm((CVMAddr*) p, v.d); break;			\
    default:							        \
	/* NOTE: requires local variable "ee" */			\
	CVMthrowIllegalArgumentException(ee, "reference type encountered " \
					 "during REFLECT_SET");		\
    }                                                                   \
}

/*
 * void
 * REFLECT_WIDEN(jvalue v, CVMBasicType fromCode,
 *		 CVMBasicType toCode, <label> fail);
 *
 * Widen the primitive value stored in V from the basic type indicated
 * by FROMCODE to that indicated by TOCODE. ** NOTE: FROMCODE and
 * TOCODE must not be equal. ** TOCODE must represent a "wider" type
 * than FROMCODE according to the Java language's rules for primitive
 * type assignment without the requirement of an explicit cast; for
 * example, truncation from float to int is not allowed.  FAIL must be
 * a label to which a goto may be executed in the case that the
 * wideness criterion between FROMCODE and TOCODE is not met.
 */

#define	REFLECT_WIDEN(v, fromCode, toCode, fail)			\
{									\
    switch(toCode) {							\
    case CVM_T_BOOLEAN:							\
    case CVM_T_BYTE:							\
    case CVM_T_CHAR:							\
	goto fail;							\
    case CVM_T_SHORT:							\
	switch (fromCode) {						\
	case CVM_T_BYTE:						\
            v.s = (jshort) v.b; break;					\
	default:							\
	    goto fail;							\
	}								\
	break;								\
    case CVM_T_INT:							\
	switch (fromCode) {						\
	case CVM_T_BYTE:						\
            v.i = (jint) v.b; break;                                    \
	case CVM_T_CHAR:						\
            v.i = (jint) v.c; break;                                    \
	case CVM_T_SHORT:						\
            v.i = (jint) v.s; break;                                    \
	default:							\
	    goto fail;							\
	}								\
	break;								\
    case CVM_T_LONG:							\
	switch (fromCode) {						\
	case CVM_T_BYTE:						\
	    v.j = CVMint2Long((jint) v.b); break;			\
	case CVM_T_CHAR:						\
            v.j = CVMint2Long((jint) v.c); break;                       \
	case CVM_T_SHORT:						\
            v.j = CVMint2Long((jint) v.s); break;                       \
	case CVM_T_INT:							\
            v.j = CVMint2Long(v.i); break;                              \
	default:							\
	    goto fail;							\
	}								\
	break;								\
    case CVM_T_FLOAT:							\
	switch (fromCode) {						\
	case CVM_T_BYTE:						\
            v.f = (jfloat) v.b; break;                                  \
	case CVM_T_CHAR:						\
            v.f = (jfloat) v.c; break;                                  \
	case CVM_T_SHORT:						\
            v.f = (jfloat) v.s; break;                                  \
	case CVM_T_INT:							\
	    v.f = (jfloat) v.i; break;					\
	case CVM_T_LONG:						\
	    v.f = CVMlong2Float(v.j); break;				\
	default:							\
	    goto fail;							\
	}								\
	break;								\
    case CVM_T_DOUBLE:							\
	switch (fromCode) {						\
	case CVM_T_BYTE:						\
            v.d = (jdouble) v.b; break;                                 \
	case CVM_T_CHAR:						\
            v.d = (jdouble) v.c; break;                                 \
	case CVM_T_SHORT:						\
            v.d = (jdouble) v.s; break;                                 \
	case CVM_T_INT:							\
	    v.d = (jdouble) v.i; break;					\
	case CVM_T_FLOAT:						\
	    v.d = (jdouble) v.f; break;					\
	case CVM_T_LONG:						\
	    v.d = CVMlong2Double(v.j); break;				\
	default:							\
	    goto fail;							\
	}								\
	break;								\
    default:								\
	goto fail;							\
    }									\
}

/** Reflect fields' information from the classblock CB, providing an
  array of java.lang.reflect.Field objects in RESULT. WHICH must be
  either REFLECT_MEMBER_PUBLIC or REFLECT_MEMBER_DECLARED. If it is
  REFLECT_MEMBER_PUBLIC then only the public fields are returned;
  otherwise all fields (even private ones) are returned. If an error
  occurs then RESULT is set to NULL and an exception is thrown in the
  VM. */

void
CVMreflectFields(CVMExecEnv* ee,
		 CVMClassBlock* cb,
		 int which,
		 CVMArrayOfRefICell* result);

/** Reflect information about field named NAME from the classblock CB,
  providing a java.lang.reflect.Field object in RESULT. WHICH must be
  either REFLECT_MEMBER_PUBLIC or REFLECT_MEMBER_DECLARED. If it is
  REFLECT_MEMBER_PUBLIC then only the public fields are searched;
  otherwise all fields (even private ones) are searched. If an error
  occurs, including the case where the field is not found, then RESULT
  is set to NULL and an exception (NoSuchFieldException,
  OutOfMemoryError, InternalError) is thrown in the VM. */

void
CVMreflectField(CVMExecEnv* ee,
		CVMClassBlock* cb,
		const char* name,
		int which,
		CVMObjectICell* result);

/* Utility function used by various reflection code. FB is the field
   block which will be reflected up to Java. RESULT is the ICell into
   which a new java.lang.reflect.Field object wrapping FB will be
   stored. Upon error, RESULT's value is set to NULL and an
   OutOfMemoryError is thrown in the VM. */

void
CVMreflectNewJavaLangReflectField(CVMExecEnv* ee,
				  CVMFieldBlock* fb,
				  CVMObjectICell* result);

/* Extract the fieldblock from a java.lang.reflect.Field object. This
   version should be called from GC-safe code. */
CVMFieldBlock*
CVMreflectGCSafeGetFieldBlock(CVMExecEnv* ee, CVMObjectICell* obj);

/* Extract the fieldblock from a java.lang.reflect.Field object. This
   version should only be called from GC-unsafe code and is guaranteed
   not to become GC-safe internally. */
CVMFieldBlock*
CVMreflectGCUnsafeGetFieldBlock(CVMObject* obj);

/* NOTE: removed unused "CVMreflectInvocationTargetException". */

/* Instance method resolution. Given the classblock OCB of a Java
   object and an arbitrary NON-STATIC methodblock MB which this object
   presumably implements or overrides, attempt to find and return
   methodblock of the correct method to invoke in OCB's method
   table. Upon successful resolution, returns the methodblock in OCB
   to invoke. Upon failure, returns NULL and, since this is only
   called from the native code for Method.invoke(), throws a
   Method$ArgumentException in the VM. */

CVMMethodBlock*
CVMreflectGetObjectMethodblock(CVMExecEnv* ee,
			       CVMClassBlock* ocb,
			       CVMMethodBlock* mb);

/*
 * Count parameters in a method's signature.
 */
int
CVMreflectGetParameterCount(CVMMethodTypeID sig);

/*
 * Convert method signature SIG of the class whose classblock is CB to
 * an array of java.lang.Class objects representing the argument
 * types, resolving classes relative to CB. Returns list of classes in
 * RESULT, which must be a valid ICell. If an error occurs (for
 * example, out of memory allocating the array, or unable to resolve
 * one of the classes), RESULT's value is set to NULL and an exception
 * is thrown in the VM. If the pointer CPP is not NULL, it is set to
 * the pointer in the signature after parsing of the argument list;
 * this is used for error checking.
 */
void
CVMreflectGetParameterTypes(CVMExecEnv* ee,
			    CVMSigIterator * sigp,
			    CVMClassBlock* cb,
			    CVMArrayOfRefICell* result);

/*
 * Returns the set of checked exceptions for the methodblock MB in
 * classblock CB in a newly-created array which is placed in the
 * passed ICell RESULT. Exception classes are resolved with respect to
 * class indicated by CB. In case of error, RESULT's value is set to
 * NULL and an exception is thrown in the VM.
 */
void
CVMreflectGetExceptionTypes(CVMExecEnv* ee,
			    CVMClassBlock* cb,
			    CVMMethodBlock* mb,
			    CVMArrayOfRefICell* result);

/*
 * Push one argument ARG of type ARGTYPE onto operand stack of passed
 * Java frame, with typechecking. Unwrap and widen primitives if
 * necessary. Returns CVM_TRUE upon success. Upon error (i.e., unable
 * to coerce ARG's wrapped value into type described by ARGTYPE)
 * returns CVM_FALSE. The CVMClassBlock* of the exception to be
 * thrown in the case of failure is passed in exceptionCb, and is
 * passed directly on to CVMsignalError. If this exceptionCb is
 * NULL and an error occurs, defaults to IllegalArgumentException or
 * NullPointerException (if an unwrapping conversion failed because of
 * a null object). This version should only be called from GC-unsafe
 * code and is guaranteed not to become GC-safe internally.
 */
CVMBool
CVMreflectGCUnsafePushArg(CVMExecEnv* ee,
			  CVMFrame* currentJavaFrame,
			  CVMObject* arg,
			  CVMClassBlock* argType,
			  CVMClassBlock* exceptionCb);

/* NOTE: removed unused "CVMreflectPopResult". */

/*
 * Support code for class Class native code
 */

/* Utility function used by various reflection code. MB is the method
   block which will be reflected up to Java. RESULT is the ICell into
   which a new java.lang.reflect.Method object wrapping MB will be
   stored. Upon error (e.g., out of memory), RESULT's value is
   NULL and an exception is thrown in the VM. */

void
CVMreflectNewJavaLangReflectMethod(CVMExecEnv* ee,
				   CVMMethodBlock* mb,
				   CVMObjectICell* result);

/** Reflect methods' information from the classblock CB, providing an
  array of java.lang.reflect.Method objects in RESULT. WHICH must be
  either REFLECT_MEMBER_PUBLIC or REFLECT_MEMBER_DECLARED. If it is
  REFLECT_MEMBER_PUBLIC then only the public methods are returned;
  otherwise all methods (even private ones) are returned. If an
  exception occurs then RESULT is set to NULL. */

void
CVMreflectMethods(CVMExecEnv* ee,
		  CVMClassBlock* cb,
		  int which,
		  CVMArrayOfRefICell* result);

/*
 * Parameter counts have already been matched, and cnt > 0.
 * Check the array of java.lang.Class objects and ensure that their
 * types match the parameter types of the given methodblock.
 * Requires: CLASSES is array of instances of java/lang/Class objects.
 */
CVMBool
CVMreflectMatchParameterTypes(CVMExecEnv *ee,
			      CVMMethodBlock* mb,
			      CVMArrayOfRefICell* classes,
			      CVMInt32 cnt);

/** Reflect information about method named NAME with argument types
  TYPES (a Java array of java.lang.Class objects) from the classblock
  CB, providing a java.lang.reflect.Method object in RESULT. WHICH
  must be either REFLECT_MEMBER_PUBLIC or REFLECT_MEMBER_DECLARED. If
  it is REFLECT_MEMBER_PUBLIC then only the public methods are
  searched; otherwise all methods (even private ones) are searched. If
  an error occurs, including the case where the method is not found,
  then RESULT is set to NULL and an exception is thrown in the VM. */

void
CVMreflectMethod(CVMExecEnv* ee,
		 CVMClassBlock* cb,
		 const char* name,
		 CVMArrayOfRefICell* types,
		 int which,
		 CVMObjectICell* result);

/* Utility function used by various reflection code. MB is the method
   block of the constructor which will be reflected up to Java. RESULT
   is the ICell into which a new java.lang.reflect.Constructor object
   wrapping MB will be stored. Upon error (e.g., out of memory),
   RESULT's value is NULL and an exception is thrown in the VM. */
void
CVMreflectNewJavaLangReflectConstructor(CVMExecEnv* ee,
					CVMMethodBlock* mb,
					CVMObjectICell* result);

/** Reflect constructors' information from the classblock CB,
  providing an array of java.lang.reflect.Constructor objects in
  RESULT. WHICH must be either REFLECT_MEMBER_PUBLIC or
  REFLECT_MEMBER_DECLARED. If it is REFLECT_MEMBER_PUBLIC then only
  the public fields are returned; otherwise all fields (even private
  ones) are returned. If an error occurs then RESULT is set to NULL
  and an exception is thrown in the VM. */

void
CVMreflectConstructors(CVMExecEnv* ee,
		       CVMClassBlock* cb,
		       int which,
		       CVMArrayOfRefICell* result);

/** Reflect information about constructor with argument types TYPES (a
  Java array of java.lang.Class objects) from the classblock CB,
  providing a java.lang.reflect.Method object in RESULT. WHICH must be
  either REFLECT_MEMBER_PUBLIC or REFLECT_MEMBER_DECLARED. If it is
  REFLECT_MEMBER_PUBLIC then only the public methods are searched;
  otherwise all methods (even private ones) are searched. If an error
  occurs, including the case where the method is not found, then
  RESULT is set to NULL and an exception is thrown in the VM. */

void
CVMreflectConstructor(CVMExecEnv* ee,
		      CVMClassBlock* cb,
		      CVMArrayOfRefICell* types,
		      int which,
		      CVMObjectICell* result);

/** This is a new addition in JDK 1.2 to allow JNI to easily translate
  between jmethodIDs and java.lang.reflect.Method/Constructor
  objects. Constructs either a new java.lang.reflect.Method or
  java.lang.reflect.Constructor object and returns in RESULT. Upon
  error RESULT is set to NULL and an exception is thrown in the VM. */

void
CVMreflectMethodBlockToNewJavaMethod(CVMExecEnv* ee,
				     CVMMethodBlock* mb,
				     CVMObjectICell* result);

/** This is a new addition in JDK 1.2 to allow JNI to easily translate
  between jmethodIDs and java.lang.reflect.Method/Constructor
  objects. Obtains the internal CVMMethodBlock pointer from a
  java.lang.reflect.Method or java.lang.reflect.Constructor object in
  METHOD. If the passed object is not either of these two types,
  returns NULL and throws an exception in the VM. The CVMClassBlock*
  of the exception to be thrown in the case of failure is passed
  in exceptionCb, and is passed directly on to CVMsignalError. If
  this exceptionCb is NULL and an error occurs, defaults to
  IllegalArgumentException. This version should only be called from
  GC-safe code. */

CVMMethodBlock*
CVMreflectGCSafeGetMethodBlock(CVMExecEnv* ee,
			       CVMObjectICell* method,
			       CVMClassBlock* exceptionCb);

/** This is a new addition in JDK 1.2 to allow JNI to easily translate
  between jmethodIDs and java.lang.reflect.Method/Constructor
  objects. Obtains the internal CVMMethodBlock pointer from a
  java.lang.reflect.Method or java.lang.reflect.Constructor object in
  METHOD. If the passed object is not either of these two types,
  returns NULL and throws an exception in the VM. The CVMClassBlock*
  of the exception to be thrown in the case of failure is passed
  in exceptionCb, and is passed directly on to CVMsignalError. If
  this exceptionCb is NULL and an error occurs, defaults to
  IllegalArgumentException. This version should only be called from
  GC-unsafe code and is guaranteed not to become GC-safe
  internally. */

CVMMethodBlock*
CVMreflectGCUnsafeGetMethodBlock(CVMExecEnv* ee,
				 CVMObject* method,
				 CVMClassBlock* exceptionCb);

/** Helper routine to implement JVM_GetClassInterfaces (jvm.c). RESULT
  is set to a newly-created array of java.lang.Class objects
  corresponding to the interfaces this class directly implements in
  left-to-right order; see javadocs for
  java.lang.Class.getInterfaces[] for specifics. NOTE that this takes
  advantage of the invariant in CVMClassBlock (which is not present in
  JDK 1.2's classblock) that the directly implemented interfaces are
  at the beginning of the class's interface table. In the case of an
  error (i.e., OutOfMemoryError), RESULT is set to NULL and an
  exception is thrown in the VM. */

void
CVMreflectInterfaces(CVMExecEnv* ee,
		     CVMClassBlock* cb,
		     CVMArrayOfRefICell* result);

/** Helper routine to implement JVM_GetDeclaredClasses (jvm.c). RESULT
  is set to a newly-created array of java.lang.Class objects
  corresponding to the inner classes of the class CB. If this class is
  a primitive class, array class, or Void.TYPE, or has no inner
  classes, returns a zero-length array. Upon error (i.e.,
  OutOfMemory), RESULT is set to NULL and an exception is thrown in
  the VM. */
void
CVMreflectInnerClasses(CVMExecEnv* ee,
		       CVMClassBlock* cb,
		       CVMArrayOfRefICell* result);

/** Helper routine to implement JVM_GetDeclaringClass (jvm.c). If this
  class is an inner class or interface of another class, returns that
  class; otherwise, returns NULL. */
CVMClassBlock*
CVMreflectGetDeclaringClass(CVMExecEnv* ee,
			    CVMClassBlock* cb);

/********************************************************************
 * Utility routines from JDK 1.2's reflect_util.c (and others added) */

/*
 * Check if field or method is accessible to client. If not, return
 * false and throw an exception. The CVMClassBlock* of the
 * exception to be thrown in the case of failure is passed in
 * exceptionCb, and is passed directly on to CVMsignalError. If this
 * exceptionCb is NULL and an error occurs, defaults to
 * IllegalAccessException. This is necessary because this potentially
 * needs to throw multiple types of exceptions: for example,
 * IllegalAccessException, Method$AccessException, and
 * Constructor$AccessException.
 *
 * NOTE: the "forInvoke" parameter is a carry-over from the JDK and
 * was needed for them to handle the cases of
 * {Method,Constructor}.invoke() and Field.get() separately, but
 * perform access checks in each. In CVM, Field.get() is currently
 * implemented using the "CNI" calling convention, which does not push
 * an intermediary frame before jumping into native code. For this
 * reason the access check in that method does not need to skip a
 * frame, so all of the callers of this function currently pass in
 * CVM_TRUE for "forInvoke". This would need to change if Field.get()
 * was implemented using JNI, however, so the parameter has been left
 * in.
 */

CVMBool
CVMreflectCheckAccess(CVMExecEnv* ee,
		      CVMClassBlock* fieldClass,	/* declaring class */
		      CVMUint32 acc,			/* declared field
							   access */
		      CVMClassBlock* targetClass,	/* for protected */
		      CVMBool forInvoke,		/* Is this being
							   called from
							   implementation of
							   Method.invoke()? */
		      CVMClassBlock* exceptionCb);

/** Utility function from JDK's reflect_util.c. Allocate an array of
    length LENGTH whose components are objects of type CB (which may
    correspond to a primitive type), returning result in ICell RESULT,
    which must be a valid ICell. Upon error such as OutOfMemory,
    RESULT is set to NULL and an exception is thrown in the VM. */

void
CVMreflectNewArray(CVMExecEnv* ee,
		   CVMClassBlock* cb,
		   CVMInt32 length,
		   CVMArrayOfAnyTypeICell* result);

/** Utility function from JDK's reflect_util.c. Allocate an array of
    length LENGTH whose components are objects of type java.lang.Class,
    returning result in ICell RESULT, which must be a valid
    ICell. Upon error such as OutOfMemory, RESULT is set to NULL and
    an exception is thrown in the VM. */
void
CVMreflectNewClassArray(CVMExecEnv* ee, int length,
			CVMArrayOfRefICell* result);

#endif /* _INCLUDED_REFLECT_H */
