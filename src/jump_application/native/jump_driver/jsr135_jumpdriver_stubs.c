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

#include <jump_messaging.h>
#define JSR135_KNI_LAYER
#include <jsr135_jumpdriver.h>
#include <shared_memory.h>
#include <javacall_multimedia.h>
#include <jsr135_jumpdriver_impl.h>

int jumpProcessRunDriver(char *drvName, char *libName);
static JUMPAddress driver={0};

#define ALIGN_BITS                  64
#define ALIGN_BYTES                 (ALIGN_BITS >> 3)
#define ALIGN_MASK                  (ALIGN_BYTES - 1)
#define ALIGN_IS_ALIGNED(ptr_)      (ALIGN_SHIFT(ptr_) == 0)
#define ALIGN_SHIFT(ptr_)           ((ptr_) & ALIGN_MASK)
#define ALIGN_GAP                   (ALIGN_BYTES - 1)
#define ALIGN(ptr_)                 ((ptr_) = (ALIGN_IS_ALIGNED((int)ptr_) ? (ptr_) : ((ptr_) + ALIGN_SHIFT((int)ptr_))))

#define START_INTERFACE()
#define END_INTERFACE()


#define MM_DRIVER_PID 100

#define START(type_, name_, args_)   \
type_ D##name_ args_ { \
    JUMPMessageStatusCode code;   \
    JUMPMessageReader r;          \
    JUMPOutgoingMessage outMessage = jumpMessageNewOutgoingByType((JUMPPlatformCString)"native/jsr135DriverMain", &code); \
    if (code != JUMP_SUCCESS) { \
        goto err; \
    } \
    jumpMessageAddInt(outMessage, (int32)ID_##name_); \
    {
        
#define ARG(type_, arg_)    \
    jumpMessageAdd##type_(outMessage, arg_);

#define INVOKE(result_, function_, arg_)   \
    INVOKE_VOID(function_, arg_) 

#define INVOKE_VOID(function_, arg_) {\
    JUMPMessage responseMessage;  \
    JUMPMessageStatusCode stcode; \
    if (driver.processId == 0) { \
        driver.processId = jumpProcessRunDriver("jsr135DriverMain", "jsr135"); \
        if (driver.processId == -1) { \
            goto err; \
        } \
        CVMthreadYield(); \
    } \
    responseMessage = jumpMessageSendSync(driver, outMessage, 0, &stcode); \
    if (responseMessage == NULL) { \
        goto err; \
    } \
    jumpMessageReaderInit(&r, responseMessage); \
    if (jumpMessageGetByte(&r) < 0) { \
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
    *(arg_) = jumpMessageGet##type_(&r); \
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
    jumpMessageQueueDestroy(jumpMessageGetReturnTypeName()); \
    END(status, -1)

#define DECL_FREE_FUNCTION(function_, type_) \
    void D##function_(type_ ptr__) { \
        free((void *)ptr__); \
    }

#include "./jsr135_jumpdriver_interface.incl"

