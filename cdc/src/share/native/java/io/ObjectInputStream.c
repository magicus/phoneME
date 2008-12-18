/*
 * @(#)ObjectInputStream.c	1.48 06/10/10
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

#include "jni.h"
#include "jvm.h"
#include "jni_util.h"
#include "jlong.h"

#include "java_lang_Float.h"
#include "java_lang_Double.h"
#include "java_io_ObjectInputStream.h"

		
#define READ_JLONG_FROM_BUF(buf, off)					\
    CVMlongAdd(CVMlongAdd(CVMlongAdd(CVMlongAdd(			\
               CVMlongAdd(CVMlongAdd(CVMlongAdd(			\
		    CVMlongShl(CVMint2Long(buf[off + 0] & 0xFF), 56),	\
		    CVMlongShl(CVMint2Long(buf[off + 1] & 0xFF), 48)),	\
		    CVMlongShl(CVMint2Long(buf[off + 2] & 0xFF), 40)),	\
		    CVMlongShl(CVMint2Long(buf[off + 3] & 0xFF), 32)),	\
		    CVMlongShl(CVMint2Long(buf[off + 4] & 0xFF), 24)),	\
		    CVMlongShl(CVMint2Long(buf[off + 5] & 0xFF), 16)),	\
		    CVMlongShl(CVMint2Long(buf[off + 6] & 0xFF), 8)),	\
		    CVMlongShl(CVMint2Long(buf[off + 7] & 0xFF), 0))

/*
 * Class:     java_io_ObjectInputStream
 * Method:    setPrimitiveFieldValues
 * Signature: (Ljava/lang/Object;[J[C[B)V
 * 
 * Sets the values of the primitive fields of object obj.  fieldIDs is an array
 * of field IDs (the primFieldsID field of the appropriate ObjectStreamClass)
 * identifying which fields to set.  typecodes is an array of characters
 * designating the primitive type of each field (e.g., 'C' for char, 'Z' for
 * boolean, etc.)  data is the byte buffer from which the primitive field values
 * are read, in the order of their field IDs.
 * 
 * For efficiency, this method does not check all of its arguments for safety.
 * Specifically, it assumes that obj's type is compatible with the given field
 * IDs, and that the data array is long enough to contain all of the byte values
 * that will be read out of it.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectInputStream_setPrimitiveFieldValues(JNIEnv *env, 
						       jclass thisObj, 
						       jobject obj, 
						       jlongArray fieldIDs, 
						       jcharArray typecodes, 
						       jbyteArray data)
{
    jchar *tcodes = NULL;
    jbyte *dbuf = NULL;
    jlong *fids = NULL;
    jsize nfids, off, i;

    /* check object */
    if (obj == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	goto end;
    }

    /* get field ids */
    if (fieldIDs == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	goto end;
    }
    nfids = (*env)->GetArrayLength(env, fieldIDs);
    if (nfids == 0)
	goto end;
    fids = (*env)->GetLongArrayElements(env, fieldIDs, NULL);
    if (fids == NULL)	/* exception thrown */
	goto end;
    
    /* get typecodes */
    if (typecodes == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	goto end;
    }
    if ((*env)->GetArrayLength(env, typecodes) < nfids) {
	JNU_ThrowArrayIndexOutOfBoundsException(env, NULL);
	goto end;
    }
    tcodes = (*env)->GetCharArrayElements(env, typecodes, NULL);
    if (tcodes == NULL)	/* exception thrown */
	goto end;
    
    /* get data buffer */
    if (data == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	goto end;
    }
    dbuf = (*env)->GetByteArrayElements(env, data, NULL);
    if (dbuf == NULL)	/* exception thrown */
	goto end;
    
    /* loop through fields, setting values */
    for (i = 0, off = 0; i < nfids; i++) {
	jfieldID fid = (jfieldID) jlong_to_ptr(fids[i]);
	switch (tcodes[i]) {
	    case 'Z':
		if (fid != NULL) {
		    jboolean val = (dbuf[off] != 0) ? JNI_TRUE : JNI_FALSE;
		    (*env)->SetBooleanField(env, obj, fid, val);
		}
		off++;
		break;
		
	    case 'B':
		if (fid != NULL) {
		    (*env)->SetByteField(env, obj, fid, dbuf[off]);
		}
		off++;
		break;
		
	    case 'C':
		if (fid != NULL) {
		    jchar val = ((dbuf[off + 0] & 0xFF) << 8) +
				((dbuf[off + 1] & 0xFF) << 0);
		    (*env)->SetCharField(env, obj, fid, val);
		}
		off += 2;
		break;

	    case 'S':
		if (fid != NULL) {
		    jshort val = ((dbuf[off + 0] & 0xFF) << 8) +
				 ((dbuf[off + 1] & 0xFF) << 0);
		    (*env)->SetShortField(env, obj, fid, val);
		}
		off += 2;
		break;
		
	    case 'I':
		if (fid != NULL) {
		    jint val = ((dbuf[off + 0] & 0xFF) << 24) +
			       ((dbuf[off + 1] & 0xFF) << 16) +
			       ((dbuf[off + 2] & 0xFF) << 8) +
			       ((dbuf[off + 3] & 0xFF) << 0);
		    (*env)->SetIntField(env, obj, fid, val);
		}
		off += 4;
		break;
		
	    case 'F':
		if (fid != NULL) {
		    jint ival = ((dbuf[off + 0] & 0xFF) << 24) +
				((dbuf[off + 1] & 0xFF) << 16) +
				((dbuf[off + 2] & 0xFF) << 8) +
				((dbuf[off + 3] & 0xFF) << 0);
		    jfloat fval = Java_java_lang_Float_intBitsToFloat(env,
			    NULL, ival);
		    (*env)->SetFloatField(env, obj, fid, fval);
		}
		off += 4;
		break;

	    case 'J':
		if (fid != NULL) {
		    jlong val = READ_JLONG_FROM_BUF(dbuf, off);
		    (*env)->SetLongField(env, obj, fid, val);
		}
		off += 8;
		break;
		
	    case 'D':
		if (fid != NULL) {
		    jlong lval = READ_JLONG_FROM_BUF(dbuf, off);
		    jdouble dval = Java_java_lang_Double_longBitsToDouble(env,
			    NULL, lval);
		    (*env)->SetDoubleField(env, obj, fid, dval);
		}
		off += 8;
		break;
		
	    default:
		/* Illegal typecode */
		JNU_ThrowIllegalArgumentException(env, "illegal typecode");
		goto end;
	}
    }
    
    
