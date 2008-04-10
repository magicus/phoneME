/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _JUMP_EVENTS_MD_H
#define _JUMP_EVENTS_MD_H

#ifdef __cplusplus
extern "C" {
#endif

#define JUMP_EVENTS_MAGIC_MD    (('E' << 24)|('V' << 16)|('N' << 8)|'T')

struct _JUMPEventListener_tag {
    struct _JUMPEventListener_tag *next;
    int done;
    unsigned int serial;
    int pipe[2];
};

struct _JUMPEventIMPL_tag {
    int magic;
    int flag;
    pthread_mutex_t locked;
    struct _JUMPEventListener_tag *listeners;
};

#ifndef LOG1
#define EV_DEBUG
#ifdef EV_DEBUG
#include <stdio.h>
#define LOG(str)  do { \
    fprintf(stderr, "[%d] (%s:%d): %s\n", getpid(), \
        __FUNCTION__, __LINE__, (str)); \
    fflush(stderr); \
} while (0)

#define LOG1(fmt, par)  do { \
    char _b[100]; \
    snprintf(_b, sizeof _b, (fmt), (par)); \
    _b[sizeof _b - 1] = '\0'; \
    fprintf(stderr, "[%d] (%s:%d): %s\n", getpid(), \
        __FUNCTION__, __LINE__, _b); \
    fflush(stderr); \
} while (0)
#define LOG2(fmt, par1, par2)  do { \
    char _b[100]; \
    snprintf(_b, sizeof _b, (fmt), (par1), (par2)); \
    _b[sizeof _b - 1] = '\0'; \
    fprintf(stderr, "[%d] (%s:%d): %s\n", getpid(), \
        __FUNCTION__, __LINE__, _b); \
    fflush(stderr); \
} while (0)

#define nmsg_error(s)   do {LOG1("%s --> exit", (s)); exit(1);} while (0)
#else
#define LOG(x)
#define LOG1(x,arg)
#define LOG2(x,arg1,arg2)
#define nmsg_error(s)   do {printf("%s --> exit\n", (s)); exit(1);} while (0)
#endif /* EV_DEBUG */
#endif /* LOG1 */

#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JUMP_EVENTS_MD_H */
