/*
 * Copyright 1990-2009 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/*
 * Implementation of class sun.misc.Unsafe
 */

#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "native/common/jni_util.h"
#include "javavm/include/unsaferom2native.h"
#include "javavm/include/clib.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/reflect.h"
#include "javavm/include/sync.h"
#include "java_lang_ClassLoader.h"

#define UNSAFE_ENTRY(result_type, header) \
    result_type JNICALL header { 

#define UNSAFE_END }

/* mlam #define UnsafeWrapper(arg)  */

#define CVMjniNonNullICellPtrFor(jobj) \
    CVMID_NonNullICellPtrFor(jobj)
#undef CVMjniGcUnsafeRef2Class
#define CVMjniGcUnsafeRef2Class(ee, cl) CVMgcUnsafeClassRef2ClassBlock(ee, cl)

#undef CVMjniGcSafeRef2Class
#define CVMjniGcSafeRef2Class(ee, cl) CVMgcSafeClassRef2ClassBlock(ee, cl)

#define CAST_FROM_FN_PTR(new_type, func_ptr) ((new_type)((uintptr_t)(func_ptr)))


static void* addr_from_java(jlong addr) {
    /* This assert fails in a variety of ways on 32-bit systems. 
     * It is impossible to predict whether native code that converts 
     * pointers to longs will sign-extend or zero-extend the addresses. 
     * assert(addr == (uintptr_t)addr, "must not be odd high bits"); 
     */
    return (void*)(uintptr_t)addr;
}

static jlong addr_to_java(void* p) {
    CVMassert(p == (void*)(uintptr_t)p); /* must not be odd high bits */
    return (uintptr_t)p;
}


/* 
 * Note: The VM's obj_field and related accessors use word-scaled
 * offsets, just as the unsafe methods do. 
 */

/* 
 * However, the method Unsafe.fieldOffset explicitly declines to 
 * guarantee this.  The field offset values manipulated by the Java user 
 * through the Unsafe API are opaque cookies that just happen to be word 
 * offsets.  We represent this state of affairs by passing the cookies 
 * through conversion functions when going between the VM and the Unsafe API. 
 * The conversion functions just happen to be no-ops at present. 
 *
 * That being said, for CVM we actually want the word-scaled field
 * offset for VM object access APIs. So this API is still a nop,
 * although it does not return the "byte" offset as the name would suggest.
 */

static jlong field_offset_to_byte_offset(jlong field_offset) {
    return field_offset;
}

static jlong field_offset_from_byte_offset(jlong byte_offset) {
    return byte_offset;
}

static jint invocation_key_from_method_slot(jint slot) {
    return slot;
}

static jint invocation_key_to_method_slot(jint key) {
    return key;
}

/* 
 * Externally callable versions: 
 * (Use these in compiler intrinsics which emulate unsafe primitives.) 
 */
jlong Unsafe_field_offset_to_byte_offset(jlong field_offset) {
    return field_offset;
}
jlong Unsafe_field_offset_from_byte_offset(jlong byte_offset) {
    return byte_offset;
}
jint Unsafe_invocation_key_from_method_slot(jint slot) {
    return invocation_key_from_method_slot(slot);
}
jint Unsafe_invocation_key_to_method_slot(jint key) {
    return invocation_key_to_method_slot(key);
}


/* 
 * Data in the Java heap. 
 */

/* 
 * Get/SetObject must be special-cased, since it works with handles. 
 */

/* 
 * The normal variants allow a null base pointer with an arbitrary address. 
 * But if the base pointer is non-null, the offset should make some sense. 
 * That is, it should be in the range [0, MAX_OBJECT_SIZE]. 
 */
UNSAFE_ENTRY(jobject, Unsafe_GetObject(JNIEnv *env, jobject unsafe, jobject obj, jlong offset))
/* mlam UnsafeWrapper("Unsafe_GetObject"); */
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env); 
    CVMObjectICell* resultCell = CVMjniCreateLocalRef(ee);

    if (resultCell == NULL) {
        return NULL;
    }

    CVMID_fieldReadRef(ee, 
                       obj, 
                       field_offset_to_byte_offset(offset), 
                       resultCell);

    /* Don't return ICells containing NULL */
    if (CVMID_icellIsNull(resultCell)) {
        CVMjniDeleteLocalRef(env, resultCell);
        resultCell = NULL;
    }
    return resultCell;
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_SetObject(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jobject x_h))
/* mlam UnsafeWrapper("Unsafe_SetObject"); */
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env); 
    CVMID_fieldWriteRef(ee, 
                        obj, 
                        field_offset_to_byte_offset(offset), 
                        CVMjniNonNullICellPtrFor(x_h));
