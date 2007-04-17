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

#include "./jsr120_driver_def.h"

#define START_INTERFACE(jsrNo_, name_)   \
int jsr##jsrNo_##_jump##name_##_listener(JUMPMessage m__) { \
    short id__; \
    JUMPMessageStatusCode code__ = 0; \
    JUMPMessageReader r__;\
    unsigned char par_buf__[2 * JUMP_MESSAGE_BUFFER_SIZE]; \
    unsigned char *par_buf_ptr__ = par_buf__; \
    JUMPOutgoingMessage mm__ = jumpMessageNewOutgoingByRequest(m__, &code__); \
    if (mm__ == NULL) { \
        goto nomem_err; \
    } \
    jumpMessageReaderInit(&r__, m__); \
    id__ = jumpMessageGetShort(&r__); \
    ALIGN(par_buf_ptr__); \
    switch (id__) { 

#define END_INTERFACE() \
    default: \
        return 0; \
    } \
    jumpMessageSendAsyncResponse(mm__, &code__); \
    LOG1("Send back, err code=%d", code__); \
    return 1; \
nomem_err: \
    return 2; \
err: \
    jumpMessageAddByte(mm__, IFACE_STATUS_FAIL); \
    LOG("Send error back"); \
    jumpMessageSendAsyncResponse(mm__, &code__); \
    return 1; \
}

#define SET_CLIENT_ID(clientMsId_, clientHandle_, clientKey_)    {\
    int i; \
    int pid; \
    for (i = 0; i < client_cnt__; i++) { \
        if (client_list__[i].pid == -1) { \
            break; \
        } \
    } \
    if (i == MAX_CLIENTS) { \
        goto err; \
    } \
    pid = (int)jumpMessageGetSender(m__)->processId; \
    client_list__[i].pid = pid; \
    client_list__[i].client_id1 = (int)(clientMsId_); \
    client_list__[i].client_id2 = (int)(clientHandle_); \
    client_list__[i].key = (int)(clientKey_); \
    LOG3("SET_CLIENT_ID() pid=%d, i=%d, handle=%d", pid, i, clientHandle_); \
    clientHandle_ = i + 1; \
    clientMsId_ = pid; \
    if (i >= client_cnt__) { \
        client_cnt__ = i + 1; \
    } \
}

#define CLEAR_CLIENT_ID(clientKey_)  {\
    int i; \
    int pid = (int)jumpMessageGetSender(m__)->processId; \
    for (i = 0; i < client_cnt__; i++) { \
        if (client_list__[i].key == (int)(clientKey_) && \
                client_list__[i].pid == pid) { \
            client_list__[i].pid = -1; \
        } \
    } \
}

#define START(type_, name_, args_)   \
    case ID_##name_: { \
    LOG("calling implementation of " #name_ #args_);

#define START_VOID(name_, args_)   \
    case ID_##name_: { \
    LOG("calling implementation of " #name_ #args_);

#define START_SELF(name_, args_)   \
    case ID_##name_: { \
    LOG("calling implementation of " #name_ #args_ );

#define START_CALLBACK(name_, args_, clientId_)   \
    case ID_##name_: { \
    LOG("calling implementation of " #name_  #args_);


#define ARG(type_, arg_)    {\
    type##type_ tmp_arg__ = (type##type_)jumpMessageGet##type_(&r__); \
    LOG1("read " #type_ " arg. sizeof=%d", sizeof (arg_)); \
    arg_ = tmp_arg__; \
}

#define ARG_ARRAY(type_, arg_, arglen_)     \
    ARG_ARRAY_INTERNAL(arg_, arglen_) \
    ALIGN(par_buf_ptr__); 

#define ARG_ARRAY_INTERNAL(arg_, arglen_)    \
    arg_ = (void *)par_buf_ptr__; \
    if (par_buf_ptr__ + (arglen_) * sizeof *(arg_) >= par_buf__ + sizeof par_buf__) { \
        goto err; \
    } \
    par_buf_ptr__ += (arglen_) * sizeof *(arg_); \
    LOG1("read %d bytes from message into array " #arg_, (arglen_) * sizeof *(arg_)); \
    jumpMessageGetBytesInto(&r__, (int8*)arg_, (arglen_) * sizeof *(arg_)); \

#define ARG_STRING(arg_)    {\
    int len__; \
    len__ = jumpMessageGetInt(&r__); \
    if (len__ == -1) { \
        arg_ = NULL; \
    } else { \
        ARG_ARRAY_INTERNAL(arg_, len__) \
    } \
}
    
#define INVOKE(result_, function_, args_) \
    result_ = function_ args_; \
    jumpMessageAddByte(mm__, IFACE_STATUS_OK);
    
#define INVOKE_VOID(function_, args_) \
    function_ args_; \
    jumpMessageAddByte(mm__, IFACE_STATUS_OK);
    
#define INVOKE_AND_END(function_, args_) \
        LOG("trying to call " #function_); \
        function_ args_; \
        LOG("after calling " #function_);\
        return 1; \
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
    jumpMessageAdd##type_(mm__, tmp_arg__); \
}

#define OUT_ARG_ARRAY(type_, arg_, arrlen_)    \
    jumpMessageAddBytesFrom(mm__, (int8*)(arg_), (arglen_ * sizeof *(arg_)));\

#define OUT_LOCAL_STRUC(type_, struc_, field_)    \
    if (struc_ != NULL) { \
        OUT_ARG(type_, (struc_)->field_) \
    }

#define OUT_LOCAL_STRUC_STRING(struc_, field_)    \
    if (struc_ != NULL) { \
        int len__; \
        if ((struc_)->field_ == NULL) { \
            jumpMessageAddInt(mm__, -1); \
        } else { \
            len__ = strlen((struc_)->field_); \
            jumpMessageAddInt(mm__, len__); \
            OUT_LOCAL_STRUC_ARRAY(Byte, struc_, field_, len__) \
        } \
    }

#define OUT_LOCAL_STRUC_ARRAY(type_, struc_, field_, arrlen_)    \
    if (struc_ != NULL) { \
        jumpMessageAddBytesFrom(mm__, (int8*)((struc_)->field_), (arrlen_ * sizeof *((struc_)->field_)));\
    }

#define OUT_LOCAL(type_, ptr_)    \
    OUT_ARG(type_, ptr_)

#define OUT_LOCAL_ARRAY(type_, arg_, arrlen_)    \
    jumpMessageAddBytesFrom(mm__, (int8*)(arg_), (arglen_ * sizeof *(arg_)));

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
        jumpMessageAddInt(mm__, -1); \
    } else { \
        jumpMessageAddInt(mm__, (int)(size_)); \
    } \
}

#define FREE_STRUC(function_, name_)   \
   /* function_(name_); */

#define DECL_FREE_FUNCTION(function_, type_) 

