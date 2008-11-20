/*
 * @(#)DMFieldRWLongTest.c	1.9 06/10/10
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
#include "generated/offsets/java_lang_Integer.h"
#include "generated/offsets/java_lang_Long.h"
#include "generated/offsets/java_lang_Class.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/offsets/java_lang_Throwable.h"
#include "generated/jni/java_lang_reflect_Modifier.h"
#include <stdio.h>
#include "DMFieldRWLongTest.h"

JNIEXPORT jlongArray JNICALL 
Java_DMFieldRWLongTest_nGetValues (JNIEnv *env, jobject obj) 
{
   jint i, j, count = 0;
   jint numFields = 0;
   jlong var;
   jlongArray longArray;
   jobject lock;
   jfieldID iFid, oFid;
   jclass gcClazz, testClazz;
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

   longArray = (*env)->NewLongArray(env, numFields);

   CVMD_gcUnsafeExec( ee, {
      CVMFieldBlock* fb;
      int objOffset;
      CVMObject *directObject = CVMID_icellDirect(ee, obj);
      CVMArrayOfLong* directLongArray = (CVMArrayOfLong *)CVMID_icellDirect(ee, longArray);

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
            CVMD_fieldReadLong(directObject, objOffset, var);

            if(ee->barrier == R_BARRIER_64)
               count++;

            CVMD_arrayWriteLong(directLongArray, j, var);
            ++j;
         }
      }
	
      if(count == numFields)
         printf("PASS: DMFieldRWLongTest:Read, Read Barrier 64 was invoked.\n");
      else
         printf("FAIL: DMFieldRWLongTest:Read, Read Barrier 64 was not invoked.\n");

      if(CVMfbStaticField(ee, iFid).i == -1)
	printf("PASS: DMFieldRWLongTest:Read, Gc did not happen in the gc unsafe section\n");
      else
	printf("FAIL: DMFieldRWLongTest:Read, Gc happened in the gc unsafe section\n");
   });

   return longArray;
}

JNIEXPORT void JNICALL
Java_DMFieldRWLongTest_nSetValues(JNIEnv *env, jobject obj, 
                       jlong v1, jlong v2, jlong v3)
{
   jint i, count = 0;
   jobject lock;
   jfieldID iFid, oFid;
   jclass gcClazz, testClazz;
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

      CVMD_fieldWriteLong(directObject, objOffset[0], v1);
      if(ee->barrier == W_BARRIER_64)
         count++;
      CVMD_fieldWriteLong(directObject, objOffset[1], v2);
      if(ee->barrier == W_BARRIER_64)
         count++;
      CVMD_fieldWriteLong(directObject, objOffset[2], v3);
      if(ee->barrier == W_BARRIER_64)
         count++;

      if(count == numFields)
         printf("PASS: DMFieldRWLongTest:Write, Write Barrier 64\n");
      else
         printf("FAIL: DMFieldRWLongTest:Write, Write Barrier 64\n");

      if(CVMfbStaticField(ee, iFid).i == -1)
	printf("PASS: DMFieldRWLongTest:Write, Gc did not happen in the gc unsafe section\n");
      else
	printf("FAIL: DMFieldRWLongTest:Write, Gc happened in the gc unsafe section\n");
   } );
}
