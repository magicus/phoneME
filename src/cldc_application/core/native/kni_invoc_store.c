/*
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
 */

/**
 * @file
 * @brief This is the MIDP stack implementation for in-memory storage
 * of Invocation requests and responses.
 *
 * @defgroup invocStore Invocation Storage
 *
 * invocStore.c contains the implementation for in-memory storage of
 * InvocationImpl requests. 
 * The native heap is used to store the Invocations that are pending.
 * If the VM is restarted in the same process space; the invocations are
 * retained. This is used in SVM mode where the VM must be restarted
 * to process the next invocation.  In MVM mode, the invocations are
 * communicated across Isolates through this native storage.
 * <br>
 */

#include <kni.h>
#include <sni.h>

#include <pcsl_string.h>

#include <midpUtilKni.h>
#include <midpError.h>
#include <midp_logging.h>
#include <midpServices.h>
#include <midpMalloc.h>
#include <suitestore_common.h>
#include <jsr211_invoc.h>
#include <jsr211_platform_invoc.h>
#include <jsrop_memory.h>
#include <jsrop_kni.h>
#include <jsrop_suitestore.h>
#include <javautil_unicode.h>
#include <javacall_memory.h>

#ifdef _DEBUG
#define DEBUG_211
#define DEBUG_INVOCLC
#define TRACE_INVOCFIND
#define TRACE_MIDLETREG
#define TRACE_BLOCKING
#endif

/*
 * The mode for get to retrieve a new request.
 */
#define MODE_REQUEST 0

/* The mode for get to retrieve a new response. */
#define MODE_RESPONSE 1

/* The mode for get to retrieve a new cleanup. */
#define MODE_CLEANUP 2

/* The mode for listen for a new unnotified request. */
#define MODE_LREQUEST 3

/* The mode for listen for a new unnotified response. */
#define MODE_LRESPONSE 4

/* The mode for get to retrieve a new ACTIVE, HOLD, or WAITING request. */
/* #define MODE_PENDING 5 */

/* The mode for get to retrieve by <code>tid</code>. */
#define MODE_TID 6

/* The mode to get the Invocation after <code>tid</code>. */
#define MODE_TID_NEXT 7

/* The mode to get the Invocation before <code>tid</code>. */
#define MODE_TID_PREV 8

/**
 * A double linked list head used to store invocations.
 */
typedef struct _StoredLink {
    struct _StoredLink* flink; /**< The forward link */
    struct _StoredLink* blink;    /**< The backward link */
    struct _StoredInvoc* invoc;    /**< The stored invocation */
} StoredLink;

static int copyOut(StoredInvoc *invoc, int mode, 
           jobject invocObj, jobject argsObj, jobject obj);
static void removeEntry(StoredLink *entry);

/* Function to free memory for a StoredInvoc. */
static void invocFree(StoredInvoc* stored);
/* Function to put a new entry in the queue. */
static jboolean invocPut(StoredInvoc* invoc);

static StoredLink* invocFind(SuiteIdType suiteId, 
                   const pcsl_string* classname, int mode);
static StoredLink* invocFindTid(int tid);
static int invocNextTid();
static jboolean modeCheck(StoredInvoc* invoc, int mode);
static void blockThread();
static void unblockWaitingThreads(int newStatus);
static jboolean isThreadCancelled();

#define isEmpty() (invocQueue == NULL)

/*
 * Head of queue of stored Invocations.
 */
static StoredLink* invocQueue = NULL;

/*
 * Transaction ID of next transaction. Acts as virtual time.
 * Incremented on every put.
 * Must be non-zero, but doesn't matter where it starts
 */
static int nextTid = 0;

/*
 * The cached fieldIDs for each field of the InvocationImpl class
 */
static jfieldID urlFid;
static jfieldID typeFid;
static jfieldID actionFid;
static jfieldID IDFid;
static jfieldID argumentsFid;
static jfieldID argsLenFid;
static jfieldID dataFid;
static jfieldID dataLenFid;
static jfieldID responseRequiredFid;
static jfieldID usernameFid;
static jfieldID passwordFid;
static jfieldID tidFid;
static jfieldID previousTidFid;
static jfieldID suiteIdFid;
static jfieldID classnameFid;
static jfieldID statusFid;
static jfieldID invokingAuthorityFid;
static jfieldID invokingAppNameFid;
static jfieldID invokingSuiteIdFid;
static jfieldID invokingClassnameFid;
static jfieldID invokingIDFid;

/**
 * Cache the fieldIDs for each fields of the InvocationImpl class.
 * Initialize the invocation queue head.
 * @param invocObj the invocation object
 * @param classObj the Class object of Invocation
 */
static void init(jobject invocObj, jclass classObj) {
    if (urlFid != 0)
        return;

    KNI_GetObjectClass(invocObj, classObj);

    /* Get these field IDs from InvocationImpl */
    tidFid = KNI_GetFieldID(classObj, "tid", "I" );
    previousTidFid = KNI_GetFieldID(classObj, "previousTid", "I" );
    suiteIdFid = KNI_GetFieldID(classObj, "suiteId", "I");
    classnameFid = KNI_GetFieldID(classObj, "classname", "Ljava/lang/String;");
    statusFid = KNI_GetFieldID(classObj, "status", "I");
    invokingAuthorityFid = KNI_GetFieldID(classObj, "invokingAuthority", "Ljava/lang/String;");
    invokingAppNameFid = KNI_GetFieldID(classObj, "invokingAppName", "Ljava/lang/String;");
    invokingSuiteIdFid = KNI_GetFieldID(classObj, "invokingSuiteId", "I");
    invokingClassnameFid = KNI_GetFieldID(classObj, "invokingClassname", "Ljava/lang/String;");
    invokingIDFid = KNI_GetFieldID(classObj, "invokingID", "Ljava/lang/String;");

   /*
    * Get the rest of the fields from Invocation (the InvocationImpl superclass)
    *    KNI_GetSuperClass(classObj, classObj);
    */
    urlFid = KNI_GetFieldID(classObj, "url", "Ljava/lang/String;");
    typeFid = KNI_GetFieldID(classObj, "type", "Ljava/lang/String;");
    actionFid = KNI_GetFieldID(classObj, "action", "Ljava/lang/String;");
    IDFid = KNI_GetFieldID(classObj, "ID", "Ljava/lang/String;");
    argumentsFid = KNI_GetFieldID(classObj, "arguments", "[Ljava/lang/String;");
    argsLenFid = KNI_GetFieldID(classObj, "argsLen", "I");
    dataFid = KNI_GetFieldID(classObj, "data", "[B");
    dataLenFid = KNI_GetFieldID(classObj, "dataLen", "I");
    responseRequiredFid = KNI_GetFieldID(classObj, "responseRequired", "Z");
    usernameFid = KNI_GetFieldID(classObj, "username", "Ljava/lang/String;");
    passwordFid = KNI_GetFieldID(classObj, "password", "Ljava/lang/String;");

#if REPORT_LEVEL <= LOG_CRITICAL
    if (urlFid == 0 ||
        typeFid == 0 ||
        actionFid == 0 ||
        IDFid == 0 ||
        statusFid == 0 ||
        responseRequiredFid == 0 ||
        tidFid == 0 ||
        previousTidFid == 0 ||
        suiteIdFid == 0 ||
        classnameFid == 0 ||
        invokingAuthorityFid == 0 ||
        invokingAppNameFid == 0 ||
        invokingSuiteIdFid == 0 ||
        invokingClassnameFid == 0 ||
        invokingIDFid == 0 ||
        usernameFid == 0 ||
        passwordFid == 0 ||
        argumentsFid == 0 ||
        argsLenFid == 0 ||
        dataLenFid == 0 ||
        dataFid == 0) {
        REPORT_CRIT(LC_NONE, "invocStore.c: fieldID initialization failed");
    }
#endif
}

/**
 * Helper function to initialize fields from Strings.
 * @param str string to store
 * @param object into which it should be stored
 * @param fieldid of field to store the jstring
 * @return false if the String could not allocate
 */
static jboolean storeField(const pcsl_string* str,
               jobject invocObj,
               jfieldID fid,
               jobject tmpobj) {
    if (str == NULL || pcsl_string_is_null(str)) {
        KNI_ReleaseHandle(tmpobj);
    } else {
        if (PCSL_STRING_OK != midp_jstring_from_pcsl_string(str, tmpobj)) {
            return KNI_FALSE;
        }
    }
    KNI_SetObjectField(invocObj, fid, tmpobj);
    return KNI_TRUE;
}

/**
 * Extract the parameters ID, Type, URL, arguments and data
 * and update/copy their values to the InvocStore instance.
 * @param invoc the StoreInvoc to update
 * @param invocObj the Java invoc instance
 * @param tmp1 a temporary object
 * @param tmp2 a 2nd temporary object 
 * @return TRUE if all the allocations and modifications worked
 */
