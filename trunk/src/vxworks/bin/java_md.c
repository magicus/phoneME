/*
 * @(#)java_md.c	1.20 06/10/10
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
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "portlibs/ansi_c/java.h"
#include "javavm/export/jni.h"

#define VXWORKS_SAFE_EXIT

#define isspacetab(x) ((x == ' ') || (x == '\t'))

static int
parseArgs(const char *cmd, const char ***argv)
{
    int argc = 1;
    const char *p = cmd;
    const char *endp;
    const char **tmpArgv;
    char *arg;
    int sz;
    int i;

    /* Count number of args */
    /* Eat leading whitespace */
    while ((*p != 0) && isspacetab(*p)) {
	p++;
    }

    while (*p != 0) {
	argc++;

	/* Eat string */
	while ((*p != 0) && (!isspacetab(*p))) {
	    p++;
	}
	
	/* Eat whitespace */
	while ((*p != 0) && isspacetab(*p)) {
	    p++;
	}
    }
    
    /* Allocate argv */
    tmpArgv = (const char **) malloc(sizeof(const char *) * argc);
    *argv = tmpArgv;
    tmpArgv[0] = "runJava";

    /* Split up args */
    p = cmd;
    i = 1;
    while (*p != 0) {
	/* Eat whitespace */
	while ((*p != 0) && isspacetab(*p)) {
	    p++;
	}
	
	if (*p != 0) {
	    /* Find end of string */
	    endp = p;
	    while ((*endp != 0) && (!isspacetab(*endp))) {
		endp++;
	    }

	    /* Allocate room for new string (remembering null terminator) */
	    sz = endp - p + 1;
	    arg = malloc(sz * sizeof(char));
	    memcpy(arg, p, sz - 1);
	    arg[sz - 1] = 0;

	    /* Insert into argv array */
	    tmpArgv[i++] = arg;

	    p = endp;
	}
    }

    assert(i == argc);
    return argc;
}

int
runJava(const char *cmd)
{
    int argc;
    int result;
    const char **argv;
    int i;

#ifdef VXWORKS_SAFE_EXIT
    {
	char *safeOpt = "-XsafeExit ";
	char *buf = malloc(strlen(cmd) + strlen(safeOpt) + 1);
	strcpy(buf, safeOpt);
	strcat(buf, cmd);
	cmd = buf;
    }
#endif
    argc = parseArgs(cmd, &argv);
    result = ansiJavaMain(argc, argv, JNI_CreateJavaVM);
    for (i = 1; i < argc; i++) {
	free((char*)argv[i]);
    }
    free(argv);
#ifdef VXWORKS_SAFE_EXIT
    free((char *)cmd);
#endif
    return result;
}

#ifdef VXWORKS_SAFE_EXIT
#define NUM_ARGS 12
#else
#define NUM_ARGS 11
#endif

int
runJavaArgs(const char *a0, const char *a1, const char *a2, const char *a3,
	    const char *a4, const char *a5, const char *a6, const char *a7,
	    const char *a8, const char *a9)
{
    int i = 0, argc;
    const char *argv[NUM_ARGS];

    argv[i++] = "runJava";
#ifdef VXWORKS_SAFE_EXIT
    argv[i++] = "-XsafeExit";
#endif
    argv[i++] = a0; argv[i++] = a1;
    argv[i++] = a2; argv[i++] = a3;
    argv[i++] = a4; argv[i++] = a5;
    argv[i++] = a6; argv[i++] = a7;
    argv[i++] = a8; argv[i] = a9;

    assert(i == NUM_ARGS - 1); 
    while (i >= 1 && argv[i] == NULL) {
	--i;
    }
    argc = i + 1;
    return ansiJavaMain(argc, argv, JNI_CreateJavaVM);
}


int
runJavaJdb(const char* cmd)
{
    int argc;
    int result;
    const char **argv;
    int i;

/* add JDB flags */
    char *jdbOpt = "-Xdebug " \
	"-Xrunjdwp:transport=dt_socket,server=y,address=8000 " \
	"-Dsun.boot.library.path=../../../jdk_build/vxworks/lib/PENTIUM ";

    char *buf = malloc(strlen(cmd) + strlen(jdbOpt) + 1);
    strcpy(buf, jdbOpt);
    strcat(buf, cmd);
    cmd = buf;

#ifdef VXWORKS_SAFE_EXIT
    {
	char *safeOpt = "-XsafeExit ";
	char *buf = malloc(strlen(cmd) + strlen(safeOpt) + 1);
	strcpy(buf, safeOpt);
	strcat(buf, cmd);
	cmd = buf;
    }
#endif
    argc = parseArgs(cmd, &argv);
    result = ansiJavaMain(argc, argv, JNI_CreateJavaVM);
    for (i = 1; i < argc; i++) {
	free((char*)argv[i]);
    }
    free(argv);
#ifdef VXWORKS_SAFE_EXIT
    free((char *)cmd);
#endif
    return result;
}
