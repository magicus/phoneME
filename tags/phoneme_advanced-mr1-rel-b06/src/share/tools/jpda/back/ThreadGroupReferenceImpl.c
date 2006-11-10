/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
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
#include <string.h>

#include "ThreadGroupReferenceImpl.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"

static jboolean 
name(PacketInputStream *in, PacketOutputStream *out) 
{
    JNIEnv *env = getEnv();
    JVMDI_thread_group_info info;

    jthreadGroup group = inStream_readThreadGroupRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    threadGroupInfo(group, &info);
    outStream_writeString(out, info.name);

    (*env)->DeleteGlobalRef(env, info.parent);
    jdwpFree(info.name);
    return JNI_TRUE;
}

static jboolean 
parent(PacketInputStream *in, PacketOutputStream *out) 
{
    JNIEnv *env = getEnv();
    JVMDI_thread_group_info info;

    jthreadGroup group = inStream_readThreadGroupRef(in);
    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    threadGroupInfo(group, &info);
    WRITE_GLOBAL_REF(env, out, info.parent);

    jdwpFree(info.name);
    return JNI_TRUE;
}

static jboolean 
children(PacketInputStream *in, PacketOutputStream *out) 
{
     jint error;
     jint i;
     jint threadCount;
     jint groupCount;
     jthread *theThreads;
     jthread *theGroups;
     JNIEnv *env = getEnv();
 
     jthreadGroup group = inStream_readThreadGroupRef(in);
     if (inStream_error(in)) {
         return JNI_TRUE;
     }
 
     error = jvmdi->GetThreadGroupChildren(group,
                                          &threadCount,&theThreads,
                                          &groupCount, &theGroups);
     if (error != JVMDI_ERROR_NONE) {
         outStream_setError(out, error);
         return JNI_TRUE;
     }


     /* Squish out all of the debugger-spawned threads */
     threadCount = filterDebugThreads(theThreads, threadCount);
  
     outStream_writeInt(out, threadCount);
     for (i = 0; i < threadCount; i++) {
         WRITE_GLOBAL_REF(env, out, theThreads[i]);
     }
     outStream_writeInt(out, groupCount);
     for (i = 0; i < groupCount; i++) {
         WRITE_GLOBAL_REF(env, out, theGroups[i]);
     }

     jdwpFree(theGroups);
     jdwpFree(theThreads);
     return JNI_TRUE;
}

void *ThreadGroupReference_Cmds[] = { (void *)3,
                                      (void *)name,
                                      (void *)parent,
                                      (void *)children };

