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


#define NM_DEBUG
#ifdef NM_DEBUG
extern char *prog_name;
#define LOG(str)  do { \
    fprintf(stderr, "[%s-%d] (%s:%d): %s\n", prog_name, getpid(), \
        __FUNCTION__, __LINE__, (str)); \
    fflush(stderr); \
} while (0)

#define LOG1(fmt, par)  do { \
    char _b[100]; \
    snprintf(_b, sizeof _b, (fmt), (par)); \
    _b[sizeof _b - 1] = '\0'; \
    fprintf(stderr, "[%s-%d] (%s:%d): %s\n", prog_name, getpid(), \
        __FUNCTION__, __LINE__, _b); \
    fflush(stderr); \
} while (0)
#define LOG2(fmt, par1, par2)  do { \
    char _b[100]; \
    snprintf(_b, sizeof _b, (fmt), (par1), (par2)); \
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
#define nmsg_error(s)   do {printf("%s --> exit\n", (s)); exit(1);} while (0)
#endif


#include <jsr120_sms_protocol.h>
#include <jsr120_cbs_protocol.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_cbs_listeners.h>
#include <app_package.h>

#include <stdio.h>
#include <string.h>

#include <JUMPMessages.h>
// #define JSR120_KNI_LAYER
#include <jsr120_jumpdriver.h>

#include <javacall_sms.h>
#include <javacall_cbs.h>

static int driver = -1;

extern struct {
    int pid;
    int client_id1;
    int client_id2;
    int key;
} client_list__[];
extern int client_cnt__;

#define ALIGN_BITS                  64
#define ALIGN_BYTES                 (ALIGN_BITS >> 3)
#define ALIGN_MASK                  (ALIGN_BYTES - 1)
#define ALIGN_IS_ALIGNED(ptr_)      (ALIGN_SHIFT(ptr_) == 0)
#define ALIGN_SHIFT(ptr_)           ((ptr_) & ALIGN_MASK)
#define ALIGN_GAP                   (ALIGN_BYTES - 1)
#define ALIGN(ptr_)                 ((ptr_) = (ALIGN_IS_ALIGNED((int)ptr_) ? (ptr_) : ((ptr_) + ALIGN_SHIFT((int)ptr_))))

#define START_INTERFACE()
#define END_INTERFACE()
#define SET_CLIENT_ID(clientId1_, clientId2_, clientKey_)
#define CLEAR_CLIENT_ID(clientKey_)
#define SET_SERVER_BY_ID(clientHandle_, serverHandle_) {\
    int i = (int)(clientHandle_); \
    int pid; \
	if (i < client_cnt__ && (pid = client_list__[i].pid) != -1) { \
	    /* TODO: multithread issues: maybe mutex is required */ \
        server_pid__ = pid; \
        serverHandle_ = client_list__[i].client_id2; \
	} else { \
        LOG2("bad client handle: handle=%d, cnt=%d", i, client_cnt__); \
	    goto err; \
	} \
}

#define START_INTERNAL(msg_, type_, name_, args_)   \
type_ D##name_ args_ { \
    unsigned char buf[JUMP_MSG_MAX_LENGTH]; \
    JUMPMessage *mm__ = jumpMessageCreateInBuffer(msg_, buf, sizeof buf); \
    int offset__ = 0; \
    int server_pid__; \
    jumpMessageWriteShort(mm__, &offset__, ID_##name_); 

#define SET_SERVER_ID(pid_)    server_pid = pid_;
#define SET_SERVER_DRIVER() {\
    if (driver == -1) { \
        driver = jumpMessageQueueOpen("WMADRIVER"); \
        if (driver == -1) { \
            LOG("cannot find WMADRIVER"); \
            goto err; \
        } \
    } \
    server_pid__ = driver; \
}

#define SET_SERVER_SELF() \
    server_pid__ = jumpProcessGetId();

#define START(type_, name_, args_)   \
    START_INTERNAL("wma/jsr120", type_, name_, args_) \
    SET_SERVER_DRIVER() {

#define START_VOID(name_, args_)   \
    START_INTERNAL("wma/jsr120", void, name_, args_) \
    SET_SERVER_DRIVER() {

#define START_SELF(name_, args_)   \
    START_INTERNAL("wma/jsr120", void, name_, args_) \
    SET_SERVER_SELF() {

#define START_CALLBACK(name_, args_, clientHandle_)   \
    START_INTERNAL("wma/jsr120", void, name_, args_) \
    SET_SERVER_BY_ID(clientHandle_, clientHandle_) {

#define ARG(type_, arg_)    {\
    type##type_ tmp_arg__ = (type##type_) (arg_); \
    jumpMessageWrite##type_(mm__, &offset__, tmp_arg__); \
}

#define ARG_ARRAY(type_, arg_, arglen_)    \
    jumpMessageWrite##type_##Array(mm__, &offset__, arg_, arglen_);

#define ARG_STRING(arg_)    \
    if (arg_ == NULL) { \
        jumpMessageWriteInt(mm__, &offset__, -1); \
    } else { \
        int len__ = strlen(arg_); \
        jumpMessageWriteInt(mm__, &offset__, len__); \
        ARG_ARRAY(Byte, arg_, len__) \
    }

#define INVOKE(result_, function_, arg_)    \
    INVOKE_VOID(function_, arg_)

#define INVOKE_VOID(function_, arg_) {\
    unsigned char iface_result__; \
    mm__ = jumpMessageSendAndWaitForResponse(server_pid__, mm__); \
    if (mm__ == NULL) { \
        LOG("cannot create response message"); \
        goto err; \
    } \
    offset__ = 0; \
    if (jumpMessageReadByte(mm__, &offset__, &iface_result__) < 0) { \
        LOG("cannot read byte"); \
        goto err; \
    } \
    if (iface_result__ != IFACE_STATUS_OK) { \
        LOG("bad result returned"); \
        goto err; \
    } \
}   

#define INVOKE_AND_END(function_, args_) \
    jumpMessageSend(server_pid__, mm__); \
    } \
err: \
    return; \
}
 
