/*
 *
 *
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

#include <string.h>
#include <wmaPushRegistry.h>
#ifndef ENABLE_PCSL
  #define pcsl_mem_malloc malloc
  #define pcsl_mem_free free
  #define pcsl_mem_strdup(x) strcpy(malloc(strlen(x)+1), x)
#else
  #include <pcsl_memory.h>
#endif
#include <jsr120_sms_protocol.h>
#include <jsr120_cbs_protocol.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_cbs_listeners.h>

/* Need revisit: delete? */
#if ENABLE_JSR_205
#include <jsr205_mms_protocol.h>
#include <jsr205_mms_listeners.h>
#endif

#ifdef ENABLE_MIDP
  #include <push_server_resource_mgmt.h>
#else
  #define CHECKED_OUT          (-2)
#endif
#include <app_package.h>
#include <kni.h>

static int registerSMSEntry(int port, AppIdType msid);
static void unregisterSMSEntry(int port, int handle);
static int registerCBSEntry(int msgID, AppIdType msid);
static void unregisterCBSEntry(int msgID, int handle);
#if ENABLE_JSR_205
static int registerMMSEntry(unsigned char *appID, AppIdType msid);
static void unregisterMMSEntry(unsigned char *appID, int handle);
#endif

#ifndef NO_PUSH
/**
 * Unregister or close the given WMA entry
 *
 * @param state Current state of push connection
 * @param entry Full text of the push entry
 * @param port Port id to listen to
 * @param msid Midlet Suite ID
 * @param appID application ID of MMS message
 * @param fd unique identifier for the connection
 */
void wmaPushCloseEntry(int state, char *entry, int port,
                       AppIdType msid, char *appID, int fd) {

#ifndef ENABLE_JSR_205
    (void)appID;
#endif

    if (state != CHECKED_OUT ) {
        if(strncmp(entry,"sms://:",7) == 0) {
            /*
             * Delete all SMS messages cached for the
             * specified midlet suite.
             */
            jsr120_sms_delete_push_msg(msid);
            /* unregister this sms push entry */
            unregisterSMSEntry(port, fd);
	} else if(strncmp(entry,"cbs://:",7) == 0) {
            /*
             * Delete all CBS messages cached for the
             * specified midlet suite.
             */
            jsr120_cbs_delete_push_msg(msid);
            /* unregister this cbs push entry */
            unregisterCBSEntry(port, fd);
	}
#if ENABLE_JSR_205
          else if(strncmp(entry,"mms://:",7) == 0) {
            /*
             * Delete all MMS messages cached for the
             * specified midlet suite.
             */
            jsr205_mms_delete_push_msg(msid);
            /* unregister this mms push entry */
            unregisterMMSEntry((unsigned char *)appID, fd);
            if (appID != NULL) {
                pcsl_mem_free(appID);
            }
        }
#endif
    }

}

/**
 * Check if given connection is a WMA connection.
 *
 * @param pushPort Port number stored in Push entry
 * @param entry Full text of the push entry
 * @param pushStoreName Persistent store name in Push entry
 * @param port incoming port
 * @param store Storage name of requesting suite
 *
 * @result returns KNI_TRUE if it is WMA protocol, KNI_FALSE otherwise
 */
jboolean isWmaProtocol(int pushPort, char *entry, char *pushStoreName,
                       int port, char *store) {

    jboolean isMMS = KNI_FALSE;

#if ENABLE_JSR_205
    isMMS = (strncmp(entry, "mms", 3) == 0);
#endif

    if ((((port == pushPort) && (strncmp(entry, "sms", 3) == 0)) ||
         ((port == pushPort) && (strncmp(entry, "cbs", 3) == 0)) ||
         isMMS) &&
         (strcmp(store, pushStoreName) == 0) ) {
        return KNI_TRUE;
    }

    return KNI_FALSE;

}

#if ENABLE_JSR_205
/**
 * Check if given connection is a MMS connection.
 *
 * @param entry Full text of the push entry
 *
 * @result returns KNI_TRUE if it is MMS protocol, KNI_FALSE otherwise
 */
jboolean isMMSProtocol(char *entry) {

    return (strncmp(entry, "mms", 3) == 0);
}
#endif

/**
 * make copy of push entry
 *
 * @param entry Full text of the push entry
 *
 * @return A copy of the  full text push entry is returned
 *         for WMA protocols, NULL otherwise
 */
