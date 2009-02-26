/*
 * @(#)ObjectOutputStream.c	1.17 06/10/10
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
#include "java_io_ObjectOutputStream.h"

/*
 * Class:     java_io_ObjectOutputStream
 * Method:    getPrimitiveFieldValues
 * Signature: (Ljava/lang/Object;[J[C[B)V
 * 
 * Gets the values of the primitive fields of object obj.  fieldIDs is an array
 * of field IDs (the primFieldsID field of the appropriate ObjectStreamClass)
 * identifying which fields to get.  typecodes is an array of characters
 * designating the primitive type of each field (e.g., 'C' for char, 'Z' for
 * boolean, etc.)  data is the byte buffer in which the primitive field values
 * are written, in the order of their field IDs.
 * 
 * For efficiency, this method does not check all of its arguments for safety.
 * Specifically, it assumes that obj's type is compatible with the given field
 * IDs, and that the data array is long enough to contain all of the byte values
 * that will be written to it.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectOutputStream_getPrimitiveFieldValues(JNIEnv *env, 
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
    
    /* loop through fields, fetching values */
    for (i = 0, off = 0; i < nfids; i++) {
	jfieldID fid = (jfieldID) jlong_to_ptr(fids[i]);
	if (fid == NULL) {
	    JNU_ThrowNullPointerException(env, NULL);
	    goto end;
	}
	
	switch (tcodes[i]) {
	    case 'Z':
		{
		    jboolean val = (*env)->GetBooleanField(env, obj, fid);
		    dbuf[off++] = (val != 0) ? 1 : 0;
		}
		break;
		
	    case 'B':
		dbuf[off++] = (*env)->GetByteField(env, obj, fid);
		break;
		
	    case 'C':
		{
		    jchar val = (*env)->GetCharField(env, obj, fid);
		    dbuf[off++] = (val >> 8) & 0xFF;
		    dbuf[off++] = (val >> 0) & 0xFF;
		}
		break;

	    case 'S':
		{
		    jshort val = (*env)->GetShortField(env, obj, fid);
		    dbuf[off++] = (val >> 8) & 0xFF;
		    dbuf[off++] = (val >> 0) & 0xFF;
		}
		break;
		
	    case 'I':
		{
		    jint val = (*env)->GetIntField(env, obj, fid);
		    dbuf[off++] = (val >> 24) & 0xFF;
		    dbuf[off++] = (val >> 16) & 0xFF;
		    dbuf[off++] = (val >> 8) & 0xFF;
		    dbuf[off++] = (val >> 0) & 0xFF;
		}
		break;
		
	    case 'F':
		{
		    jfloat fval = (*env)->GetFloatField(env, obj, fid);
		    jint ival = Java_java_lang_Float_floatToIntBits(env,
			    NULL, fval);
		    dbuf[off++] = (ival >> 24) & 0xFF;
		    dbuf[off++] = (ival >> 16) & 0xFF;
		    dbuf[off++] = (ival >> 8) & 0xFF;
		    dbuf[off++] = (ival >> 0) & 0xFF;
		}
		break;
		
	    case 'J':
		{
		    jlong val = (*env)->GetLongField(env, obj, fid);
		    jlong ffl = CVMint2Long(0xff);
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(val, 56), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(val, 48), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(val, 40), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(val, 32), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(val, 24), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(val, 16), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(val, 8), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(val, 0), ffl));
		}
		break;
		
	    case 'D':
		{
		    jdouble dval = (*env)->GetDoubleField(env, obj, fid);
		    jlong lval = Java_java_lang_Double_doubleToLongBits(env, 
			    NULL, dval);
		    jlong ffl = CVMint2Long(0xff);
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 56), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 48), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 40), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 32), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 24), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 16), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 8), ffl));
		    dbuf[off++] = (jbyte)
			CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 0), ffl));
		}
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
    if (dbuf != NULL)	/* commit changes to dbuf */
	(*env)->ReleaseByteArrayElements(env, data, dbuf, 0);
}


/*
 * Class:     java_io_ObjectOutputStream
 * Method:    getObjectFieldValue
 * Signature: (Ljava/lang/Object;J)Ljava/lang/Object;
 * 
 * Gets the value of an object field of object obj.  fieldID is the field ID
 * identifying which field to set (obtained from the objFieldsID array field of
 * the appropriate ObjectStreamClass).
 * 
 * For efficiency, this method does not check to make sure that obj's type is
 * compatible with the given field ID.
 */
JNIEXPORT jobject JNICALL 
Java_java_io_ObjectOutputStream_getObjectFieldValue(JNIEnv *env, 
						    jclass thisObj, 
						    jobject obj, 
						    jlong fieldID)
{
    jfieldID fid = (jfieldID) jlong_to_ptr(fieldID);

    if (obj == NULL || fid == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	return NULL;
    }
    return (*env)->GetObjectField(env, obj, fid);
}


