/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

static const JUMPPlatformCString driverMessageType__ = "native/wmaDriver";
static const JUMPPlatformCString jsropMessageType__ = "jsrop/wma";

#define START_INTERFACE(jsrNo_, name_)
#define END_INTERFACE()
#define SET_CLIENT_ID(type_, clientId1_, clientId2_, clientKey_)
#define CLEAR_CLIENT_ID(type_, clientKey_)
#define CLEAR_MS_ID(type_, clientMsId_)
#define SET_SERVER_BY_ID(clientHandle_, serverHandle_) {\
    int i = (int)(clientHandle_) - 1; \
    int pid; \
	if (i>= 0 && i < client_cnt__ && \
            (pid = client_list__[i].pid) != -1) { \
	    /* TODO: multithread issues: maybe mutex is required */ \
        SET_SERVER_ID(pid) \
        serverHandle_ = client_list__[i].client_id2; \
        LOG3("SET_SERVER_BY_ID() pid=%d, i=%d, handle=%d", pid, i, serverHandle_); \
	} else { \
        LOG2("bad client handle: handle=%d, cnt=%d", i, client_cnt__); \
	    goto err; \
	} \
}

#define START_INTERNAL(msg_, type_, name_, args_)   \
type_ D##name_ args_ { \
    JUMPMessageStatusCode code__ = 0; \
    JUMPOutgoingMessage mm__ = jumpMessageNewOutgoingByType(msg_, &code__); \
    JUMPMessage m__; \
    JUMPAddress server_pid__; \
    JUMPMessageReader r__;\
    (void)r__; (void)m__; /* for callback methods */ \
    jumpMessageAddShort(mm__, ID_##name_); \
    LOG("calling stub of " #name_ "(" #args_ ")");

#define SET_SERVER_ID(pid_)    server_pid__.processId = pid_;
#define SET_SERVER_DRIVER() {\
    if (driver == -1) { \
/* TODO: multithread issues: maybe mutex is required */ \
        driver = jumpProcessRunDriver("wmaDriver", "jsr120"); \
        if (driver == -1) { \
            LOG("cannot find WMA driver"); \
            goto err; \
        } \
    } \
    SET_SERVER_ID(driver) \
}

#define SET_SERVER_SELF() \
    SET_SERVER_ID(jumpProcessGetId())

#define START(type_, name_, args_)   \
    START_INTERNAL(driverMessageType__, type_, name_, args_) \
    SET_SERVER_DRIVER() {

#define START_VOID(name_, args_)   \
    START_INTERNAL(driverMessageType__, void, name_, args_) \
    SET_SERVER_DRIVER() {

#define START_SELF(name_, args_)   \
    START_INTERNAL(driverMessageType__, void, name_, args_) \
    SET_SERVER_SELF() {

#define START_CALLBACK(name_, args_, clientHandle_)   \
    START_INTERNAL(jsropMessageType__, void, name_, args_) \
    SET_SERVER_BY_ID(clientHandle_, clientHandle_) {

#define ARG(type_, arg_)    {\
    type##type_ tmp_arg__ = (type##type_) (arg_); \
    jumpMessageAdd##type_(mm__, tmp_arg__); \
}

#define ARG_ARRAY(type_, arg_, arglen_)    \
    jumpMessageAddBytesFrom(mm__, (int8*)(arg_), (arglen_ * sizeof *(arg_)));

#define ARG_STRING(arg_)    \
    if (arg_ == NULL) { \
        jumpMessageAddInt(mm__, -1); \
    } else { \
        int len__ = strlen(arg_); \
        jumpMessageAddInt(mm__, len__); \
        ARG_ARRAY(Byte, arg_, len__) \
    }

#define INVOKE(result_, function_, arg_)    \
    INVOKE_VOID(function_, arg_)

#define INVOKE_VOID(function_, arg_) {\
    unsigned char iface_result__; \
    LOG(#function_ ": before send"); \
    m__ = jumpMessageSendSync(server_pid__, mm__, 0, &code__); \
    LOG(#function_ ": after send"); \
    /* jumpMessageFreeOutgoing(mm__); */ \
    if (code__ != JUMP_SUCCESS || m__ == NULL) { \
        LOG("cannot create message"); \
        goto err; \
    } \
    jumpMessageReaderInit(&r__, m__); \
    iface_result__ = jumpMessageGetByte(&r__); \
    if (iface_result__ != IFACE_STATUS_OK) { \
        LOG("bad result returned"); \
        goto err; \
    } \
}   

#define INVOKE_AND_END(function_, args_) \
    jumpMessageSendAsync(server_pid__, mm__, &code__);\
    /*jumpMessageFreeOutgoing(mm__);*/ \
    if (code__ != JUMP_SUCCESS) {\
        LOG1("cannot create message, error=%d", code__); \
        goto err; \
    } \
    END_VOID()

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
    size__ = jumpMessageGetInt(&r__); \
    if (size__ != -1) { \
        name_##_ptr = malloc(size__ + (argcnt_) * ALIGN_GAP * 2); \
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
    LOG1("STRUC_SIZE: " #type_ " " #name_ " " #size_ " " #argcnt_ "=%d", size__); \
}

#define OUT_ARG(type_, arg_)    { \
    type##type_ tmp_arg__ = (type##type_)jumpMessageGet##type_(&r__); \
    *(arg_) = tmp_arg__; \
    LOG2("OUT_ARG: " #type_ " " #arg_ "=(%d 0x%X)", (int)tmp_arg__, (int)tmp_arg__); \
}

#define OUT_ARG_ARRAY(type_, ptr_, arrlen_)    \
    LOG("OUT_ARG_ARRAY: " #type_ " " #ptr_ " " #arrlen_); \
    jumpMessageGetBytesInto(&r__, (int8*)ptr_, (arrlen_) * sizeof *(ptr_)); \
    LOG_ARRAY(ptr_, arrlen_); 

#define OUT_LOCAL_STRUC(type_, struc_, field_)    \
    LOG("OUT_LOCAL_STRUC: " #type_ " " #struc_ " " #field_); \
    if (struc_ != NULL) { \
        OUT_ARG(type_, &(struc_)->field_) \
    }
    
#define OUT_LOCAL_STRUC_STRING(struc_, field_)    \
    LOG("OUT_LOCAL_STRUC_STRING: " #struc_ " " #field_); \
    if (struc_ != NULL) { \
        int len__ = -1; \
        len__ = jumpMessageGetInt(&r__); \
        if (len__ == -1) { \
            (struc_)->field_ = NULL; \
        } else { \
            OUT_LOCAL_STRUC_ARRAY_INTERNAL(Byte, struc_, field_, len__) \
            *struc_##_ptr++ = '\0'; \
            ALIGN(struc_##_ptr); \
        } \
    }

#define OUT_LOCAL_STRUC_ARRAY(type_, struc_, field_, fieldlen_)    \
    LOG("OUT_LOCAL_STRUC_ARRAY: " #type_ " " #struc_ " " #field_ " " #fieldlen_); \
    if (struc_ != NULL) { \
        OUT_LOCAL_STRUC_ARRAY_INTERNAL(type_, struc_, field_, fieldlen_) \
        ALIGN(struc_##_ptr); \
    }


#define OUT_LOCAL_STRUC_ARRAY_INTERNAL(type_, struc_, field_, fieldlen_)    \
    (struc_)->field_ = (void*)struc_##_ptr; \
    LOG1("OUT_LOCAL_STRUC_ARRAY_INTERNAL: " #fieldlen_ "=%d", (fieldlen_)); \
    LOG1("OUT_LOCAL_STRUC_ARRAY_INTERNAL: size=%d", (fieldlen_) * sizeof *((struc_)->field_)); \
    if ((fieldlen_) > 0) {\
        struc_##_ptr += (fieldlen_) * sizeof (type##type_); \
        jumpMessageGetBytesInto(&r__, (int8*)(struc_)->field_, (fieldlen_) * sizeof *((struc_)->field_)); \
        LOG_ARRAY((int8*)(struc_)->field_, (fieldlen_) * sizeof *((struc_)->field_)); \
    }

#define OUT_LOCAL(type_, arg_)    \
    OUT_ARG(type_, &arg_)

#define OUT_LOCAL_ARRAY(type_, ptr_, arrlen_)    \
    OUT_ARG_ARRAY(type_, ptr_, arrlen_)

#define OUT_SHMEM(buffer_, size_) { \
        char name[MAX_SHMEM_NAME_LEN + 1]; \
        int namelen; \
        CVMSharedMemory *smh; \
        char *mem; \
        JUMPOutgoingMessage kill; \
        namelen = jumpMessageGetInt(&r__); \
        jumpMessageGetBytesInto(&r__, name, namelen); \
        *(name + namelen) = '\0'; \
        smh = CVMsharedMemOpen(name); \
        if (smh == NULL) { \
            goto err; \
        } \
        mem = CVMsharedMemGetAddress(smh); \
        if (mem == NULL) { \
            goto err; \
        } \
        size_ = CVMsharedMemGetSize(smh); \
        memcpy(buffer_, mem, size_); \
        CVMsharedMemClose(smh); \
        kill = jumpMessageNewOutgoingByType(driverMessageType__, &code__); \
        jumpMessageAddShort(kill, ID_KILL_SHMEM); \
        jumpMessageAddInt(kill, namelen); \
        jumpMessageAddBytesFrom(kill, name, namelen); \
        jumpMessageSendAsync(server_pid__, kill, &code__); \
    }

#define END_VOID() \
        return; \
    } \
err: \
    if (server_pid__.processId == driver) { \
        driver = -1; \
    } \
    return; \
}
    
#define END(okvalue_, errvalue_) \
        LOG2("return ok %s=%d", #okvalue_, (int)(okvalue_)); \
        return okvalue_; \
    } \
err: \
    if (server_pid__.processId == driver) { \
        driver = -1; \
    } \
    LOG2("return error %s=%d", #errvalue_, (int)(errvalue_)); \
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

