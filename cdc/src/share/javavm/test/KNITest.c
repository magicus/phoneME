/*
 * @(#)KNITest.c	1.6 06/10/17
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
 */

#include "kni.h"
#include "sni.h"

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_returnThis) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(this);
    KNI_GetThisPointer(this);
    KNI_EndHandlesAndReturnObject(this);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_returnClass) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(class);
    KNI_GetClassPointer(class);
    KNI_EndHandlesAndReturnObject(class);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(KNITest_testIntArgs) {
    jint x = KNI_GetParameterAsInt(1);
    jint y = KNI_GetParameterAsInt(2);
    jint z = KNI_GetParameterAsInt(3);
    KNI_ReturnInt(x*y+z);
}

KNIEXPORT KNI_RETURNTYPE_FLOAT
KNIDECL(KNITest_testFloatArgs) {
    jfloat x = KNI_GetParameterAsFloat(1);
    jfloat y = KNI_GetParameterAsFloat(2);
    jfloat z = KNI_GetParameterAsFloat(3);
    KNI_ReturnFloat(x*y+z);
}

KNIEXPORT KNI_RETURNTYPE_LONG
KNIDECL(KNITest_testLongArgs) {
    jlong x = KNI_GetParameterAsLong(1);
    jlong y = KNI_GetParameterAsLong(3);
    jlong z = KNI_GetParameterAsLong(5);
    KNI_ReturnLong(x*y+z);
}

KNIEXPORT KNI_RETURNTYPE_DOUBLE
KNIDECL(KNITest_testDoubleArgs) {
    jdouble x = KNI_GetParameterAsDouble(1);
    jdouble y = KNI_GetParameterAsDouble(3);
    jdouble z = KNI_GetParameterAsDouble(5);
    KNI_ReturnDouble(x*y+z);
}

KNIEXPORT KNI_RETURNTYPE_LONG
KNIDECL(KNITest_testIntLongArgs) {
    jlong x = KNI_GetParameterAsLong(1);
    jint y = KNI_GetParameterAsInt(3);
    jlong z = KNI_GetParameterAsLong(4);
    KNI_ReturnLong(x*y+z);
}

KNIEXPORT KNI_RETURNTYPE_DOUBLE
KNIDECL(KNITest_testFloatDoubleArgs) {
    jdouble x = KNI_GetParameterAsDouble(1);
    jfloat y = KNI_GetParameterAsFloat(3);
    jdouble z = KNI_GetParameterAsDouble(4);
    KNI_ReturnDouble(x*y+z);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(KNITest_testVirtualArg) {
    jint x = KNI_GetParameterAsInt(1);
    KNI_ReturnInt(x*2);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(KNITest_testGetSetIntField) {
    jint x = KNI_GetParameterAsInt(1);
    jint y;
    jfieldID typeFieldID;
    KNI_StartHandles(2);
    KNI_DeclareHandle(this);
    KNI_DeclareHandle(clazz);
    KNI_GetThisPointer(this);
    KNI_GetObjectClass(this, clazz);
    typeFieldID = KNI_GetFieldID(clazz, "i", "I");
    y = KNI_GetIntField(this, typeFieldID);
    KNI_SetIntField(this, typeFieldID, x*y);
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(KNITest_testGetSetStaticDoubleField) {
    jdouble x = KNI_GetParameterAsDouble(1);
    jdouble y;
    jfieldID typeFieldID;
    KNI_StartHandles(1);
    KNI_DeclareHandle(clazz);
    KNI_GetClassPointer(clazz);
    typeFieldID = KNI_GetStaticFieldID(clazz, "d", "D");
    y = KNI_GetStaticDoubleField(clazz, typeFieldID);
    KNI_SetStaticDoubleField(clazz, typeFieldID, x*y);
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_testFindClass) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(class);
    KNI_FindClass("java/lang/String", class);
    KNI_EndHandlesAndReturnObject(class);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_getSuperClass) {
    KNI_StartHandles(2);
    KNI_DeclareHandle(class);
    KNI_DeclareHandle(superClass);
    KNI_GetParameterAsObject(1, class);
    KNI_GetSuperClass(class, superClass);
    KNI_EndHandlesAndReturnObject(superClass);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(KNITest_isAssignableFrom) {
    jboolean result;
    KNI_StartHandles(2);
    KNI_DeclareHandle(srclass);
    KNI_DeclareHandle(destClass);
    KNI_GetParameterAsObject(1, srclass);
    KNI_GetParameterAsObject(2, destClass);
    result = KNI_IsAssignableFrom(srclass, destClass);
    KNI_EndHandles();
    KNI_ReturnBoolean(result);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(KNITest_throwException) {
    KNI_ThrowNew("java/io/IOException", "catch me!");
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(KNITest_isInstanceOf) {
    jboolean result;
    KNI_StartHandles(2);
    KNI_DeclareHandle(srcObj);
    KNI_DeclareHandle(srcClass);
    KNI_GetParameterAsObject(1, srcObj);
    KNI_GetParameterAsObject(2, srcClass);
    result = KNI_IsInstanceOf(srcObj, srcClass);
    KNI_EndHandles();
    KNI_ReturnBoolean(result);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(KNITest_isSameObject) {
    jboolean result;
    KNI_StartHandles(2);
    KNI_DeclareHandle(obj1);
    KNI_DeclareHandle(obj2);
    KNI_GetParameterAsObject(1, obj1);
    KNI_GetParameterAsObject(2, obj2);
    result = KNI_IsSameObject(obj1, obj2);
    KNI_EndHandles();
    KNI_ReturnBoolean(result);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(KNITest_getStringLength) {
    jint result;
    KNI_StartHandles(1);
    KNI_DeclareHandle(str);
    KNI_GetParameterAsObject(1, str);
    result = KNI_GetStringLength(str);
    KNI_EndHandles();
    KNI_ReturnInt(result);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_getStringRegion) {
    jchar buf[20];
    jint offset = KNI_GetParameterAsInt(2);
    jint len = KNI_GetParameterAsInt(3);
    KNI_StartHandles(2);
    KNI_DeclareHandle(str);
    KNI_DeclareHandle(newStr);
    KNI_GetParameterAsObject(1, str);
    KNI_GetStringRegion(str, offset, len, buf);
    KNI_NewString(buf, len, newStr);
    KNI_EndHandlesAndReturnObject(newStr);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_newStringUTF) {
    const char* buf = "hello";
    KNI_StartHandles(1);
    KNI_DeclareHandle(newStr);
    KNI_NewStringUTF(buf, newStr);
    KNI_EndHandlesAndReturnObject(newStr);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_newString) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(newStr);
    KNI_NewString(NULL, -1, newStr);
    KNI_EndHandlesAndReturnObject(newStr);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_newStringArray) {
    jint len = KNI_GetParameterAsInt(1);
    KNI_StartHandles(1);
    KNI_DeclareHandle(newStrArray);
    SNI_NewArray(SNI_STRING_ARRAY, len, newStrArray);
    KNI_EndHandlesAndReturnObject(newStrArray);
}


KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(KNITest_newObjectArray) {
    jint len;
    KNI_StartHandles(2);
    KNI_DeclareHandle(newArr);
    KNI_DeclareHandle(clazz);
    KNI_GetParameterAsObject(1, clazz);
    len = KNI_GetParameterAsInt(2);
    SNI_NewObjectArray(clazz, len, newArr);
    KNI_EndHandlesAndReturnObject(newArr);
}
