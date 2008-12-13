/*
 *   
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

/** \file KniUncommon.cpp
 *
 * This file implements the less commonly used features of KNI. If the
 * MIDP code doesn't use any of these functions, then this object file
 * will not be linked, thus reducing static code footprint.
 */

#include "incls/_precompiled.incl"
#include "incls/_KniUncommon.cpp.incl"


extern "C" void* kni_get_java_class_statics(jclass classHandle) {
  JavaClassObj::Raw mirror = kni_read_handle(classHandle);
  GUARANTEE(mirror.not_null(), "null argument to kni_get_java_class");

#if ENABLE_ISOLATES
  JavaClass::Raw jc = mirror().java_class();
  OopDesc* java_class = jc().real_task_mirror();
#else
  OopDesc* java_class = mirror().java_class();
#endif
  return java_class;
}

KNIEXPORT jboolean KNI_GetBooleanField(jobject objectHandle, jfieldID fieldID)
{
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetBooleanField");
  return *object->bool_field_addr((int)fieldID);
}

KNIEXPORT jbyte KNI_GetByteField(jobject objectHandle, jfieldID fieldID) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetByteField");
  return *object->byte_field_addr((int)fieldID);
}

KNIEXPORT jchar KNI_GetCharField(jobject objectHandle, jfieldID fieldID) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetCharField");
  return *object->char_field_addr((int)fieldID);
}

KNIEXPORT jshort KNI_GetShortField(jobject objectHandle, jfieldID fieldID) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetShortField");
  return *object->short_field_addr((int)fieldID);
}

KNIEXPORT jlong KNI_GetLongField(jobject objectHandle, jfieldID fieldID) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetLongField");
  return *object->long_field_addr((int)fieldID);
}

#if ENABLE_FLOAT

KNIEXPORT jfloat KNI_GetFloatField(jobject objectHandle, jfieldID fieldID) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetFloatField");
  return *object->float_field_addr((int)fieldID);
}

KNIEXPORT jdouble KNI_GetDoubleField(jobject objectHandle, jfieldID fieldID) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_GetDoubleField");
  return *object->double_field_addr((int)fieldID);
}

#endif // ENABLE_FLOAT

KNIEXPORT void
KNI_SetBooleanField(jobject objectHandle, jfieldID fieldID, jboolean value) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetBooleanField");
  *object->bool_field_addr((int)fieldID) = value;
}

KNIEXPORT void
KNI_SetByteField(jobject objectHandle, jfieldID fieldID, jbyte value) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetByteField");
  *object->byte_field_addr((int)fieldID) = value;
}

KNIEXPORT void
KNI_SetCharField(jobject objectHandle, jfieldID fieldID, jchar value) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetCharField");
  *object->char_field_addr((int)fieldID) = value;
}

KNIEXPORT void
KNI_SetShortField(jobject objectHandle, jfieldID fieldID, jshort value) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetShortField");
  *object->short_field_addr((int)fieldID) = value;
}

KNIEXPORT void
KNI_SetIntField(jobject objectHandle, jfieldID fieldID, jint value) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetIntField");
  *object->int_field_addr((int)fieldID) = value;
}

KNIEXPORT void
KNI_SetLongField(jobject objectHandle, jfieldID fieldID, jlong value) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetLongField");
  *object->long_field_addr((int)fieldID) = value;
}

#if ENABLE_FLOAT

KNIEXPORT void
KNI_SetFloatField(jobject objectHandle, jfieldID fieldID, jfloat value) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetFloatField");
  *object->float_field_addr((int)fieldID) = value;
}

KNIEXPORT void
KNI_SetDoubleField(jobject objectHandle, jfieldID fieldID, jdouble value) {
  OopDesc* object = kni_read_handle(objectHandle);
  GUARANTEE(object != 0, "null argument to KNI_SetDoubleField");
  *object->double_field_addr((int)fieldID) = value;
}

#endif // ENABLE_FLOAT

//
// Static field access
//
KNIEXPORT jfieldID KNI_GetStaticFieldID(jclass classHandle, const char* name,
                                        const char* signature) {
  return _KNI_field_lookup_helper(classHandle, name, signature, true);
}

