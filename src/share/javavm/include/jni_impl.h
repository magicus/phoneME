/*
 * @(#)jni_impl.h	1.39 06/10/27
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

#ifndef _INCLUDED_JNI_IMPL_H
#define _INCLUDED_JNI_IMPL_H

#include "javavm/include/defs.h"
#include "javavm/include/stacks.h"
#include "javavm/export/jni.h"

/*
 * The CVMJNIFrame is used for both JNI method invocations and for
 * the JNI Push/PopLocalFrame calls. If the mb field is NULL then it
 * was created by JNIPushLocalFrame.
 *
 * Before allocating a jni local ref from topOfStack, freeList is
 * first checked to make sure the list is empty. Note that JNI methods
 * are allowed to try to exceed the ensured capacity. This can 
 * fail if a new CVMStackChunk is needed and can't be allocated.
 *
 * Scanning is done by looking at every cell from
 * frame->vals[0] through frame->topOfStack (which might not be
 * in the same CVMStackChunk frame). Cells that have
 * the lowest bit set are ignored because these are cells that
 * are on the freeList.
 */
typedef CVMFreelistFrame CVMJNIFrame;

#define CVMjniFrameInit(f_, mbIsAlreadyInited_) \
    CVMfreelistFrameInit((f_), (mbIsAlreadyInited_))

#define CVM_FRAMETYPE_JNI	CVM_FRAMETYPE_FREELIST


#define CVMJNIFrameCapacity CVMfreelistFrameCapacity  
#define CVM_JNI_MIN_CAPACITY 16

#define CVMgetJNIFrame(frame_)	CVMgetFreelistFrame((frame_))

#define CVMframeIsJNI(frame_)	CVMframeIsFreelist((frame_))

/*
 * Reset the topOfStack to point to the beginning of the stack.
 */
#define CVM_RESET_JNI_TOS(topOfStack_, frame_) \
    (topOfStack_) = (CVMStackVal32*)CVMgetJNIFrame(frame_)->vals;

#define CVM_INVOKE_VIRTUAL	0x0100
#define CVM_INVOKE_NONVIRTUAL	0x0200
#define CVM_INVOKE_STATIC	0x0300

#define CVM_INVOKE_TYPE_MASK	0xff00
#define CVM_INVOKE_RETTYPE_MASK	0x00ff

/*
 * CVMjniCreateLocalRef is used to create a local ref in the current frame.
 */
extern CVMObjectICell *
CVMjniCreateLocalRef(CVMExecEnv *);
/*
 * CVMjniCreateLocalRef is used to create a local ref for another thread.
 */
extern CVMObjectICell *
CVMjniCreateLocalRef0(CVMExecEnv *, CVMExecEnv *targetEE);

typedef struct {
    JNIEnv vector;
    /* other private data? */
} CVMJNIEnv;

#define CVMjniPrivEnv2PubEnv(priv)	(&(priv)->vector)

extern void CVMinitJNIEnv(CVMJNIEnv *);
extern void CVMdestroyJNIEnv(CVMJNIEnv *);

struct JNIInvokeInterface;

typedef struct {
    const struct JNIInvokeInterface * vector;

    /* other private data? */

    /*  Initialization state for JNI routines relating to
     *  java.nio.DirectBuffers:
     */
    volatile CVMBool directBufferSupportInitialized;
    volatile CVMBool directBufferSupportInitializeFailed;
    jclass bufferClass;
    jclass directBufferClass;
    jclass directByteBufferClass;
    jmethodID directByteBufferLongConstructor;
    jmethodID directByteBufferIntConstructor;
    jfieldID  directBufferAddressLongField;
    jfieldID  directBufferAddressIntField;
    jfieldID  bufferCapacityField;


} CVMJNIJavaVM;

extern void 
CVMinitJNIJavaVM(CVMJNIJavaVM *);

extern void 
CVMdestroyJNIJavaVM(CVMJNIJavaVM *);

#ifdef CVM_JVMTI

/* NOTE: This function should ONLY be called by routines whose
   task it is to instrument the JNI vector somehow (currently only
   CVMjvmtiInstrumentJNINativeInterface, though if we incorporated the
   "checked" version of the JNI, that would need to call this as
   well). Note that this returns a non-const version of the vector
   with the intent that it will be mutated, so it can't be declared
   const in jni_impl.c. */
extern struct JNINativeInterface *
CVMjniGetInstrumentableJNINativeInterface();

#endif /* CVM_JVMTI */

/*
 * Prototypes for JNI routines. These are only for use by other parts
 * of the VM. The intent is to eliminate multiple versions of the JNI
 * vector for JVMTI and the "checked" JNI interface, if that is later
 * incorporated. Instead, there is one JNI function vector, possibly
 * instrumented. These instrumented routines will call the underlying,
 * implementing JNI routines directly, rather than going through
 * another, "unchecked" JNI vector.  */

