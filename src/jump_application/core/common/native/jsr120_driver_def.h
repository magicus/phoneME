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

#ifndef _JSR120_DRIVER_DEF_H
#define _JSR120_DRIVER_DEF_H

//#define NM_DEBUG
extern char *prog_name;
#ifdef NM_DEBUG
#define LOG(str)  do { \
    fprintf(stderr, "[%s-%d] (%s:%d): %s\n", prog_name, getpid(), \
        __FUNCTION__, __LINE__, (str)); \
    fflush(stderr); \
} while (0)

#define LOG1(fmt, par)  do { \
    char _b[1000]; \
    snprintf(_b, sizeof _b, (fmt), (par)); \
    _b[sizeof _b - 1] = '\0'; \
    fprintf(stderr, "[%s-%d] (%s:%d): %s\n", prog_name, getpid(), \
        __FUNCTION__, __LINE__, _b); \
    fflush(stderr); \
} while (0)
#define LOG2(fmt, par1, par2)  do { \
    char _b[1000]; \
    snprintf(_b, sizeof _b, (fmt), (par1), (par2)); \
    _b[sizeof _b - 1] = '\0'; \
    fprintf(stderr, "[%s-%d] (%s:%d): %s\n", prog_name, getpid(), \
        __FUNCTION__, __LINE__, _b); \
    fflush(stderr); \
} while (0)

#define LOG3(fmt, par1, par2, par3)  do { \
    char _b[1000]; \
    snprintf(_b, sizeof _b, (fmt), (par1), (par2), (par3)); \
    _b[sizeof _b - 1] = '\0'; \
    fprintf(stderr, "[%s-%d] (%s:%d): %s\n", prog_name, getpid(), \
        __FUNCTION__, __LINE__, _b); \
    fflush(stderr); \
} while (0)

#define nmsg_error(s)   do {LOG1("%s --> exit", (s)); exit(1);} while (0)
#else
#define LOG(x)  (void)prog_name /* avoid warining about unused variable */
#define LOG1(x,arg) (void)prog_name /* avoid warining about unused variable */
#define LOG2(x,arg1,arg2)   (void)prog_name /* avoid warining about unused variable */
#define LOG3(x,arg1,arg2,arg3)   (void)prog_name /* avoid warining about unused variable */
#define nmsg_error(s)   do {printf("%s --> exit\n", (s)); exit(1);} while (0)
#endif

#include <stdio.h>
#include <string.h>

#include <jsr120_sms_protocol.h>
#include <jsr120_cbs_protocol.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_cbs_listeners.h>
#ifdef ENABLE_JSR_205
#include <jsr120_sms_protocol.h>
#include <jsr120_sms_listeners.h>
#endif

#include <app_package.h>

#include <jump_messaging.h>
#include <porting/JUMPProcess.h>
#include <porting/JUMPTypes.h>
#include <jsr120_jumpdriver.h>

#include <javacall_sms.h>
#include <javacall_cbs.h>

#define ALIGN_BITS                  64
#define ALIGN_BYTES                 (ALIGN_BITS >> 3)
#define ALIGN_MASK                  (ALIGN_BYTES - 1)
#define ALIGN_IS_ALIGNED(ptr_)      (ALIGN_SHIFT(ptr_) == 0)
#define ALIGN_SHIFT(ptr_)           ((ptr_) & ALIGN_MASK)
#define ALIGN_GAP                   (ALIGN_BYTES - 1)
#define ALIGN(ptr_)                 ((ptr_) = (ALIGN_IS_ALIGNED((int)ptr_) ? (ptr_) : ((ptr_) + ALIGN_SHIFT((int)ptr_))))

#define IFACE_STATUS_OK     0
#define IFACE_STATUS_FAIL   1

#define STRING_LEN(str_)    (str_ == NULL ? 0 : strlen(str_))

typedef int32 typeInt;
typedef int8 typeByte;
typedef int16 typeShort;
typedef int64 typeLong;

#define MAX_CLIENTS         30

#define MAX_SHMEM_NAME_LEN 20

int jumpProcessRunDriver(char *driver_name, char *lib_name);

#endif /* #ifdef _JSR120_DRIVER_DEF_H */
