/*
 * @(#)packages.c	1.19 06/10/10
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
#include "jni_util.h"
#include "javavm/include/globals.h"
#include "javavm/include/ansi2cvm.h"


/* Hash function */
static unsigned CVMhash(const char* s, int n)
{
    unsigned val = 0;
    while (--n >= 0) {
	val = *s++ + 31 * val;
    }
    return val % CVM_PKGINFO_HASHSIZE;
}

/*
 * Returns the package file name corresponding to the specified package
 * or class name, or null if not found.
 */
char*
CVMpackagesGetEntry(const char* name)
{
    char* cp = strrchr(name, '/');

    if (cp != 0) {
	CVMPackage* pp;
	int n = (int)(cp - name + 1);
	for (pp = CVMglobals.packages[CVMhash(name,n)];
	     pp != NULL; pp = pp->next) {
	    if (strncmp(name, pp->packageName, n) == 0) {
		return pp->filename;
	    }
	}
    }
    return NULL;
}

/*
 * Fills the names[] array with all of the names in the packages hash table.
 */
static void
CVMpackagesGetNames(char* names[], int size)
{
    int n = 0;
    int i;

    for (i = 0; i < CVM_PKGINFO_HASHSIZE; i++) {
	CVMPackage* pp = CVMglobals.packages[i];
	while (pp != 0) {
	    names[n++] = pp->packageName;
	    pp = pp->next;
	}
    }
}

/*
 * Adds a new package entry for the specified class or package name and
 * corresponding directory or jar file name. If there was already an existing
 * package entry then it is replaced. Returns the previous entry or null if
 * none.
 */
CVMBool
CVMpackagesAddEntry(const char* name, const char* filename)
{
    char* cp = strrchr(name, '/');

    if (cp != 0) {
	CVMPackage* pp;
	int n = (int)(cp - name + 1);
	int i = CVMhash(name, n);

	/* First check for previously loaded entry */
	for (pp = CVMglobals.packages[i]; pp != 0; pp = pp->next) {
	    if (strncmp(name, pp->packageName, n) == 0) {
		CVMassert(!strcmp(filename, pp->filename));
		return CVM_TRUE;
	    }
	}

	/* If not found then allocate a new entry and insert into table */
	pp = (CVMPackage *)malloc(sizeof(CVMPackage));
	if (pp == NULL) {
	    return CVM_FALSE;
	}
	pp->packageName = (char *)malloc(n + 1);
	if (pp->packageName == NULL) {
	    free(pp);
	    return CVM_FALSE;
	}
	memcpy(pp->packageName, name, n);
	pp->packageName[n] = '\0';
	pp->filename = strdup(filename);
	if (pp->filename == 0) {
	    free(pp->packageName);
	    free(pp);
	    return CVM_FALSE;
	}
	pp->next = CVMglobals.packages[i];
	CVMglobals.packages[i] = pp;
	CVMglobals.numPackages++;
    }
    return CVM_TRUE;
}

/*
 * Destroys package information on VM exit
 */
void
CVMpackagesDestroy()
{
    CVMPackage* pp;
    int i;
    for (i = 0; i < CVM_PKGINFO_HASHSIZE; i++) {
	CVMPackage* pnext;
	for (pp = CVMglobals.packages[i]; pp != NULL; pp = pnext) {
	    pnext = pp->next;
	    free(pp->packageName);
	    free(pp->filename);
	    free(pp);
	}
    }
}

/*
 * If the specified package has been loaded by the system, then returns
 * the name of the directory or ZIP file that the package was loaded from.
 * Returns null if the package was not loaded.
 * Note: The specified name can either be the name of a class or package.
 * If a package name is specified, then it must be "/"-separator and also
 * end with a trailing "/".
 */
JNIEXPORT jstring JNICALL
JVM_GetSystemPackage(JNIEnv *env, jstring str)
{
    const char* name = (*env)->GetStringUTFChars(env, str, 0);
    if (name != NULL) {
	char* fn;
	CVM_NULL_CLASSLOADER_LOCK(CVMjniEnv2ExecEnv(env));
	fn = CVMpackagesGetEntry(name);
	CVM_NULL_CLASSLOADER_UNLOCK(CVMjniEnv2ExecEnv(env));
	(*env)->ReleaseStringUTFChars(env, str, name);
	if (fn != 0) {
	    return JNU_NewStringPlatform(env, fn);
	}
    }
    return NULL;
}

/*
 * Returns an array of Java strings representing all of the currently
 * loaded system packages.
 * Note: The package names returned are "/"-separated and end with a
 * trailing "/".
 */
JNIEXPORT jobjectArray JNICALL
JVM_GetSystemPackages(JNIEnv *env)
{
    char** names;
    int    size;

    /*
     * Get all of the package names in one char* array.
     */
    CVM_NULL_CLASSLOADER_LOCK(CVMjniEnv2ExecEnv(env));
    size = CVMglobals.numPackages;
    names = (char **)malloc(size * sizeof(char*));
    if (names != NULL) {
	CVMpackagesGetNames(names, size);
    }
    CVM_NULL_CLASSLOADER_UNLOCK(CVMjniEnv2ExecEnv(env));

    if (names != NULL) {
	jobjectArray result = NULL;
	jclass cls = CVMcbJavaInstance(CVMsystemClass(java_lang_String));

	/* allocate a String array for the result */
	result = (*env)->NewObjectArray(env, size, cls, 0);
	if (result == NULL) {
	    return NULL;
	}

	/* 
	 * Allocate a String object for each entry in the name[] array.
	 * Store the String in the "result" array.
	 */
	while (size > 0 && !(*env)->ExceptionCheck(env)) {
	    jstring str;
	    size--;
	    str = (*env)->NewStringUTF(env, names[size]);
	    if (str != NULL) {
		(*env)->SetObjectArrayElement(env, result, size, str);
		(*env)->DeleteLocalRef(env, str);
	    }
	}
	
	free(names);
	return (*env)->ExceptionCheck(env) ? NULL : result;
    }
    return NULL;
}