end:
    /* cleanup */
    if (fids != NULL)
	(*env)->ReleaseLongArrayElements(env, fieldIDs, fids, JNI_ABORT);
    if (tcodes != NULL)
	(*env)->ReleaseCharArrayElements(env, typecodes, tcodes, JNI_ABORT);
    if (dbuf != NULL)
	(*env)->ReleaseByteArrayElements(env, data, dbuf, JNI_ABORT);
}


/*
 * Class:     java_io_ObjectInputStream
 * Method:    setObjectFieldValue
 * Signature: (Ljava/lang/Object;JLjava/lang/Class;Ljava/lang/Object;)V
 * 
 * Sets the value of an object field of object obj.  fieldID is the field ID
 * identifying which field to set (obtained from the objFieldsID array field of
 * the appropriate ObjectStreamClass).  type is the field type; it is provided
 * so that the native method can ensure that the passed value val is assignable
 * to the field.
 * 
 * For efficiency, this method does not check all of its arguments for safety.
 * Specifically, it assumes that obj's type is compatible with the given field
 * IDs, and that type is indeed the class type of the field designated by
 * fieldID.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectInputStream_setObjectFieldValue(JNIEnv *env, 
						   jclass thisObj, 
						   jobject obj, 
						   jlong fieldID, 
						   jclass type, 
						   jobject val)
{
    jfieldID fid = (jfieldID) jlong_to_ptr(fieldID);

    if (obj == NULL || fid == NULL || type == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    
    /* check type */
    if (val != NULL && (! (*env)->IsInstanceOf(env, val, type))) {
	JNU_ThrowByName(env, "java/lang/ClassCastException", NULL);
	return;
    }
    (*env)->SetObjectField(env, obj, fid, val);
}


