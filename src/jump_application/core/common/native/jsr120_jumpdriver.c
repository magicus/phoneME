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

#include <stdio.h>
#include <string.h>

#include <jsr120_sms_protocol.h>
#include <jsr120_cbs_protocol.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_cbs_listeners.h>
#include <app_package.h>

#include <JUMPMessages.h>
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

#define MAX_CLIENTS         30
struct {
        int pid;
        int client_id1;
        int client_id2;
        int key;
} client_list__[MAX_CLIENTS];
int client_cnt__ = 0;


#define START_INTERFACE()   \
void jsr120_jumpdriver_listener(JUMPMessage *m__, jmpMessageQueue queue__, void *context__) { \
    int offset__ = 0; \
    short id__; \
    unsigned char par_buf__[2 * JUMP_MSG_MAX_LENGTH]; \
    unsigned char *par_buf_ptr__ = par_buf__; \
    unsigned char buf__[JUMP_MSG_MAX_LENGTH]; \
    JUMPMessage *mm__ = jumpMessageResponseInBuffer(m__, buf__, sizeof buf__); \
    if (mm__ == NULL) { \
        goto nomem_err; \
    } \
    if (jumpMessageReadShort(m__, &offset__, &id__) < 0) { \
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

#define SET_CLIENT_ID(clientMsId_, clientHandle_, clientKey_)    {\
    int i; \
    for (i = 0; i < client_cnt__; i++) { \
        if (client_list__[i].pid == -1) { \
            break; \
        } \
    } \
    if (i == MAX_CLIENTS) { \
        goto err; \
    } \
    client_list__[i].pid = (int)m__->senderProcessId; \
    client_list__[i].client_id1 = (int)(clientMsId_); \
    client_list__[i].client_id2 = (int)(clientHandle_); \
    client_list__[i].key = (int)(clientKey_); \
    clientHandle_ = i; \
    clientMsId_ = m__->senderProcessId; \
    if (i >= client_cnt__) { \
        client_cnt__ = i + 1; \
    } \
}

#define CLEAR_CLIENT_ID(clientKey_)  {\
    int i; \
    for (i = 0; i < client_cnt__; i++) { \
        if (client_list__[i].key == (int)(clientKey_) && \
                client_list__[i].pid == (int)m__->senderProcessId) { \
            client_list__[i].pid = -1; \
        } \
    } \
}

#define START(type_, name_, args_)   \
    case ID_##name_: {

#define START_VOID(name_, args_)   \
    case ID_##name_: {

#define START_SELF(name_, args_)   \
    case ID_##name_: {

#define START_CALLBACK(name_, args_, clientId_)   \
    case ID_##name_: {

#define ARG(type_, arg_)    {\
    type##type_ tmp_arg__; \
    if (jumpMessageRead##type_(m__, &offset__, &tmp_arg__) < 0) { \
        goto err; \
    } \
    arg_ = tmp_arg__; \
}

#define ARG_ARRAY(type_, arg_, arglen_)     \
    ARG_ARRAY_INTERNAL(type_, arg_, arglen_) \
    ALIGN(par_buf_ptr__); 

#define ARG_ARRAY_INTERNAL(type_, arg_, arglen_)    \
    arg_ = (void *)par_buf_ptr__; \
    if (par_buf_ptr__ + (arglen_) * sizeof *arg_ >= par_buf__ + sizeof par_buf__) { \
        goto err; \
    } \
    par_buf_ptr__ += (arglen_) * sizeof *arg_; \
    if (jumpMessageRead##type_##Array(m__, &offset__, arg_, arglen_) < 0) { \
        goto err; \
    } 

#define ARG_STRING(arg_)    {\
    int len__; \
    if (jumpMessageReadInt(m__, &offset__, &len__) < 0) { \
        goto err; \
    } \
    if (len__ == -1) { \
        arg_ = NULL; \
    } else { \
        ARG_ARRAY_INTERNAL(Byte, arg_, len__) \
    } \
}
    
#define INVOKE(result_, function_, args_) \
    result_ = function_ args_; \
    offset__ = 0; \
    jumpMessageWriteByte(mm__, &offset__, IFACE_STATUS_OK);
    
#define INVOKE_VOID(function_, args_) \
    function_ args_; \
    offset__ = 0; \
    jumpMessageWriteByte(mm__, &offset__, IFACE_STATUS_OK);
    
#define INVOKE_AND_END(function_, args_) \
        function_ args_; \
        return; \
    } /* end of case */
    
#define DECL_ARG(type_, arg_) \
    type_ arg_;

#define DECL_ARG_STRING(arg_) \
    char *arg_;

#define DECL_ARG_ARRAY(type_, arg_, arrlen_) \
    type_ * arg_;

#define DECL_LOCAL(type_, name_) \
    type_ name_;

#define DECL_LOCAL_STRUC(type_, name_) \
    type_ name_;

#define DECL_STATUS()   \
    DECL_LOCAL(int, status)

#define OUT_ARG(type_, arg_)    {\
    type##type_ tmp_arg__ = (type##type_)arg_; \
    jumpMessageWrite##type_(mm__, &offset__, tmp_arg__); \
}

#define OUT_ARG_ARRAY(type_, arg_, arrlen_)    \
    jumpMessageWrite##type_##Array(mm__, &offset__, arg_, arglen_);

#define OUT_LOCAL_STRUC(type_, struc_, field_)    \
    if (struc_ != NULL) { \
        OUT_ARG(type_, (struc_)->field_) \
    }

#define OUT_LOCAL_STRUC_STRING(struc_, field_)    \
    if (struc_ != NULL) { \
        int len__; \
        if ((struc_)->field_ == NULL) { \
            jumpMessageWriteInt(mm__, &offset__, -1); \
        } else { \
            len__ = strlen((struc_)->field_); \
            jumpMessageWriteInt(mm__, &offset__, len__); \
            OUT_LOCAL_STRUC_ARRAY(Byte, struc_, field_, len__) \
        } \
    }

#define OUT_LOCAL_STRUC_ARRAY(type_, struc_, field_, arrlen_)    \
    if (struc_ != NULL) { \
        jumpMessageWrite##type_##Array(mm__, &offset__, (struc_)->field_, arrlen_); \
    }

#define OUT_LOCAL(type_, ptr_)    \
    OUT_ARG(type_, ptr_)

#define OUT_LOCAL_ARRAY(type_, arg_, arrlen_)    \
    jumpMessageWrite##type_##Array(mm__, &offset__, arg_, arglen_);

#define END_VOID() \
        break; \
    } /* end of case */
    
#define END(okvalue_, errvalue_) \
    END_VOID()

#define END_STATUS() \
    OUT_LOCAL(Int, status)    \
    END(status, WMA_ERR)

#define STRUC_SIZE(type_, name_, size_, argcnt_) {\
    if (name_ == NULL) { \
        jumpMessageWriteInt(mm__, &offset__, -1); \
    } else { \
        jumpMessageWriteInt(mm__, &offset__, size_); \
    } \
}

#define FREE_STRUC(function_, name_)   \
    function_(name_);

#define DECL_FREE_FUNCTION(function_, type_) 

#include "./jsr120_jumpdriver_interface.incl"