UNSAFE_END


#define DEFINE_GETSETOOP(jboolean, Boolean, WideType) \
 \
UNSAFE_ENTRY(jboolean, Unsafe_Get##Boolean(JNIEnv *env, jobject unsafe, jobject obj, jlong offset)) \
    jboolean v; \
    /* mlam UnsafeWrapper("Unsafe_Get"#Boolean); */	\
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env); \
    CVMID_fieldRead##WideType(ee, \
                             obj, \
                             field_offset_to_byte_offset(offset), \
                             v); \
    return v; \
UNSAFE_END \
 \
UNSAFE_ENTRY(void, Unsafe_Set##Boolean(JNIEnv *env, jobject unsafe, jobject obj, jlong offset, jboolean x)) \
    /* mlam UnsafeWrapper("Unsafe_Set"#Boolean); */ \
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env); \
    CVMID_fieldWrite##WideType(ee, \
                              obj, \
                              field_offset_to_byte_offset(offset), \
                              x); \
UNSAFE_END \
 \
/* END DEFINE_GETSETOOP. */

DEFINE_GETSETOOP(jboolean, Boolean, Int)
DEFINE_GETSETOOP(jbyte, Byte, Int)
DEFINE_GETSETOOP(jshort, Short, Int);
DEFINE_GETSETOOP(jchar, Char, Int);
DEFINE_GETSETOOP(jint, Int, Int);
DEFINE_GETSETOOP(jlong, Long, Long);
DEFINE_GETSETOOP(jfloat, Float, Float);
DEFINE_GETSETOOP(jdouble, Double, Double);

#undef DEFINE_GETSETOOP


/* Data in the C heap. */

/* Note:  These do not throw NullPointerException for bad pointers. 
 * They just crash.  Only an Object base pointer can generate a 
 * NullPointerException. 
 *
 * [RGV] Hotspot sets a thread specific flag to assist in SEGV processing
 *  void* p = addr_from_java(addr); \
 *  JavaThread* t = JavaThread::current(); \
 *  t->set_doing_unsafe_access(true); \
 *  jbyte x = *(signed_char*)p; \
 *  t->set_doing_unsafe_access(false); \
 */

#define DEFINE_GETSETNATIVE(jbyte, Byte, signed_char) \
 \
UNSAFE_ENTRY(jbyte, Unsafe_GetNative##Byte(JNIEnv *env, jobject unsafe, jlong addr)) \
    /* mlam UnsafeWrapper("Unsafe_GetNative"#Byte);	*/	\
    void* p = addr_from_java(addr); \
    jbyte x = *(signed_char*)p; \
    return x; \
UNSAFE_END \
 \
UNSAFE_ENTRY(void, Unsafe_SetNative##Byte(JNIEnv *env, jobject unsafe, jlong addr, jbyte x)) \
    /* mlam UnsafeWrapper("Unsafe_SetNative"#Byte); */	\
    void* p = addr_from_java(addr); \
    *(signed_char*)p = x; \
UNSAFE_END \
 \
/* END DEFINE_GETSETNATIVE. */

DEFINE_GETSETNATIVE(jbyte, Byte, signed char)
DEFINE_GETSETNATIVE(jshort, Short, signed short);
DEFINE_GETSETNATIVE(jchar, Char, unsigned short);
DEFINE_GETSETNATIVE(jint, Int, jint);
DEFINE_GETSETNATIVE(jlong, Long, jlong);
DEFINE_GETSETNATIVE(jfloat, Float, float);
DEFINE_GETSETNATIVE(jdouble, Double, double);

#undef DEFINE_GETSETNATIVE


UNSAFE_ENTRY(jlong, Unsafe_GetNativeAddress(JNIEnv *env, jobject unsafe, jlong addr))
/* mlam UnsafeWrapper("Unsafe_GetNativeAddress"); */
    void* p = addr_from_java(addr);
    return addr_to_java(*(void**)p);
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_SetNativeAddress(JNIEnv *env, jobject unsafe, jlong addr, jlong x))
/* mlam    UnsafeWrapper("Unsafe_SetNativeAddress"); */
    void* p = addr_from_java(addr);
    *(void**)p = addr_from_java(x);
UNSAFE_END


/* 
 * Allocation requests 
 */

UNSAFE_ENTRY(jobject, Unsafe_AllocateInstance(JNIEnv *env, jobject unsafe, jclass cls))
/* mlam    UnsafeWrapper("Unsafe_AllocateInstance"); */
    return CVMjniAllocObject(env, cls);
UNSAFE_END

UNSAFE_ENTRY(jlong, Unsafe_AllocateMemory(JNIEnv *env, jobject unsafe, jlong size))
/* mlam    UnsafeWrapper("Unsafe_AllocateMemory"); */
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env); 
    size_t sz = (size_t)size;
    void* x;
    if (sz != size || size < 0) 
        CVMthrowIllegalArgumentException(ee, "invalid size in Unsafe_AllocateMemory");
    if (sz == 0)  return 0;
    x = malloc(sz);
    if (x == NULL)  
        CVMthrowOutOfMemoryError(ee, "Unsafe_AllocateMemory");
    return addr_to_java(x);
