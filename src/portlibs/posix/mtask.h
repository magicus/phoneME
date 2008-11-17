/*
 * @(#)mtask.h	1.14 06/10/25
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

#ifndef _POSIX_MTASK_H
#define _POSIX_MTASK_H

#ifdef CVM_MTASK

#include "javavm/export/jni.h"

/*
 * Notify the child process on the socket that will
 * link it to the mTASK instance
 */
extern void
CVMmtaskServerCommSocket(JNIEnv* env, CVMInt32 commSocket);

/*
 * Initialize child VM. The client ID is recorded by the
 * child process.
 */
extern void
CVMmtaskReinitializeChildVM(JNIEnv* env, CVMInt32 clientId);

/*
 * Record the port number which the server is listening to
 */
extern void
CVMmtaskServerPort(JNIEnv* env, CVMInt32 serverPort);

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)
extern void
CVMmtaskHandleSuspendChecker();
#endif

#ifdef CVM_TIMESTAMPING
extern jboolean
CVMmtaskTimeStampReinitialize(JNIEnv* env);

extern jboolean
CVMmtaskTimeStampRecord(JNIEnv* env, const char* loc, int pos);
#endif

typedef struct {
    jboolean initialized;
    int serverfd; /* Server */
    int connfd;   /* Connection */
    FILE *connfp; /* Buffered connection */

    JNIEnv *env;
    jboolean wasSourceCommand;

    jboolean isTestingMode;
    char* testingModeFilePrefix;

    /* Java method ID's to use for stopping and re-starting system threads */
    jclass referenceClass;
    jclass cvmClass;
    jmethodID stopSystemThreadsID;
    jmethodID restartSystemThreadsID;
    jmethodID resetMainID;

} ServerState;

extern int
MTASKserverInitialize(ServerState* state,
    CVMParsedSubOptions* serverOpts,
    JNIEnv* env, jclass cvmClass);

extern int
MTASKnextRequest(ServerState *state);

extern void
MTASKserverSourceReset(ServerState *state);

#endif

#endif /* _POSIX_MTASK_H */