/* NOTE: these are listed in the order they appear in the JNI vector */
extern jint JNICALL CVMjniGetVersion(JNIEnv *env);
extern jclass JNICALL CVMjniFindClass(JNIEnv *env, const char *name);
extern jmethodID JNICALL CVMjniFromReflectedMethod(JNIEnv* env,
						   jobject method);
extern jfieldID JNICALL CVMjniFromReflectedField(JNIEnv* env, jobject field);
extern jobject JNICALL CVMjniToReflectedMethod(JNIEnv *env, jclass clazz,
					       jmethodID methodID,
					       jboolean isStatic);
#define CVMjniGetSuperclass Java_java_lang_Class_getSuperclass

extern jboolean JNICALL CVMjniIsAssignableFrom(JNIEnv* env,
					       jclass clazz1, jclass clazz2);
extern jobject JNICALL CVMjniToReflectedField(JNIEnv *env, jclass clazz,
					      jfieldID fieldID,
					      jboolean isStatic);
extern jint JNICALL CVMjniThrow(JNIEnv *env, jthrowable obj);
extern jint JNICALL CVMjniThrowNew(JNIEnv *env,
				   jclass clazz, const char *message);
extern jthrowable JNICALL CVMjniExceptionOccurred(JNIEnv *env);
extern void JNICALL CVMjniExceptionDescribe(JNIEnv *env);
extern void JNICALL CVMjniExceptionClear(JNIEnv *env);
extern void JNICALL CVMjniFatalError(JNIEnv *env, const char *msg);
extern jint JNICALL CVMjniPushLocalFrame(JNIEnv *env, jint capacity);
extern jobject JNICALL CVMjniPopLocalFrame(JNIEnv *env, jobject resultArg);
extern jobject JNICALL CVMjniNewGlobalRef(JNIEnv *env, jobject ref);
extern void JNICALL CVMjniDeleteGlobalRef(JNIEnv *env, jobject ref);
extern void JNICALL CVMjniDeleteLocalRef(JNIEnv *env, jobject obj);
extern jboolean JNICALL CVMjniIsSameObject(JNIEnv *env,
					   jobject ref1, jobject ref2);
extern jobject JNICALL CVMjniNewLocalRef(JNIEnv *env, jobject obj);
extern jint JNICALL CVMjniEnsureLocalCapacity(JNIEnv *env, jint capacity);
extern jobject JNICALL CVMjniAllocObject(JNIEnv *env, jclass clazz);
extern jobject JNICALL CVMjniNewObject(JNIEnv *env, jclass clazz,
				       jmethodID methodID, ...);
extern jobject JNICALL CVMjniNewObjectV(JNIEnv *env, jclass clazz,
					jmethodID methodID, va_list args);
extern jobject JNICALL CVMjniNewObjectA(JNIEnv *env, jclass clazz,
					jmethodID methodID, const jvalue *args);
extern jclass JNICALL CVMjniGetObjectClass(JNIEnv *env, jobject obj);
extern jboolean JNICALL CVMjniIsInstanceOf(JNIEnv* env, jobject obj,
					   jclass clazz);
extern jmethodID JNICALL CVMjniGetMethodID(JNIEnv *env, jclass clazz,
					   const char *name, const char *sig);

/*
 * Instance method calls
 */
extern jobject JNICALL CVMjniCallObjectMethod(JNIEnv *env, jobject obj,
					      jmethodID methodID, ...);
extern jobject JNICALL CVMjniCallObjectMethodV(JNIEnv *env, jobject obj,
					       jmethodID methodID,
					       va_list args);
extern jobject JNICALL CVMjniCallObjectMethodA(JNIEnv *env, jobject obj,
					       jmethodID methodID,
					       const jvalue *args);

extern jboolean JNICALL CVMjniCallBooleanMethod(JNIEnv *env, jobject obj,
						jmethodID methodID, ...);
extern jboolean JNICALL CVMjniCallBooleanMethodV(JNIEnv *env, jobject obj,
						 jmethodID methodID,
						 va_list args);
extern jboolean JNICALL CVMjniCallBooleanMethodA(JNIEnv *env, jobject obj,
						 jmethodID methodID,
						 const jvalue *args);

extern jbyte JNICALL CVMjniCallByteMethod(JNIEnv *env, jobject obj,
					  jmethodID methodID, ...);
extern jbyte JNICALL CVMjniCallByteMethodV(JNIEnv *env, jobject obj,
					   jmethodID methodID,
					   va_list args);
extern jbyte JNICALL CVMjniCallByteMethodA(JNIEnv *env, jobject obj,
					   jmethodID methodID,
					   const jvalue *args);

extern jchar JNICALL CVMjniCallCharMethod(JNIEnv *env, jobject obj,
					  jmethodID methodID, ...);
