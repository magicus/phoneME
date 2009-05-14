/*
 * @(#)DMArrayCopyCharTest.c	1.7 06/10/10
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
#include "DMArrayCopyCharTest.h"


JNIEXPORT void JNICALL
Java_DMArrayCopyCharTest_nSetArray(JNIEnv *env, jobject obj, 
                                  jcharArray srcArray, jcharArray dstArray)
{
   jint i;
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
      CVMArrayOfChar* dSrcCharArray = (CVMArrayOfChar *)CVMID_icellDirect(ee, srcArray);
      CVMArrayOfChar* dDstCharArray = (CVMArrayOfChar *)CVMID_icellDirect(ee, dstArray);

      CVMfbStaticField(ee, iFid).i = -1;

      CVMobjectLock(ee, lock);
      CVMobjectNotify(ee, lock);
      CVMobjectUnlock(ee, lock);

      for(i = 0; i < 10000000; i++)
         ;

      CVMD_arrayCopyChar(dSrcCharArray, 0, dDstCharArray, 0, CVMD_arrayGetLength(dDstCharArray));

      if(ee->barrier == W_BARRIER_CHAR)
         printf("PASS: DMArrayCopyCharTest, Write Barrier Char was invoked\n");
      else
         printf("FAIL: DMArrayCopyCharTest, Write Barrier Char was not invoked\n");

      printf("\n");
       
      if(CVMfbStaticField(ee, iFid).i == -1)
         printf("PASS: DMArrayCopyCharTest, Gc did not happen in the gc unsafe section\n");
      else
         printf("FAIL: DMArrayCopyCharTest, Gc happened in the gc unsafe section\n");

   } );
}