#define DECL_ARG(type_, arg_) \
    ;

#define DECL_ARG_STRING(arg_) \
    ;

#define DECL_ARG_ARRAY(type_, arg_, arrlen_) \
    ;

#define DECL_STATUS()   \
    DECL_LOCAL(int, status)

#define DECL_LOCAL(type_, name_) \
    type_ name_;

#define DECL_LOCAL_STRUC(type_, name_) \
    type_ name_; unsigned char *name_##_ptr;

#define STRUC_SIZE(type_, name_, size_, argcnt_) {\
    int size__; \
    if (jumpMessageReadInt(mm__, &offset__, &size__) < 0) { \
        LOG("cannot read structure size"); \
        goto err; \
    } \
    if (size__ != -1) { \
        name_##_ptr = malloc(size__ + argcnt_ * ALIGN_GAP * 2); \
        if (name_##_ptr == NULL) { \
            LOG("no memory"); \
            goto err; \
        } \
        name_ = (type_) name_##_ptr; \
        name_##_ptr += sizeof *name_; \
        ALIGN(name_##_ptr); \
    } else { \
        name_ = NULL; \
    } \
}

#define OUT_ARG(type_, arg_)    { \
    type##type_ tmp_arg__; \
    if (jumpMessageRead##type_(mm__, &offset__, &tmp_arg__) < 0) { \
        LOG("cannot read OUT_ARG from message"); \
        goto err; \
    } \
    *(arg_) = tmp_arg__; \
}

#define OUT_ARG_ARRAY(type_, ptr_, arrlen_)    \
    if (jumpMessageRead##type_##Array(mm__, &offset__, ptr_, arrlen_) < 0) { \
        LOG("cannot read OUT_ARG_ARRAY from message"); \
        goto err; \
    }

#define OUT_LOCAL_STRUC(type_, struc_, field_)    \
    if (struc_ != NULL) { \
        OUT_ARG(type_, &(struc_)->field_) \
    }
    
#define OUT_LOCAL_STRUC_STRING(struc_, field_)    \
    if (struc_ != NULL) { \
        int len__ = -1; \
        if (jumpMessageReadInt(mm__, &offset__, &len__) < 0) { \
            LOG("cannot read length of STRUC_FIELD"); \
            goto err; \
        } \
        if (len__ == -1) { \
            (struc_)->field_ = NULL; \
        } else { \
            (struc_)->field_ = struc_##_ptr; \
            OUT_LOCAL_STRUC_ARRAY_INTERNAL(Byte, struc_, field_, len__) \
            *struc_##_ptr++ = '\0'; \
            ALIGN(struc_##_ptr); \
        } \
    }

#define OUT_LOCAL_STRUC_ARRAY(type_, struc_, field_, fieldlen_)    \
    if (struc_ != NULL) { \
        OUT_LOCAL_STRUC_ARRAY_INTERNAL(type_, struc_, field_, fieldlen_) \
        ALIGN(struc_##_ptr); \
    }


#define OUT_LOCAL_STRUC_ARRAY_INTERNAL(type_, struc_, field_, fieldlen_)    \
    struc_->field_ = (void*)struc_##_ptr; \
    struc_##_ptr += fieldlen_ * sizeof (type##type_); \
    if (jumpMessageRead##type_##Array(mm__, &offset__, struc_->field_, fieldlen_) < 0) { \
        LOG("cannot read array"); \
        goto err; \
    } 

#define OUT_LOCAL(type_, arg_)    \
    OUT_ARG(type_, &arg_)

#define OUT_LOCAL_ARRAY(type_, ptr_, arrlen_)    \
    OUT_ARG_ARRAY(type_, ptr_, arrlen_)

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
    END(status, WMA_ERR)

#define FREE_STRUC(function_, name_)   \
    ;

#define DECL_FREE_FUNCTION(function_, type_) \
    void D##function_(type_ ptr__) { \
        free((void *)ptr__); \
    } 
    
#include "./jsr120_jumpdriver_interface.incl"