extern jchar JNICALL CVMjniCallCharMethodV(JNIEnv *env, jobject obj,
					   jmethodID methodID,
					   va_list args);
extern jchar JNICALL CVMjniCallCharMethodA(JNIEnv *env, jobject obj,
					   jmethodID methodID,
					   const jvalue *args);

extern jshort JNICALL CVMjniCallShortMethod(JNIEnv *env, jobject obj,
					    jmethodID methodID, ...);
extern jshort JNICALL CVMjniCallShortMethodV(JNIEnv *env, jobject obj,
					     jmethodID methodID,
					     va_list args);
extern jshort JNICALL CVMjniCallShortMethodA(JNIEnv *env, jobject obj,
					     jmethodID methodID,
					     const jvalue *args);

extern jint JNICALL CVMjniCallIntMethod(JNIEnv *env, jobject obj,
					jmethodID methodID, ...);
extern jint JNICALL CVMjniCallIntMethodV(JNIEnv *env, jobject obj,
					 jmethodID methodID,
					 va_list args);
extern jint JNICALL CVMjniCallIntMethodA(JNIEnv *env, jobject obj,
					 jmethodID methodID,
					 const jvalue *args);

extern jlong JNICALL CVMjniCallLongMethod(JNIEnv *env, jobject obj,
					  jmethodID methodID, ...);
extern jlong JNICALL CVMjniCallLongMethodV(JNIEnv *env, jobject obj,
					   jmethodID methodID,
					   va_list args);
extern jlong JNICALL CVMjniCallLongMethodA(JNIEnv *env, jobject obj,
					   jmethodID methodID,
					   const jvalue *args);

extern jfloat JNICALL CVMjniCallFloatMethod(JNIEnv *env, jobject obj,
					    jmethodID methodID, ...);
extern jfloat JNICALL CVMjniCallFloatMethodV(JNIEnv *env, jobject obj,
					     jmethodID methodID,
					     va_list args);
extern jfloat JNICALL CVMjniCallFloatMethodA(JNIEnv *env, jobject obj,
					     jmethodID methodID,
					     const jvalue *args);

extern jdouble JNICALL CVMjniCallDoubleMethod(JNIEnv *env, jobject obj,
					      jmethodID methodID, ...);
extern jdouble JNICALL CVMjniCallDoubleMethodV(JNIEnv *env, jobject obj,
					       jmethodID methodID,
					       va_list args);
extern jdouble JNICALL CVMjniCallDoubleMethodA(JNIEnv *env, jobject obj,
					       jmethodID methodID,
					       const jvalue *args);

extern void JNICALL CVMjniCallVoidMethod(JNIEnv *env, jobject obj,
					 jmethodID methodID, ...);
extern void JNICALL CVMjniCallVoidMethodV(JNIEnv *env, jobject obj,
					  jmethodID methodID,
					  va_list args);
extern void JNICALL CVMjniCallVoidMethodA(JNIEnv *env, jobject obj,
					  jmethodID methodID,
					  const jvalue *args);

/*
 * Non-virtual instance method calls
 */
extern jobject JNICALL CVMjniCallNonvirtualObjectMethod(JNIEnv *env,
							jobject obj,
							jclass clazz,
							jmethodID methodID,
							...);
extern jobject JNICALL CVMjniCallNonvirtualObjectMethodV(JNIEnv *env,
							 jobject obj,
							 jclass clazz,
							 jmethodID methodID,
							 va_list args);
extern jobject JNICALL CVMjniCallNonvirtualObjectMethodA(JNIEnv *env,
							 jobject obj,
							 jclass clazz,
							 jmethodID methodID,
							 const jvalue *args);

extern jboolean JNICALL CVMjniCallNonvirtualBooleanMethod(JNIEnv *env,
							  jobject obj,
							  jclass clazz,
							  jmethodID methodID,
							  ...);
extern jboolean JNICALL CVMjniCallNonvirtualBooleanMethodV(JNIEnv *env,
							   jobject obj,
							   jclass clazz,
							   jmethodID methodID,
							   va_list args);
extern jboolean JNICALL CVMjniCallNonvirtualBooleanMethodA(JNIEnv *env,
							   jobject obj,
							   jclass clazz,
							   jmethodID methodID,
							   const jvalue *args);

extern jbyte JNICALL CVMjniCallNonvirtualByteMethod(JNIEnv *env,
						    jobject obj,
						    jclass clazz,
						    jmethodID methodID,
						    ...);
extern jbyte JNICALL CVMjniCallNonvirtualByteMethodV(JNIEnv *env,
						     jobject obj,
						     jclass clazz,
						     jmethodID methodID,
						     va_list args);
extern jbyte JNICALL CVMjniCallNonvirtualByteMethodA(JNIEnv *env,
						     jobject obj,
						     jclass clazz,
						     jmethodID methodID,
						     const jvalue *args);

