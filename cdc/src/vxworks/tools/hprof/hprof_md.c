/*
 * @(#)hprof_md.c	1.8 06/10/10
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

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <hostLib.h>

#include "jni.h"
#include "jlong.h"
#include "hprof.h"

void hprof_close(int fd)
{
    close(fd);
}

int hprof_send(int s, const char *msg, int len, int flags)
{
    int res;
    do {
        res = send(s, (char *)msg, len, flags);
    } while ((res < 0) && (errno == EINTR));
    
    return res;
}

int hprof_write(FILE *filedes, const void *buf, size_t nbyte)
{
    int res;
    res = fwrite((char *)buf, 1, nbyte, filedes);
    return res;
}


jint hprof_get_milliticks()
{
    jlong tmp = hprof_get_timemillis();
    jint millis = jlong_to_jint(tmp);
    return millis;
}

jlong hprof_get_timemillis()
{
    jlong millis;
    jlong one_thousand;
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    one_thousand = jint_to_jlong(1000);
    millis = jint_to_jlong((jint)ts.tv_sec);
    millis = jlong_mul(millis, one_thousand);
    {
        jlong tmp1, tmp2;
        jlong one_million;

        one_million = jint_to_jlong(1000000);
        tmp1 = jint_to_jlong((jint)ts.tv_nsec);
        tmp2 = jlong_div(tmp1, one_million);
        millis = jlong_add(millis, tmp2);
    }
    return millis;
}

void hprof_get_prelude_path(char *path)
{
    extern JavaVM *jvm;
    int res;
    JNIEnv *env;
    jstring propname;
    jclass clazz;
    jmethodID methodID;
    jstring prop;
    const char *propUTF;
    jboolean isCopy;

    res = (*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_2);
    if (res < 0) {
        path[0] = '\0';
        return;
    }

    propname = (*env)->NewStringUTF(env, "java.home");
    clazz = (*env)->FindClass(env, "java.lang.System");
    methodID = (*env)->GetStaticMethodID(env, clazz, "getProperty",
                                "(Ljava.lang.String;)Ljava.lang.String;");
    prop = (jstring) (*env)->CallStaticObjectMethod(env, clazz,
                                                    methodID, propname);
    propUTF = (*env)->GetStringUTFChars(env, prop, &isCopy);
    sprintf(path, "%s/hprof/lib/jvm.hprof.txt", propUTF);
    (*env)->ReleaseStringUTFChars(env, prop, propUTF);
    (*env)->DeleteLocalRef(env, (jobject) prop);
    (*env)->DeleteLocalRef(env, (jobject) propname);
    (*env)->DeleteLocalRef(env, (jobject) clazz);
}

char *hprof_strdup(const char *str)
{
    char *result;
    result = malloc(strlen(str) + 1);
    if (result != NULL) {
        strcpy(result, str);
    }
    return result;
}

/*
 * Return a socket descriptor connect()ed to a "hostname" that is
 * accept()ing heap profile data on "port." Return a value <= 0 if
 * such a connection can't be made.
 */
int hprof_real_connect(char *hostname, unsigned short port)
{
    int hostID;
    struct sockaddr_in s;
    int fd;
    
    /* This check is redundant because port is of type unsigned short.
       Hence, it will never be outside of this range.
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "HPROF ERROR: bad port number\n");
	return -1;
    }
    */
    if (hostname == NULL) {
        fprintf(stderr, "HPROF ERROR: hostname is NULL\n");
        return -1;
    }

    /* create a socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);

    hostID = hostGetByName(hostname);
    if (hostID == ERROR) {
        return -1;
    }
    /* set remote host's addr; its already in network byte order */
    memcpy(&s.sin_addr.s_addr, &hostID, sizeof(s.sin_addr.s_addr));

    /* set remote host's port */
    s.sin_port = htons(port);
    s.sin_family = AF_INET;

    /* now try connecting */
    if (-1 == connect(fd, (struct sockaddr*)&s, sizeof(s))) {
	return 0;
    } else {
	return fd;
    }
}