UNSAFE_END

UNSAFE_ENTRY(jlong, Unsafe_ReallocateMemory(JNIEnv *env, jobject unsafe, jlong addr, jlong size))
/* mlam    UnsafeWrapper("Unsafe_ReallocateMemory"); */
    CVMExecEnv*     ee = CVMjniEnv2ExecEnv(env); 
    void* p = addr_from_java(addr);
    size_t sz = (size_t)size;
    void *x;
    if (sz != size || size < 0)  
      CVMthrowIllegalArgumentException(ee, "invalid size in Unsafe_ReallocateMemory");
    if (sz == 0) { 
        free(p); return 0; 
    }
    x = (p == NULL) ? malloc(sz) : realloc(p, sz);
    if (x == NULL) 
        CVMthrowOutOfMemoryError(ee, "Unsafe_AllocateMemory");
    return addr_to_java(x);
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_FreeMemory(JNIEnv *env, jobject unsafe, jlong addr))
/* mlam    UnsafeWrapper("Unsafe_FreeMemory"); */
    void* p = addr_from_java(addr);
    if (p == NULL) return;
    free(p);
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_SetMemory(JNIEnv *env, jobject unsafe, jlong addr, jlong size, jbyte value))
/* mlam    UnsafeWrapper("Unsafe_SetMemory"); */
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env); 
    size_t sz = (size_t)size;
    char* p;
    if (sz != size || size < 0) 
        CVMthrowIllegalArgumentException(ee, 
                                         "invalid size in Unsafe_SetMemory");
    p = (char*) addr_from_java(addr);
    memset((void *)p, value, sz);
UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_CopyMemory(JNIEnv *env, jobject unsafe, jlong srcAddr, jlong dstAddr, jlong size))
/* mlam    UnsafeWrapper("Unsafe_CopyMemory"); */
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env); 
    size_t sz;
    void* src;
    void* dst;
    if (size == 0) {
      return;
    }
    sz = (size_t)size;
    if (sz != size || size < 0) {
        CVMthrowIllegalArgumentException(ee, "invalid size in Unsafe_CopyMemory");
    }
    src = addr_from_java(srcAddr);
    dst = addr_from_java(dstAddr);
    memmove(dst, src, sz);
UNSAFE_END


/* 
 * Random queries 
 */

/* 
 * See comment at file start about UNSAFE_LEAF 
 */
UNSAFE_ENTRY(jint, Unsafe_AddressSize(JNIEnv *env, jobject unsafe))
/* mlam    UnsafeWrapper("Unsafe_AddressSize"); */
    return sizeof(void*);
UNSAFE_END

UNSAFE_ENTRY(jint, Unsafe_PageSize(JNIEnv *env, jobject unsafe))
/* mlam    UnsafeWrapper("Unsafe_PageSize"); */
    /* mlam return CVMgetPagesize(); */
    return CVMmemPageSize();
UNSAFE_END

