/*
 * @(#)DMFieldRWRefTest.c	1.9 06/10/10
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
#include "generated/offsets/java_lang_String.h"
#include <stdio.h>
#include "DMFieldRWRefTest.h"

JNIEXPORT jobjectArray JNICALL 
Java_DMFieldRWRefTest_nGetStrings(JNIEnv *env, jobject obj)
{
   jint i, j, count = 0;
   jint numFields = 0;
   jobject lock;
   jobjectArray objArray;
   jfieldID iFid, oFid;
   jclass clazz, gcClazz, testClazz;
   CVMClassBlock *cb;
   CVMExecEnv *ee;

   ee = CVMjniEnv2ExecEnv(env);

   gcClazz = (*env)->FindClass(env, "GcThread");
   iFid = (*env)->GetStaticFieldID(env, gcClazz, "gcCalled", "I");

   testClazz = (*env)->GetObjectClass(env, obj);
   oFid = (*env)->GetStaticFieldID(env, testClazz, "lock", "Ljava/lang/Object;");
   lock = (*env)->GetStaticObjectField(env, testClazz, oFid);

   cb = CVMgcSafeClassRef2ClassBlock(ee, testClazz);

   CVMD_gcUnsafeExec( ee, {
      CVMFieldBlock* fb;
      for(i=0; i< CVMcbFieldCount(cb); i++) {
         fb = CVMcbFieldSlot(cb, i);         
         if(!CVMfbIs(fb, STATIC))
            ++numFields;
      }
   });

   clazz = (*env)->FindClass(env, "java/lang/String");
   objArray = (*env)->NewObjectArray(env, numFields, clazz, NULL);

   CVMD_gcUnsafeExec( ee, {
      CVMFieldBlock* fb;
      int objOffset;
      CVMObject *directObject = CVMID_icellDirect(ee, obj);
      CVMArrayOfRef* refArray = (CVMArrayOfRef *)CVMID_icellDirect(ee, objArray);
      CVMObject *stringDirect;

      CVMfbStaticField(ee, iFid).i = -1;

      CVMobjectLock(ee, lock);
      CVMobjectNotify(ee, lock); 
      CVMobjectUnlock(ee, lock);

      for(i = 0; i < 10000000; i++)
         ;

      for(i=0, j=0; i< CVMcbFieldCount(cb); i++) {
         fb = CVMcbFieldSlot(cb, i);         

         if(!CVMfbIs(fb, STATIC)) {
            objOffset = CVMfbOffset(fb);
            CVMD_fieldReadRef(directObject, objOffset, stringDirect);

            if(ee->barrier == R_BARRIER_REF)
               count++;

            CVMD_arrayWriteRef(refArray, i, stringDirect);
            j++;
         }
      }

      if(count == numFields)
         printf("PASS: DMFieldRWRefTest:Read, Read Barrier Ref was invoked.\n");
      else
         printf("FAIL: DMFieldRWRefTest:Read, Read Barrier Ref was not invoked.\n");

      if(CVMfbStaticField(ee, iFid).i == -1)
        printf("PASS: DMFieldRWRefTest:Read, Gc did not happen in the gc unsafe section\n");
      else
        printf("FAIL: DMFieldRWRefTest:Read, Gc happened in the gc unsafe section\n");

   });

   return objArray;
}


JNIEXPORT void JNICALL
Java_DMFieldRWRefTest_nSetStrings(JNIEnv *env, jobject obj, 
                           jstring str1, jstring str2, jstring str3)
{
   jint i, count = 0;
   jfieldID iFid, oFid;
   jclass gcClazz, testClazz;
   jobject lock;
   CVMClassBlock *cb;
   CVMExecEnv *ee;

   ee = CVMjniEnv2ExecEnv(env);

   gcClazz = (*env)->FindClass(env, "GcThread");
   iFid = (*env)->GetStaticFieldID(env, gcClazz, "gcCalled", "I");

   testClazz = (*env)->GetObjectClass(env, obj);
   oFid = (*env)->GetStaticFieldID(env, testClazz, "lock", "Ljava/lang/Object;");
   lock = (*env)->GetStaticObjectField(env, testClazz, oFid);

   cb = CVMgcSafeClassRef2ClassBlock(ee, testClazz);

   CVMD_gcUnsafeExec( ee, {
      CVMFieldBlock* fb;
      int objOffset[CVMcbFieldCount(cb)];
      int numFields = 0;
      CVMObject *directObject = CVMID_icellDirect(ee, obj);

      CVMObject *str1StringDirect = CVMID_icellDirect(ee, str1);
      CVMObject *str2StringDirect = CVMID_icellDirect(ee, str2);
      CVMObject *str3StringDirect = CVMID_icellDirect(ee, str3);

      CVMfbStaticField(ee, iFid).i = -1;

      CVMobjectLock(ee, lock);
      CVMobjectNotify(ee, lock); 
      CVMobjectUnlock(ee, lock);

      for(i = 0; i < 10000000; i++)
         ;

      for(i=0; i< CVMcbFieldCount(cb); i++) {
         fb = CVMcbFieldSlot(cb, i);         
         if(!CVMfbIs(fb, STATIC))
            objOffset[numFields++] = CVMfbOffset(fb);
      }

      CVMD_fieldWriteRef(directObject, objOffset[0], str1StringDirect);
      if(ee->barrier == W_BARRIER_REF)
         count++;
      CVMD_fieldWriteRef(directObject, objOffset[1], str2StringDirect);
      if(ee->barrier == W_BARRIER_REF)
         count++;
      CVMD_fieldWriteRef(directObject, objOffset[2], str3StringDirect);
      if(ee->barrier == W_BARRIER_REF)
         count++;

      if(count == numFields)
         printf("PASS: DMFieldRWRefTest:Write, Write Barrier Ref\n");
      else
         printf("FAIL: DMFieldRWRefTest:Write, Write Barrier Ref\n");

      if(CVMfbStaticField(ee, iFid).i == -1)
        printf("PASS: DMFieldRWRefTest:Write, Gc did not happen in the gc unsafe section\n");
      else
        printf("FAIL: DMFieldRWRefTest:Write, Gc happened in the gc unsafe section\n");
   } );
}
