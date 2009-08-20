/*
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
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
 */ 

/**
 *  Macro Native Interface - a collection of macros
 *  providing an abstraction layer for KNI and JNI.
 *
 * @created    January 15, 2003
 */

/************************** Example #1 *******************************

MNI_RET_TYPE_INT
Java_com_sun_mmedia_RTPPlayer_nConnectorOpen
MNI_FUNCTION_PARAMS_3( jstring jhost, jint jport, jboolean jmulticast) {
    RTP_MEMORY *mem_ptr;
    const char *hostname;
    int port;
    RTP_BOOL multicast;

    MNI_GET_STRING_PARAM( hostname, jhost, 1);
    MNI_GET_INT_PARAM( port, jport, 2);
    MNI_GET_BOOL_PARAM( multicast, jmulticast, 3);

    mem_ptr = rtp_init_memory();
    connector_open( mem_ptr->connector, hostname, port, multicast);

    MNI_RELEASE_STRING_PARAM( hostname, jhost);
    
    MNI_RET_VALUE_INT( mem_ptr);
}

Explanation: Example #1 defines a function with three input parameters
which returns an integer value. Local variables must be declared 
immediately after the function declaration. The next section features
marcros to retrieve funtion parameters, i.e. MNI_GET_<TYPE>_PARAM.
The first parameter to this macro is the local variable, the second
parameter is the equivalent JNI function parameter which would be
used in the JNI case.

After processing the information it is important to release all
string and byte array parameters.

Finally, the MNI return value is specified at the end of the function
body.

*************************** Example #2 *******************************

Retrieving a byte array (code fragments, not complete).

MNI_RET_TYPE_BOOLEAN
Java_com_sun_mmedia_RTPPlayer_nCopyRGBBuffer
MNI_FUNCTION_PARAMS_2( jint jpeer, jbyteArray jrgbBuffer) {

    ...

    MNI_GET_BYTE_ARRAY_PARAM( data, jrgbBuffer, 1);

    MNI_RELEASE_BYTE_ARRAY_PARAM( data, jrgbBuffer, 0);

    ...
}

*************************** Notes ***** *******************************
For JNI, MNI_GET_STRING_PARAM returns a pointer to the string while
in the KNI case MNI_GET_STRING_PARAM physically copies the string
to the local variable. The local variable is allocated within the
macro definitions and free/released when MNI_RELEASE_STRING_PARAM
is being called.

*************************** Limitations *******************************

Currently, only one parameter of type string or byte array
is supported. I'm planning to provide versions for 2 or 3 parameters
with a future version of MNI.
**********************************************************************/

#ifdef CDC
    #include <jni.h>
    #include "jni-util.h"
#endif

#ifdef JTWI_HI
#include <kni.h>
#include <ROMStructs.h>
#include <midpMalloc.h>
/* #include <machine_md.h> */
#endif

#ifdef WTK
#include <kni.h>
#include <midpMalloc.h>
#endif