jint find_field_offset(CVMExecEnv* ee, jobject field, int must_be_static) {
    CVMFieldBlock *fb;
    int modifiers;

    if (field == NULL) CVMthrowNullPointerException(ee, NULL);

    fb = CVMreflectGCSafeGetFieldBlock(ee, field);
    modifiers = CVMfbAccessFlags(fb);

    if (must_be_static >= 0) {
        int really_is_static = ((modifiers & JVM_ACC_STATIC) != 0);
        if (must_be_static != really_is_static) {
            CVMthrowIllegalArgumentException(ee, "field must be static");
        }
    }

    return field_offset_from_byte_offset(CVMfbOffset(fb));
}

UNSAFE_ENTRY(jlong, Unsafe_ObjectFieldOffset(JNIEnv *env, jobject unsafe, jobject field))
/* mlam    UnsafeWrapper("Unsafe_ObjectFieldOffset"); */
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env); 
    return find_field_offset(ee, field, 0);
UNSAFE_END

UNSAFE_ENTRY(jlong, Unsafe_StaticFieldOffset(JNIEnv *env, jobject unsafe, jobject field))
/* mlam    UnsafeWrapper("Unsafe_StaticFieldOffset"); */
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env); 
    return find_field_offset(ee, field, 1);
UNSAFE_END

UNSAFE_ENTRY(jobject, Unsafe_StaticFieldBaseFromField(JNIEnv *env, jobject unsafe, jobject field))

/* mlam    UnsafeWrapper("Unsafe_StaticFieldBase"); */
    CVMFieldBlock *fb;
    int modifiers;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env); 

    /* 
     * Note:  In this VM implementation, a field address is always a short 
     * offset from the base of a a klass metaobject.  Thus, the full dynamic 
     * range of the return type is never used.  However, some implementations 
     * might put the static field inside an array shared by many classes, 
     * or even at a fixed address, in which case the address could be quite 
     * large.  In that last case, this function would return NULL, since 
     * the address would operate alone, without any base pointer. 
     */
  
    if (field == NULL) CVMthrowNullPointerException(ee, NULL);
    
    fb = CVMreflectGCSafeGetFieldBlock(ee, field);
    modifiers = CVMfbAccessFlags(fb);
  
    if ((modifiers & JVM_ACC_STATIC) == 0) {
        CVMthrowIllegalArgumentException(ee, "field must be static");
    } 

    /* [RGV] should we be returning ClassBlock or JavaInstance? */
    return CVMjniNewLocalRef(env, CVMcbJavaInstance(CVMfbClassBlock(fb)));

UNSAFE_END

UNSAFE_ENTRY(void, Unsafe_EnsureClassInitialized(JNIEnv *env, jobject unsafe, jobject clazz))
/* mlam    UnsafeWrapper("Unsafe_EnsureClassInitialized"); */
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env); 
    CVMClassBlock* cb;

    if (clazz == NULL) 
        CVMthrowNullPointerException(ee, NULL);

    cb = CVMjniGcSafeRef2Class(ee, clazz);
    CVMreflectEnsureInitialized(ee, cb);
UNSAFE_END

static void getBaseAndScale(CVMExecEnv *ee, int *base, int *scale, jclass acls) {
    CVMClassBlock *cb;

    if ( acls == NULL || CVMID_icellIsNull(acls) ) 
        CVMthrowNullPointerException(ee, NULL);

    cb = CVMjniGcSafeRef2Class(ee, acls);

    if (!CVMisArrayClass(cb)) {
        CVMthrowInvalidClassException(ee, "%C", cb); 
        return;
    }

    *scale = CVMarrayElemSize(cb);

    /* [RGV] May need to handle Object Arrays differently 
     * Not sure that this is even needed.
     */
    *base = CVMoffsetof(CVMArrayOfAnyType, elems);
}

UNSAFE_ENTRY(jint, Unsafe_ArrayBaseOffset(JNIEnv *env, jobject unsafe, jclass acls))
/* mlam    UnsafeWrapper("Unsafe_ArrayBaseOffset"); */
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env); 
    int base, scale;
    getBaseAndScale(ee, &base, &scale, acls);
    return field_offset_from_byte_offset(base);
UNSAFE_END