extern jchar JNICALL CVMjniCallNonvirtualCharMethod(JNIEnv *env,
						    jobject obj,
						    jclass clazz,
						    jmethodID methodID,
						    ...);
extern jchar JNICALL CVMjniCallNonvirtualCharMethodV(JNIEnv *env,
						     jobject obj,
						     jclass clazz,
						     jmethodID methodID,
						     va_list args);
extern jchar JNICALL CVMjniCallNonvirtualCharMethodA(JNIEnv *env,
						     jobject obj,
						     jclass clazz,
						     jmethodID methodID,
						     const jvalue *args);

extern jshort JNICALL CVMjniCallNonvirtualShortMethod(JNIEnv *env,
						      jobject obj,
						      jclass clazz,
						      jmethodID methodID,
						      ...);
extern jshort JNICALL CVMjniCallNonvirtualShortMethodV(JNIEnv *env,
						       jobject obj,
						       jclass clazz,
						       jmethodID methodID,
						       va_list args);
extern jshort JNICALL CVMjniCallNonvirtualShortMethodA(JNIEnv *env,
						       jobject obj,
						       jclass clazz,
						       jmethodID methodID,
						       const jvalue *args);

extern jint JNICALL CVMjniCallNonvirtualIntMethod(JNIEnv *env,
						  jobject obj,
						  jclass clazz,
						  jmethodID methodID,
						  ...);
extern jint JNICALL CVMjniCallNonvirtualIntMethodV(JNIEnv *env,
						   jobject obj,
						   jclass clazz,
						   jmethodID methodID,
						   va_list args);
extern jint JNICALL CVMjniCallNonvirtualIntMethodA(JNIEnv *env,
						   jobject obj,
						   jclass clazz,
						   jmethodID methodID,
						   const jvalue *args);

extern jlong JNICALL CVMjniCallNonvirtualLongMethod(JNIEnv *env,
						    jobject obj,
						    jclass clazz,
						    jmethodID methodID,
						    ...);
extern jlong JNICALL CVMjniCallNonvirtualLongMethodV(JNIEnv *env,
						     jobject obj,
						     jclass clazz,
						     jmethodID methodID,
						     va_list args);
extern jlong JNICALL CVMjniCallNonvirtualLongMethodA(JNIEnv *env,
						     jobject obj,
						     jclass clazz,
						     jmethodID methodID,
						     const jvalue *args);

extern jfloat JNICALL CVMjniCallNonvirtualFloatMethod(JNIEnv *env,
						      jobject obj,
						      jclass clazz,
						      jmethodID methodID,
						      ...);
extern jfloat JNICALL CVMjniCallNonvirtualFloatMethodV(JNIEnv *env,
						       jobject obj,
						       jclass clazz,
						       jmethodID methodID,
						       va_list args);
extern jfloat JNICALL CVMjniCallNonvirtualFloatMethodA(JNIEnv *env,
						       jobject obj,
						       jclass clazz,
						       jmethodID methodID,
						       const jvalue *args);

extern jdouble JNICALL CVMjniCallNonvirtualDoubleMethod(JNIEnv *env,
							jobject obj,
							jclass clazz,
							jmethodID methodID,
							...);
extern jdouble JNICALL CVMjniCallNonvirtualDoubleMethodV(JNIEnv *env,
							 jobject obj,
							 jclass clazz,
							 jmethodID methodID,
							 va_list args);
extern jdouble JNICALL CVMjniCallNonvirtualDoubleMethodA(JNIEnv *env,
							 jobject obj,
							 jclass clazz,
							 jmethodID methodID,
							 const jvalue *args);

extern void JNICALL CVMjniCallNonvirtualVoidMethod(JNIEnv *env,
						   jobject obj,
						   jclass clazz,
						   jmethodID methodID,
						   ...);
extern void JNICALL CVMjniCallNonvirtualVoidMethodV(JNIEnv *env,
						    jobject obj,
						    jclass clazz,
						    jmethodID methodID,
						    va_list args);
extern void JNICALL CVMjniCallNonvirtualVoidMethodA(JNIEnv *env,
						    jobject obj,
						    jclass clazz,
						    jmethodID methodID,
						    const jvalue *args);

/*
 * Field setters/getters
 */
extern jfieldID JNICALL CVMjniGetFieldID(JNIEnv *env, jclass clazz,
					 const char *name, const char *sig);

extern jobject JNICALL CVMjniGetObjectField(JNIEnv* env, jobject obj,
					    jfieldID fid);
extern jboolean JNICALL CVMjniGetBooleanField(JNIEnv* env, jobject obj,
					      jfieldID fid);
extern jbyte JNICALL CVMjniGetByteField(JNIEnv* env, jobject obj,
					jfieldID fid);