#ifdef CDC

    // Boolean constants
    #define MNI_TRUE JNI_TRUE
    #define MNI_FALSE JNI_FALSE

    // Memory allocation
    #define MNI_MALLOC(x) malloc(x)
    #define MNI_CALLOC(x,y) calloc(x,y)
    #define MNI_FREE(x) free(x)

    // return types:
    #define MNI_RET_TYPE_VOID JNIEXPORT void JNICALL
    #define MNI_RET_TYPE_BOOLEAN JNIEXPORT jboolean JNICALL
    #define MNI_RET_TYPE_BYTE JNIEXPORT jbyte JNICALL
    #define MNI_RET_TYPE_CHAR JNIEXPORT jchar JNICALL
    #define MNI_RET_TYPE_SHORT JNIEXPORT jshort JNICALL
    #define MNI_RET_TYPE_INT JNIEXPORT jint JNICALL
    #define MNI_RET_TYPE_LONG JNIEXPORT jlong JNICALL
    #define MNI_RET_TYPE_OBJECT JNIEXPORT jobject JNICALL

    // function parameters:
    #define MNI_FUNCTION_PARAMS_0() JNIEnv *env, jobject obj
    #define MNI_FUNCTION_PARAMS_1( p1) JNIEnv *env, jobject obj, p1
    #define MNI_FUNCTION_PARAMS_2( p1, p2) JNIEnv *env, jobject obj, p1, p2
    #define MNI_FUNCTION_PARAMS_3( p1, p2, p3) \
        JNIEnv *env, jobject obj, p1, p2, p3
    #define MNI_FUNCTION_PARAMS_4( p1, p2, p3, p4) \
        JNIEnv *env, jobject obj, p1, p2, p3, p4
    #define MNI_FUNCTION_PARAMS_5( p1, p2, p3, p4, p5) \
        JNIEnv *env, jobject obj, p1, p2, p3, p4, p5
    #define MNI_FUNCTION_PARAMS_6( p1, p2, p3, p4, p5, p6) \
        JNIEnv *env, jobject obj, p1, p2, p3, p4, p5, p6
    #define MNI_FUNCTION_PARAMS_9( p1, p2, p3, p4, p5, p6, p7, p8, p9) \
        JNIEnv *env, jobject obj, p1, p2, p3, p4, p5, p6, p7, p8, p9


    // retrieving parameters:
    #define MNI_GET_STRING_PARAM( local_var, jni_param, index) \
        local_var = (*env)->GetStringUTFChars( env, jni_param, 0)

    #define MNI_GET_INT_PARAM( local_var, jni_param, index) \
        local_var = jni_param

    #define MNI_GET_LONG_PARAM( local_var, jni_param, index) \
        local_var = jni_param

    #define MNI_GET_BOOL_PARAM( local_var, jni_param, index) \
        local_var = jni_param

    #define MNI_GET_BYTE_ARRAY_PARAM( local_var, jni_param, array_len, index) \
        array_len = (*env)->GetArrayLength(env, jni_param); \
        local_var = (jbyte *) (*env)->GetByteArrayElements(env, jni_param, 0)

    #define MNI_GET_INT_ARRAY_PARAM( local_var, jni_param, array_len, index) \
        array_len = (*env)->GetArrayLength(env, jni_param); \
        local_var = (jint *) (*env)->GetIntArrayElements(env, jni_param, 0)

    // setting variable defined in Java:
    #define MNI_SET_INT_FIELD( fieldName, value) \
        SetIntField( env, obj, fieldName, value)

    #define MNI_SET_BOOL_FIELD( fieldName, value) \
        SetBooleanField( env, obj, fieldName, value)

    // jni_param is of type jbyteArray
    #define MNI_SET_BYTE_ARRAY_REGION( srcBuffer, jni_param, offset, size, index) \
        (*env)->SetByteArrayRegion(env, jni_param, offset, size, (jbyte *)srcBuffer)

    #define MNI_SET_INT_ARRAY_REGION( srcBuffer, jni_param, offset, size, index) \
        (*env)->SetIntArrayRegion(env, jni_param, offset, size, (jint *)srcBuffer)

    // return values:
    #define MNI_RET_VALUE_INT(x) return (jint) x
    #define MNI_RET_VALUE_SHORT(x) return (jshort) x
    #define MNI_RET_VALUE_LONG(x) return (jlong) x
    #define MNI_RET_VALUE_BOOLEAN(x) return (jboolean) x
    #define MNI_RET_VALUE_VOID return

    // releasing references:
    #define MNI_RELEASE_STRING_PARAM( local_var, jni_param) \
        (*env)->ReleaseStringUTFChars(env, jni_param, local_var)
    #define MNI_RELEASE_BYTE_ARRAY_PARAM( local_var, jni_param, flag) \
        (*env)->ReleaseByteArrayElements(env, jni_param, local_var, flag)
    #define MNI_RELEASE_INT_ARRAY_PARAM( local_var, jni_param, flag) \
        (*env)->ReleaseIntArrayElements(env, jni_param, local_var, flag)
        
    //throwing exceptions:
    #define MNI_THROW_NEW( class_name, message) \
        { jclass excCls = (*env)->FindClass(env, (class_name); \
          if (excCls != 0) (*env)->ThrowNew(env, excCls, (message)); }
#endif

#ifdef JTWI_HI

    #define unhand(__type, __ptr)  (*((__type**)(__ptr)))

    // Boolean constants
    #define MNI_TRUE KNI_TRUE
    #define MNI_FALSE KNI_FALSE

    // Memory allocation
    #define MNI_MALLOC(x) midpMalloc(x)
    #define MNI_CALLOC(x,y) midpCalloc(x,y)
    #define MNI_FREE(x) midpFree(x)

    // return types:
    #define MNI_RET_TYPE_VOID KNIEXPORT KNI_RETURNTYPE_VOID
    #define MNI_RET_TYPE_BOOLEAN KNIEXPORT KNI_RETURNTYPE_BOOLEAN
    #define MNI_RET_TYPE_BYTE KNIEXPORT KNI_RETURNTYPE_BYTE
    #define MNI_RET_TYPE_CHAR KNIEXPORT KNI_RETURNTYPE_CHAR
    #define MNI_RET_TYPE_SHORT KNIEXPORT KNI_RETURNTYPE_SHORT
    #define MNI_RET_TYPE_INT KNIEXPORT KNI_RETURNTYPE_INT
    #define MNI_RET_TYPE_LONG KNIEXPORT KNI_RETURNTYPE_LONG
    #define MNI_RET_TYPE_OBJECT KNIEXPORT KNI_RETURNTYPE_OBJECT

    // function parameters:
    #define MNI_FUNCTION_PARAMS_0( ) KNITRAPS
    #define MNI_FUNCTION_PARAMS_1( p1) KNITRAPS
    #define MNI_FUNCTION_PARAMS_2( p1, p2) KNITRAPS
    #define MNI_FUNCTION_PARAMS_3( p1, p2, p3) KNITRAPS
    #define MNI_FUNCTION_PARAMS_4( p1, p2, p3, p4) KNITRAPS
    #define MNI_FUNCTION_PARAMS_5( p1, p2, p3, p4, p5) KNITRAPS
    #define MNI_FUNCTION_PARAMS_6( p1, p2, p3, p4, p5, p6) KNITRAPS
    #define MNI_FUNCTION_PARAMS_9( p1, p2, p3, p4, p5, p6, p7, p8, p9) \
				KNITRAPS

    // retrieving parameters:
    // problem with unicode to char conversion!!!
    // sequential Set_Int
    #define MNI_GET_STRING_PARAM( local_var, jni_param, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
        local_var = midpMalloc( KNI_GetArrayLength( arrHandle)); \
        KNI_GetRawArrayRegion(arrHandle, 0, KNI_GetArrayLength( arrHandle), \
			      (jbyte *)local_var)   

    #define MNI_GET_BYTE_ARRAY_PARAM( local_var, jni_param, array_len, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
	array_len = KNI_GetArrayLength( arrHandle ); \
	local_var = unhand(jbyte_array, arrHandle)->elements

    #define MNI_GET_INT_ARRAY_PARAM( local_var, jni_param, array_len, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
	array_len = KNI_GetArrayLength( arrHandle ); \
	local_var = unhand(jint_array, arrHandle)->elements

    #define MNI_GET_INT_PARAM( local_var, jni_param, index) \
        local_var = KNI_GetParameterAsInt(index)

     #define MNI_GET_LONG_PARAM( local_var, jni_param, index) \
	        local_var = KNI_GetParameterAsLong(index)
	
    #define MNI_GET_BOOL_PARAM( local_var, jni_param, index) \
        local_var = KNI_GetParameterAsBoolean(index)

    // setting variable defined in Java:
    #define MNI_SET_INT_FIELD( fieldName, value) \
        KNI_StartHandles(2); \
        KNI_DeclareHandle( objectHandle); \
        KNI_DeclareHandle( classHandle); \
        KNI_GetParameterAsObject( 0, objectHandle); \
        KNI_GetObjectClass( objectHandle, classHandle); \
        KNI_SetIntField( objectHandle, KNI_GetFieldID( classHandle, fieldName,"I"), value); \
        KNI_EndHandles()

    #define MNI_SET_BOOL_FIELD( fieldName, value) \
        KNI_StartHandles(2); \
        KNI_DeclareHandle( objectHandle); \
        KNI_DeclareHandle( classHandle); \
        KNI_GetThisPointer( objectHandle); \
        KNI_GetObjectClass( objectHandle, classHandle); \
        KNI_SetBooleanField( objectHandle, KNI_GetFieldID( classHandle, fieldName,"Z"), value); \
        KNI_EndHandles()

    #define MNI_SET_BYTE_ARRAY_REGION( srcBuffer, jni_param, offset, size, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
        KNI_SetRawArrayRegion( arrHandle, offset, size, srcBuffer); \
        KNI_EndHandles()

    #define MNI_SET_INT_ARRAY_REGION( srcBuffer, jni_param, offset, size, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
        KNI_SetRawArrayRegion( arrHandle, offset * 4, size * 4, srcBuffer); \
        KNI_EndHandles()

    // return values:
    #define MNI_RET_VALUE_INT(x) KNI_ReturnInt((KNI_RETURNTYPE_INT) x)
    #define MNI_RET_VALUE_VOID KNI_ReturnVoid()
    #define MNI_RET_VALUE_SHORT(x) KNI_ReturnShort((KNI_RETURNTYPE_SHORT) x)
    #define MNI_RET_VALUE_LONG(x) KNI_ReturnLong((KNI_RETURNTYPE_LONG) x)
    #define MNI_RET_VALUE_BOOLEAN(x) KNI_ReturnBoolean((KNI_RETURNTYPE_BOOLEAN) x)

    // releasing references:
    #define MNI_RELEASE_STRING_PARAM( local_var, jni_param) \
        midpFree( (void *)local_var); \
        KNI_EndHandles()
    #define MNI_RELEASE_BYTE_ARRAY_PARAM( local_var, jni_param, flag) \
        KNI_EndHandles()
    #define MNI_RELEASE_INT_ARRAY_PARAM( local_var, jni_param, flag) \
        KNI_EndHandles()

    //throwing exceptions:
    #define MNI_THROW_NEW( class_name, message) KNI_ThrowNew((class_name), (message))
#endif

#ifdef WTK

    // Boolean constants
    #define MNI_TRUE KNI_TRUE
    #define MNI_FALSE KNI_FALSE

    // Memory allocation
    #define MNI_MALLOC(x) midpMalloc(x)
    #define MNI_CALLOC(x,y) midpCalloc(x,y)
    #define MNI_FREE(x) midpFree(x)

    // return types:
    #define MNI_RET_TYPE_VOID KNIEXPORT KNI_RETURNTYPE_VOID
    #define MNI_RET_TYPE_BOOLEAN KNIEXPORT KNI_RETURNTYPE_BOOLEAN
    #define MNI_RET_TYPE_BYTE KNIEXPORT KNI_RETURNTYPE_BYTE
    #define MNI_RET_TYPE_CHAR KNIEXPORT KNI_RETURNTYPE_CHAR
    #define MNI_RET_TYPE_SHORT KNIEXPORT KNI_RETURNTYPE_SHORT
    #define MNI_RET_TYPE_INT KNIEXPORT KNI_RETURNTYPE_INT
    #define MNI_RET_TYPE_LONG KNIEXPORT KNI_RETURNTYPE_LONG
    #define MNI_RET_TYPE_OBJECT KNIEXPORT KNI_RETURNTYPE_OBJECT

    // function parameters:
    #define MNI_FUNCTION_PARAMS_0( ) void
    #define MNI_FUNCTION_PARAMS_1( p1) void
    #define MNI_FUNCTION_PARAMS_2( p1, p2) void
    #define MNI_FUNCTION_PARAMS_3( p1, p2, p3) void
    #define MNI_FUNCTION_PARAMS_4( p1, p2, p3, p4) void
    #define MNI_FUNCTION_PARAMS_5( p1, p2, p3, p4, p5) void
    #define MNI_FUNCTION_PARAMS_6( p1, p2, p3, p4, p5, p6) void
    #define MNI_FUNCTION_PARAMS_9( p1, p2, p3, p4, p5, p6, p7, p8, p9) void

    // retrieving parameters:
    // problem with unicode to char conversion!!!
    // sequential Set_Int
    #define MNI_GET_STRING_PARAM( local_var, jni_param, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
        local_var = midpMalloc( KNI_GetArrayLength( arrHandle)); \
        KNI_GetRawArrayRegion(arrHandle, 0, KNI_GetArrayLength( arrHandle), \
			      (jbyte *)local_var)   

    #define MNI_GET_BYTE_ARRAY_PARAM( local_var, jni_param, array_len, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
	array_len = KNI_GetArrayLength( arrHandle ); \
	local_var = midpMalloc(array_len); \
	KNI_GetRawArrayRegion(arrHandle, 0, array_len, (jbyte*) local_var);

    #define MNI_GET_INT_ARRAY_PARAM( local_var, jni_param, array_len, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
	array_len = KNI_GetArrayLength( arrHandle ); \
	local_var = unhand(jint_array, arrHandle)->elements

    #define MNI_GET_INT_PARAM( local_var, jni_param, index) \
        local_var = KNI_GetParameterAsInt(index)

     #define MNI_GET_LONG_PARAM( local_var, jni_param, index) \
	        local_var = KNI_GetParameterAsLong(index)
	
    #define MNI_GET_BOOL_PARAM( local_var, jni_param, index) \
        local_var = KNI_GetParameterAsBoolean(index)

    // setting variable defined in Java:
    #define MNI_SET_INT_FIELD( fieldName, value) \
        KNI_StartHandles(2); \
        KNI_DeclareHandle( objectHandle); \
        KNI_DeclareHandle( classHandle); \
        KNI_GetParameterAsObject( 0, objectHandle); \
        KNI_GetObjectClass( objectHandle, classHandle); \
        KNI_SetIntField( objectHandle, KNI_GetFieldID( classHandle, fieldName,"I"), value); \
        KNI_EndHandles()

    #define MNI_SET_BOOL_FIELD( fieldName, value) \
        KNI_StartHandles(2); \
        KNI_DeclareHandle( objectHandle); \
        KNI_DeclareHandle( classHandle); \
        KNI_GetThisPointer( objectHandle); \
        KNI_GetObjectClass( objectHandle, classHandle); \
        KNI_SetBooleanField( objectHandle, KNI_GetFieldID( classHandle, fieldName,"Z"), value); \
        KNI_EndHandles()

    #define MNI_SET_BYTE_ARRAY_REGION( srcBuffer, jni_param, offset, size, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
        KNI_SetRawArrayRegion( arrHandle, offset, size, srcBuffer); \
        KNI_EndHandles()

    #define MNI_SET_INT_ARRAY_REGION( srcBuffer, jni_param, offset, size, index) \
        KNI_StartHandles(1); \
        KNI_DeclareHandle(arrHandle); \
        KNI_GetParameterAsObject( index, arrHandle); \
        KNI_SetRawArrayRegion( arrHandle, offset * 4, size * 4, srcBuffer); \
        KNI_EndHandles()

    // return values:
    #define MNI_RET_VALUE_INT(x) KNI_ReturnInt(x)
    #define MNI_RET_VALUE_VOID KNI_ReturnVoid()
    #define MNI_RET_VALUE_SHORT(x) KNI_ReturnShort(x)
    #define MNI_RET_VALUE_LONG(x) KNI_ReturnLong(x)
    #define MNI_RET_VALUE_BOOLEAN(x) KNI_ReturnBoolean(x)

    // releasing references:
    #define MNI_RELEASE_STRING_PARAM( local_var, jni_param) \
        midpFree( (void *)local_var); \
        KNI_EndHandles()

    #define MNI_RELEASE_BYTE_ARRAY_PARAM( local_var, jni_param, flag) \
	midpFree( (void*) local_var); \
        KNI_EndHandles()

    #define MNI_RELEASE_INT_ARRAY_PARAM( local_var, jni_param, flag) \
        KNI_EndHandles()
        
    //throwing exceptions:
    #define MNI_THROW_NEW( class_name, message) KNI_ThrowNew((class_name), (message))

#endif


// JavaCall case
#ifndef MNI_MALLOC
#include "javacall_memory.h"
    #define MNI_MALLOC(x) javacall_malloc(x)
    #define MNI_CALLOC(x,y) javacall_calloc(x,y)
    #define MNI_FREE(x) javacall_free(x)

#endif