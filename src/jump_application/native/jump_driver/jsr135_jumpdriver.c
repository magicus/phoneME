/*
 * 
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

#include <kni.h>
//#include <app_package.h>

#include <JUMPMessages.h>
#define JSR135_KNI_LAYER
#include <jsr135_jumpdriver.h>
#include <shared_memory.h>
#include <javacall_multimedia.h>
#include <jsr135_jumpdriver_impl.h>
    

#define ALIGN_BITS                  64
#define ALIGN_BYTES                 (ALIGN_BITS >> 3)
#define ALIGN_MASK                  (ALIGN_BYTES - 1)
#define ALIGN_IS_ALIGNED(ptr_)      (ALIGN_SHIFT(ptr_) == 0)
#define ALIGN_SHIFT(ptr_)           ((ptr_) & ALIGN_MASK)
#define ALIGN(ptr_)                 ((ptr_) = (ALIGN_IS_ALIGNED((int)ptr_) ? (ptr_) : ((ptr_) + ALIGN_SHIFT((int)ptr_))))

#define START_INTERFACE()   \
void jsr135_jumpdriver_listener(JUMPMessage *m__, jmpMessageQueue queue__, void *context__) { \
    int offset__ = 0; \
    int id__; \
    unsigned char par_buf__[2 * JUMP_MSG_MAX_LENGTH]; \
    unsigned char *par_buf_ptr__ = par_buf__; \
    unsigned char buf__[JUMP_MSG_MAX_LENGTH]; \
    JUMPMessage *mm__ = jumpMessageResponseInBuffer(m__, buf__, sizeof buf__); \
    if (mm__ == NULL) { \
        goto nomem_err; \
    } \
    if (jumpMessageReadInt(m__, &offset__, &id__) < 0) { \
        goto err; \
    } \
    ALIGN(par_buf_ptr__); \
    (void)queue__; \
    (void)context__; \
    switch (id__) {

#define END_INTERFACE() \
    default: \
        goto err; \
        break; \
    } \
    jumpMessageSend(m__->senderProcessId, mm__); \
    return; \
nomem_err: \
    return; \
err: \
    offset__ = 0; \
    jumpMessageWriteByte(mm__, &offset__, IFACE_STATUS_FAIL); \
    jumpMessageSend(m__->senderProcessId, mm__); \
}

#define START(type_, name_, args_)   \
    case ID_##name_: {

#define START_VOID(name_, args_)   \
    case ID_##name_: {

#define ARG(type_, arg_)    \
    if (jumpMessageRead##type_(m__, &offset__, &arg_) < 0) { \
        goto err; \
    }

#define INVOKE(result_, function_, args_) \
    result_ = function_ args_; \
    offset__ = 0; \
    jumpMessageWriteByte(mm__, &offset__, IFACE_STATUS_OK);

#define INVOKE_VOID(function_, args_) \
    function_ args_; \
    offset__ = 0; \
    jumpMessageWriteByte(mm__, &offset__, IFACE_STATUS_OK);

#define DECL_ARG(type_, arg_) \
    type_ arg_;

#define DECL_LOCAL(type_, name_) \
    type_ name_;

#define DECL_STATUS()   \
    DECL_LOCAL(int, status)

#define OUT_ARG(type_, arg_)    {\
    type##type_ tmp_arg__ = (type##type_)arg_; \
    jumpMessageWrite##type_(mm__, &offset__, tmp_arg__); \
}

#define OUT_LOCAL(type_, ptr_)    \
    OUT_ARG(type_, ptr_)

#define END_VOID() \
        break; \
    } /* end of case */

#define END(okvalue_, errvalue_) \
    END_VOID()

#define END_STATUS() \
    OUT_LOCAL(Int, status)    \
    END(status, -1)

#define DECL_FREE_FUNCTION(function_, type_)

#include "./jsr135_jumpdriver_interface.incl"