extern jchar JNICALL CVMjniGetCharField(JNIEnv* env, jobject obj,
					jfieldID fid);
extern jshort JNICALL CVMjniGetShortField(JNIEnv* env, jobject obj,
					  jfieldID fid);
extern jint JNICALL CVMjniGetIntField(JNIEnv* env, jobject obj,
				      jfieldID fid);
extern jlong JNICALL CVMjniGetLongField(JNIEnv* env, jobject obj,
					jfieldID fid);
extern jfloat JNICALL CVMjniGetFloatField(JNIEnv* env, jobject obj,
					  jfieldID fid);
extern jdouble JNICALL CVMjniGetDoubleField(JNIEnv* env, jobject obj,
					    jfieldID fid);

extern void JNICALL CVMjniSetObjectField(JNIEnv* env, jobject obj,
					 jfieldID fid, jobject rhs);
extern void JNICALL CVMjniSetBooleanField(JNIEnv* env, jobject obj,
					  jfieldID fid, jboolean rhs);
extern void JNICALL CVMjniSetByteField(JNIEnv* env, jobject obj,
				       jfieldID fid, jbyte rhs);
extern void JNICALL CVMjniSetCharField(JNIEnv* env, jobject obj,
				       jfieldID fid, jchar rhs);
extern void JNICALL CVMjniSetShortField(JNIEnv* env, jobject obj,
					jfieldID fid, jshort rhs);
extern void JNICALL CVMjniSetIntField(JNIEnv* env, jobject obj,
				      jfieldID fid, jint rhs);
extern void JNICALL CVMjniSetLongField(JNIEnv* env, jobject obj,
				       jfieldID fid, jlong rhs);
extern void JNICALL CVMjniSetFloatField(JNIEnv* env, jobject obj,
					jfieldID fid, jfloat rhs);
extern void JNICALL CVMjniSetDoubleField(JNIEnv* env, jobject obj,
					 jfieldID fid, jdouble rhs);

/*
 * Static method calls
 */
extern jmethodID JNICALL CVMjniGetStaticMethodID(JNIEnv *env, jclass clazz,
						 const char *name,
						 const char *sig);

extern jobject JNICALL CVMjniCallStaticObjectMethod(JNIEnv *env, jclass clazz,
						    jmethodID methodID, ...);
extern jobject JNICALL CVMjniCallStaticObjectMethodV(JNIEnv *env, jclass clazz,
						     jmethodID methodID,
						     va_list args);
extern jobject JNICALL CVMjniCallStaticObjectMethodA(JNIEnv *env, jclass clazz,
						     jmethodID methodID,
						     const jvalue *args);

extern jboolean JNICALL CVMjniCallStaticBooleanMethod(JNIEnv *env,
						      jclass clazz,
						      jmethodID methodID,
						      ...);
extern jboolean JNICALL CVMjniCallStaticBooleanMethodV(JNIEnv *env,
						       jclass clazz,
						       jmethodID methodID,
						       va_list args);
extern jboolean JNICALL CVMjniCallStaticBooleanMethodA(JNIEnv *env,
						       jclass clazz,
						       jmethodID methodID,
						       const jvalue *args);

extern jbyte JNICALL CVMjniCallStaticByteMethod(JNIEnv *env, jclass clazz,
						jmethodID methodID, ...);
extern jbyte JNICALL CVMjniCallStaticByteMethodV(JNIEnv *env, jclass clazz,
						 jmethodID methodID,
						 va_list args);
extern jbyte JNICALL CVMjniCallStaticByteMethodA(JNIEnv *env, jclass clazz,
						 jmethodID methodID,
						 const jvalue *args);

extern jchar JNICALL CVMjniCallStaticCharMethod(JNIEnv *env, jclass clazz,
						jmethodID methodID, ...);
extern jchar JNICALL CVMjniCallStaticCharMethodV(JNIEnv *env, jclass clazz,
						 jmethodID methodID,
						 va_list args);
extern jchar JNICALL CVMjniCallStaticCharMethodA(JNIEnv *env, jclass clazz,
						 jmethodID methodID,
						 const jvalue *args);

extern jshort JNICALL CVMjniCallStaticShortMethod(JNIEnv *env, jclass clazz,
						  jmethodID methodID, ...);
extern jshort JNICALL CVMjniCallStaticShortMethodV(JNIEnv *env, jclass clazz,
						   jmethodID methodID,
						   va_list args);
extern jshort JNICALL CVMjniCallStaticShortMethodA(JNIEnv *env, jclass clazz,
						   jmethodID methodID,
						   const jvalue *args);

extern jint JNICALL CVMjniCallStaticIntMethod(JNIEnv *env, jclass clazz,
					      jmethodID methodID, ...);
extern jint JNICALL CVMjniCallStaticIntMethodV(JNIEnv *env, jclass clazz,
					       jmethodID methodID,
					       va_list args);