char *getWmaPushEntry(char *entry) {

    if((strncmp(entry,"sms://:",7) == 0)  ||
       (strncmp(entry,"cbs://:",7) == 0)
#if ENABLE_JSR_205
       || (strncmp(entry,"mms://:",7) == 0)
#endif
        ) {
        return pcsl_mem_strdup(entry);
    }

    return NULL;
}

/**
 * Perform a connection appropriate open
 * call. The returned identifier will be included in
 * the connection registry until needed by the
 * application level connection open request.
 *
 * @param entry A full-text push entry string from the registry
 * @param fd A pointer to a unique identifier.
 *           Used to return the identifier
 * @param port A port ID.
 * @param msid Midlet Suite ID.
 * @param appID Application ID of MMS message.
 */
void wmaPushProcessPort(char *entry, int *fd, int port,
                        AppIdType msid, char *appID){

#ifndef ENABLE_JSR_205
    (void)appID;
#endif

    if(strncmp(entry,"sms://:",7) == 0) {
        /*
         * register entry and port and get a unique
         * identifier back.
         */
        *fd = registerSMSEntry(port, msid);
    } else if(strncmp(entry,"cbs://:",7) == 0) {
        *fd = registerCBSEntry(port, msid);
    }
#if ENABLE_JSR_205
      else if(strncmp(entry,"mms://:",7) == 0) {
        *fd = registerMMSEntry((unsigned char *)appID, msid);
    }
#endif
}

static int registerSMSEntry(int port, AppIdType msid) {

    int handle = -1;

    /* register SMS port */
    if (jsr120_is_sms_push_listener_registered((jchar)port) == WMA_ERR) {

	/* Get a unique handle that will identify this SMS "session" */
	handle = (int)(pcsl_mem_malloc(1));

	if (handle == 0) {
            return -1;
	}

        if (jsr120_register_sms_push_listener((jchar)port,
                                          msid,
                                          handle) == WMA_ERR) {
	    return -1;
        }
    } else {
	/* port already registered, throw exception */
	return -1;
    }

    return handle;
}

static void unregisterSMSEntry(int port, int handle) {

    /** unregister SMS port from SMS pool */
    jsr120_unregister_sms_push_listener((jchar)port);

    /* Release the handle associated with this connection. */
    if (handle) {
        pcsl_mem_free((void *)handle);
    }
}

static int registerCBSEntry(int msgID, AppIdType msid) {

    int handle = -1;

    /* register CBS message ID */
    if (jsr120_cbs_is_push_listener_registered((jchar)msgID) == WMA_ERR) {

	/* Get a unique handle that will identify this CBS "session" */
	handle = (int)(pcsl_mem_malloc(1));

	if (handle == 0) {
            return -1;
	}

        if (jsr120_cbs_register_push_listener((jchar)msgID,
                                           msid,
                                           handle) == WMA_ERR) {
	    return -1;
        }
    } else {
	/* port already registered, throw exception */
	return -1;
    }

    return handle;
}

static void unregisterCBSEntry(int msgID, int handle) {

    /** unregister CBS msg ID from CBS pool */
    jsr120_cbs_unregister_push_listener((jchar)msgID);

    /* Release the handle associated with this connection. */
    pcsl_mem_free((void *)handle);

}

#if ENABLE_JSR_205
static int registerMMSEntry(unsigned char *appID,
                            AppIdType msid) {

    int handle = -1;

    /* register MMS message ID */
    if (appID != NULL) {
        if (jsr205_mms_is_push_listener_registered(appID) == WMA_ERR) {

	    /* Get a unique handle that will identify this MMS "session" */
            handle = (int)(pcsl_mem_malloc(1));

	    if (handle == 0) {
                return -1;
	    }

            if (jsr205_mms_register_push_listener(appID,
                                                  msid,
                                                  handle) == WMA_ERR) {
	        return -1;
            }
        } else {
	    /* app ID already registered, throw exception */
	    return -1;
        }
    }

    return handle;
}

static void unregisterMMSEntry(unsigned char *appID, int handle) {

    /** unregister MMS app ID from MMS pool */
    if (appID != NULL) {
        jsr205_mms_unregister_push_listener(appID);
    }

    /* Release the handle associated with this connection. */
    pcsl_mem_free((void *)handle);

}

/**
 * Get MMS app ID from push entry string
 *
 * @param entry Full text of the push entry
 *
 * @return Returns app ID string if successful, NULL otherwise
 *         (Caller ersponsible for freeing this string)
 */
