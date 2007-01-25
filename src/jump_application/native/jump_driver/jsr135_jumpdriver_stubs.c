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


// #define NM_DEBUG
#ifdef NM_DEBUG
#define LOG(str)  do { \
    fprintf(stderr, "[%s-%d] (%d): %s\n", prog_name, getpid(), \
        __LINE__, (str)); \
    fflush(stderr); \
} while (0)

#define LOG1(fmt, par)  do { \
    char _b[100]; \
    snprintf(_b, sizeof _b, (fmt), (par)); \
    _b[sizeof _b - 1] = '\0'; \
    fprintf(stderr, "[%s-%d] (%d): %s\n", prog_name, getpid(), \
        __LINE__, _b); \
    fflush(stderr); \
} while (0)
#define LOG2(fmt, par1, par2)  do { \
    char _b[100]; \
    snprintf(_b, sizeof _b, (fmt), (par1), (par2)); \
    _b[sizeof _b - 1] = '\0'; \
    fprintf(stderr, "[%s-%d] (%d): %s\n", prog_name, getpid(), \
        __LINE__, _b); \
    fflush(stderr); \
} while (0)

#define nmsg_error(s)   do {LOG1("%s --> exit", (s)); exit(1);} while (0)
#else
#define LOG(x)  (void)prog_name /* avoid warinig about unused variable */
#define LOG1(x,arg) (void)prog_name /* avoid warinig about unused variable */
#define LOG2(x,arg1,arg2)   (void)prog_name /* avoid warinig about unused variable */
#define nmsg_error(s)   do {printf("%s --> exit\n", (s)); exit(1);} while (0)
#endif


#include <stdio.h>
#include <string.h>

#include <JUMPMessages.h>
#define JSR135_KNI_LAYER
#include <jsr135_jumpdriver.h>
#include <shared_memory.h>
#include <javacall_multimedia.h>
#include <jsr135_jumpdriver_impl.h>

static int driver = -1;

#define ALIGN_BITS                  64
#define ALIGN_BYTES                 (ALIGN_BITS >> 3)
#define ALIGN_MASK                  (ALIGN_BYTES - 1)
#define ALIGN_IS_ALIGNED(ptr_)      (ALIGN_SHIFT(ptr_) == 0)
#define ALIGN_SHIFT(ptr_)           ((ptr_) & ALIGN_MASK)
#define ALIGN_GAP                   (ALIGN_BYTES - 1)
#define ALIGN(ptr_)                 ((ptr_) = (ALIGN_IS_ALIGNED((int)ptr_) ? (ptr_) : ((ptr_) + ALIGN_SHIFT((int)ptr_))))

#define START_INTERFACE()
#define END_INTERFACE()

#define START(type_, name_, args_)   \
type_ D##name_ args_ { \
    unsigned char buf[JUMP_MSG_MAX_LENGTH]; \
    JUMPMessage *mm__ = jumpMessageCreateInBuffer("mm/jsr135", buf, sizeof buf); \
    int offset__ = 0; \
    jumpMessageWriteInt(mm__, &offset__, ID_##name_); \
    {

#define START_VOID(name_, args_)   \
void D##name_ args_ { \
    unsigned char buf[JUMP_MSG_MAX_LENGTH]; \
    JUMPMessage *mm__ = jumpMessageCreateInBuffer("mm/jsr135", buf, sizeof buf); \
    int offset__ = 0; \
    jumpMessageWriteInt(mm__, &offset__, ID_##name_); \
    {

#define ARG(type_, arg_)    \
    jumpMessageWrite##type_(mm__, &offset__, arg_);

#define INVOKE(result_, function_, arg_)   \
    INVOKE_VOID(function_, arg_) 

#define INVOKE_VOID(function_, arg_) {\
    unsigned char iface_result__; \
    if (driver == -1) { \
        driver = jumpMessageQueueOpen("MMDRIVER"); \
        if (driver == -1) { \
            goto err; \
        } \
    } \
    mm__ = jumpMessageSendAndWaitForResponse(driver, mm__); \
    if (mm__ == NULL) { \
        goto err; \
    } \
    offset__ = 0; \
    if (jumpMessageReadByte(mm__, &offset__, &iface_result__) < 0) { \
        goto err; \
    } \
    if (iface_result__ != IFACE_STATUS_OK) { \
        goto err; \
    } \
}

#define DECL_ARG(type_, arg_) \
    ;

#define DECL_STATUS()   \
    DECL_LOCAL(int, status)

#define DECL_LOCAL(type_, name_) \
    type_ name_;

#define OUT_ARG(type_, arg_)    { \
    type##type_ tmp_arg__; \
    if (jumpMessageRead##type_(mm__, &offset__, &tmp_arg__) < 0) { \
        goto err; \
    } \
    *(arg_) = tmp_arg__; \
}

#define OUT_LOCAL(type_, arg_)    \
    OUT_ARG(type_, &arg_)

#define END_VOID() \
    } \
err: \
    return; \
}

#define END(okvalue_, errvalue_) \
        return okvalue_; \
    } \
err: \
    return errvalue_; \
}

#define END_STATUS() \
    OUT_LOCAL(Int, status)    \
    END(status, -1)

#define DECL_FREE_FUNCTION(function_, type_) \
    void D##function_(type_ ptr__) { \
        free((void *)ptr__); \
    }

#include "./jsr135_jumpdriver_interface.incl"