extern jint JNICALL CVMjniCallStaticIntMethodA(JNIEnv *env, jclass clazz,
					       jmethodID methodID,
					       const jvalue *args);

extern jlong JNICALL CVMjniCallStaticLongMethod(JNIEnv *env, jclass clazz,
						jmethodID methodID, ...);
extern jlong JNICALL CVMjniCallStaticLongMethodV(JNIEnv *env, jclass clazz,
						 jmethodID methodID,
						 va_list args);
extern jlong JNICALL CVMjniCallStaticLongMethodA(JNIEnv *env, jclass clazz,
						 jmethodID methodID,
						 const jvalue *args);

extern jfloat JNICALL CVMjniCallStaticFloatMethod(JNIEnv *env, jclass clazz,
						  jmethodID methodID, ...);
extern jfloat JNICALL CVMjniCallStaticFloatMethodV(JNIEnv *env, jclass clazz,
						   jmethodID methodID,
						   va_list args);
extern jfloat JNICALL CVMjniCallStaticFloatMethodA(JNIEnv *env, jclass clazz,
						   jmethodID methodID,
						   const jvalue *args);

extern jdouble JNICALL CVMjniCallStaticDoubleMethod(JNIEnv *env, jclass clazz,
						    jmethodID methodID, ...);
extern jdouble JNICALL CVMjniCallStaticDoubleMethodV(JNIEnv *env, jclass clazz,
						     jmethodID methodID,
						     va_list args);
extern jdouble JNICALL CVMjniCallStaticDoubleMethodA(JNIEnv *env, jclass clazz,
						     jmethodID methodID,
						     const jvalue *args);

extern void JNICALL CVMjniCallStaticVoidMethod(JNIEnv *env, jclass clazz,
					       jmethodID methodID, ...);
extern void JNICALL CVMjniCallStaticVoidMethodV(JNIEnv *env, jclass clazz,
						jmethodID methodID,
						va_list args);
extern void JNICALL CVMjniCallStaticVoidMethodA(JNIEnv *env, jclass clazz,
						jmethodID methodID,
						const jvalue *args);

/*
 * Static field access
 */
extern jfieldID JNICALL CVMjniGetStaticFieldID(JNIEnv *env,
					       jclass clazz,
					       const char *name,
					       const char *sig);

extern jobject JNICALL CVMjniGetStaticObjectField(JNIEnv* env,
						  jclass clazz, jfieldID fid);
extern jboolean JNICALL CVMjniGetStaticBooleanField(JNIEnv* env, jclass clazz,
						    jfieldID fid);
extern jbyte JNICALL CVMjniGetStaticByteField(JNIEnv* env, jclass clazz,
					      jfieldID fid);
extern jchar JNICALL CVMjniGetStaticCharField(JNIEnv* env, jclass clazz,
					      jfieldID fid);
extern jshort JNICALL CVMjniGetStaticShortField(JNIEnv* env, jclass clazz,
						jfieldID fid);
extern jint JNICALL CVMjniGetStaticIntField(JNIEnv* env, jclass clazz,
					    jfieldID fid);
extern jlong JNICALL CVMjniGetStaticLongField(JNIEnv* env, jclass clazz,
					      jfieldID fid);
extern jfloat JNICALL CVMjniGetStaticFloatField(JNIEnv* env, jclass clazz,
						jfieldID fid);
extern jdouble JNICALL CVMjniGetStaticDoubleField(JNIEnv* env, jclass clazz,
						  jfieldID fid);

extern void JNICALL CVMjniSetStaticObjectField(JNIEnv* env, jclass clazz,
					       jfieldID fid, jobject rhs);
extern void JNICALL CVMjniSetStaticBooleanField(JNIEnv* env, jclass clazz,
						jfieldID fid, jboolean rhs);
extern void JNICALL CVMjniSetStaticByteField(JNIEnv* env, jclass clazz,
					     jfieldID fid, jbyte rhs);
extern void JNICALL CVMjniSetStaticCharField(JNIEnv* env, jclass clazz,
					     jfieldID fid, jchar rhs);
extern void JNICALL CVMjniSetStaticShortField(JNIEnv* env, jclass clazz,
					      jfieldID fid, jshort rhs);
extern void JNICALL CVMjniSetStaticIntField(JNIEnv* env, jclass clazz,
					    jfieldID fid, jint rhs);
extern void JNICALL CVMjniSetStaticLongField(JNIEnv* env, jclass clazz,
					     jfieldID fid, jlong rhs);
extern void JNICALL CVMjniSetStaticFloatField(JNIEnv* env, jclass clazz,
					      jfieldID fid, jfloat rhs);
extern void JNICALL CVMjniSetStaticDoubleField(JNIEnv* env, jclass clazz,
					       jfieldID fid, jdouble rhs);

