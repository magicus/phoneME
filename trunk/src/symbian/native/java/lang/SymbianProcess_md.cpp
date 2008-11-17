/*
 * @(#)SymbianProcess_md.cpp	1.3 06/10/10
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

#include "jni.h"
#include "jvm.h"
#include "jvm_md.h"
#include "jni_util.h"
#include "jlong.h"

/*
 * Platform-specific support for java.lang.SymbianProcess
 */

#include <e32std.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "jni_statics.h"
#include "javavm/include/porting/ansi/stdlib.h"


JNIEXPORT jlong JNICALL
Java_java_lang_SymbianProcess_create(JNIEnv *env, jobject process,
				     jstring cmd0,
				     jstring cline0,
				     jstring env0,
				     jstring path,
				     jobject in_fd,
				     jobject out_fd,
				     jobject err_fd)
{
    if (cmd0 == 0) {
	JNU_ThrowNullPointerException(env, 0);
	return 0;
    }

    RProcess *p = (RProcess *)malloc(sizeof *p);
    if (p == NULL) {
	JNU_ThrowOutOfMemoryError(env, 0);
	return 0;
    }
    new (p) RProcess();
#if 0
    if (env0 != 0) {
	envcmd = (char *)JNU_GetStringPlatformChars(env, env0, 0);
    }
#endif
#ifdef _UNICODE
    jint exelen = (*env)->GetStringLength(env, cmd0);
    const jchar *jexe = (*env)->GetStringChars(env, cmd0, NULL);
    TPtr16 exe((TUint16 *)jexe, exelen);
    jint argslen = (*env)->GetStringLength(env, cline0);
    const jchar *args = (*env)->GetStringChars(env, cline0, NULL);
    TPtr16 cline((TUint16 *)args, argslen);
#else
    char *jexe = (char *)JNU_GetStringPlatformChars(env, cmd0, 0);
    TPtr8 exe((TUint8 *)jexe, strlen(jexe));
    char *args = (char *)JNU_GetStringPlatformChars(env, cline0, 0);
    TPtr8 cline((TUint8 *)args, strlen(args));
#endif

    TInt err = p->Create(exe, cline);

    if (err != KErrNone) {
	JNU_ThrowByName(env, "java/io/IOException", 0);
	goto cleanup;
    }

#if 0
    (*env)->SetIntField(env, in_fd, IO_fd_fdID, (jint)hwrite[0]);
    (*env)->SetIntField(env, out_fd, IO_fd_fdID, (jint)hread[1]);
    (*env)->SetIntField(env, err_fd, IO_fd_fdID, (jint)hread[2]);
#endif

 cleanup:
#ifdef _UNICODE
    (*env)->ReleaseStringChars(env, cmd0, jexe);
    (*env)->ReleaseStringChars(env, cline0, args);
#else
    JNU_ReleaseStringPlatformChars(env, cmd0, jexe);
    JNU_ReleaseStringPlatformChars(env, cline0, args);
#endif
#if 0
    if (env0 != 0) {
	JNU_ReleaseStringPlatformChars(env, env0, envcmd);
    }
#endif
    return ptr_to_jlong((void *)p);
}

JNIEXPORT jint JNICALL
Java_java_lang_SymbianProcess_exitValue(JNIEnv *env, jobject process)
{
    jint exit_code;
    jboolean exc;
    jlong handle = JNU_GetFieldByName(env, &exc, process, "handle", "J").j;
    if (exc) {
        return 0;
    }

    RProcess *p = (RProcess *)jlong_to_ptr(handle);
    TExitType t = p->ExitType();
    if (t == EExitPending) {
	JNU_ThrowByName(env, "java/lang/IllegalThreadStateException",
			"process has not exited");
	return -1;
    }
    return p->ExitReason();
}

#include <stdio.h>
JNIEXPORT jint JNICALL
Java_java_lang_SymbianProcess_waitFor0(JNIEnv *env, jobject process)
{
    long exit_code;
    int which;
    jboolean exc;
    jlong handle = JNU_GetFieldByName(env, &exc, process, "handle", "J").j;

    if (exc) {
        return 0;
    }

    RProcess *p = (RProcess *)jlong_to_ptr(handle);
    TRequestStatus rs;
    p->Logon(rs);
    return p->ExitReason();
}

JNIEXPORT void JNICALL
Java_java_lang_SymbianProcess_destroy(JNIEnv *env, jobject process)
{
    jboolean exc;
    jlong handle = JNU_GetFieldByName(env, &exc, process, "handle", "J").j;
    if (exc) {
        return;
    }
    RProcess *p = (RProcess *)jlong_to_ptr(handle);
    p->Kill(-1);
}

JNIEXPORT void JNICALL
Java_java_lang_SymbianProcess_close(JNIEnv *env, jobject process)
{
    jboolean exc;
    jlong handle = JNU_GetFieldByName(env, &exc, process, "handle", "J").j;
    if (exc) {
        return;
    }
    RProcess *p = (RProcess *)jlong_to_ptr(handle);
    p->Close();
}
