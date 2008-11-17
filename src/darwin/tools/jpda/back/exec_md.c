/*
 * @(#)exec_md.c	1.3 06/10/26
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "sys.h"
#include "util.h"
  
#ifdef CVM_JVMTI
#define jdwpAlloc jvmtiAllocate
#define jdwpFree jvmtiDeallocate
#endif

static char *skipWhitespace(char *p) {
    while ((*p != '\0') && isspace(*p)) {
        p++;
    }
    return p;
}

static char *skipNonWhitespace(char *p) {
    while ((*p != '\0') && !isspace(*p)) {
        p++;
    }
    return p;
}

int 
dbgsysExec(char *cmdLine)
{
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

    argv = jdwpAlloc((argc + 1) * sizeof(char *));
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

    if ((pid = fork()) == 0) {
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
}