extern jstring JNICALL CVMjniNewString(JNIEnv *env, const jchar* unicodeChars,
				       jsize len);
extern jsize JNICALL CVMjniGetStringLength(JNIEnv* env, jstring string);
extern const jchar* JNICALL CVMjniGetStringChars(JNIEnv *env, jstring string,
						 jboolean *isCopy);
extern void JNICALL CVMjniReleaseStringChars(JNIEnv *env, jstring str,
					     const jchar *chars);

extern jstring JNICALL CVMjniNewStringUTF(JNIEnv* env, const char* utf8Bytes);
extern jsize JNICALL CVMjniGetStringUTFLength(JNIEnv *env, jstring string);
extern const char* JNICALL CVMjniGetStringUTFChars(JNIEnv *env,
						   jstring string,
						   jboolean *isCopy);
extern void JNICALL CVMjniReleaseStringUTFChars(JNIEnv *env, jstring str,
						const char *chars);

extern jsize JNICALL CVMjniGetArrayLength(JNIEnv* env, jarray arrArg);

extern jarray JNICALL CVMjniNewObjectArray(JNIEnv* env, jsize length,
					   jclass elementClass,
					   jobject initialElement);
extern jobject JNICALL CVMjniGetObjectArrayElement(JNIEnv* env,
						   jarray arrArg, jsize index);
extern void JNICALL CVMjniSetObjectArrayElement(JNIEnv* env, jarray arrArg,
						jsize index, jobject value);

extern jbooleanArray JNICALL CVMjniNewBooleanArray(JNIEnv *env, jsize length); 
extern jbyteArray JNICALL CVMjniNewByteArray(JNIEnv *env, jsize length); 
extern jcharArray JNICALL CVMjniNewCharArray(JNIEnv *env, jsize length); 
extern jshortArray JNICALL CVMjniNewShortArray(JNIEnv *env, jsize length); 
extern jintArray JNICALL CVMjniNewIntArray(JNIEnv *env, jsize length); 
extern jlongArray JNICALL CVMjniNewLongArray(JNIEnv *env, jsize length); 
extern jfloatArray JNICALL CVMjniNewFloatArray(JNIEnv *env, jsize length); 
extern jdoubleArray JNICALL CVMjniNewDoubleArray(JNIEnv *env, jsize length); 

extern jboolean * JNICALL CVMjniGetBooleanArrayElements(JNIEnv *env,
							jbooleanArray array,
							jboolean *isCopy);
extern jbyte * JNICALL CVMjniGetByteArrayElements(JNIEnv *env,
						  jbyteArray array,
						  jboolean *isCopy);
extern jchar * JNICALL CVMjniGetCharArrayElements(JNIEnv *env,
						  jcharArray array,
						  jboolean *isCopy);
extern jshort * JNICALL CVMjniGetShortArrayElements(JNIEnv *env,
						    jshortArray array,
						    jboolean *isCopy);
extern jint * JNICALL CVMjniGetIntArrayElements(JNIEnv *env,
						jintArray array,
						jboolean *isCopy);
extern jlong * JNICALL CVMjniGetLongArrayElements(JNIEnv *env,
						  jlongArray array,
						  jboolean *isCopy);
extern jfloat * JNICALL CVMjniGetFloatArrayElements(JNIEnv *env,
						    jfloatArray array,
						    jboolean *isCopy);
extern jdouble * JNICALL CVMjniGetDoubleArrayElements(JNIEnv *env,
						      jdoubleArray array,
						      jboolean *isCopy);

extern void JNICALL CVMjniReleaseBooleanArrayElements(JNIEnv *env, 
						      jbooleanArray array,
						      jboolean *elems,
						      jint mode);
extern void JNICALL CVMjniReleaseByteArrayElements(JNIEnv *env, 
						   jbyteArray array,
						   jbyte *elems,
						   jint mode);
extern void JNICALL CVMjniReleaseCharArrayElements(JNIEnv *env, 
						   jcharArray array,
						   jchar *elems,
						   jint mode);
extern void JNICALL CVMjniReleaseShortArrayElements(JNIEnv *env, 
						    jshortArray array,
						    jshort *elems,
						    jint mode);
extern void JNICALL CVMjniReleaseIntArrayElements(JNIEnv *env, 
						  jintArray array,
						  jint *elems,
						  jint mode);
extern void JNICALL CVMjniReleaseLongArrayElements(JNIEnv *env, 
						   jlongArray array,
						   jlong *elems,
						   jint mode);
extern void JNICALL CVMjniReleaseFloatArrayElements(JNIEnv *env, 
						    jfloatArray array,
						    jfloat *elems,
						    jint mode);
extern void JNICALL CVMjniReleaseDoubleArrayElements(JNIEnv *env, 
						     jdoubleArray array,
						     jdouble *elems,
						     jint mode);

