/*
 * @(#)IDFieldRW32Test.c	1.8 06/10/10
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
#include "IDFieldRW32Test.h"

JNIEXPORT jintArray JNICALL 
Java_IDFieldRW32Test_nGetValues (JNIEnv *env, jobject obj) 
{
   jint i, j;
   jint numFields = 0;
   jclass clazz;
   jintArray intArray;
   CVMJavaVal32 var;
   CVMFieldBlock* fb;
   CVMClassBlock *cb;
   CVMExecEnv *ee;

   ee = CVMjniEnv2ExecEnv(env);

   clazz = (*env)->GetObjectClass(env, obj);

   cb = CVMgcSafeClassRef2ClassBlock(ee, clazz);

   for(i=0; i< CVMcbFieldCount(cb); i++) {
      fb = CVMcbFieldSlot(cb, i);
      if(!CVMfbIs(fb, STATIC))
         ++numFields;
   }

   intArray = (*env)->NewIntArray(env, numFields);

   CVMD_gcUnsafeExec( ee, {
      CVMD_gcSafeExec(ee, {
         jint objOffset;

         for(i=0, j=0; i< CVMcbFieldCount(cb); i++) {
            fb = CVMcbFieldSlot(cb, i);

            if(!CVMfbIs(fb, STATIC)) {
               objOffset = CVMfbOffset(fb);
               CVMID_fieldRead32(ee, (CVMObjectICell*) obj, objOffset,  var);
               CVMID_arrayWriteInt(ee, (CVMArrayOfIntICell*) intArray, i, var.i);
               ++j;
            }
         }

      });
   });

   return intArray;
}

JNIEXPORT void JNICALL
Java_IDFieldRW32Test_nSetValues(JNIEnv *env, jobject obj, 
                       jint v1, jint v2, jint v3)
{
   jint i, numFields = 0;
   jclass clazz;
   CVMFieldBlock* fb;
   CVMClassBlock *cb;
   CVMExecEnv *ee;
   CVMJavaVal32 var;

   ee = CVMjniEnv2ExecEnv(env);

   clazz = (*env)->GetObjectClass(env, obj);

   cb = CVMgcSafeClassRef2ClassBlock(ee, clazz);

   CVMD_gcUnsafeExec( ee, {
      int objOffset[CVMcbFieldCount(cb)];

      for(i=0; i< CVMcbFieldCount(cb); i++) {
           fb = CVMcbFieldSlot(cb, i);
           if(!CVMfbIs(fb, STATIC))
              objOffset[numFields++] = CVMfbOffset(fb);
      }

      CVMD_gcSafeExec(ee, {
         var.i = v1;
         CVMID_fieldWrite32(ee, (CVMObjectICell*) obj, objOffset[0], var);
         var.i = v2;
         CVMID_fieldWrite32(ee, (CVMObjectICell*) obj, objOffset[1], var);
         var.i = v3;
         CVMID_fieldWrite32(ee, (CVMObjectICell*) obj, objOffset[2], var);
      });

   } );
}
