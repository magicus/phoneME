/*
 * @(#)DMArrayRWBooleanTest.c	1.7 06/10/10
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
#include "DMArrayRWBooleanTest.h"


JNIEXPORT void JNICALL
Java_DMArrayRWBooleanTest_nSetArray(JNIEnv *env, jobject obj, 
                           jbooleanArray dstArray, jbooleanArray srcArray, jint arrayLength)
{
   jint i;
   jboolean val;
   jobject lock;
   jfieldID iFid, oFid;
   jclass gcClazz, testClazz;
   CVMExecEnv *ee;

   gcClazz = (*env)->FindClass(env, "GcThread");
   iFid = (*env)->GetStaticFieldID(env, gcClazz, "gcCalled", "I");

   testClazz = (*env)->GetObjectClass(env, obj);
   oFid = (*env)->GetStaticFieldID(env, testClazz, "lock", "Ljava/lang/Object;");
   lock = (*env)->GetStaticObjectField(env, testClazz, oFid);

   ee = CVMjniEnv2ExecEnv(env);

   CVMD_gcUnsafeExec( ee, {
      CVMArrayOfBoolean* directSrcArray = (CVMArrayOfBoolean *)CVMID_icellDirect(ee, srcArray);
      CVMArrayOfBoolean* directDstArray = (CVMArrayOfBoolean *)CVMID_icellDirect(ee, dstArray);
      jint rBarrier = 0;
      jint wBarrier = 0;

      CVMfbStaticField(ee, iFid).i = -1;

      CVMobjectLock(ee, lock);
      CVMobjectNotify(ee, lock);
      CVMobjectUnlock(ee, lock);

      for(i = 0; i < 1000000; i++)
         ;

     for(i=0; i<arrayLength; i++) {
      CVMD_arrayReadBoolean(directSrcArray, i, val);
      if(ee->barrier == R_BARRIER_BOOLEAN)
         rBarrier++;
      CVMD_arrayWriteBoolean(directDstArray, i, val);
      if(ee->barrier == W_BARRIER_BOOLEAN)
         wBarrier++;
     }

      if(rBarrier == arrayLength && wBarrier == arrayLength)
         printf("PASS: DMArrayRWBooleanTest, Read & Write Barrier Boolean were invoked\n");
      else
         printf("FAIL: DMArrayRWBooleanTest, Read & Write Barrier Boolean were not invoked\n");

      printf("\n");
       
      if(CVMfbStaticField(ee, iFid).i == -1)
         printf("PASS: DMArrayRWBooleanTest, Gc did not happen in the gc unsafe section\n");
      else
         printf("FAIL: DMArrayRWBooleanTest, Gc happened in the gc unsafe section\n");
   } );
}