static jboolean setParamsFromObj(StoredInvoc* invoc,
                     jobject invocObj,
                     jobject tmp1, jobject tmp2) {
    jboolean ret = KNI_ENOMEM;    /* Assume failure */
    do {
        /* On any error break out of this block */
        int len;
        pcsl_string* args;
    
        KNI_GetObjectField(invocObj, urlFid, tmp1);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(tmp1, &invoc->url))
            break;
    
        KNI_GetObjectField(invocObj, typeFid, tmp1);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(tmp1, &invoc->type ))
            break;
    
        KNI_GetObjectField(invocObj, actionFid, tmp1);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(tmp1, &invoc->action ))
            break;
    
        KNI_GetObjectField(invocObj, IDFid, tmp1);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(tmp1, &invoc->ID))
            break;
    
        /*
         * Copy the arguments if non-empty.
         * Always keep the pointers safe so invocFree()
         * can function correctly.
         */
        KNI_GetObjectField(invocObj, argumentsFid, tmp2);
        len = invoc->argsLen;
        invoc->argsLen = (KNI_IsNullHandle(tmp2)? 0: KNI_GetArrayLength(tmp2));

        if (len != invoc->argsLen) {
            if (invoc->args != NULL) {
                args = invoc->args;
                while (len-- > 0) {
                    pcsl_string_free(args++);
                }
                JAVAME_FREE(invoc->args);
                invoc->args = NULL;
            }
            len = invoc->argsLen;
            if (len > 0) {
                invoc->args = (pcsl_string*)JAVAME_CALLOC(len, sizeof(pcsl_string));
	            if (invoc->args == NULL)
		            break;
            }
        }

        args = invoc->args;
        if (len > 0) {
            args += len;
            while (len-- > 0) {
                KNI_GetObjectArrayElement(tmp2, len, tmp1);
                if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(tmp1, --args))
                    break;
            }
        }
    
        /* Copy any data from the Invocation to malloc's memory. */
        KNI_GetObjectField(invocObj, dataFid, tmp2);
        len = (KNI_IsNullHandle(tmp2)? 0: KNI_GetArrayLength(tmp2));

        if (invoc->dataLen != len) {
            if (invoc->data != NULL) {
                JAVAME_FREE(invoc->data);
                invoc->data = NULL;
            }
            if (len > 0) {
	            invoc->data = JAVAME_MALLOC(len);
	            if (invoc->data == NULL)
		            break;
            }
            invoc->dataLen = len;
        }
        
        if (len > 0) {
            KNI_GetRawArrayRegion(tmp2, 0, len, invoc->data);
        }
    
        /* Clear to indicate everything worked. */
        ret = KNI_TRUE;
    } while (0);

    return ret;
}

/**
 * 
 * Gets an InvocationImpl from the store using a MIDlet suiteId
 * and optional classname.
 * Getting an Invocation from the store removes it from the store.
 * If an OutOfMemory exception is thrown the matched Invocation
 * is NOT removed from the queue. If the heap memory can be
 * replenished then the operation can be retried.
 *
 * @param invoc an Invocation Object to fill in
 * @param suiteId to match a pending invocation
 * @param classname to match a pending invocation
 * @param mode one of {@link #MODE_REQUEST}, {@link #MODE_RESPONSE},
 *    or {@link #MODE_CLEANUP}
 * @param blocking true to block until a matching invocation is available
 * @return 1 if a matching invocation was found and returned 
 *    in its entirety; zero if there was no matching invocation;
 *    -1 if the sizes of the arguments or parameter array were wrong
 * @see StoredInvoc
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_j2me_content_InvocationStore_get0(void) {
    int ret = 0;          /* return value = nothing matched */
    KNI_StartHandles(4);
    KNI_DeclareHandle(obj);      /* multipurpose handle */
    KNI_DeclareHandle(argsObj);      /* handle for argument array */
    KNI_DeclareHandle(invocObj);  /* Arg1: Invocation object; non-null */
    KNI_DeclareHandle(classname); /* Arg3: non-null classname */
    int mode = MODE_REQUEST;      /* Arg4: mode for get */
    jboolean blocking = KNI_FALSE; /* Arg5: true if should block */

    /* Argument indices must match Java native method declaration */
#define getInvokeObjArg 1
#define getSuiteIdArg 2
#define getClassnameArg 3
#define getModeArg 4
#define getBlockingArg 5

    StoredLink* match = NULL;

    SuiteIdType desiredSuiteId;
    pcsl_string desiredClassname = PCSL_STRING_NULL_INITIALIZER;

    do {/* Block to break out of on exceptions */
        /* Check if blocked invocation was cancelled. */
        if (isThreadCancelled()) {
            /* blocking is always false to cleanup and exit immediately */
            break;
        }
    
        /* Get the desired blocking mode. */
        blocking = KNI_GetParameterAsBoolean(getBlockingArg);
    
        if (!isEmpty()) {
            /* Queue is not empty, get InvocationImpl obj and init. */
            KNI_GetParameterAsObject(getInvokeObjArg, invocObj);
            init(invocObj, obj);
    
            /* Get the desired type of invocation. */
            mode = KNI_GetParameterAsInt(getModeArg);
            if (mode == MODE_TID || mode == MODE_TID_NEXT || mode == MODE_TID_PREV) {
                int tid = KNI_GetIntField(invocObj, tidFid);
                if (tid != 0) {
                    match = invocFindTid(tid);
                    if (match != NULL) {
                        /* Link to the next or previous depending on the mode. */
                        if (mode == MODE_TID_NEXT) {
                            match = match->flink;
                        } else if (mode == MODE_TID_PREV) {
                            match = match->blink;
                        }
                    }
                } else {
                    /* start with the root */
                    match = invocQueue;
                }
            } else {
                desiredSuiteId = KNI_GetParameterAsInt(getSuiteIdArg);
                KNI_GetParameterAsObject(getClassnameArg, classname);
                if (PCSL_STRING_OK != 
                        midp_jstring_to_pcsl_string(classname, &desiredClassname)) {
                    KNI_ThrowNew(midpOutOfMemoryError, 
                       "InvocationStore_get0 no memory for [desiredClassname]");
                    break;
                }
                match = invocFind(desiredSuiteId, &desiredClassname, mode);
            }
        }
    } while (KNI_FALSE);

    /* Always free strings allocated */
    pcsl_string_free(&desiredClassname);

    if (match != NULL) {
        StoredInvoc *invoc = match->invoc;
        int st = copyOut(invoc, mode, invocObj, argsObj, obj);
        switch (st) {
        case 1:
            /* If a match was found and successfully copied;
             * do final set of actions on the Invocation selected.
             */
            switch (mode) {
            case MODE_REQUEST:
                if (invoc->status == STATUS_WAITING) {
                    /* 
                     * Returning new request, change status to ACTIVE
                     * Keep this entry in the queue.
                     */
                    invoc->status = STATUS_ACTIVE;
                    KNI_SetIntField(invocObj, statusFid, invoc->status);
                }
                break;
            case MODE_RESPONSE:
                if (invoc->status >= STATUS_OK && invoc->status <= STATUS_INITIATED) {
                    /*
                     * Remove responses from the list and free.
                     */
                    removeEntry(match);
                    invocFree(invoc);
                }
                break;
            case MODE_LREQUEST:
            case MODE_LRESPONSE:
            case MODE_CLEANUP:
            case MODE_TID:
            case MODE_TID_NEXT:
            case MODE_TID_PREV:
            default:
                /* No additional action */
                break;
            }
            /* Returning an Invocation */
            ret = 1;
            break;
        case 0:
            /* Insufficient memory for strings. */
            KNI_ThrowNew(midpOutOfMemoryError, "invocStore returning strings");
            KNI_ReleaseHandle(invocObj);
            ret = 0;
            break;
        case -1:
            /* Either args array or data array is incorrect size. */
            ret = -1;
        }
    } else {
        /* No match found. */
        /* If blocking, setup to block. */
        if (blocking) {
            blockThread();
            /* Fall into the return to manage handles correctly */
        }
        ret = 0;
    }
    KNI_EndHandles();
    KNI_ReturnInt(ret);
}

/**
 * Copy a native Invocation to the supplied Invocation instance.
 * @param invoc the native InvocStore 
 * @param mode the mode of copyout
 * @param invocObj the Invocation object to copy to
 * @param argsObj an object to use to refer to the arguments array
 * @param obj a temporary object handle
 * @return 0 if there were problems allocating Java Strings;
 *    1 if all the copies succeeded;
 *    -1 if the Java Arrays allocated for args or data were insufficient
 */
