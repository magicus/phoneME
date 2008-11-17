/*
 * @(#)exec_md.c	1.12 06/10/10
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

#include <assert.h>
#include <stdio.h>
#ifdef JDK
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "sys.h"
#include "util.h"
#endif  

static char *skipWhitespace(char *p) {
#ifdef JDK
    while ((*p != '\0') && isspace(*p)) {
        p++;
    }
    return p;
#else
    fprintf(stderr, "exec-related jdb functionality "
	    "not supported on VxWorks\n");
    assert(0);
    return NULL;
#endif
}

static char *skipNonWhitespace(char *p) {
#ifdef JDK
    while ((*p != '\0') && !isspace(*p)) {
        p++;
    }
    return p;
#else
    fprintf(stderr, "exec-related jdb functionality "
	    "not supported on VxWorks\n");
    assert(0);
    return NULL;
#endif
}

int 
dbgsysExec(char *cmdLine)
{
#ifdef JDK
    int i;
    int argc;
    int pid = -1; /* this is the error return value */
    char **argv = NULL;
    char *p;
    char *args;

    /* Skip leading whitespace */
    cmdLine = skipWhitespace(cmdLine);

    args = jdwpAlloc(strlen(cmdLine)+1);
    if (args == NULL) {
        return SYS_NOMEM;
    }
    strcpy(args, cmdLine);

    p = args;

    argc = 0;
    while (*p != '\0') {
        p = skipNonWhitespace(p);
        argc++;
        if (*p == '\0') {
            break;
        }
        p = skipWhitespace(p);
    }

    /* calloc null terminates for us */
    argv = calloc((argc + 1) * sizeof(char *), 1);
    if (argv == 0) {
        jdwpFree(args);
        return SYS_NOMEM;
    }

    for (i = 0, p = args; i < argc; i++) {
        argv[i] = p;
        p = skipNonWhitespace(p);
        *p++ = '\0';
        p = skipWhitespace(p);
    }

    if ((pid = fork1()) == 0) {
        /* Child process */
        int i, max_fd;

        /* close everything */
        max_fd = sysconf(_SC_OPEN_MAX);
        for (i = 3; i < max_fd; i++) {
            close(i);
        }

        execvp(argv[0], argv);

        exit(-1);
    }
    jdwpFree(args);
    jdwpFree(argv);
    if (pid < 0) {
        return SYS_ERR;
    } else {
        return SYS_OK;
    }
#else
    fprintf(stderr, "exec-related jdb functionality "
	    "not supported on VxWorks\n");
    assert(0);
    return -1;
#endif
}