UNSAFE_ENTRY(jint, Unsafe_ArrayIndexScale(JNIEnv *env, jobject unsafe, jclass acls))
/* mlam    UnsafeWrapper("Unsafe_ArrayIndexScale"); */
    int base, scale;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env); 
    getBaseAndScale(ee, &base, &scale, acls);
    /* HOTSPOT IMPLEMENTATION SPECIFIC COMMENT   
     * This VM packs both fields and array elements down to the byte. 
     * But watch out:  If this changes, so that array references for 
     * a given primitive type (say, T_BOOLEAN) use different memory units 
     * than fields, this method MUST return zero for such arrays. 
     * For example, the VM used to store sub-word sized fields in full 
     * words in the object layout, so that accessors like getByte(Object,int) 
     * did not really do what one might expect for arrays.  Therefore, 
     * this function used to report a zero scale factor, so that the user 
     * would know not to attempt to access sub-word array elements. 
     *  * Code for unpacked fields: 
     * if (scale < wordSize)  return 0; 
     * The following allows for a pretty general fieldOffset cookie scheme, 
     * but requires it to be linear in byte offset. 
     */
    return field_offset_from_byte_offset(scale) - field_offset_from_byte_offset(0);
UNSAFE_END


static void throw_new(JNIEnv *env, const char *ename) {
    char buf[100];
    jclass cls;
    char* msg;
    strcpy(buf, "java/lang/");
    strcat(buf, ename);
    cls = CVMjniFindClass(env, buf);
    msg = NULL;
    CVMjniThrowNew(env, cls, msg);
}

UNSAFE_ENTRY(jclass, Unsafe_DefineClass0(JNIEnv *env, jobject unsafe, jstring name, jbyteArray data, int offset, int length))
/* mlam  UnsafeWrapper("Unsafe_DefineClass"); */

    jclass classRoot;

    int depthFromDefineClass0 = 1;
    jclass  caller = JVM_GetCallerClass(env, depthFromDefineClass0);
    jobject loader = (caller == NULL) ? NULL : JVM_GetClassLoader(env, caller);
    jobject pd     = (caller == NULL) ? NULL : JVM_GetProtectionDomain(env, caller);
    classRoot = Java_java_lang_ClassLoader_defineClass0(env, loader, 
                                                        name, data, 
                                                        offset, length, pd);
    /* make sure superclasses get loaded */
    (*env)->CallVoidMethod(env, classRoot,
                           CVMglobals.java_lang_Class_loadSuperClasses);
    return classRoot;
UNSAFE_END


UNSAFE_ENTRY(jclass, Unsafe_DefineClass1(JNIEnv *env, jobject unsafe, jstring name, jbyteArray data, int offset, int length, jobject loader, jobject pd))
/* mlam    UnsafeWrapper("Unsafe_DefineClass"); */
    jclass classRoot;
    classRoot = Java_java_lang_ClassLoader_defineClass0(env, loader, 
                                                        name, data, 
                                                        offset, length, pd);
    /* make sure superclasses get loaded */
    (*env)->CallVoidMethod(env, classRoot,
                             CVMglobals.java_lang_Class_loadSuperClasses);
    return classRoot;
UNSAFE_END


UNSAFE_ENTRY(void, Unsafe_MonitorEnter(JNIEnv *env, jobject unsafe, jobject obj))
/* mlam    UnsafeWrapper("Unsafe_MonitorEnter"); */
    CVMjniMonitorEnter(env, obj);
UNSAFE_END


UNSAFE_ENTRY(void, Unsafe_MonitorExit(JNIEnv *env, jobject unsafe, jobject obj))
/* mlam    UnsafeWrapper("Unsafe_MonitorExit"); */
    CVMjniMonitorExit(env,obj);
UNSAFE_END


UNSAFE_ENTRY(void, Unsafe_ThrowException(JNIEnv *env, jobject unsafe, jthrowable thr))
/* mlam    UnsafeWrapper("Unsafe_ThrowException"); */
    CVMjniThrow(env, thr);
UNSAFE_END


/* 
 * JVM_RegisterUnsafeMethod 
 */

#define ADR "J"

#define LANG "Ljava/lang/"

