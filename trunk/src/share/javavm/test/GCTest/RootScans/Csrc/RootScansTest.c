/*
 * @(#)RootScansTest.c	1.8 06/10/10
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

#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "native/common/jni_util.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/clib.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/directmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/localroots.h"
#include "javavm/include/preloader.h"
#include "javavm/include/common_exceptions.h"
#include "generated/offsets/java_lang_Class.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/offsets/java_lang_Throwable.h"
#include "generated/jni/java_lang_reflect_Modifier.h"
#include <stdio.h>
#include "RootScansTest.h"

JNIEXPORT void JNICALL 
Java_RootScansTest_scanRootsTest1(JNIEnv *env, jobject obj)
{
   jint i, failCount = 0;
   jmethodID mid;
   CVMObject *beforeGC[100], *afterGC[100];
   jclass clazz, iclazz, fclazz, sclazz, lclazz;
   jclass dclazz, bclazz, shclazz, blclazz;
   jobject iobj, fobj, sobj, lobj;
   jobject dobj, bobj, shobj, blobj;
   jobject ilref, flref, slref, llref;
   jobject dlref, blref, shlref, bllref;
   jobject igref, fgref, sgref, lgref;
   jobject dgref, bgref, shgref, blgref;
   CVMObjectICell *objICell, *boolICell, *exceptionICell;
   CVMObjectICell *threadICell, *intArrayICell, *floatArrayICell;
   CVMExecEnv *ee;

   ee = CVMjniEnv2ExecEnv(env);

   clazz = (*env)->FindClass(env, "java/lang/System");
   mid = (*env)->GetStaticMethodID(env, clazz, "gc", "()V");

   iclazz = (*env)->FindClass(env, "java/lang/Integer");
   fclazz = (*env)->FindClass(env, "java/lang/Float");
   sclazz = (*env)->FindClass(env, "java/lang/String");
   lclazz = (*env)->FindClass(env, "java/lang/Long");
   dclazz = (*env)->FindClass(env, "java/lang/Double");
   bclazz = (*env)->FindClass(env, "java/lang/Byte");
   shclazz = (*env)->FindClass(env, "java/lang/Short");
   blclazz = (*env)->FindClass(env, "java/lang/Boolean");

   iobj = (*env)->AllocObject(env, iclazz);
   fobj = (*env)->AllocObject(env, fclazz);
   sobj = (*env)->AllocObject(env, sclazz);
   lobj = (*env)->AllocObject(env, lclazz);
   dobj = (*env)->AllocObject(env, dclazz);
   bobj = (*env)->AllocObject(env, bclazz);
   shobj = (*env)->AllocObject(env, shclazz);
   blobj = (*env)->AllocObject(env, blclazz);

   /* Create JNI Local and Global Refs */
   ilref = (*env)->NewLocalRef(env, iobj);
   flref = (*env)->NewLocalRef(env, fobj);
   slref = (*env)->NewLocalRef(env, sobj);
   llref = (*env)->NewLocalRef(env, lobj);
   dlref = (*env)->NewLocalRef(env, dobj);
   blref = (*env)->NewLocalRef(env, bobj);
   shlref = (*env)->NewLocalRef(env, shobj);
   bllref = (*env)->NewLocalRef(env, blobj);

   igref = (*env)->NewGlobalRef(env, iobj);
   fgref = (*env)->NewGlobalRef(env, fobj);
   sgref = (*env)->NewGlobalRef(env, sobj);
   lgref = (*env)->NewGlobalRef(env, lobj);
   dgref = (*env)->NewGlobalRef(env, dobj);
   bgref = (*env)->NewGlobalRef(env, bobj);
   shgref = (*env)->NewGlobalRef(env, shobj);
   blgref = (*env)->NewGlobalRef(env, blobj);

   /* Create Global CVM roots */
   objICell = CVMID_getGlobalRoot(ee);
   boolICell = CVMID_getGlobalRoot(ee);
   exceptionICell = CVMID_getGlobalRoot(ee);
   threadICell = CVMID_getGlobalRoot(ee);
   intArrayICell = CVMID_getGlobalRoot(ee);
   floatArrayICell = CVMID_getGlobalRoot(ee);

   CVMID_allocNewInstance(ee, 
         (CVMClassBlock*)CVMsystemClass(java_lang_Object), objICell);

   CVMID_allocNewInstance(ee, 
         (CVMClassBlock*)CVMsystemClass(java_lang_Boolean), boolICell);

   CVMID_allocNewInstance(ee, 
         (CVMClassBlock*)CVMsystemClass(java_lang_Exception), exceptionICell);

   CVMID_allocNewInstance(ee, 
         (CVMClassBlock*)CVMsystemClass(java_lang_Thread), threadICell);

   CVMID_allocNewArray(ee, CVM_T_INT,
            (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_INT],
            10, (CVMObjectICell*)intArrayICell);

   CVMID_allocNewArray(ee, CVM_T_FLOAT,
            (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_FLOAT],
            10, (CVMObjectICell*)floatArrayICell);

   CVMID_localrootBegin(ee);{
      CVMID_localrootDeclare(CVMObjectICell, lObjICell);
      CVMID_localrootDeclare(CVMThrowableICell, lExceptionICell);
      CVMID_localrootDeclare(CVMStringICell, stringICell);
      CVMID_localrootDeclare(CVMArrayOfCharICell, charArrayICell);
      CVMID_localrootDeclare(CVMArrayOfRefICell, refArrayICell);

      CVMID_allocNewInstance(ee, 
            (CVMClassBlock*)CVMsystemClass(java_lang_Object), lObjICell);

      CVMID_allocNewInstance(ee, 
            (CVMClassBlock*)CVMsystemClass(java_lang_Exception), lExceptionICell);

      CVMID_allocNewInstance(ee, 
            (CVMClassBlock*)CVMsystemClass(java_lang_String), stringICell);

      CVMID_allocNewArray(ee, CVM_T_CHAR, 
            (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_CHAR],
            10, (CVMObjectICell*)charArrayICell);

      CVMID_allocNewArray(ee, CVM_T_CLASS,
            (CVMClassBlock*) CVMbasicTypeArrayClassblocks[CVM_T_CLASS],
            1, (CVMObjectICell*)refArrayICell);

      CVMD_gcUnsafeExec( ee, {
          beforeGC[0] = CVMID_icellDirect(ee, ilref);
          beforeGC[1] = CVMID_icellDirect(ee, flref);
          beforeGC[2] = CVMID_icellDirect(ee, slref);
          beforeGC[3] = CVMID_icellDirect(ee, llref);
          beforeGC[4] = CVMID_icellDirect(ee, dlref);
          beforeGC[5] = CVMID_icellDirect(ee, blref);
          beforeGC[6] = CVMID_icellDirect(ee, shlref);
          beforeGC[7] = CVMID_icellDirect(ee, bllref);
          beforeGC[8] = CVMID_icellDirect(ee, igref);
          beforeGC[9] = CVMID_icellDirect(ee, fgref);
          beforeGC[10] = CVMID_icellDirect(ee, sgref);
          beforeGC[11] = CVMID_icellDirect(ee, lgref);
          beforeGC[12] = CVMID_icellDirect(ee, dgref);
          beforeGC[13] = CVMID_icellDirect(ee, bgref);
          beforeGC[14] = CVMID_icellDirect(ee, shgref);
          beforeGC[15] = CVMID_icellDirect(ee, blgref);
          beforeGC[16] = CVMID_icellDirect(ee, objICell);
          beforeGC[17] = CVMID_icellDirect(ee, boolICell);
          beforeGC[18] = CVMID_icellDirect(ee, exceptionICell);
          beforeGC[19] = CVMID_icellDirect(ee, threadICell);
          beforeGC[20] = CVMID_icellDirect(ee, intArrayICell);
          beforeGC[21] = CVMID_icellDirect(ee, floatArrayICell);
          beforeGC[22] = CVMID_icellDirect(ee, lObjICell);
          beforeGC[23] = CVMID_icellDirect(ee, lExceptionICell);
          beforeGC[24] = CVMID_icellDirect(ee, stringICell);
          beforeGC[25] = (CVMObject *)CVMID_icellDirect(ee, charArrayICell);
          beforeGC[26] = (CVMObject *)CVMID_icellDirect(ee, refArrayICell);
      });
      
      (*env)->CallStaticVoidMethod(env, clazz, mid);

      CVMD_gcUnsafeExec( ee, {
          afterGC[0] = CVMID_icellDirect(ee, ilref);
          afterGC[1] = CVMID_icellDirect(ee, flref);
          afterGC[2] = CVMID_icellDirect(ee, slref);
          afterGC[3] = CVMID_icellDirect(ee, llref);
          afterGC[4] = CVMID_icellDirect(ee, dlref);
          afterGC[5] = CVMID_icellDirect(ee, blref);
          afterGC[6] = CVMID_icellDirect(ee, shlref);
          afterGC[7] = CVMID_icellDirect(ee, bllref);
          afterGC[8] = CVMID_icellDirect(ee, igref);
          afterGC[9] = CVMID_icellDirect(ee, fgref);
          afterGC[10] = CVMID_icellDirect(ee, sgref);
          afterGC[11] = CVMID_icellDirect(ee, lgref);
          afterGC[12] = CVMID_icellDirect(ee, dgref);
          afterGC[13] = CVMID_icellDirect(ee, bgref);
          afterGC[14] = CVMID_icellDirect(ee, shgref);
          afterGC[15] = CVMID_icellDirect(ee, blgref);
          afterGC[16] = CVMID_icellDirect(ee, objICell);
          afterGC[17] = CVMID_icellDirect(ee, boolICell);
          afterGC[18] = CVMID_icellDirect(ee, exceptionICell);
          afterGC[19] = CVMID_icellDirect(ee, threadICell);
          afterGC[20] = CVMID_icellDirect(ee, intArrayICell);
          afterGC[21] = CVMID_icellDirect(ee, floatArrayICell);
          afterGC[22] = CVMID_icellDirect(ee, lObjICell);
          afterGC[23] = CVMID_icellDirect(ee, lExceptionICell);
          afterGC[24] = CVMID_icellDirect(ee, stringICell);
          afterGC[25] = (CVMObject *) CVMID_icellDirect(ee, charArrayICell);
          afterGC[26] = (CVMObject *) CVMID_icellDirect(ee, refArrayICell);

          for(i=0; i < 27; i++) {
             if(beforeGC[i] == afterGC[i])
                ++failCount;
          }
      });

   }CVMID_localrootEnd();

   if(failCount)
      printf("FAIL: RootScansTest1, All roots were not scanned\n");
   else
      printf("PASS: RootScansTest1, All roots were scanned\n");
}