extern void JNICALL CVMjniGetBooleanArrayRegion(JNIEnv *env,
						jbooleanArray array,
						jsize start, jsize len,
						jboolean *buf);
extern void JNICALL CVMjniGetByteArrayRegion(JNIEnv *env,
					     jbyteArray array,
					     jsize start, jsize len,
					     jbyte *buf);
extern void JNICALL CVMjniGetCharArrayRegion(JNIEnv *env,
					     jcharArray array,
					     jsize start, jsize len,
					     jchar *buf);
extern void JNICALL CVMjniGetShortArrayRegion(JNIEnv *env,
					      jshortArray array,
					      jsize start, jsize len,
					      jshort *buf);
extern void JNICALL CVMjniGetIntArrayRegion(JNIEnv *env,
					    jintArray array,
					    jsize start, jsize len,
					    jint *buf);
extern void JNICALL CVMjniGetLongArrayRegion(JNIEnv *env,
					     jlongArray array,
					     jsize start, jsize len,
					     jlong *buf);
extern void JNICALL CVMjniGetFloatArrayRegion(JNIEnv *env,
					      jfloatArray array,
					      jsize start, jsize len,
					      jfloat *buf);
extern void JNICALL CVMjniGetDoubleArrayRegion(JNIEnv *env,
					       jdoubleArray array,
					       jsize start, jsize len,
					       jdouble *buf);

extern void JNICALL CVMjniSetBooleanArrayRegion(JNIEnv *env,
						jbooleanArray array, 
						jsize start, jsize len,
						const jboolean *buf);
extern void JNICALL CVMjniSetByteArrayRegion(JNIEnv *env,
					     jbyteArray array, 
					     jsize start, jsize len,
					     const jbyte *buf);
extern void JNICALL CVMjniSetCharArrayRegion(JNIEnv *env,
					     jcharArray array, 
					     jsize start, jsize len,
					     const jchar *buf);
extern void JNICALL CVMjniSetShortArrayRegion(JNIEnv *env,
					      jshortArray array, 
					      jsize start, jsize len,
					      const jshort *buf);
extern void JNICALL CVMjniSetIntArrayRegion(JNIEnv *env,
					    jintArray array, 
					    jsize start, jsize len,
					    const jint *buf);
extern void JNICALL CVMjniSetLongArrayRegion(JNIEnv *env,
					     jlongArray array, 
					     jsize start, jsize len,
					     const jlong *buf);
extern void JNICALL CVMjniSetFloatArrayRegion(JNIEnv *env,
					      jfloatArray array, 
					      jsize start, jsize len,
					      const jfloat *buf);
extern void JNICALL CVMjniSetDoubleArrayRegion(JNIEnv *env,
					       jdoubleArray array, 
					       jsize start, jsize len,
					       const jdouble *buf);

extern jint JNICALL CVMjniRegisterNatives(JNIEnv *env, jclass clazz,
					  const JNINativeMethod *methods,
					  jint nMethods);
extern jint JNICALL CVMjniUnregisterNatives(JNIEnv *env, jclass clazz);
extern jint JNICALL CVMjniMonitorEnter(JNIEnv *env, jobject obj);
extern jint JNICALL CVMjniMonitorExit(JNIEnv *env, jobject obj);
extern jint JNICALL CVMjniGetJavaVM(JNIEnv *env, JavaVM **p_jvm);
extern void JNICALL CVMjniGetStringRegion(JNIEnv *env,
					  jstring string, jsize start,
					  jsize len, jchar *buf);
extern void JNICALL CVMjniGetStringUTFRegion(JNIEnv *env,
					     jstring string, jsize start,
					     jsize len, char *buf);
extern void* JNICALL CVMjniGetPrimitiveArrayCritical(JNIEnv *env,
						     jarray array,
						     jboolean *isCopy);
extern void JNICALL CVMjniReleasePrimitiveArrayCritical(JNIEnv *env,
							jarray array,
							void* buf, jint mode);
extern const jchar* JNICALL CVMjniGetStringCritical(JNIEnv *env,
						    jstring string,
						    jboolean *isCopy);
extern void JNICALL CVMjniReleaseStringCritical(JNIEnv *env, jstring str,
						const jchar *chars);
extern jboolean JNICALL CVMjniExceptionCheck(JNIEnv *env);

/* JNI_VERSION_1_4 additions: */
extern jobject JNICALL
CVMjniNewDirectByteBuffer(JNIEnv *env, void* address, jlong capacity);

extern void* JNICALL
CVMjniGetDirectBufferAddress(JNIEnv *env, jobject buf);

extern jlong JNICALL
CVMjniGetDirectBufferCapacity(JNIEnv *env, jobject buf);

#ifdef CVM_JVMTI_IOVEC
#include "jni_io.h"
#endif

#endif /* _INCLUDED_JNI_IMPL_H */