KNIEXPORT jboolean
KNI_GetStaticBooleanField(jclass classHandle, jfieldID fieldID) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  return (jboolean) (*static_vars->int_field_addr((int)fieldID));
}

KNIEXPORT jbyte KNI_GetStaticByteField(jclass classHandle, jfieldID fieldID) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  return (jbyte) (*static_vars->int_field_addr((int)fieldID));
}

KNIEXPORT jchar KNI_GetStaticCharField(jclass classHandle, jfieldID fieldID) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  return (jchar) (*static_vars->uint_field_addr((int)fieldID));
}

KNIEXPORT jshort KNI_GetStaticShortField(jclass classHandle, jfieldID fieldID)
{
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  return (jshort) (*static_vars->int_field_addr((int)fieldID));
}

KNIEXPORT jint KNI_GetStaticIntField(jclass classHandle, jfieldID fieldID) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  return *static_vars->int_field_addr((int)fieldID);
}

KNIEXPORT jlong KNI_GetStaticLongField(jclass classHandle, jfieldID fieldID) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  return *static_vars->long_field_addr((int)fieldID);
}

#if ENABLE_FLOAT

KNIEXPORT jfloat KNI_GetStaticFloatField(jclass classHandle, jfieldID fieldID)
{
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  return *static_vars->float_field_addr((int)fieldID);
}

KNIEXPORT jdouble
KNI_GetStaticDoubleField(jclass classHandle, jfieldID fieldID) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  return *static_vars->double_field_addr((int)fieldID);
}

#endif // ENABLE_FLOAT

// IMPL_NOTE: to save footprint, all 8, 16 and 32 bit static field get/set
// functions can be aliased into a single C function -- small static
// fields are internally expanded to 32-bit
KNIEXPORT void
KNI_SetStaticBooleanField(jclass classHandle, jfieldID fieldID, jboolean value)
{
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  *static_vars->int_field_addr((int)fieldID) = (jint)value;
}

KNIEXPORT void
KNI_SetStaticByteField(jclass classHandle, jfieldID fieldID, jbyte value) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  *static_vars->int_field_addr((int)fieldID) = (jint)value;
}

KNIEXPORT void
KNI_SetStaticCharField(jclass classHandle, jfieldID fieldID, jchar value) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  *static_vars->uint_field_addr((int)fieldID) = (juint)value;
}

KNIEXPORT void
KNI_SetStaticShortField(jclass classHandle, jfieldID fieldID, jshort value) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  *static_vars->int_field_addr((int)fieldID) = (jint)value;
}

KNIEXPORT void
KNI_SetStaticIntField(jclass classHandle, jfieldID fieldID, jint value) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  *static_vars->int_field_addr((int)fieldID) = value;
}

KNIEXPORT void
KNI_SetStaticLongField(jclass classHandle, jfieldID fieldID, jlong value) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  *static_vars->long_field_addr((int)fieldID) = value;
}

#if ENABLE_FLOAT

KNIEXPORT void
KNI_SetStaticFloatField(jclass classHandle, jfieldID fieldID, jfloat value) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  *static_vars->float_field_addr((int)fieldID) = value;
}

KNIEXPORT void
KNI_SetStaticDoubleField(jclass classHandle, jfieldID fieldID, jdouble value) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  *static_vars->double_field_addr((int)fieldID) = value;
}

#endif // ENABLE_FLOAT

KNIEXPORT void KNI_GetStaticObjectField(jclass classHandle, jfieldID fieldID,
                                        jobject toHandle) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  kni_set_handle(toHandle, *static_vars->obj_field_addr((int)fieldID));
}

KNIEXPORT void KNI_SetStaticObjectField(jclass classHandle, jfieldID fieldID,
                                        jobject fromHandle) {
  OopDesc* static_vars = (OopDesc*)kni_get_java_class_statics(classHandle);
  oop_write_barrier(static_vars->obj_field_addr((int)fieldID),
                   kni_read_handle(fromHandle));
}