#define OBJ LANG"Object;"
#define CLS LANG"Class;"
#define CTR LANG"reflect/Constructor;"
#define FLD LANG"reflect/Field;"
#define MTH LANG"reflect/Method;"
#define THR LANG"Throwable;"

#define DC0_Args LANG"String;[BII"
#define DC1_Args DC0_Args LANG"ClassLoader;" "Ljava/security/ProtectionDomain;"

#define CC (char*)  /*cast a literal from (const char*)*/
#define FN_PTR(f) CAST_FROM_FN_PTR(void*, &f)

/* Note:  getObject and kin take both int and long offsets. */
#define DECLARE_GETSETOOP(Boolean, Z) \
    {CC"get"#Boolean,      CC"("OBJ"J)"#Z,      FN_PTR(Unsafe_Get##Boolean)}, \
    {CC"put"#Boolean,      CC"("OBJ"J"#Z")V",   FN_PTR(Unsafe_Set##Boolean)}

#define DECLARE_GETSETNATIVE(Byte, B) \
    {CC"get"#Byte,         CC"("ADR")"#B,       FN_PTR(Unsafe_GetNative##Byte)}, \
    {CC"put"#Byte,         CC"("ADR#B")V",      FN_PTR(Unsafe_SetNative##Byte)}


static JNINativeMethod methods[] = {

    {CC"getObject",        CC"("OBJ"J)"OBJ"",   FN_PTR(Unsafe_GetObject)},
    {CC"putObject",        CC"("OBJ"J"OBJ")V",  FN_PTR(Unsafe_SetObject)},

    DECLARE_GETSETOOP(Boolean, Z),
    DECLARE_GETSETOOP(Byte, B),
    DECLARE_GETSETOOP(Short, S),
    DECLARE_GETSETOOP(Char, C),
    DECLARE_GETSETOOP(Int, I),
    DECLARE_GETSETOOP(Long, J),
    DECLARE_GETSETOOP(Float, F),
    DECLARE_GETSETOOP(Double, D),

    DECLARE_GETSETNATIVE(Byte, B),
    DECLARE_GETSETNATIVE(Short, S),
    DECLARE_GETSETNATIVE(Char, C),
    DECLARE_GETSETNATIVE(Int, I),
    DECLARE_GETSETNATIVE(Long, J),
    DECLARE_GETSETNATIVE(Float, F),
    DECLARE_GETSETNATIVE(Double, D),

    {CC"getAddress",         CC"("ADR")"ADR,             FN_PTR(Unsafe_GetNativeAddress)},
    {CC"putAddress",         CC"("ADR""ADR")V",          FN_PTR(Unsafe_SetNativeAddress)},

    {CC"allocateMemory",     CC"(J)"ADR,                 FN_PTR(Unsafe_AllocateMemory)},
    {CC"reallocateMemory",   CC"("ADR"J)"ADR,            FN_PTR(Unsafe_ReallocateMemory)},
    {CC"setMemory",          CC"("ADR"JB)V",             FN_PTR(Unsafe_SetMemory)},
    {CC"copyMemory",         CC"("ADR ADR"J)V",          FN_PTR(Unsafe_CopyMemory)},
    {CC"freeMemory",         CC"("ADR")V",               FN_PTR(Unsafe_FreeMemory)},

    {CC"objectFieldOffset",  CC"("FLD")J",               FN_PTR(Unsafe_ObjectFieldOffset)},
    {CC"staticFieldOffset",  CC"("FLD")J",               FN_PTR(Unsafe_StaticFieldOffset)},
    {CC"staticFieldBase",    CC"("FLD")"OBJ,             FN_PTR(Unsafe_StaticFieldBaseFromField)},
    {CC"ensureClassInitialized",CC"("CLS")V",            FN_PTR(Unsafe_EnsureClassInitialized)},
    {CC"arrayBaseOffset",    CC"("CLS")I",               FN_PTR(Unsafe_ArrayBaseOffset)},
    {CC"arrayIndexScale",    CC"("CLS")I",               FN_PTR(Unsafe_ArrayIndexScale)},
    {CC"addressSize",        CC"()I",                    FN_PTR(Unsafe_AddressSize)},
    {CC"pageSize",           CC"()I",                    FN_PTR(Unsafe_PageSize)},

    {CC"defineClass",        CC"("DC0_Args")"CLS,        FN_PTR(Unsafe_DefineClass0)},
    {CC"defineClass",        CC"("DC1_Args")"CLS,        FN_PTR(Unsafe_DefineClass1)},
    {CC"allocateInstance",   CC"("CLS")"OBJ,             FN_PTR(Unsafe_AllocateInstance)},
    {CC"monitorEnter",       CC"("OBJ")V",               FN_PTR(Unsafe_MonitorEnter)},
    {CC"monitorExit",        CC"("OBJ")V",               FN_PTR(Unsafe_MonitorExit)},
    {CC"throwException",     CC"("THR")V",               FN_PTR(Unsafe_ThrowException)}
};

#undef CC
#undef FN_PTR

#undef ADR
#undef LANG
#undef OBJ
#undef CLS
#undef CTR
#undef FLD
#undef MTH
#undef THR
#undef DC0_Args
#undef DC1_Args

#undef DECLARE_GETSETOOP
#undef DECLARE_GETSETNATIVE


/* 
 * This one function is exported, used by NativeLookup. 
 */

JNIEXPORT void JNICALL
Java_sun_misc_Unsafe_registerNatives(JNIEnv *env, jclass unsafecls) 
{
    /* mlam    UnsafeWrapper("JVM_RegisterUnsafeMethods"); */

    jboolean ok = CVMjniRegisterNatives(env, unsafecls, methods, sizeof(methods)/sizeof(JNINativeMethod));
    CVMassert(ok == 0);
    (void)ok;
}

/*
 * TODO - Need JIT intriniscs for the compareAndSwap methods.
 */

JNIEXPORT jboolean JNICALL
Java_sun_misc_Unsafe_compareAndSwapInt(
    JNIEnv* env, jobject this,
    jobject o, jlong offset, jint expected, jint x)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jint previous;
    
    CVMD_gcUnsafeExec(ee, {
	CVMObject* directObj = CVMID_icellDirect(ee, o);
	CVMJavaLong addr_long = CVMlongAdd(CVMvoidPtr2Long(directObj),
					   offset * sizeof(CVMAddr));
	CVMAddr* addr = CVMlong2VoidPtr(addr_long);
	previous = CVMatomicCompareAndSwap(addr, x, expected);
    });

    return previous == expected;
}

JNIEXPORT jboolean JNICALL
Java_sun_misc_Unsafe_compareAndSwapLong(
    JNIEnv* env, jobject this,
    jobject o, jlong offset, jlong expected, jlong x)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jlong previous;
    jboolean result;
    
    CVMD_gcUnsafeExec(ee, {
	CVMObject* directObj = CVMID_icellDirect(ee, o);
	CVMJavaLong addr_long = CVMlongAdd(CVMvoidPtr2Long(directObj),
					   offset * sizeof(CVMAddr));
	CVMAddr* addr = CVMlong2VoidPtr(addr_long);
	CVM_ACCESS_VOLATILE_LOCK(ee);
	previous = CVMvoidPtr2Long(addr);
	if (CVMlongEq(previous, expected)) {
	    CVMlong2Jvm(addr, x);
	    result = JNI_TRUE;
	} else {
	    result = JNI_FALSE;
	}
	CVM_ACCESS_VOLATILE_UNLOCK(ee);
    });

    return result;
}

JNIEXPORT jboolean JNICALL
Java_sun_misc_Unsafe_compareAndSwapObject(
    JNIEnv* env, jobject this,
    jobject o, jlong offset, jobject expected, jobject x)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject previous;
    
    CVMD_gcUnsafeExec(ee, {
	CVMObject* directObj = CVMID_icellDirect(ee, o);
	CVMObject* expectedObj = 
	    (expectedObj == NULL ? NULL : CVMID_icellDirect(ee, expected));
	CVMObject* xObj = 
	    (x == NULL ? NULL : CVMID_icellDirect(ee, x));
	CVMJavaLong addr_long = CVMlongAdd(CVMvoidPtr2Long(directObj),
					   offset * sizeof(CVMAddr));
	CVMAddr* addr = CVMlong2VoidPtr(addr_long);
	previous = (jobject)
	    CVMatomicCompareAndSwap(addr, (CVMAddr)xObj, (CVMAddr)expectedObj);
    });

    return previous == expected;
}