char *getMMSAppID(char *entry) {
    char *p = entry;
    char *comma = NULL;
    char *colon1 = NULL;
    char *colon2 = NULL;
    char *appid = NULL;
    int len = 0;

    /*
     * Find the first ',' occurence. The chars before this
     * is the connection string, which is what we want
     */
    comma = strchr(p, ',');

    /*
     * Find the second colon occurence. The chars after this
     * and before the comma, are the app ID chars
     */
    colon1 = strchr(p, ':');
    colon2 = strchr(colon1 + 1, ':');

    if ((comma != NULL)  &&
        (colon2 != NULL) &&
        (colon2 < comma)){
        len = comma - colon2 - 1;
        if (len > 0) {
            appid = (char *)pcsl_mem_malloc(len + 1);
            if (appid != NULL) {
                strncpy((char *)appid, colon2 + 1, len);
                appid[len] = '\0';
                return appid;
            }
        }
    }

    return NULL;

}
#endif

#if (ENABLE_CDC == 1)

typedef struct filter_struct {
    int port;
    int handle;
    char* filter;
    struct filter_struct* next;
} filter_struct;

static filter_struct* filter_list;

static void register_filter(int port, int handle, char* filter) {

    filter_struct* ptr = (filter_struct*)pcsl_mem_malloc(sizeof(filter_struct));
    ptr->port = port;
    ptr->handle = handle;
    ptr->filter = filter;
    ptr->next = filter_list;
    filter_list = ptr;
}

static int unregister_filter(int port) {

    filter_struct* ptr = filter_list;
    if (ptr == NULL) {
        return 0; //error
    }
    if (ptr->port == port) {
        filter_list = filter_list->next;
        return 0;
    }

    do {
        if (ptr->next == NULL) {
            return 0; //error
        }
        if (ptr->next->port == port) {
            filter_struct* delptr = ptr->next;
            ptr->next = ptr->next->next;
            pcsl_mem_free(delptr->filter);
            pcsl_mem_free(delptr);
            return ptr->handle;
        }
    } while (1);
}

static char* find_filter(int port) {

    filter_struct* ptr = filter_list;
    if (ptr != NULL) {
        do {
            if (ptr->port == port) { 
                return ptr->filter;
            }
            ptr = ptr->next;
        } while (ptr != NULL);
    }    
    return NULL;
}

static int checkfilter_recursive(char *p1, char *p2) {
    if (*p1 == 0) return (*p2 == 0);
    if (*p1 == '*') {
        while (*++p1 == '*');
        do {
            if (checkfilter_recursive(p1, p2)) return 1;
        } while (*(p2++));
        return 0;
    }
    if (*p2 == 0) return 0;
    if (*p1 == *p2 || *p1 == '?') return checkfilter_recursive(++p1, ++p2);
    return 0;
} 

int check_push_filter(int port, char* senderPhone) {

    char* filter = find_filter(port);
    if (filter == NULL) {
        return 1;
    } else {
        return checkfilter_recursive(filter, senderPhone);
    }
}

#include <kni.h>
#include <sni.h>

//    private static native void addPushPort(int port, String pushFilter) throws IOException, IllegalArgumentException;
//    private static native int  removePushPort(int port) throws IllegalStateException;
//    private native        int  waitPushEvent() throws java.io.InterruptedIOException;
//    private static native int  hasAvailableData(int port);

#ifdef ENABLE_MIDP
  #include "midpError.h"
#else
  static const char* const midpOutOfMemoryError = "java/lang/OutOfMemoryError";
  static const char* const midpIOException = "java/io/IOException"; 
  static const char* const midpRuntimeException = "java/lang/RuntimeException";
  static const char* const midpIllegalArgumentException = "java/lang/IllegalArgumentException";
  static const char* const midpIllegalStateException = "java/lang/IllegalStateException";