/*
 * Class:     java_io_ObjectInputStream
 * Method:    bytesToFloats
 * Signature: ([BI[FII)V
 * 
 * Reconstitutes nfloats float values from their byte representations.  Byte
 * values are read from array src starting at offset srcpos; the resulting
 * float values are written to array dst starting at dstpos.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectInputStream_bytesToFloats(JNIEnv *env, 
					     jclass thisObj, 
					     jbyteArray src, 
					     jint srcpos, 
					     jfloatArray dst, 
					     jint dstpos, 
					     jint nfloats)
{
    union {
	int i;
	float f;
    } u;
    jfloat *floats;
    jbyte *bytes;
    jsize dstend;
    jint ival;

    if (nfloats == 0)
	return;
    
    /* fetch source array */
    if (src == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    bytes = (jbyte *)(*env)->GetPrimitiveArrayCritical(env, src, NULL);
    if (bytes == NULL)		/* exception thrown */
	return;
    
    /* fetch dest array */
    if (dst == NULL) {
	(*env)->ReleasePrimitiveArrayCritical(env, src, bytes, JNI_ABORT);
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    floats = (jfloat *)(*env)->GetPrimitiveArrayCritical(env, dst, NULL);
    if (floats == NULL) {	/* exception thrown */
	(*env)->ReleasePrimitiveArrayCritical(env, src, bytes, JNI_ABORT);
	return;
    }
    
    /* do conversion */
    dstend = dstpos + nfloats;
    for ( ; dstpos < dstend; dstpos++) {
	ival = ((bytes[srcpos + 0] & 0xFF) << 24) +
	       ((bytes[srcpos + 1] & 0xFF) << 16) +
	       ((bytes[srcpos + 2] & 0xFF) << 8) +
	       ((bytes[srcpos + 3] & 0xFF) << 0);
	u.i = (long) ival;
	floats[dstpos] = (jfloat) u.f;
	srcpos += 4;
    }
    
    (*env)->ReleasePrimitiveArrayCritical(env, src, bytes, JNI_ABORT);
    (*env)->ReleasePrimitiveArrayCritical(env, dst, floats, 0);
}

/*
 * Class:     java_io_ObjectInputStream
 * Method:    bytesToDoubles
 * Signature: ([BI[DII)V
 * 
 * Reconstitutes ndoubles double values from their byte representations.
 * Byte values are read from array src starting at offset srcpos; the
 * resulting double values are written to array dst starting at dstpos.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectInputStream_bytesToDoubles(JNIEnv *env, 
					      jclass thisObj, 
					      jbyteArray src, 
					      jint srcpos, 
					      jdoubleArray dst, 
					      jint dstpos, 
					      jint ndoubles)

{
    union {
	jlong l;
	double d;
    } u;
    jdouble *doubles;
    jbyte *bytes;
    jsize dstend;
    jlong lval;

    if (ndoubles == 0)
	return;
    
    /* fetch source array */
    if (src == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    bytes = (jbyte *)(*env)->GetPrimitiveArrayCritical(env, src, NULL);
    if (bytes == NULL)		/* exception thrown */
	return;
    
    /* fetch dest array */
    if (dst == NULL) {
	(*env)->ReleasePrimitiveArrayCritical(env, src, bytes, JNI_ABORT);
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    doubles = (jdouble *)(*env)->GetPrimitiveArrayCritical(env, dst, NULL);
    if (doubles == NULL) {	/* exception thrown */
	(*env)->ReleasePrimitiveArrayCritical(env, src, bytes, JNI_ABORT);
	return;
    }
    
    /* do conversion */
    dstend = dstpos + ndoubles;
    for ( ; dstpos < dstend; dstpos++) {
	lval = READ_JLONG_FROM_BUF(bytes, srcpos);
	jlong_to_jdouble_bits(&lval);
	u.l = lval;
	doubles[dstpos] = (jdouble) u.d;
	srcpos += 8;
    }
    
    (*env)->ReleasePrimitiveArrayCritical(env, src, bytes, JNI_ABORT);
    (*env)->ReleasePrimitiveArrayCritical(env, dst, doubles, 0);
}

JNIEXPORT jobject JNICALL 
Java_java_io_ObjectInputStream_allocateNewObject (JNIEnv * env, 
						  jclass thisObj, 
						  jclass curClass, 
						  jclass initClass)
{
    return JVM_AllocateNewObject(env, thisObj, curClass, initClass);
}


JNIEXPORT jobject JNICALL 
Java_java_io_ObjectInputStream_allocateNewArray (JNIEnv * env, 
						 jclass thisObj, 
						 jclass thisClass, 
						 jint length)
{
    return JVM_AllocateNewArray(env, thisObj, thisClass, length);  /* test this */
}

/*
 * Class:     java_io_ObjectInputStream
 * Method:    latestUserDefinedLoader
 * Signature: ()Ljava/lang/ClassLoader;
 * 
 * Returns the first non-null class loader up the execution stack, or null
 * if only code from the null class loader is on the stack.
 */
JNIEXPORT jobject JNICALL
Java_java_io_ObjectInputStream_latestUserDefinedLoader(JNIEnv *env, jclass cls)
{
    return JVM_LatestUserDefinedLoader(env);
}
