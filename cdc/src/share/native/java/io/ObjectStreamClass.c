/*
 * @(#)ObjectStreamClass.c	1.51 06/10/10
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

#include "javavm/include/porting/ansi/stdlib.h"

#include "jni.h"
#include "jvm.h"
#include "jlong.h"
#include "jni_util.h"

#include "java_io_ObjectStreamClass.h"

#include "jni_statics.h"
/* static jfieldID osf_field_id; */	/* ObjectStreamField.field */
/* static jfieldID osf_typecode_id;	*/	/* ObjectStreamField.typeCode */

/*
 * Class:     java_io_ObjectStreamClass
 * Method:    initNative
 * Signature: ()V
 * 
 * Native code initialization hook.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectStreamClass_initNative(JNIEnv *env, jclass thisObj)
{
    jclass osf_class;
    
    osf_class = (*env)->FindClass(env, "java/io/ObjectStreamField");
    if (osf_class == NULL)	/* exception thrown */
	return;

    JNI_STATIC(java_io_ObjectStreamClass, osf_field_id) 
	= (*env)->GetFieldID(env, osf_class, "field", "Ljava/lang/reflect/Field;");
    if (JNI_STATIC(java_io_ObjectStreamClass, osf_field_id) == NULL)	/* exception thrown */
	return;
    
    JNI_STATIC(java_io_ObjectStreamClass, osf_typecode_id) = 
        (*env)->GetFieldID(env, osf_class, "typeCode", "C");
    if (JNI_STATIC(java_io_ObjectStreamClass, osf_typecode_id) == NULL)	
        /* exception thrown */
	return;
}


/* Class:     java_io_ObjectStreamClass
 * Method:    getFieldIDs
 * Signature: ([Ljava/io/ObjectStreamField;[J[J)V
 * 
 * Get the field IDs associated with the given fields.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectStreamClass_getFieldIDs(JNIEnv *env, 
					   jclass thisObj, 
					   jobjectArray fields, 
					   jlongArray primFieldIDs, 
					   jlongArray objFieldIDs)
{
    jsize nfields, pi, oi, i;
    jlong *primIDs = NULL, *objIDs = NULL;

    /* get number of fields */
    if (fields == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	goto end;
    }
    nfields = (*env)->GetArrayLength(env, fields);
    
    /* loop through fields, fetching and storing field IDs */
    pi = oi = 0;
    for (i = 0; i < nfields; i++) {
	jobject field, rfield;
	jfieldID fid;
	jchar tcode;
	
	/* get field descriptor */
	field = (*env)->GetObjectArrayElement(env, fields, i);
	if (field == NULL) {
	    JNU_ThrowNullPointerException(env, NULL);
	    goto end;
	}
	
	/* get reflective field */
	rfield = (*env)->GetObjectField(env, field, JNI_STATIC(java_io_ObjectStreamClass, osf_field_id));
	fid = (rfield != NULL) ? 
	    (*env)->FromReflectedField(env, rfield) : NULL;

	/* typecode indicates whether field has primitive or object type */
	tcode = (*env)->GetCharField(env, field, JNI_STATIC(java_io_ObjectStreamClass, osf_typecode_id));
	switch (tcode) {
	    case 'Z':
	    case 'B':
	    case 'S':
	    case 'C':
	    case 'I':
	    case 'J':
	    case 'F':
	    case 'D':
		if (primIDs == NULL) {		/* get prim field ID array */
		    if (primFieldIDs == NULL) {
			JNU_ThrowNullPointerException(env, NULL);
			goto end;
		    }
		    primIDs = 
			(*env)->GetLongArrayElements(env, primFieldIDs, NULL);
		    if (primIDs == NULL)	/* exception thrown */
			goto end;
		}
		primIDs[pi++] = ptr_to_jlong(fid);
		break;
		
	    case '[':
	    case 'L':
		if (objIDs == NULL) {		/* get obj field ID array */
		    if (objFieldIDs == NULL) {
			JNU_ThrowNullPointerException(env, NULL);
			goto end;
		    }
		    objIDs = 
			(*env)->GetLongArrayElements(env, objFieldIDs, NULL);
		    if (objIDs == NULL)		/* exception thrown */
			goto end;
		}
		objIDs[oi++] = ptr_to_jlong(fid);
		break;
		
	    default:
		JNU_ThrowIllegalArgumentException(env, "illegal typecode");
		goto end;
	}
	
	/* free local refs before next loop around */
	(*env)->DeleteLocalRef(env, field);
	(*env)->DeleteLocalRef(env, rfield);
    }
    
end:
    /* cleanup */
    if (primIDs != NULL)
	(*env)->ReleaseLongArrayElements(env, primFieldIDs, primIDs, 0);
    if (objIDs != NULL)
	(*env)->ReleaseLongArrayElements(env, objFieldIDs, objIDs, 0);
}


/*
 * Class:     java_io_ObjectStreamClass
 * Method:    hasStaticInitializer
 * Signature: (Ljava/lang/Class;)Z
 * 
 * Returns true if the given class defines a <clinit>()V method; returns false
 * otherwise.
 */
JNIEXPORT jboolean JNICALL
Java_java_io_ObjectStreamClass_hasStaticInitializer(JNIEnv *env, jclass thisObj,
						    jclass clazz)
{
    jclass superclazz = NULL;
    jmethodID superclinit = NULL;

    jmethodID clinit = (*env)->GetStaticMethodID(env, clazz,
						 "<clinit>", "()V");
    if (clinit == NULL || (*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionClear(env);
	return 0;
    }

    /* Ask the superclass the same question
     * If the answer is the same then the constructor is from a superclass.
     * If different, it's really defined on the subclass.
     */
    superclazz = (*env)->GetSuperclass(env, clazz);
    if ((*env)->ExceptionOccurred(env)) {
	return 0;
    }

    if (superclazz == NULL)
	return 1;

    superclinit = (*env)->GetStaticMethodID(env, superclazz,
					    "<clinit>", "()V");
    if ((*env)->ExceptionOccurred(env)) {
	(*env)->ExceptionClear(env);
	superclinit = NULL;
    }

    return (superclinit != clinit);
}