/*
 * Class:     java_io_ObjectOutputStream
 * Method:    floatsToBytes
 * Signature: ([FI[BII)V
 * 
 * Convert nfloats float values to their byte representations.  Float values
 * are read from array src starting at offset srcpos and written to array
 * dst starting at offset dstpos.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectOutputStream_floatsToBytes(JNIEnv *env, 
					      jclass thisObj, 
					      jfloatArray src, 
					      jint srcpos, 
					      jbyteArray dst, 
					      jint dstpos, 
					      jint nfloats)
{
    union {
	int i;
	float f;
    } u;
    jfloat *floats;
    jbyte *bytes;
    jsize srcend;
    jint ival;
    float fval;

    if (nfloats == 0)
	return;
    
    /* fetch source array */
    if (src == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    floats = (jfloat *)(*env)->GetPrimitiveArrayCritical(env, src, NULL);
    if (floats == NULL)		/* exception thrown */
	return;
    
    /* fetch dest array */
    if (dst == NULL) {
	(*env)->ReleasePrimitiveArrayCritical(env, src, floats, JNI_ABORT);
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    bytes = (jbyte *)(*env)->GetPrimitiveArrayCritical(env, dst, NULL);
    if (bytes == NULL) {	/* exception thrown */
	(*env)->ReleasePrimitiveArrayCritical(env, src, floats, JNI_ABORT);
	return;
    }
    
    /* do conversion */
    srcend = srcpos + nfloats;
    for ( ; srcpos < srcend; srcpos++) {
	fval = (float) floats[srcpos];
	if (JVM_IsNaN(fval)) {		/* collapse NaNs */
	    ival = 0x7fc00000;
	} else {
	    u.f = fval;
	    ival = (jint) u.i;
	}
	bytes[dstpos++] = (ival >> 24) & 0xFF;
	bytes[dstpos++] = (ival >> 16) & 0xFF;
	bytes[dstpos++] = (ival >> 8) & 0xFF;
	bytes[dstpos++] = (ival >> 0) & 0xFF;
    }
    
    (*env)->ReleasePrimitiveArrayCritical(env, src, floats, JNI_ABORT);
    (*env)->ReleasePrimitiveArrayCritical(env, dst, bytes, 0);
}

/*
 * Class:     java_io_ObjectOutputStream
 * Method:    doublesToBytes
 * Signature: ([DI[BII)V
 * 
 * Convert ndoubles double values to their byte representations.  Double
 * values are read from array src starting at offset srcpos and written to
 * array dst starting at offset dstpos.
 */
JNIEXPORT void JNICALL 
Java_java_io_ObjectOutputStream_doublesToBytes(JNIEnv *env, 
					       jclass thisObj, 
					       jdoubleArray src, 
					       jint srcpos, 
					       jbyteArray dst, 
					       jint dstpos, 
					       jint ndoubles)
{
    union {
	jlong l;
	double d;
    } u;
    jdouble *doubles;
    jbyte *bytes;
    jsize srcend;
    jdouble dval;
    jlong lval;
    jlong ffl = CVMint2Long(0xff);

    if (ndoubles == 0)
	return;
    
    /* fetch source array */
    if (src == NULL) {
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    doubles = (jdouble *)(*env)->GetPrimitiveArrayCritical(env, src, NULL);
    if (doubles == NULL)		/* exception thrown */
	return;
    
    /* fetch dest array */
    if (dst == NULL) {
	(*env)->ReleasePrimitiveArrayCritical(env, src, doubles, JNI_ABORT);
	JNU_ThrowNullPointerException(env, NULL);
	return;
    }
    bytes = (jbyte *)(*env)->GetPrimitiveArrayCritical(env, dst, NULL);
    if (bytes == NULL) {	/* exception thrown */
	(*env)->ReleasePrimitiveArrayCritical(env, src, doubles, JNI_ABORT);
	return;
    }
    
    /* do conversion */
    srcend = srcpos + ndoubles;
    for ( ; srcpos < srcend; srcpos++) {
	dval = doubles[srcpos];
	if (JVM_IsNaN((double) dval)) {		/* collapse NaNs */
	    lval = jint_to_jlong(0x7ff80000);
	    jlong_shl(lval, 32);
	} else {
	    jdouble_to_jlong_bits(&dval);
	    u.d = (double) dval;
	    lval = u.l;
	}
	bytes[dstpos++] = (jbyte)
	    CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 56), ffl));
	bytes[dstpos++] = (jbyte)
	    CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 48), ffl));
	bytes[dstpos++] = (jbyte)
	    CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 40), ffl));
	bytes[dstpos++] = (jbyte)
	    CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 32), ffl));
	bytes[dstpos++] = (jbyte)
	    CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 24), ffl));
	bytes[dstpos++] = (jbyte)
	    CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 16), ffl));
	bytes[dstpos++] = (jbyte)
	    CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 8), ffl));
	bytes[dstpos++] = (jbyte)
	    CVMlong2Int(CVMlongAnd(CVMlongShr(lval, 0), ffl));
    }
    
    (*env)->ReleasePrimitiveArrayCritical(env, src, doubles, JNI_ABORT);
    (*env)->ReleasePrimitiveArrayCritical(env, dst, bytes, 0);
}