JNIEXPORT void JNICALL
Java_RootScansTest_scanRootsTest2(JNIEnv *env, jobject thisObj) {
   jbyteArray byteArray;
   jshortArray shortArray;
   jintArray intArray;
   jlongArray longArray;
   jfloatArray floatArray;
   jdoubleArray doubleArray;
   jcharArray charArray;
   jbooleanArray boolArray;
   jmethodID mid1, mid2;
   static jobject savedRef;
   jclass clazz, iclazz, fclazz, sclazz, lclazz;
   jclass dclazz, bclazz, shclazz, blclazz;
   jobject iobj, fobj, sobj, lobj;
   jobject dobj, bobj, shobj, blobj;
   jobject ilref, flref, slref, llref;
   jobject dlref, blref, shlref, bllref;
   jobject igref, fgref, sgref, lgref;
   jobject dgref, bgref, shgref, blgref;
   jobject iwref, fwref, swref, lwref;
   jobject dwref, bwref, shwref, blwref;
   jobjectArray iarr, farr, sarr, larr;
   jobjectArray darr, barr, sharr, blarr;
   jobject obj;
   CVMExecEnv *ee;
   CVMObjectICell *objICell;
   CVMClassICell *classICell;
   CVMThreadICell *threadICell;

   
   #ifndef CVM_DEBUG_ASSERTS
      printf("UNRESOLVED: RootScansTest2, can be tested only when debug is turned on");
      return;
   #endif

   ee = CVMjniEnv2ExecEnv(env);

   clazz = (*env)->FindClass(env, "RootScansTest");

   mid1 = (*env)->GetMethodID(env, clazz, "<init>", "()V");
   mid2 = (*env)->GetMethodID(env, clazz, "createJavaRoots", "()V");

   obj = (*env)->NewObject(env, clazz, mid1);

   iclazz = (*env)->FindClass(env, "java/lang/Integer");
   fclazz = (*env)->FindClass(env, "java/lang/Float");
   sclazz = (*env)->FindClass(env, "java/lang/String");
   lclazz = (*env)->FindClass(env, "java/lang/Long");
   dclazz = (*env)->FindClass(env, "java/lang/Double");
   bclazz = (*env)->FindClass(env, "java/lang/Byte");
   shclazz = (*env)->FindClass(env, "java/lang/Short");
   blclazz = (*env)->FindClass(env, "java/lang/Boolean");

   /* Create JNI local refs */
   byteArray = (*env)->NewByteArray(env, 10);
   shortArray = (*env)->NewShortArray(env, 10);
   intArray = (*env)->NewIntArray(env, 10);
   longArray = (*env)->NewLongArray(env, 10);
   floatArray = (*env)->NewFloatArray(env, 10);
   doubleArray = (*env)->NewDoubleArray(env, 10);
   charArray = (*env)->NewCharArray(env, 10);
   boolArray = (*env)->NewBooleanArray(env, 10);

   iarr = (*env)->NewObjectArray(env, 3, iclazz, NULL);
   farr = (*env)->NewObjectArray(env, 3, fclazz, NULL);
   sarr = (*env)->NewObjectArray(env, 3, sclazz, NULL);
   larr = (*env)->NewObjectArray(env, 3, lclazz, NULL);
   darr = (*env)->NewObjectArray(env, 3, dclazz, NULL);
   barr = (*env)->NewObjectArray(env, 3, bclazz, NULL);
   sharr = (*env)->NewObjectArray(env, 3, shclazz, NULL);
   blarr = (*env)->NewObjectArray(env, 3, blclazz, NULL);

   iobj = (*env)->AllocObject(env, iclazz);
   fobj = (*env)->AllocObject(env, fclazz);
   sobj = (*env)->AllocObject(env, sclazz);
   lobj = (*env)->AllocObject(env, lclazz);
   dobj = (*env)->AllocObject(env, dclazz);
   bobj = (*env)->AllocObject(env, bclazz);
   shobj = (*env)->AllocObject(env, shclazz);
   blobj = (*env)->AllocObject(env, blclazz);

   ilref = (*env)->NewLocalRef(env, iobj);
   flref = (*env)->NewLocalRef(env, fobj);
   slref = (*env)->NewLocalRef(env, sobj);
   llref = (*env)->NewLocalRef(env, lobj);
   dlref = (*env)->NewLocalRef(env, dobj);
   blref = (*env)->NewLocalRef(env, bobj);
   shlref = (*env)->NewLocalRef(env, shobj);
   bllref = (*env)->NewLocalRef(env, blobj);

   /* Create JNI Global and Weak Global Refs */
   savedRef = (*env)->NewGlobalRef(env, thisObj);

   igref = (*env)->NewGlobalRef(env, iobj);
   fgref = (*env)->NewGlobalRef(env, fobj);
   sgref = (*env)->NewGlobalRef(env, sobj);
   lgref = (*env)->NewGlobalRef(env, lobj);
   dgref = (*env)->NewGlobalRef(env, dobj);
   bgref = (*env)->NewGlobalRef(env, bobj);
   shgref = (*env)->NewGlobalRef(env, shobj);
   blgref = (*env)->NewGlobalRef(env, blobj);

   iwref = (*env)->NewWeakGlobalRef(env, iobj);
   fwref = (*env)->NewWeakGlobalRef(env, fobj);
   swref = (*env)->NewWeakGlobalRef(env, sobj);
   lwref = (*env)->NewWeakGlobalRef(env, lobj);
   dwref = (*env)->NewWeakGlobalRef(env, dobj);
   bwref = (*env)->NewWeakGlobalRef(env, bobj);
   shwref = (*env)->NewWeakGlobalRef(env, shobj);
   blwref = (*env)->NewWeakGlobalRef(env, blobj);

   /* Create Global CVM roots */
   objICell = CVMID_getGlobalRoot(ee);
   threadICell = CVMID_getGlobalRoot(ee);
   classICell = CVMID_getGlobalRoot(ee);

   CVMID_allocNewInstance(ee,
         (CVMClassBlock*)CVMsystemClass(java_lang_Object), objICell);

   CVMID_allocNewInstance(ee,
         (CVMClassBlock*)CVMsystemClass(java_lang_Thread), threadICell);

   CVMID_allocNewInstance(ee,
         (CVMClassBlock*)CVMsystemClass(java_lang_Object), classICell);

   /* Create CVM local roots */
   CVMID_localrootBegin(ee);{
      CVMID_localrootDeclare(CVMObjectICell, lObjICell);
      CVMID_localrootDeclare(CVMThrowableICell, exceptionICell);
      CVMID_localrootDeclare(CVMStringICell, stringICell);
      CVMID_localrootDeclare(CVMArrayOfCharICell, charArrayICell);
      CVMID_localrootDeclare(CVMArrayOfRefICell, rootArray);

      CVMID_allocNewInstance(ee,
            (CVMClassBlock*)CVMsystemClass(java_lang_Object), lObjICell);

      CVMID_allocNewInstance(ee,
            (CVMClassBlock*)CVMsystemClass(java_lang_Exception), exceptionICell)
;

      CVMID_allocNewInstance(ee,
            (CVMClassBlock*)CVMsystemClass(java_lang_String), stringICell);

      CVMID_allocNewArray(ee, CVM_T_CHAR,
            (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_CHAR],
            10, (CVMObjectICell*)charArrayICell);

      CVMID_allocNewArray(ee, CVM_T_CLASS,
            (CVMClassBlock*) CVMbasicTypeArrayClassblocks[CVM_T_CLASS],
            1, (CVMObjectICell*)rootArray);
   
      /* Create Java Roots */
      (*env)->CallVoidMethod(env, obj, mid2);

   } CVMID_localrootEnd();

   /* Free JNI Global Roots */
   (*env)->DeleteGlobalRef(env, igref);
   (*env)->DeleteGlobalRef(env, fgref);
   (*env)->DeleteGlobalRef(env, sgref);
   (*env)->DeleteGlobalRef(env, lgref);
   (*env)->DeleteGlobalRef(env, dgref);
   (*env)->DeleteGlobalRef(env, bgref);
   (*env)->DeleteGlobalRef(env, shgref);
   (*env)->DeleteGlobalRef(env, blgref);

   (*env)->DeleteWeakGlobalRef(env, iwref);
   (*env)->DeleteWeakGlobalRef(env, fwref);
   (*env)->DeleteWeakGlobalRef(env, swref);
   (*env)->DeleteWeakGlobalRef(env, lwref);
   (*env)->DeleteWeakGlobalRef(env, dwref);
   (*env)->DeleteWeakGlobalRef(env, bwref);
   (*env)->DeleteWeakGlobalRef(env, shwref);
   (*env)->DeleteWeakGlobalRef(env, blwref);

   /* Free CVM Global Roots */
   CVMID_freeGlobalRoot(ee, objICell);
   CVMID_freeGlobalRoot(ee, threadICell);
   CVMID_freeGlobalRoot(ee, classICell);

   printf("PASS: RootScansTest2, All roots were scanned only once\n");
}