static int copyOut(StoredInvoc *invoc, int mode, 
            jobject invocObj, jobject argsObj, jobject obj)
{
    int datalen = 0;
    int arraylen = 0;

    /* Set the required lengths for args and data arrays. */
    KNI_SetIntField(invocObj, argsLenFid, invoc->argsLen);
    KNI_SetIntField(invocObj, dataLenFid, invoc->dataLen);

    /* Check if size of argument array and data array are correct. */
    KNI_GetObjectField(invocObj, dataFid, obj);
    datalen = KNI_GetArrayLength(obj);
    if (datalen != invoc->dataLen) {
        /* Data array allocated by Java is not correct size. */
        return -1;
    }
    KNI_GetObjectField(invocObj, argumentsFid, obj);
    arraylen = KNI_GetArrayLength(obj);
    if (arraylen != invoc->argsLen) {
        /* Args array allocated by Java is not correct size. */
        return -1;
    }

    /* Copy out all the string fields. */
    if (!(storeField(&invoc->url, invocObj, urlFid, obj) &&
          storeField(&invoc->type, invocObj, typeFid, obj) &&
          storeField(&invoc->action, invocObj, actionFid, obj) &&
          storeField(&invoc->ID, invocObj, IDFid, obj) &&
          storeField(&invoc->invokingClassname, invocObj, invokingClassnameFid, obj) &&
          storeField(&invoc->invokingAuthority, invocObj, invokingAuthorityFid, obj) &&
          storeField(&invoc->invokingAppName, invocObj, invokingAppNameFid, obj) &&
          storeField(&invoc->invokingID, invocObj, invokingIDFid, obj) &&
          storeField(&invoc->username, invocObj, usernameFid, obj) &&
          storeField(&invoc->password, invocObj, passwordFid, obj))) {
        /* Some String allocation failed. */
        return 0;
    }
    KNI_SetIntField(invocObj, invokingSuiteIdFid, invoc->invokingSuiteId);
    /* See if the suite and classname are needed. */
    if (mode == MODE_TID || mode == MODE_TID_PREV || mode == MODE_TID_NEXT) {
        if (!storeField(&invoc->classname, invocObj, classnameFid, obj)) {
            /* A string allocation failed. */
            return 0;
        }
        KNI_SetIntField(invocObj, suiteIdFid, invoc->suiteId);
    }

    /* Return the arguments if any; array length already checked. */
    if (invoc->argsLen > 0) {
        int ndx;
        pcsl_string* args;
    
        /* For each stored arg create a string and store in the array.
         * No stored arg is null.  If a string cannot be created
         * it is due to insufficient heap memory. 
         */
        KNI_GetObjectField(invocObj, argumentsFid, argsObj);
        args = invoc->args;
        for (ndx = 0; ndx < invoc->argsLen; ndx++, args++) {
            if (!pcsl_string_is_null(args)) {
                if (PCSL_STRING_OK != 
                        midp_jstring_from_pcsl_string(args, obj)) {
                    /* String create failed; exit now. */
                    return 0;
                }
            } else {
                KNI_ReleaseHandle(obj);
            }
            KNI_SetObjectArrayElement(argsObj, ndx, obj);
        }
    }

    /* Return the data array if any; array length was already checked. */
    if (invoc->dataLen > 0) {
        KNI_GetObjectField(invocObj, dataFid, obj);
        KNI_SetRawArrayRegion(obj, 0, invoc->dataLen, invoc->data);
    }

    KNI_SetBooleanField(invocObj, responseRequiredFid,
            invoc->responseRequired);
    KNI_SetIntField(invocObj, statusFid, invoc->status);
    KNI_SetIntField(invocObj, tidFid, invoc->tid);
    KNI_SetIntField(invocObj, previousTidFid, invoc->previousTid);
    /* Successful copy out. */
    return 1;
}

/**
 * 
 * Listens for  an InvocationImpl from the store using a MIDlet suiteId
 * and optional, and status. Each Invocation must be returned only
 * once.  When an Invocation is returned; it is marked as being
 * notified.
 *
 * @param suiteId to match a pending invocation
 * @param classname to match a pending invocation
 * @param mode one of {@link #MODE_REQUEST}, {@link #MODE_RESPONSE},
 *    or {@link #MODE_CLEANUP}
 * @param blocking true to block until a matching invocation is available
 * @return true if an Invocation is found with 
 *  the same MIDlet suiteId and classname; false is returne dotherwise
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_j2me_content_InvocationStore_listen0(void) {
    StoredLink* match = NULL;
    SuiteIdType desiredSuiteId;
    pcsl_string desiredClassname = PCSL_STRING_NULL_INITIALIZER;

    KNI_StartHandles(1);
    KNI_DeclareHandle(classname); /* Arg2: non-null classname */
    int mode;                  /* Arg3: requested invocation mode */
    jboolean blocking = KNI_FALSE; /* Arg4: true if should block */

    /* Argument indices must match Java native method declaration */
#define listenSuiteIdArg 1
#define listenClassnameArg 2
#define listenModeArg 3
#define listenBlockingArg 4

    do {/* Block to break out of on exceptions */
        /* Check if blocked invocation was cancelled. */
        if (isThreadCancelled()) {
            /* blocking is always false to cleanup and exit immediately */
            break;
        }
    
        /* Get the desired blocking mode. */
        blocking = KNI_GetParameterAsBoolean(listenBlockingArg);
    
        if (!isEmpty()) {
            /* Queue is not empty
             * Need a string copy of the desired suiteID and classname
             * to use for comparisons
             */
            desiredSuiteId = KNI_GetParameterAsInt(listenSuiteIdArg);
            KNI_GetParameterAsObject(listenClassnameArg, classname);
            if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(classname, &desiredClassname)) {
                KNI_ThrowNew(midpOutOfMemoryError, 
                    "InvocationStore_listen0 no memory for [desiredClassname]");
                break;
            }

            /* Get the desired request mode. */
            mode = KNI_GetParameterAsInt(listenModeArg);
            match = invocFind(desiredSuiteId, &desiredClassname, mode);
        }
    } while (KNI_FALSE);

    /* Always free strings allocated */
    pcsl_string_free(&desiredClassname);

    if (match != NULL) {
        match->invoc->notified = KNI_TRUE;
    } else {
        if (blocking) {
            /* No found; block the thread in the VM */
            blockThread();
            /* Fall into the return to manage handles correctly */
        }
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(match != NULL);
}

/**
 * 
 * Resets the request or response flags for listener notification.
 * Each request or response is marked as not having been notified.
 *
 * @param suiteId to match a pending invocation
 * @param classname to match a pending invocation
 * @param mode one of {@link #MODE_LREQUEST}, {@link #MODE_LRESPONSE}
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_InvocationStore_setListenNotify0(void) {
    StoredLink* link;
    StoredInvoc* invoc;
    SuiteIdType desiredSuiteId;
    pcsl_string desiredClassname = PCSL_STRING_NULL_INITIALIZER;

    KNI_StartHandles(1);
    KNI_DeclareHandle(classname); /* Arg2: non-null classname */
    int mode;                  /* Arg3: requested invocation mode */

    /* Argument indices must match Java native method declaration */
#define listenSuiteIdArg 1
#define listenClassnameArg 2
#define listenModeArg 3

    do {/* Block to break out of on exceptions */
        if (!isEmpty()) {
            /* Queue is not empty
             * Need a string copy of the desired suiteId and classname
             * to use for comparisons
             */
            desiredSuiteId = KNI_GetParameterAsInt(listenSuiteIdArg);
    
            KNI_GetParameterAsObject(listenClassnameArg, classname);
            if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(classname, &desiredClassname)) {
                KNI_ThrowNew(midpOutOfMemoryError, 
                    "InvocationStore_setListenNotify0 no memory for [desiredClassname]");
                break;
            }
    
            /* Get the desired request mode. */
            mode = KNI_GetParameterAsInt(listenModeArg);
    
            /* Inspect the queue of Invocations and pick one that
             * matches the suiteId and classname
             */
            for (link = invocQueue; link != NULL; link = link->flink) {
                invoc = link->invoc;
                /*
                 * If the status matches the request
                 * and the suite matches and the classname matches.
                 */
                if (mode == MODE_LREQUEST && invoc->status == STATUS_WAITING) {
                   /* This invocation is a match; check classname and suite below */
                } else if (mode == MODE_LRESPONSE &&
                            (invoc->status >= STATUS_OK && invoc->status <= STATUS_INITIATED)) {
                    /* A pending response; check classname and suite below */
                } else {
                    /* Not this invocation; on to the next */
                    continue;
                }
                /* Check if this is the right class and suite */
                if (desiredSuiteId == invoc->suiteId &&
                        pcsl_string_equals(&desiredClassname, &invoc->classname)) {
                    /* Reset the flag so this Invocation will notify. */
                    invoc->notified = KNI_FALSE;
                }
            }
        }
    } while (KNI_FALSE);

    /* TBD: MidpMalloc failure not reported; not found is returned */

    /* Always free strings allocated */
    pcsl_string_free(&desiredClassname);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * The utility allocates memory for StoredInvoc struct and initializes it with
 * zeroes.
 * It replaces standard 'calloc'-family function to double-check that allocated
 * buffer is zeroed.
 *
 * @return pointer on zeroed StoredInvoc buffer or NULL if EOM occurs.
 */
static StoredInvoc * newStoredInvoc() {
    StoredInvoc * buf = JAVAME_MALLOC(sizeof(StoredInvoc));
    memset( buf, '\0', sizeof(*buf) );
    return buf;
}


/**
 * Implementation of native method to queue a new Invocation.
 * The state of the InvocationImpl is copied to the heap
 * and inserted in the head of the invocation queue.
 * An StoredInvoc struct is allocated on the native heap and 
 * the non-null strings for each field of the InvocationImpl class
 * are copied to the heap.
 * A new transaction ID is assigned to this Invocation
 * and returned in the tid field of the InvocationImpl.
 * @param invoc the InvocationImpl to store
 * @throws OutOfMemoryError if the memory allocation fails
 * @see StoredInvoc
 * @see #invocQueue
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_InvocationStore_put0(void) {
    StoredInvoc* invoc = NULL;

    KNI_StartHandles(4);
    KNI_DeclareHandle(invocObj);
    KNI_DeclareHandle(classObj);
    KNI_DeclareHandle(argsObj);
    KNI_DeclareHandle(str);

    KNI_GetParameterAsObject(1, invocObj);
    init(invocObj, classObj);

    do {
        /* On any error break out of this block */
        /* Allocate a new zero'ed struct to save the values in */
        invoc = newStoredInvoc();
        if (invoc == NULL) {
            KNI_ThrowNew(midpOutOfMemoryError, 
                                "InvocationStore_put0 no memory for [invoc]");
            break;
        }

        /* Assign a new transaction id and set it */
        invoc->tid = invocNextTid();
        KNI_SetIntField(invocObj, tidFid, invoc->tid);
    
        /*
         * Copy all the parameters to native
         * Includes ID, type,url, action, args, data
         */
        if (KNI_TRUE != setParamsFromObj(invoc, invocObj, str, argsObj))
            break;
    
        invoc->previousTid = KNI_GetIntField(invocObj, previousTidFid);
        invoc->status = KNI_GetIntField(invocObj, statusFid);
        invoc->responseRequired = KNI_GetBooleanField(invocObj, responseRequiredFid);
        invoc->suiteId = KNI_GetIntField(invocObj, suiteIdFid);