#endif

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_wma_PushConnectionsPool_addPushPort) {

    jint port;

    jint msAddress_len;
    jchar* msAddress_data = NULL;
    unsigned char *pAddress = NULL;

    AppIdType msid = 0;
    int handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(address);
    port = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, address);

    jsr120_sms_pool_init();
    handle = registerSMSEntry(port, msid);

    if (handle != 0) {
        if (KNI_IsNullHandle(address)) {
            KNI_ThrowNew(midpIllegalArgumentException, "wrong push filer");
        } else {
            msAddress_len = KNI_GetStringLength(address);
            msAddress_data = (jchar *)pcsl_mem_malloc(msAddress_len * sizeof(jchar));
            if (msAddress_data == NULL) {
                KNI_ThrowNew(midpOutOfMemoryError, "");
            } else {
                KNI_GetStringRegion(address, 0, msAddress_len, msAddress_data);
                pAddress = (unsigned char*)pcsl_mem_malloc(msAddress_len + 1);
                if (pAddress != NULL) {
                    int i;
                    for (i = 0; i < msAddress_len; i++) {
                        pAddress[i] = (unsigned char)msAddress_data[i];
                    }	
                    pAddress[msAddress_len] = 0;
                }
                pcsl_mem_free(msAddress_data);
	    }
	}

	if (pAddress) {
            register_filter(port, handle, pAddress);
	}
    } else {
        KNI_ThrowNew(midpIOException, "access denied");
    }

    KNI_EndHandles();
    KNI_ReturnVoid();

}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_wma_PushConnectionsPool_removePushPort) {

    int port = KNI_GetParameterAsInt(1);
    int handle = 0;
    handle = unregister_filter(port);
    unregisterSMSEntry(port, handle);

    KNI_ReturnInt(1);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_wma_PushConnectionsPool_waitPushEvent) {
    int handle = 0;
    int port = 0;
    int WMA_SMS_READ_SIGNAL = 0;
    SmsMessage *psmsData = NULL;
    filter_struct* ptr;

    do {
        CVMD_gcSafeExec(_ee, {
           jsr120_wait_for_signal(handle, WMA_SMS_READ_SIGNAL);
        });

        for (ptr=filter_list; ptr; ptr=ptr->next) {
            psmsData = jsr120_sms_pool_peek_next_msg((jchar)ptr->port);
            if (psmsData) {
                port = ptr->port;
                break;
            }
        }
    } while (psmsData == NULL);

    KNI_ReturnInt(port);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_wma_PushConnectionsPool_hasAvailableData) {
    int port = KNI_GetParameterAsInt(1);
    SmsMessage *psmsData = NULL;
    int result = 0;

    psmsData = jsr120_sms_pool_peek_next_msg((jchar)port);
    if (psmsData != NULL) {
        result = port;
    }

    KNI_ReturnInt(result);
}

#endif

#endif

/**
 * check the SMS header against the push filter.
 * @param filter The filter string to be used
 * @param cmsidn The caller's MSIDN number to be tested by the filter
 * @return <code>1</code> if the comparison is successful; <code>0</code>,
 *     otherwise.
 */
int jsr120_check_filter(char *filter, char *cmsidn) {
    char *p1 = NULL;
    char *p2 = NULL;

    if ((cmsidn == NULL) || (filter == NULL)) return 0;

    /* Filter is exactly "*", then all MSIDN numbers are allowed. */
    if (strcmp(filter, "*") == 0) return 1;

    /*
     * Otherwise walk through the filter string looking for character
     * matches and wildcard matches.
     * The filter pointer is incremented in the main loop and the
     * MSIDN pointer is incremented as characters and wildcards
     * are matched. Checking continues until there are no more filter or
     * MSIDN characters available.
     */
    for (p1=filter, p2=cmsidn; *p1 && *p2; p1++) {
        /*
         * For an asterisk, consume all the characters up to
         * a matching next character.
         */
        if (*p1 == '*') {
            /* Initialize the next two filter characters. */
            char f1 = *(p1+1);
            char f2 = '\0';
            if (f1 != '\0') {
                f2 = *(p1+2);
            }

            /* Skip multiple wild cards. */
            if (f1 == '*') {
                continue;
            }

            /*
             * Consume all the characters up to a match of the next
             * character from the filter string. Stop consuming
             * characters, if the address is fully consumed.
             */
            while (*p2) {
                /*
                 * When the next character matches, check the second character
                 * from the filter string. If it does not match, continue
                 * consuming characters from the address string.
                 */
                if(*p2 == f1 || f1 == '?') {
                    if (*(p2+1) == f2 || f2 == '?' || f2 == '*') {
                        /* Always consume an address character. */
                        p2++;
                        if (f2 != '?' || *(p2+1) == '.' || *(p2+1) == '\0') {
                            /* Also, consume a filter character. */
                            p1++;
                        }
                        break;
                    }
                }
                p2++;
            }
        } else if (*p1 == '?') {
            p2 ++;
        } else if (*p1 != *p2) {
            /* If characters do not match, filter failed. */
            return 0;
        } else {
            p2 ++;
 	}
    }

    if (!(*p1)  && !(*p2) ) {
        /* 
         * All available filter and MSIDN characters were checked.
         */
        return 1;
    } else {
        /*
         * Mismatch in length of filter and MSIDN string
         */
        return 0;
    }
}

