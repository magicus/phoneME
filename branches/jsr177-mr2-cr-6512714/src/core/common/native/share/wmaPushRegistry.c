/*
 *
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

#include <string.h>
#include <wmaPushRegistry.h>
#include <pcsl_memory.h>
#include <jsr120_sms_protocol.h>
#include <jsr120_cbs_protocol.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_cbs_listeners.h>

/* Need revisit: delete? */
#if ENABLE_JSR_205
#include <jsr205_mms_protocol.h>
#include <jsr205_mms_listeners.h>
#endif

#include <push_server_resource_mgmt.h>
#include <suitestore_common.h>
#include <kni.h>

static int registerSMSEntry(int port, SuiteIdType msid);
static void unregisterSMSEntry(int port, int handle);
static int registerCBSEntry(int msgID, SuiteIdType msid);
static void unregisterCBSEntry(int msgID, int handle);
#if ENABLE_JSR_205
static int registerMMSEntry(unsigned char *appID, SuiteIdType msid);
static void unregisterMMSEntry(unsigned char *appID, int handle);
#endif

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
                       SuiteIdType msid, char *appID, int fd) {

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
                        SuiteIdType msid, char *appID){

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

static int registerSMSEntry(int port, SuiteIdType msid) {

    int handle = -1;

    /* register SMS port */
    if (jsr120_is_sms_push_port_registered((jchar)port) == WMA_ERR) {

	/* Get a unique handle that will identify this SMS "session" */
	handle = (int)(pcsl_mem_malloc(1));

	if (handle == 0) {
            return -1;
	}

        if (jsr120_register_sms_push_port((jchar)port,
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
    jsr120_unregister_sms_push_port((jchar)port);

    /* Release the handle associated with this connection. */
    pcsl_mem_free((void *)handle);

}

static int registerCBSEntry(int msgID, SuiteIdType msid) {

    int handle = -1;

    /* register CBS message ID */
    if (jsr120_cbs_is_push_msgID_registered((jchar)msgID) == WMA_ERR) {

	/* Get a unique handle that will identify this CBS "session" */
	handle = (int)(pcsl_mem_malloc(1));

	if (handle == 0) {
            return -1;
	}

        if (jsr120_cbs_register_push_msgID((jchar)msgID,
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
    jsr120_cbs_unregister_push_msgID((jchar)msgID);

    /* Release the handle associated with this connection. */
    pcsl_mem_free((void *)handle);

}

#if ENABLE_JSR_205
static int registerMMSEntry(unsigned char *appID,
                            SuiteIdType msid) {

    int handle = -1;

    /* register MMS message ID */
    if (appID != NULL) {
        if (jsr205_mms_is_push_appID_registered(appID) == WMA_ERR) {

	    /* Get a unique handle that will identify this MMS "session" */
            handle = (int)(pcsl_mem_malloc(1));

	    if (handle == 0) {
                return -1;
	    }

            if (jsr205_mms_register_push_appID(appID,
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
        jsr205_mms_unregister_push_appID(appID);
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