#define GET_STRING(name) \
        KNI_GetObjectField(invocObj, name##Fid, str); \
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(str, &invoc->name)) \
            break; \
    
        GET_STRING(classname)
        GET_STRING(invokingAuthority)
        GET_STRING(invokingAppName)

        invoc->invokingSuiteId = KNI_GetIntField(invocObj, invokingSuiteIdFid);
    
        GET_STRING(invokingClassname)
        GET_STRING(invokingID)
        GET_STRING(username)
        GET_STRING(password)

#undef GET_STRING
    
        /* Insert the new Invocation at the end of the queue */
        if (!invocPut(invoc))
            break;
    
        unblockWaitingThreads(STATUS_OK);
    
        /* Clear to skip cleanup and throwing exception */
        invoc = NULL;
    } while (0);

    if (invoc != NULL) {
        /* An allocation error occurred; free any remaining */
        invocFree(invoc);
        KNI_ThrowNew(midpOutOfMemoryError, "invocStore.c allocation failed");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Implementation of native method to set the status of an Invocation.
 * If the status is set to a response status then the "finish"
 * behavior is performed.  If a response is required, the Invocation
 * is requeued to the invoking application. Otherwise, the Invocation
 * is discarded.
 *
 * @param invoc the InvocationImpl to update the native status
 * @see StoredInvoc
 * @see #invocQueue
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_InvocationStore_setStatus0(void) {
    StoredLink* link;
    StoredInvoc* invoc;
    int tid;

    KNI_StartHandles(3);
    KNI_DeclareHandle(invocObj);
    KNI_DeclareHandle(obj1);
    KNI_DeclareHandle(obj2);

    KNI_GetParameterAsObject(1, invocObj);
    init(invocObj, obj1);

    /* Find the matching entry in the queue */
    tid = KNI_GetIntField(invocObj, tidFid);
    link = invocFindTid(tid);
    if (link != NULL) {
        invoc = link->invoc;
        /* Update the status */
        invoc->status = KNI_GetIntField(invocObj, statusFid);
    
        switch (invoc->status) {
        case STATUS_OK:
        case STATUS_CANCELLED:
        case STATUS_ERROR:
        case STATUS_INITIATED:
            /* 
             * If a response is required, switch the target
             * application; if not then discard the Invocation.
             */
            if (invoc->responseRequired) {
                /* Swap the source and target suite and classname */
                SuiteIdType tmpSuiteId = invoc->invokingSuiteId, tmpSuiteId2;
                pcsl_string tmpClassname = invoc->invokingClassname;
                invoc->invokingSuiteId = invoc->suiteId;
                invoc->invokingClassname = invoc->classname;
                invoc->suiteId = tmpSuiteId;
                invoc->classname = tmpClassname;
        
                /* Unmark the response it is "new" to the target */
                invoc->cleanup = KNI_FALSE;
                invoc->notified = KNI_FALSE;
        
                /* Swap the references in the Invocation object. */
                tmpSuiteId = KNI_GetIntField(invocObj, suiteIdFid);
                tmpSuiteId2 = KNI_GetIntField(invocObj, invokingSuiteIdFid);
                KNI_SetIntField(invocObj, invokingSuiteIdFid, tmpSuiteId);
                KNI_SetIntField(invocObj, suiteIdFid, tmpSuiteId2);

                KNI_GetObjectField(invocObj, invokingClassnameFid, obj1);
                KNI_GetObjectField(invocObj, classnameFid, obj2);
                KNI_SetObjectField(invocObj, classnameFid, obj1);
                KNI_SetObjectField(invocObj, invokingClassnameFid, obj2);
        
                /* Unblock any waiting threads so they can retrieve this. */
                unblockWaitingThreads(STATUS_OK);
                break;
            }
            /* If no response; Fall into DISPOSE */
    
        case STATUS_DISPOSE:
            /*
             * Free the Invocation, clean the Tid in the Invocation
             */
            invoc->tid = 0;
            KNI_SetIntField(invocObj, tidFid, 0);
            removeEntry(link);
            invocFree(invoc);
            break;

        case STATUS_ACTIVE:
        case STATUS_HOLD:
        case STATUS_WAITING:
            /* Unblock any waiting threads so they can retrieve this. */
            unblockWaitingThreads(STATUS_OK);
            break;
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Updates the parameters of the invocation in the native store.
 * The ID, URL, Type, arguments, and data are stored again in native.
 * The key into the native store is the TID;
 *
 * @param invoc the InvocationImpl to update the native params
 * @see StoredInvoc
 * @see #invocQueue
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_InvocationStore_setParams0(void) {
    StoredLink* link;
    StoredInvoc* invoc;
    int tid;

    KNI_StartHandles(3);
    KNI_DeclareHandle(invocObj);
    KNI_DeclareHandle(obj1);
    KNI_DeclareHandle(obj2);

    KNI_GetParameterAsObject(1, invocObj);
    init(invocObj, obj1);

    /* Find the matching entry in the queue */
    tid = KNI_GetIntField(invocObj, tidFid);
    link = invocFindTid(tid);
    if (link != NULL) {
        invoc = link->invoc;
        if (!setParamsFromObj(invoc, invocObj, obj1, obj2)) {
            KNI_ThrowNew(midpOutOfMemoryError,
                 "invocStore.c: setParam0() allocation failed");
        }
    } else {
#if REPORT_LEVEL <= LOG_CRITICAL
    REPORT_CRIT(LC_NONE,
            "invocStore.c: setParam0() no entry for tid");
#endif
    }

    KNI_EndHandles();
    KNI_ReturnVoid();

}


/**
 * Mark of the existing Invocations for a content handler
 * by suiteId and classname so they can be cleaned up on
 * exit.
 *
 * @param suiteId to match a pending invocation
 * @param classname to match a pending invocation
 * @see StoredInvoc
 * @see #invocQueue
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_InvocationStore_setCleanup0(void) {
    StoredLink* link;
    StoredInvoc* invoc;
    SuiteIdType desiredSuiteId;
    pcsl_string desiredClassname = PCSL_STRING_NULL_INITIALIZER;
    jboolean cleanup;

    KNI_StartHandles(1);
    KNI_DeclareHandle(classname); /* Arg2: non-null classname */

    /* Argument indices must match Java native method declaration */
#define markSuiteIdArg 1
#define markClassnameArg 2
#define markCleanup 3

    do {/* Block to break out of on exceptions */

    if (!isEmpty()) {
        /* Queue is not empty
         * Need a string copy of the desired classname
         * to use for comparisons
         */
        desiredSuiteId = KNI_GetParameterAsInt(markSuiteIdArg);

        KNI_GetParameterAsObject(markClassnameArg, classname);
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(classname, 
                           &desiredClassname)) {
            KNI_ThrowNew(midpOutOfMemoryError, 
                "InvocationStore_setListenCleanup0 no memory for [desiredClassname]");
            break;
        }

        cleanup = KNI_GetParameterAsInt(markCleanup);

        /* Inspect the queue of Invocations and pick one that
         * matches the suiteId and classname.
         */
        for (link = invocQueue; link != NULL; link = link->flink) {
            invoc = link->invoc;
            /*
             * If the suite matches and the classname matches
             * set cleanup to the next tid;
             */
            if (desiredSuiteId == invoc->suiteId &&
                    pcsl_string_equals(&desiredClassname, &invoc->classname)) {
                /* Found an entry for the Invocation */
                invoc->cleanup = cleanup;
                invoc->notified = KNI_FALSE;
            }
        }
    }
    } while (0);
    
    pcsl_string_free(&desiredClassname);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Unblocks any thread blocked waiting in get0.
 * ALL threads will be unblocked; higher levels may
 * need to restart function calls.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_InvocationStore_cancel0(void) {
    unblockWaitingThreads(STATUS_CANCELLED);
    KNI_ReturnVoid();
}

/**
 * Returns the size of the queue of pending invocations.
 * @return the size of the queue.
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_j2me_content_InvocationStore_size0(void) {
    StoredLink* link;
    int size = 0;

    /* Inspect the queue of Invocations and pick one that
     * matches the suiteId and classname.
     */
    for (link = invocQueue; link != NULL; link = link->flink) {
        size++;
    }

    KNI_ReturnInt(size);
}

//---------------------------------------------------------

typedef struct _MidletIdChain {
    SuiteIdType             suiteId;
    javacall_utf16_string   className;
    struct _MidletIdChain * next;
} MidletIdChain;

static MidletIdChain * runningMidletsChain = NULL;

static MidletIdChain * newMidletIdChain( void ) {
    MidletIdChain * elem = (MidletIdChain *)JAVAME_MALLOC( sizeof(*elem) );
    if( elem != NULL ) memset( elem, '\0', sizeof(*elem) );
    return elem;
}

static void destroyMidletIdChain( MidletIdChain * elem ){
    JAVAME_FREE( elem );
}

static int compareMidletIdChain( const MidletIdChain * elem, 
                    SuiteIdType suiteId, javacall_utf16_string midletClassName ){
    javacall_int32 comparison;
    // assert( elem != NULL );
    if( elem->suiteId != suiteId )
        return elem->suiteId - suiteId;
    javautil_unicode_cmp(elem->className, midletClassName, &comparison);
    return comparison;
}

static MidletIdChain ** findMidletIdChain( SuiteIdType suiteId, javacall_utf16_string midletClassName ) {
    MidletIdChain ** p = &runningMidletsChain;
    while( *p != NULL && compareMidletIdChain( *p, suiteId, midletClassName ) < 0 )
        p = &(*p)->next;
    return p;
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_AppProxy_midletIsAdded() {
    SuiteIdType suiteId;
    KNI_StartHandles(1);

    suiteId = KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_UTF16_STRING(2, midletClassName)

    MidletIdChain ** elemPlace, * elem;
#ifdef TRACE_MIDLETREG
    printf( "AppProxy_midletIsAdded: %d, '%ls'\n", suiteId, midletClassName );
#endif
    elemPlace = findMidletIdChain( suiteId, midletClassName );
    if( *elemPlace == NULL || compareMidletIdChain(*elemPlace, suiteId, midletClassName) != 0 ){
        if( (elem = newMidletIdChain()) == NULL ){
            KNI_ThrowNew(jsropOutOfMemoryError, NULL);
        } else {
            elem->suiteId = suiteId;
            elem->className = midletClassName;
            midletClassName = NULL;
            elem->next = *elemPlace;
            *elemPlace = elem;
        }
    }

    RELEASE_UTF16_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_j2me_content_AppProxy_midletIsRemoved() {
    SuiteIdType suiteId;
    KNI_StartHandles(1);

    suiteId = KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_UTF16_STRING(2, midletClassName)

    MidletIdChain ** elemPlace;
#ifdef TRACE_MIDLETREG
    printf( "AppProxy_midletIsRemoved: %d, '%ls'\n", suiteId, midletClassName );
#endif
    elemPlace = findMidletIdChain( suiteId, midletClassName );
    if( *elemPlace != NULL && compareMidletIdChain(*elemPlace, suiteId, midletClassName) == 0 ){
        // remove elem from the chain
        MidletIdChain * elem = *elemPlace;
        *elemPlace = (*elemPlace)->next;
        destroyMidletIdChain( elem );
    }

    RELEASE_UTF16_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_j2me_content_AppProxy_isMidletRunning() {
    int res = 0;
    SuiteIdType suiteId;
    KNI_StartHandles(1);

    suiteId = KNI_GetParameterAsInt(1);
    GET_PARAMETER_AS_UTF16_STRING(2, midletClassName)

    MidletIdChain ** elemPlace = findMidletIdChain( suiteId, midletClassName );
#ifdef TRACE_MIDLETREG
    printf( "AppProxy_isMidletRunning: %d, '%ls'\n", suiteId, midletClassName );
#endif
    res = (*elemPlace != NULL && compareMidletIdChain(*elemPlace, suiteId, midletClassName) == 0);

    RELEASE_UTF16_STRING_PARAMETER
    KNI_EndHandles();
    KNI_ReturnBoolean( res );
}

//---------------------------------------------------------

/**
 * Function to put a new entry in the queue.
 * @param invoc an initialized StoredInvoc.
 *
 */
static jboolean invocPut(StoredInvoc* invoc) {
    StoredLink *link, *last;

#ifdef DEBUG_INVOCLC
    printf( "invocPut: handlerID '%ls', class = '%ls'\n", 
                            invoc->ID.data, invoc->classname.data );
#endif

    link = (StoredLink*) JAVAME_CALLOC(1, sizeof(StoredLink));
    if (link == NULL) 
        return PCSL_FALSE;

    link->invoc = invoc;
    if (invocQueue == NULL) {
        invocQueue = link;
    } else {
        for (last = invocQueue; last->flink != NULL; last = last->flink);
        link->blink = last;
        last->flink = link;
    }

    return PCSL_TRUE;
}

/**
 * Function to find a matching entry in the queue.
 * The suiteId and classname must match.  If the request param
 * is true then a new Invocation (INIT status) is returned.
 * If false, then an OK, CANCELLED, INITIATED, or ERROR
 * status is selected.  Other status values are ignored.
 *
 * @param suiteId the application suite
 * @param classname a string identifying the entry point
 * @param mode one of {@link #MODE_REQUEST}, {@link #MODE_RESPONSE},
 *    or {@link #MODE_CLEANUP}, {@link #MODE_LREQUEST},
 *    {@link #MODE_LRESPONSE}
 * @return a StoredLink if a matching one is found; NULL otherwise
 */
static StoredLink* invocFind(SuiteIdType suiteId, 
                                const pcsl_string* classname, int mode) {
    StoredLink* curr;
    StoredInvoc* invoc;

#ifdef TRACE_INVOCFIND
    {
        char * m = "unknown";
        switch( mode ){
            case MODE_REQUEST: m = "MODE_REQUEST"; break;
            case MODE_RESPONSE: m = "MODE_RESPONSE"; break;
            case MODE_LREQUEST: m = "MODE_LREQUEST"; break;
            case MODE_LRESPONSE: m = "MODE_LRESPONSE"; break;
            case MODE_CLEANUP: m = "MODE_CLEANUP"; break;
        }
        printf( "invocFind: class = '%ls', mode = %s\n", classname->data, m );
    }
#endif

    /* Inspect the queue of Invocations and pick one that
     * matches the suiteId and classname.
     */
    for (curr = invocQueue; curr != NULL; ) {
        invoc = curr->invoc;
#ifdef TRACE_INVOCFIND
        printf( "invoc: tid = %d, status = %d, responseReq = %d, ID = '%ls', class = '%ls'\n",
                invoc->tid, invoc->status, invoc->responseRequired, invoc->ID.data, invoc->classname.data );
#endif
        /*
         * If the status matches the request
         * and the suite matches and the classname matches.
         */
        if (suiteId == invoc->suiteId && pcsl_string_equals(classname, &invoc->classname)){
#ifdef TRACE_INVOCFIND
            printf( "\ttid & classname are OK\n" );
#endif
            if(modeCheck(invoc, mode)) {
#ifdef TRACE_INVOCFIND
                printf( "\tmode OK\n" );
#endif
                if (mode == MODE_CLEANUP) {
                    /* An active or waiting Invocation needs a response */
                    if ((invoc->status != STATUS_WAITING && invoc->status != STATUS_ACTIVE) ||
                            !invoc->responseRequired) {
                        /* A regular response, discard and continue */
                        StoredLink* next = curr->flink;
                        removeEntry(curr);
                        invocFree(invoc);
                        curr = next;
                        continue;
                    }
                }
                return curr;
            }
        }
        curr = curr->flink;
    }
    return NULL;
}

/**
 * Match the StoredInvoc against the desired mode.
 * @param invoc a StoredInvoc to check
 * @param mode one of {@link #MODE_REQUEST}, {@link #MODE_RESPONSE},
 *    {@link #MODE_CLEANUP}, {@link #MODE_LREQUEST}, 
 *    {@link #MODE_LRESPONSE}, {@link #MODE_PENDING}, or
 *    {@link #MODE_TID}.
 * @return true if the modes matches the StoredInvoc
 */
static jboolean modeCheck(StoredInvoc* invoc, int mode) {
    switch (mode) {
    case MODE_REQUEST:
        return (invoc->status == STATUS_WAITING);

    case MODE_RESPONSE:
        return (invoc->status >= STATUS_OK &&
                invoc->status <= STATUS_INITIATED);

    case MODE_LREQUEST:
        return (invoc->notified == KNI_FALSE && 
                invoc->status == STATUS_WAITING);

    case MODE_LRESPONSE:
        return (invoc->notified == KNI_FALSE && 
                invoc->status >= STATUS_OK &&
                invoc->status <= STATUS_INITIATED);

    case MODE_CLEANUP:
        /*
         * If the Invocation is an old one then it needs
         * to be cleaned up if it is a response or is active.
         * That's everything except HOLD.
         */
        return (invoc->status == STATUS_ACTIVE ||
               (invoc->cleanup == KNI_TRUE &&
                invoc->status != STATUS_HOLD));
    }
    return KNI_FALSE;
}


/**
 * Function to find a matching entry entry in the queue based
 * on the tid.
 *
 * @param tid the transaction id (unique)
 * @return a StoredInvoc if a matching one is found; NULL otherwise
 */
static StoredLink* invocFindTid(int tid) {
    StoredLink* curr;

    /* Inspect the queue of Invocations and pick one that
     * matches the TID.
     */
    for (curr = invocQueue; curr != NULL && tid != curr->invoc->tid; curr = curr->flink);
    return curr;
}

/**
 * Free all of the memory used by a stored invocation.
 * 
 * @param stored pointer to an StoredInvoc.
 */
static void invocFree(StoredInvoc* invoc) {
    /*
     * An error occurred allocating memory; release all of the unused
     * Strings, free the structure and throw OutOfMemoryError.
     */
    if (invoc != NULL) {
        if (invoc->args != NULL) {
            /* Free the argument array and strings, if any */
            pcsl_string* args = invoc->args;
            int i = invoc->argsLen;
            while (i--) {
                pcsl_string_free(args++);
            }
            JAVAME_FREE(invoc->args);
        }

        if (invoc->data != NULL) {
            JAVAME_FREE(invoc->data);
        }

        pcsl_string_free(&invoc->url);
        pcsl_string_free(&invoc->type);
        pcsl_string_free(&invoc->action);
        pcsl_string_free(&invoc->ID);
        pcsl_string_free(&invoc->classname);
        pcsl_string_free(&invoc->invokingAuthority);
        pcsl_string_free(&invoc->invokingAppName);
        pcsl_string_free(&invoc->invokingClassname);
        pcsl_string_free(&invoc->invokingID);
        pcsl_string_free(&invoc->username);
        pcsl_string_free(&invoc->password);

        JAVAME_FREE(invoc);
    }
}

/**
 * Function to deliver the next transaction id.
 * Java-side always generates negative transaction ids
 * 
 * @return the next transaction id.
 */
static int invocNextTid() {
    if (--nextTid >= 0)
        nextTid = -1;
    return nextTid;
}

/**
 * Block this thread until unblocked.
 * Initialize the reentry data needed to unblock this thread later.
 * Only the waitingFor setting is used.
 * The status is set to STATUS_OK.
 * If canceled the status will be set to cancelled.
 */
static void blockThread() {
    /* Initialize the re-entry data so later this thread can
     * be unblocked.
     */
    MidpReentryData* p = (MidpReentryData*)(SNI_GetReentryData(NULL));
    if (p == NULL) {
        p = (MidpReentryData*)(SNI_AllocateReentryData(sizeof (MidpReentryData)));
    }
#ifdef TRACE_BLOCKING
    printf( "blockThread %p\n", p );
#endif
    p->waitingFor = JSR211_SIGNAL;
    p->status = JSR211_INVOKE_OK;
    SNI_BlockThread();
}

/**
 * Check if this blocked thread was cancelled.
 */
static jboolean isThreadCancelled() {
    MidpReentryData* p = (MidpReentryData*)(SNI_GetReentryData(NULL));
    return (p != NULL && p->status == JSR211_INVOKE_CANCELLED);
}

/**
 * Scan the block thread data for every thread blocked
 * for a JSR211_SIGNAL block type with INVOKE status and unblock it.
 * For now, every blocked thread is awoken; it should 
 * check if the thread is blocked for the requested application,
 * classname.
 *
 * @param the new status of the blocked thread; either
 *  STATUS_CANCELLED or STATUS_OK
 */
static void unblockWaitingThreads(int newStatus) {
    int n;
    int i;
    JVMSPI_BlockedThreadInfo *blocked_threads = SNI_GetBlockedThreads(&n);
    const int status_mask = JSR211_INVOKE_OK | JSR211_INVOKE_CANCELLED;
    int st = newStatus==STATUS_OK ? JSR211_INVOKE_OK: JSR211_INVOKE_CANCELLED;

#ifdef TRACE_BLOCKING
    printf( "unblockWaitingThreads( %d ). blocked_threads count = %d\n", newStatus, n );
#endif
    for (i = 0; i < n; i++) {
        MidpReentryData *p = (MidpReentryData*)(blocked_threads[i].reentry_data);
        if (p == NULL) {
            continue;
        }
#ifdef TRACE_BLOCKING
        printf( "blocked_thread[%d] %p, waitingFor '%x', status '%x'\n", i, p, p->waitingFor, p->status );
#endif
        if (p->waitingFor == JSR211_SIGNAL && (p->status | status_mask) != 0) {
            JVMSPI_ThreadID id = blocked_threads[i].thread_id;
#ifdef TRACE_BLOCKING
            printf( "try to unblock. id = %p\n", id );
#endif
            if (id != NULL) {
                p->status = st;
                SNI_UnblockThread(id);
            }
        }
    }
}

/**
 * Remove the entry by updating the pointers in the next and 
 * previous entries.
 * The pointers in the entry are set to NULL to prevent accidental
 * dereferences.
 */
static void removeEntry(StoredLink* entry) {
#ifdef DEBUG_INVOCLC
    printf( "kni_invoc_store::removeEntry: handlerID '%ls', class = '%ls'\n", 
                        entry->invoc->ID.data, entry->invoc->classname.data );
#endif
    if (entry != NULL) {
        StoredLink *blink = entry->blink;
        StoredLink *flink = entry->flink;

        if (blink == NULL) {
            invocQueue = flink;
        } else {
            blink->flink = flink;
        }
        if (flink != NULL) {
            flink->blink = blink;
        }
        JAVAME_FREE(entry);
    }
}

static int javacall_string_len(javacall_const_utf16_string string) {
    javacall_int32 length;
    if (JAVACALL_OK != javautil_unicode_utf16_ulength (string, &length))
        length = 0;
    return length;
}

/**
 * Function to find a matching entry in the queue.
 * The handlerID must match. The function seeks among new Invocations 
 * (WAITING status).
 *
 * @param handlerID a string identifying the requested handler
 *
 * @return the found invocation, or NULL if no matched invocation.
 */
StoredInvoc* jsr211_get_invocation(javacall_const_utf16_string handlerID) {
    StoredLink* curr;
    pcsl_string id = PCSL_STRING_NULL_INITIALIZER;

    pcsl_string_convert_from_utf16(handlerID, javacall_string_len(handlerID), &id);

#ifdef DEBUG_211
    printf( "jsr211_get_invocation: %ls\n", handlerID );
#endif
    /* Inspect the queue of Invocations and pick one that
     * matches the handlerID.
     */
    for (curr = invocQueue; curr != NULL; curr = curr->flink) {
        if (curr->invoc->status == STATUS_WAITING && pcsl_string_equals(&id, &curr->invoc->ID)) {
            pcsl_string_free(&id);
            return curr->invoc;
        }
    }
    
    pcsl_string_free(&id);
    return NULL;
}

/**
 * Function to find a matching invocation in the store
 *   based on the invocation id.
 *
 * @param invoc_id the invocaction id
 * @return a StoredInvoc if a matching one is found; NULL otherwise
 */
StoredInvoc* jsr211_find_invocation(int invoc_id) {
    StoredLink* curr;

    /* Inspect the queue of Invocations and pick one that matches. */
    for (curr = invocQueue; curr != NULL; curr = curr->flink) {
        if (invoc_id == curr->invoc->tid) {
            /* Found one; return it */
            return curr->invoc;
        }
    }
    return NULL;
}

static StoredLink* findLink(StoredInvoc *invoc) {
    StoredLink* curr;

    /* Inspect the queue of Invocations and pick one that matches. */
    for (curr = invocQueue; curr != NULL; curr = curr->flink) {
        if (invoc == curr->invoc) {
            /* Found one; return it */
            return curr;
        }
    }
    return NULL;
}

void jsr211_remove_invocation(StoredInvoc* invoc) {
    StoredLink *link;
#ifdef DEBUG_211
    printf( "jsr211_remove_invocation: %d\n", invoc->tid );
#endif
    link = findLink(invoc);
    if (NULL != link)
        removeEntry(link);
    invocFree(invoc);
}

jsr211_boolean jsr211_enqueue_invocation(StoredInvoc *invoc) {
    return invocPut(invoc);
}


#include <javacall_chapi_invoke.h>

/**
 * Executes specified non-java content handler.
 * The current implementation provides executed handler 
 * only with URL and action invocation parameters.
 *
 * @param handler_id content handler ID
 * @return codes are following
 * <ul>
 * <li> JSR211_LAUNCH_OK or JSR211_LAUNCH_OK_SHOULD_EXIT if content handler 
 *   started successfully
 * <li> other code from the enum according to error codition
 * </ul>
 */
jsr211_launch_result jsr211_execute_handler(javacall_const_utf16_string handler_id) {
    javacall_chapi_invocation jc_invoc;
    StoredInvoc* invoc;
    javacall_bool without_finish_notification;
    javacall_bool should_exit;
    javacall_result jc_result;
    jsr211_launch_result result;
    int i;

    invoc = jsr211_get_invocation(handler_id);

    if (NULL == invoc)
        return JSR211_LAUNCH_ERR_NO_INVOCATION;

    result = JSR211_LAUNCH_ERROR;

    jc_invoc.url               = (jchar *)pcsl_string_get_utf16_data(&invoc->url);
    jc_invoc.type              = (jchar *)pcsl_string_get_utf16_data(&invoc->type);
    jc_invoc.action            = (jchar *)pcsl_string_get_utf16_data(&invoc->action);
    jc_invoc.invokingAppName   = (jchar *)pcsl_string_get_utf16_data(&invoc->invokingAppName);
    jc_invoc.invokingAuthority = (jchar *)pcsl_string_get_utf16_data(&invoc->invokingAuthority);
    jc_invoc.username          = (jchar *)pcsl_string_get_utf16_data(&invoc->username);
    jc_invoc.password          = (jchar *)pcsl_string_get_utf16_data(&invoc->password);
    jc_invoc.argsLen           = invoc->argsLen;
    jc_invoc.dataLen           = invoc->dataLen;
    jc_invoc.data              = invoc->data;
    jc_invoc.responseRequired  = invoc->responseRequired ? 1 : 0;
    if ( !(
         (NULL == handler_id) ||
         (NULL == jc_invoc.url) ||
         (NULL == jc_invoc.type) ||
         (NULL == jc_invoc.action) || 
         (NULL == jc_invoc.invokingAppName) ||
         (NULL == jc_invoc.invokingAuthority) ||
         (NULL == jc_invoc.username) ||
         (NULL == jc_invoc.password) 
    ) ) {
        jc_invoc.args = JAVAME_CALLOC(
            invoc->argsLen, sizeof(javacall_utf16_string));
        if (NULL != jc_invoc.args) {
            jsr211_boolean succ = JSR211_TRUE;
            for (i=0; i<invoc->argsLen; i++) {
                jc_invoc.args[i] = (jchar *)pcsl_string_get_utf16_data(&invoc->args[i]);
                if (NULL == jc_invoc.args[i])
                    succ = JSR211_FALSE;
            }
            if (succ) {
                jc_result = javacall_chapi_platform_invoke(invoc->tid,
                                (jchar*)handler_id, &jc_invoc,
                                &without_finish_notification, &should_exit);
                if (JAVACALL_OK == jc_result) {
                    result = should_exit ? JSR211_LAUNCH_OK_SHOULD_EXIT : JSR211_LAUNCH_OK;
                    invoc->status = without_finish_notification ? STATUS_INITIATED : STATUS_ACTIVE;
                }
            }
            for (i=0; i<invoc->argsLen; i++)
                if (NULL != jc_invoc.args[i])
                    pcsl_string_release_utf16_data(jc_invoc.args[i], &invoc->args[i]);
            JAVAME_FREE(jc_invoc.args);
            jc_invoc.args = NULL;
        }
    }

    pcsl_string_release_utf16_data(jc_invoc.url, &invoc->url);
    pcsl_string_release_utf16_data(jc_invoc.type, &invoc->type);
    pcsl_string_release_utf16_data(jc_invoc.action, &invoc->action);
    pcsl_string_release_utf16_data(jc_invoc.invokingAppName, &invoc->invokingAppName);
    pcsl_string_release_utf16_data(jc_invoc.invokingAuthority, &invoc->invokingAuthority);
    pcsl_string_release_utf16_data(jc_invoc.username, &invoc->username);
    pcsl_string_release_utf16_data(jc_invoc.password, &invoc->password);

    if (result == JSR211_LAUNCH_ERROR)
        invoc->status = STATUS_ERROR;

    return result;
}

/*
 * Called when platform notifies java VM that invocation of native handler 
 * is finished. This is <code>ContentHandlerServer.finish()</code> substitute 
 * after platform handler completes invocation processing.
 * @param invoc_id processed invocation Id
 * @param url if not NULL, then changed invocation URL
 * @param argsLen if greater than 0, then number of changed args
 * @param args changed args if @link argsLen is greater than 0
 * @param dataLen if greater than 0, then length of changed data buffer
 * @param data the data
 * @param status result of the invocation processing. 
 * @return result of operation.
 */
static void handle_javanotify_chapi_platform_finish(int invoc_id,
        javacall_utf16_string url,
        int argsLen, javacall_utf16_string* args,
        int dataLen, void* data,
        javacall_chapi_invocation_status status
) {
    StoredInvoc* invoc;
    int i;
    javacall_result result;
    
    invoc = jsr211_find_invocation(invoc_id);
    
    /* invalid invoc_id or the invocation is already removed. */
    if (invoc == NULL)
        return;
    result = JAVACALL_OK;

    if (PCSL_STRING_OK != pcsl_string_convert_from_utf16(
            url,javacall_string_len(url), &invoc->url))
        result = JAVACALL_FAIL;
    
    if (NULL != invoc->args) {
        for (i=0; i< invoc->argsLen; i++)
            pcsl_string_free(&invoc->args[i]);
        JAVAME_FREE(invoc->args);
        invoc->args = NULL;
    }
    invoc->argsLen = 0;

    if ( (NULL != args) && (0 != argsLen) ) {
        invoc->args = (pcsl_string*)JAVAME_CALLOC(argsLen, sizeof(pcsl_string));
        if (NULL != invoc->args) {
            for (i = 0; i< argsLen; i++) {
                invoc->args[i] = PCSL_STRING_NULL;
                if (PCSL_STRING_OK != pcsl_string_convert_from_utf16(
                        args[i], javacall_string_len(args[i]), &invoc->args[i]))
                    result = JAVACALL_FAIL;
            }
        } else
            result = JAVACALL_OUT_OF_MEMORY;
    }

    if (NULL != invoc->data) {
        JAVAME_FREE(invoc->data);
        invoc->data = NULL;
    }
    invoc->dataLen = 0;

    if ( (NULL != data) && (0 != dataLen) ) {
        invoc->data = JAVAME_MALLOC(dataLen);
        if (NULL != invoc->data) {
            invoc->dataLen = dataLen;
            memcpy (invoc->data, data, dataLen);
        } else
            result = JAVACALL_OUT_OF_MEMORY;
    }

    switch (status) {
        case INVOCATION_STATUS_OK:
            invoc->status = STATUS_OK;
            break;
        case INVOCATION_STATUS_CANCELLED:
            invoc->status = STATUS_CANCELLED;
            break;
        case INVOCATION_STATUS_ERROR:
            invoc->status = STATUS_ERROR;
            break;
        default:
            invoc->status = STATUS_ERROR;
            result = JAVACALL_INVALID_ARGUMENT;
    }
        
    if (invoc->responseRequired) {

        if (result != JAVACALL_OK)
            invoc->status = STATUS_ERROR;
        { /* swap invokee - invoker fields */
            SuiteIdType tmpSuiteId = invoc->suiteId;
            pcsl_string tmpClassname = invoc->classname;
            invoc->suiteId = invoc->invokingSuiteId;
            invoc->invokingSuiteId = tmpSuiteId;
            invoc->classname = invoc->invokingClassname;
            invoc->invokingClassname = tmpClassname;
        }
        /* Unmark the response since it is "new" to the target */
        invoc->cleanup = KNI_FALSE;
        invoc->notified = KNI_FALSE;

        /* Unblock any waiting threads so they can retrieve this. */
        unblockWaitingThreads(STATUS_OK);

    } else {
        jsr211_remove_invocation(invoc);
    }
} 

#include <javacall_chapi_registry.h>

static void jsr211_abort_platform_invocation (int tid)
{
    javacall_bool should_exit;
    javacall_chapi_java_finish (tid, NULL,
                                0, NULL,
                                0, NULL,
                                INVOCATION_STATUS_ERROR, &should_exit);
}

/**
 * Receives invocation request initiated by the platform. <BR>
 * This is <code>Registry.invoke()</code> substitute for Platform->Java call.
 * @param handler_id target Java handler Id
 * @param invocation filled out structure with invocation params
 * @param invoc_id invocation Id for further references, should be positive
 */
static void handle_javanotify_chapi_java_invoke(
        const javacall_utf16_string handler_id,
        javacall_chapi_invocation* invocation,
        int invoc_id) {
    javacall_utf16_string classname = NULL;
    int classname_len = 128;
    javacall_utf16_string suite_id_out = NULL;
    int suite_id_len = 128;
    int suite_id;
    StoredInvoc* invoc;
    javacall_chapi_handler_registration_type flag;
    javacall_result res;
    int i;

    if (invoc_id <= 0) {
        jsr211_abort_platform_invocation(invoc_id);
        return;
    }

    while (1) {
        classname = JAVAME_CALLOC(classname_len, sizeof(javacall_utf16));
        if (NULL == classname) {
            res = JAVACALL_OUT_OF_MEMORY;
            break;
        }

        suite_id_out = JAVAME_CALLOC(suite_id_len, sizeof(javacall_utf16));
        if (NULL == suite_id_out) {
            res = JAVACALL_OUT_OF_MEMORY;
            break;
        }

        res = javacall_chapi_get_handler_info(
            handler_id, /*OUT*/
            suite_id_out, &suite_id_len,
            classname, &classname_len, &flag);

        if (JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL == res) {
            JAVAME_FREE(classname); classname = NULL;
            JAVAME_FREE(suite_id_out); suite_id_out = NULL;
            continue;
        }
    }

    if (res != JAVACALL_OK) {
        JAVAME_FREE(classname);
        JAVAME_FREE(suite_id_out);
        jsr211_abort_platform_invocation(invoc_id);
        return;
    }

    if (0 == jsrop_string_to_suiteid (suite_id_out, &suite_id)) {
        suite_id = INVALID_SUITE_ID;
    }
    JAVAME_FREE(suite_id_out);

    if (INVALID_SUITE_ID == suite_id) { // attempt to invoke native handler or suite id conversion error
        JAVAME_FREE(classname);
        jsr211_abort_platform_invocation(invoc_id);
        return;
    }

    invoc = newStoredInvoc();
    if (invoc == NULL) {
        JAVAME_FREE(classname);
        jsr211_abort_platform_invocation(invoc_id);
        return;
    }

    res = JAVACALL_OK;
        
    invoc->tid = invoc_id;
    invoc->status = STATUS_INIT;
    invoc->suiteId = suite_id;
    invoc->invokingSuiteId = INVALID_SUITE_ID;

    if (PCSL_STRING_OK != pcsl_string_convert_from_utf16(handler_id, javacall_string_len(handler_id), &invoc->ID))
        res = JAVACALL_FAIL;
    if (PCSL_STRING_OK != pcsl_string_convert_from_utf16(classname, classname_len, &invoc->classname))
        res = JAVACALL_FAIL;
    JAVAME_FREE(classname);

    /* IMPL_NOTE: null suite ID is an indication of platform request */
    invoc->invokingClassname = PCSL_STRING_NULL;
    invoc->invokingID        = PCSL_STRING_NULL;

    invoc->responseRequired =
        (0 == invocation->responseRequired) ? JSR211_FALSE : JSR211_TRUE;
    if (PCSL_STRING_OK != pcsl_string_convert_from_utf16(
            invocation->url, javacall_string_len(invocation->url), &invoc->url))
        res = JAVACALL_FAIL;
    if (invocation->type != NULL && 
            PCSL_STRING_OK != pcsl_string_convert_from_utf16(
                invocation->type, javacall_string_len(invocation->type), &invoc->type))
        res = JAVACALL_FAIL;
    if (invocation->action != NULL && 
            PCSL_STRING_OK != pcsl_string_convert_from_utf16(
                invocation->action, javacall_string_len(invocation->action), &invoc->action))
        res = JAVACALL_FAIL;
    if (invocation->invokingAppName != NULL && 
            PCSL_STRING_OK != pcsl_string_convert_from_utf16(
                invocation->invokingAppName, javacall_string_len(
                    invocation->invokingAppName), &invoc->invokingAppName))
        res = JAVACALL_FAIL;
    if (invocation->invokingAuthority != NULL && 
            PCSL_STRING_OK != pcsl_string_convert_from_utf16(
                invocation->invokingAuthority, javacall_string_len(
                    invocation->invokingAuthority), &invoc->invokingAuthority))
        res = JAVACALL_FAIL;
    if (invocation->username != NULL &&
            PCSL_STRING_OK != pcsl_string_convert_from_utf16(
                invocation->username, javacall_string_len(invocation->username), &invoc->username))
        res = JAVACALL_FAIL;
    if (invocation->password != NULL &&
            PCSL_STRING_OK != pcsl_string_convert_from_utf16(
                invocation->password, javacall_string_len(invocation->password), &invoc->password))
        res = JAVACALL_FAIL;
    
    if (JAVACALL_OK == res) {
        invoc->argsLen = invocation->argsLen;
        invoc->args = JAVAME_CALLOC(sizeof(pcsl_string), invoc->argsLen);
        if (NULL != invoc->args) {
            for (i = 0; i < invocation->argsLen; i++) {
                invoc->args[i] = PCSL_STRING_NULL;
                if (invocation->args[i] != NULL && 
                        PCSL_STRING_OK != pcsl_string_convert_from_utf16(
                            invocation->args[i], javacall_string_len(invocation->args[i]), &invoc->args[i]))
                    res = JAVACALL_FAIL;
            }
            if (JAVACALL_OK == res) {
                if ((NULL != invocation->data) && (0 != invocation->dataLen)) {
                    invoc->dataLen = invocation->dataLen;
                    invoc->data = JAVAME_MALLOC(invoc->dataLen);
                    if (NULL == invoc->data)
                        res = JAVACALL_OUT_OF_MEMORY;
                }
                if (JAVACALL_OK == res) {
                    memcpy (invoc->data, invocation->data, invoc->dataLen);
                
                    /* invocation is ready to be processed */
                    invoc->status = STATUS_WAITING;
                    /* Insert the new Invocation at the end of the queue */
                    if (!jsr211_enqueue_invocation(invoc))
                        res = JAVACALL_FAIL;
                    
                    unblockWaitingThreads(STATUS_OK);
                    
                    /* IMPL_NOTE: The midlet handler should be launched on
                                  corresponding MIDP event processing */

                    if (JAVACALL_OK != res)
                        JAVAME_FREE(invoc->data);
                }
            }
            if (JAVACALL_OK != res){
                for (i = 0; i < invocation->argsLen; i++)
                    pcsl_string_free(&invoc->args[i]);
                JAVAME_FREE(invoc->args);
            }
        } else
            res = JAVACALL_OUT_OF_MEMORY;
    }
    if (JAVACALL_OK != res) {
        pcsl_string_free(&invoc->ID);
        pcsl_string_free(&invoc->classname);
        pcsl_string_free(&invoc->url);
        pcsl_string_free(&invoc->type);
        pcsl_string_free(&invoc->action);
        pcsl_string_free(&invoc->invokingAppName);
        pcsl_string_free(&invoc->invokingAuthority);
        pcsl_string_free(&invoc->username);
        pcsl_string_free(&invoc->password);
        JAVAME_FREE(invoc);
    }
}

/**
 * informs platform about finishing platform's request
 * returns should_exit flag for content handler that has processed request
 * @param tid Invocation (transaction) identifier
 * @should_exit flag for the invocation handler
 * @return true in case of success, false otherwise
 */
jsr211_boolean jsr211_platform_finish(int tid, jsr211_boolean *should_exit)
{
    javacall_const_utf16_string url;
    javacall_utf16_string *args;
    int i;
    javacall_chapi_invocation_status status;
    javacall_bool _should_exit;
    jsr211_boolean success;
    javacall_result res;
    StoredLink *link;
    StoredInvoc *invoc;

    link = invocFindTid(tid);
    if (NULL == link)
        return JSR211_FALSE;
    invoc = link->invoc;

    url = pcsl_string_get_utf16_data(&invoc->url);
    args = JAVAME_CALLOC(invoc->argsLen, sizeof(javacall_utf16_string));
    if ( (NULL != url) && (NULL != args) ) {
        success = JSR211_TRUE;
        for (i = 0; i < invoc->argsLen; i++) {
            args[i] = pcsl_string_get_utf16_data(&invoc->args[i]);
            if (NULL == args[i])
                success = JSR211_FALSE;
        }
        
        if (JSR211_FALSE != success) {
            switch (invoc->status) {
                case STATUS_OK:
                case STATUS_CANCELLED:
                case STATUS_ERROR:
                    status = invoc->status;
                    break;
                default:
                    success = JSR211_FALSE;
                    break;
            }
    
            if (JSR211_FALSE != success) {
                res = javacall_chapi_java_finish(invoc->tid,
                                                 url,
                                                 invoc->argsLen,
                                                 args,
                                                 invoc->dataLen,
                                                 invoc->data,
                                                 status,
                                                 &_should_exit);
                if (JAVACALL_OK == res) {
                    *should_exit = _should_exit ? JSR211_TRUE : JSR211_FALSE;
                } else
                    success = JSR211_FALSE;
            }
        }

        for (i = 0; i < invoc->argsLen; i++)
            pcsl_string_release_utf16_data(args[i],&invoc->args[i]);
    } else
        success = JSR211_FALSE;

    JAVAME_FREE(args);
    pcsl_string_release_utf16_data(url, &invoc->url);

    return success;
}

#define CHECK_FOR_NULL_AND_FREE(pointer) \
    if (NULL != pointer) { \
        javacall_free(pointer); \
        pointer = NULL; \
    } \

static void free_platform_event (jsr211_platform_event *event)
{
    int i;

    if (NULL != event) {
        CHECK_FOR_NULL_AND_FREE(event->handler_id);
        CHECK_FOR_NULL_AND_FREE(event->invocation.url);
        CHECK_FOR_NULL_AND_FREE(event->invocation.type);
        CHECK_FOR_NULL_AND_FREE(event->invocation.action);
        CHECK_FOR_NULL_AND_FREE(event->invocation.invokingAppName);
        CHECK_FOR_NULL_AND_FREE(event->invocation.invokingAuthority);
        CHECK_FOR_NULL_AND_FREE(event->invocation.username);
        CHECK_FOR_NULL_AND_FREE(event->invocation.password);
        if (NULL != event->invocation.args) {
            for (i = 0; i < event->invocation.argsLen; i++)
                CHECK_FOR_NULL_AND_FREE(event->invocation.args[i]);
            javacall_free(event->invocation.args);
            event->invocation.args = NULL;
        }
        CHECK_FOR_NULL_AND_FREE(event->invocation.data);

        javacall_free (event);
    }
}

void 
jsr211_process_platform_finish_notification (int invoc_id,
                                             jsr211_platform_event *event)
{
    if (NULL == event) {
        handle_javanotify_chapi_platform_finish(
            invoc_id,
            NULL,
            0, NULL, /* args */
            0, NULL, /* data */
            STATUS_ERROR
            );
        return;
    }

    handle_javanotify_chapi_platform_finish(
        invoc_id,
        event->invocation.url,
        event->invocation.argsLen,
        event->invocation.args,
        event->invocation.dataLen,
        event->invocation.data,
        event->status
        );
    
    free_platform_event(event);
}

void
jsr211_process_java_invoke_notification (int invoc_id,
                                         jsr211_platform_event *event)
{
    if (NULL == event) {
        jsr211_abort_platform_invocation(invoc_id);
        return;
    }

    handle_javanotify_chapi_java_invoke(
        event->handler_id,
        &event->invocation,
        invoc_id
        );
    
    free_platform_event(event);
}
