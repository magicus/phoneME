/*
 *
 *
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
 * Push Registry Persistent File Management module.
 *
 * Contains functions and data structures that are used to implement the
 * auto-invocation subsystem. This includes push (i.e., listening
 * for incoming connections), and alarms (responding to time events).
 * The module opens the push registry file and caches the contents in
 * memory at startup. Subsequent additions and deletions
 * update the cache and rewrite the persistent data file.
 */

#ifndef _WIN32_WCE
    #include <errno.h>
#endif
#include <string.h>
#include <java_types.h>

#include <kni.h>
#include <sni.h>

#include <pcsl_memory.h>
#include <pcsl_string.h>

#include <midpMalloc.h>
#include <midpStorage.h>
#include <midp_properties_port.h>
#include <midpResourceLimit.h>
#include <midpError.h>

#include <midpServices.h>
#include <midp_logging.h>
#include <midp_libc_ext.h>
#include <kni_globals.h>
#include <midp_thread.h>
#include <pcsl_network.h>
#include <pcsl_socket.h>
#include <pcsl_serversocket.h>
#include <pcsl_network_notifier.h>
#include <push_server_export.h>
#include <push_server_resource_mgmt.h>
#include <timer_export.h>

#if (ENABLE_JSR_205 || ENABLE_JSR_120)
    #include <wmaPushRegistry.h>
    #include <stdio.h>
#endif

#if ENABLE_JSR_180
    #include <SipPushRegistry.h>
#endif

#if ENABLE_JSR_82
    #include <stdio.h>
    #include "btPush.h"
#endif

#if ENABLE_JSR_257
    #include <ContactlessPush.h>
#endif

#if ENABLE_I3_TEST
    #include <midpUtilKni.h>
#endif

#ifndef MAX_CACHED_DATA_SIZE
    #define MAX_CACHED_DATA_SIZE 1500
#endif /* MAX_CACHED_DATA_SIZE */

/** For build a parameter string. */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(COMMA_STRING)
{',', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(COMMA_STRING);

/** Filename to save the push connections. ("pushlist.txt") */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(PUSH_LIST_FILENAME)
{'p', 'u', 's', 'h', 'l', 'i', 's', 't', '.', 't', 'x', 't', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(PUSH_LIST_FILENAME);

/** Filename to save the alarms. ("alarmlist.txt") */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(ALARM_LIST_FILENAME)
{'a', 'l', 'a', 'r', 'm', 'l', 'i', 's', 't', '.', 't', 'x', 't', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(ALARM_LIST_FILENAME);

/** Pathname for persistent push connection list. */
static pcsl_string pushpathname = PCSL_STRING_NULL_INITIALIZER;
/** Pathname for persistent alarm notification list. */
static pcsl_string alarmpathname = PCSL_STRING_NULL_INITIALIZER;

/** Pointer to error message. */
static char *errStr = NULL;

/* Maximum buffer size for line parsing. */
#define MAX_LINE 512

/**
 * The internal representation of a datagram or TCP packet.
 * Datagrams read by the push mechanism are buffered in the push
 * registry for use by the midlet after it is invoked by push.
 */
typedef struct _packetentry {
    /** The IP Address of the sender. */
    int ipAddress;
    /** The port ID on which the packet was received. */
    int senderport;
    /** The length of data in the buffer. */
    int length;
    /** The offset in the buffer where the read pointer is located. */
    int offs;
    /** The buffer that holds the data of the datagram. */
    char buffer[MAX_CACHED_DATA_SIZE];
} PacketEntry;

/**
 * The internal representation of an entry in the Push Registry list. When
 * the push registry is initialized, the persistent push registry is read
 * and stored in a null-terminated linked-list of elements like this. Updates
 * to the push registry are first made to the linked list in memory, and
 * subsequently written to persistent storage.
 */
typedef struct _pushentry {
    /** Pointer to the next entry in the list. Last entry has NULL here. */
    struct _pushentry *next;
    /** The full text entry from the persistent store. */
    char *value;
    /** The name of the midletsuitestorage persistent store. */
    char *storagename;
    /** The filter with which to filter ip addresses. */
    char *filter;
    /** The socket to listen to. */
    int fd;
    /** The socket to use for the connection. */
    int fdsock;
    /** The socket that is used when buffering the incoming TCP packet. */
    int fdAccepted;
    /** Port id to listen to. */
    int port;
    /** MMS application ID. */
    char *appID;
    /** Current state of the connection. */
    int state;
    /** Pointers for packets that have already arrived. */
    PacketEntry *pCachedData;
    /** Flag denoting whether a WMA message has arrived and been cached. */
    jboolean isWMAMessCached;
    /** True if this entry should be handled by WMA rather then by MIDP Push. */
    jboolean isWMAEntry;
#if ENABLE_JSR_180
    /** True if this entry is a SIP-entry. */
    jboolean isSIPEntry;
    /** Marks shared connection for quick search */
    jboolean isShared;
#endif
#if ENABLE_JSR_257
    /** True if this entry is a JSR 257 (NFC) entry. */
    jboolean isNFCEntry;
#endif
} PushEntry;

/**
 * The internal representation of an alarm entry in the Push Registry
 * list. When
 * the push registry is initialized, the persistent push registry is read
 * and stored in a null-terminated linked-list of elements like this. Updates
 * to the push registry are first made to the linked list in memory, and
 * subsequently written to persistent storage.
 */
typedef struct _alarmentry {
    /** Pointer to the next entry in the list. Last entry has NULL here. */
    struct _alarmentry *next;
    /**
     * Pointer to a string containing the name of the MIDlet to be
     * wakened.
     */
    char *midlet;
    /**
     * Pointer to a string containing the persistent store name of the
     * MIDletSuite.
     */
    char *storagename;
    /** Absolute time after which this alarm is to be fired. */
    jlong wakeup;
    /** Handle to the Timer instance currently counting this alarm. */
    int timerHandle;
    /** The state of the current alarm. */
    int state;
} AlarmEntry;

static PushEntry *pushlist = NULL;
static AlarmEntry *alarmlist = NULL;

typedef enum {
    NET_STATUS_DOWN       = -3,
    NET_STATUS_GOING_DOWN = -2, /* network finalization is in progress */
    NET_STATUS_ERROR      = -1,
    NET_STATUS_UNKNOWN    = 0,
    NET_STATUS_GOING_UP   = 1, /* network initialization is in progress */
    NET_STATUS_UP         = 2
} network_status_t;


/**
 * Current network status.
 */
static network_status_t networkStatus = NET_STATUS_UNKNOWN;

/**
 * Checks current status of the network.
 *
 * @return <tt>1</tt> if network is enabled, <tt>0</tt> otherwise
 */
static int isNetworkUp();

static int pushlength = 0;
static int pushProcessPort(PushEntry* pe);
static void pushStartListening();
static int alarmopen();
static void alarmsave();
static void pushListFree();
static void alarmListFree();
static int parsePushList(int);
static int parseAlarmList(int);
static int checkfilter(char *filter, char *ip);
static void pushcheckinentry(PushEntry *p);
static void pushcleanupentry(PushEntry *p);
static void pushDeleteSuiteNoVM(SuiteIdType id);
static void pushDeleteSuiteLive(SuiteIdType id);
static int pushOpenInternal(int startListening);
static void pushDeleteEntry(PushEntry *p, PushEntry **pPrevNext);
static void alarmstart(AlarmEntry *entry, jlong alarm);
static long readLine(char** ppszError, int handle, char* buffer, long length);

static void pcsl_network_initialized(int isInit, int status);

/**
 * Parses and extracts a field from the registry entry string.
 *
 * @param value address of registry entry string
 * @param field which field to extract
 *
 * @return address of the desired field (comma or null terminated)
 *
 */
static char *pushstorage(char *value, int field) {
    char *storagefield;
    char *storagename = NULL;
    int comma = 0;

    for (storagefield = value; *storagefield; storagefield++) {
        if (*storagefield == ',') {
            comma ++;
        }

        /* Push entry contains "connection, midletname, filter, storage" */
        /* Alarm entry contains "midletname, alarm, storage" */
        if (comma == field) {
            storagename = storagefield + 1;
            break;
        }
    }

    return storagename;
}

#if (ENABLE_JSR_205 || ENABLE_JSR_120)
/**
 * Converts a given string to a suite ID.
 *
 * @param strId a string to convert
 *
 * @return suite ID (UNUSED_SUITE_ID if conversion failed)
 *
 */
static SuiteIdType suiteIdFromChars(const char* strId) {
    unsigned long lid;
    SuiteIdType suiteId = UNUSED_SUITE_ID;

    if (strId) {
        /* IMPL NOTE: here it's assumed that strId represents a HEX integer */
        if (sscanf(strId, "%lx", &lid) == 1) {
            suiteId = (SuiteIdType)lid;
        }
    }

    return suiteId;
}
#endif

/**
 * Parses and extracts the filter field from the registry entry string.
 *
 * @param value address of registry entry string
 * @return a copy of the filter field from the registry entry
 *
 */
static char *pushfilter(char *value) {
    char *ptr, *filter_dup;
    char *filter = pushstorage(value, 2);

    /* Push entry contains "connection, midletname, filter, storage" */
    for (ptr = filter; *ptr; ptr++) {
        if (*ptr == ',')
            break;
    }
    *ptr = '\0';
    filter_dup = midpStrdup(filter);
    *ptr = ',';
    return filter_dup;
}

/**
 * Parses and extract the MIDlet class name field from connection entry
 * string.
 *
 * @param value address of connection entry string
 * @param pLength address of where the length of the name should be output
 *
 * @return address of the desired field
 *
 */
static char *pushclassname(char *value, int *pLength) {
    char *classfield;
    char *classname = NULL;
    int length = 0;

    /* Push entry contains "connection,midletname,filter,storage" */
    for (classfield = value; *classfield != 0; classfield++) {
        if (*classfield == ',') {
            classname = classfield + 1;

            for (classfield++; *classfield != 0 && *classfield != ',';
                classfield++, length++);

            break;
        }
    }

    *pLength = length;
    return classname;
}

/**
 * Checks if the given string represents a datagram connection
 * or a socket connection.
 *
 * @param pBuffer buffer with a string describing the connection
 * @param checkIfDatagram defines what this function should check: if 1,
 *                        it should be checked that the given string represents
 *                        a datagram connection, 0 - a socket connection
 *
 * @return 1 if the given buffer represents the given connection type
 */
static int pushIsConnectionOfGivenType(const char* pBuffer,
                                       int checkIfDatagram) {
    if (pBuffer == NULL) {
        return 0;
    }

    if (strncmp(pBuffer, "datagram://:", 12) == 0) {
        return checkIfDatagram;
    }
#if ENABLE_SERVER_SOCKET
    else if (strncmp(pBuffer, "socket://:", 10) == 0) {
        return !checkIfDatagram;
    }
#endif

#if ENABLE_JSR_180
    /* Check for JSR180 SIP/SIPS connections. */
    if ((strncmp(pBuffer, "sips:", 5) == 0) ||
        (strncmp(pBuffer, "sip:", 4) == 0)) {
        /*
         * IMPL_NOTE: the following check is very simplified, but its enough
         *            for the practical cases.
         */
        const char* pTransport = strstr(pBuffer, "transport=tcp");
        return checkIfDatagram != (pTransport != NULL);
    }
#endif

    return 0;
}

/**
 * Checks if the given string represents a socket connection.
 *
 * @param pBuffer buffer with a string describing the connection
 *
 * @return 1 if the given buffer represents a socket connection, 0 otherwise
 */
static int pushIsSocketConnection(const char* pBuffer) {
    int f = pushIsConnectionOfGivenType(pBuffer, 0);
    return f;
}

/**
 * Checks if the given string represents a datagram connection.
 *
 * @param pBuffer buffer with a string describing the connection
 *
 * @return 1 if the given buffer represents a datagram connection, 0 otherwise
 */
static int pushIsDatagramConnection(const char* pBuffer) {
    int f = pushIsConnectionOfGivenType(pBuffer, 1);
    return f;
}

/**
 * Adds a network notifier for the given push entry.
 *
 * @param pe push entry for which a network notifier should be added
 */
static void pushAddNetworkNotifier(PushEntry* pe) {
#if ENABLE_JSR_257
    if(pe->isNFCEntry) {
        return;
    }
#endif 

    /*
     * WMA connections have their own notification system.
     * So add a notifier only if its not WMA connection.
     */
    if (!pe->isWMAEntry) {
        /* Push only needs to know if a socket has data. */
        if (pushIsDatagramConnection(pe->value)) {
            pcsl_add_network_notifier((void *)pe->fd, PCSL_NET_CHECK_READ);
        } else if (pushIsSocketConnection(pe->value)) {
            pcsl_add_network_notifier((void *)pe->fd, PCSL_NET_CHECK_ACCEPT);
        }
    }
}

/**
 * Opens the pushregistry files and populate the push memory structures.
 *
 * @param <none>
 * @return <tt>0</tt> for success, non-zero if there is a resource problem
 */
int pushopen() {
#if ENABLE_JSR_82
    bt_push_startup();
#endif
    return pushOpenInternal(1);
}

/**
 * Opens the Push Registry file, if it exists and populate
 * an in memory cache of the file contents.
 *
 * @param startListening if KNI_TRUE the push system will listen for incoming data
 *    on the push connections.
 *
 * @return 0 for success else non-zero if a resource problem
 */
static int pushOpenInternal(int startListening) {
    int pushfd;
    int status, push_status, alarm_status;

    push_status = alarm_status = 0;

    /* Check whether the alarm file has been already read. */
    if (PCSL_TRUE == pcsl_string_is_null(&alarmpathname)) {
        /*
         * Initialize the fully qualified pathname
         * of the push persistent file.
         */
        if (PCSL_STRING_OK != pcsl_string_cat(
                                             storage_get_root(INTERNAL_STORAGE_ID),
                                             &ALARM_LIST_FILENAME, &alarmpathname)) {
            alarm_status = -1;
        } else {
            alarm_status = alarmopen();
        }

        if (alarm_status != 0) {
            REPORT_ERROR(LC_PROTOCOL, "Error: alarm open failed");
            pcsl_string_free(&alarmpathname);
        }

    }

    /* Check whether the push file has been already read. */
    if (PCSL_TRUE == pcsl_string_is_null(&pushpathname)) {
        /*
         * Initialize the fully qualified pathname
         * of the push persistent file.
         */
        if (PCSL_STRING_OK != pcsl_string_cat(
                                             storage_get_root(INTERNAL_STORAGE_ID),
                                             &PUSH_LIST_FILENAME, &pushpathname)) {
            push_status = -1;
        } else {
            /* Now read the registered connections. */
            pushfd = storage_open(&errStr, &pushpathname, OPEN_READ);
            if (errStr == NULL) {
                /* Read through the file one line at a time */
                status = parsePushList(pushfd);

                /* Close the storage handle */
                storageClose (&errStr, pushfd);
                storageFreeError(errStr);

                /*
                 * IMPL_NOTE: don't check for -1 because in this case some
                 * entries can be read anyway
                 */
                if (status != -2) { /* out of memory */
                    if (startListening) {
                        pushStartListening();
                    }
                } else {
                    push_status = -1;
                }
            } else {
                REPORT_WARN1(LC_PROTOCOL,
                             "Warning: could not open push registration file: %s",
                             errStr);
                /* 
                 * This is normal until the first push registration
                 * is created.
                 */
                storageFreeError(errStr);
            }
        }

        if (push_status != 0) {
            REPORT_ERROR(LC_PROTOCOL, "Error: push open failed");
            pcsl_string_free(&pushpathname);
        }
    }

    return(!push_status && !alarm_status) ? 0 : -1;
}

/**
 * Destroys the push and alarm memory resources. Maintain the push
 * registrations in the push registry file.
 */
void pushclose() {
    pushListFree();
    alarmListFree();
#if ENABLE_JSR_82
    bt_push_shutdown();
#endif
    pcsl_string_free(&pushpathname);
    pcsl_string_free(&alarmpathname);
}

/**
 * Saves the in memory cache of push registrations to a persistent
 * file for use in subsequent runs.
 */
static void pushsave() {
    int  pushfd;
    PushEntry *p;

    pushfd = storage_open(&errStr, &pushpathname, OPEN_READ_WRITE_TRUNCATE);
    if (errStr == NULL){
        /* Write a new list of push registrations to the persistent file */
        for (p = pushlist; p != NULL ; p = p->next) {
            storageWrite(&errStr, pushfd, p->value, strlen(p->value));
            storageWrite(&errStr, pushfd, "\n", 1);
        }
        /* Close the storage handle */
        storageClose (&errStr, pushfd);
    } else {
        REPORT_WARN1(LC_PROTOCOL,
                     "Warning: could not write push registration file: %s",
                     errStr);
        storageFreeError(errStr);
    }
}

/**
 * Adds one entry to the push registry.
 * If the entry already exists return IO_ERROR_LEN (midpString.h).
 *
 * @param suiteId ID of the suite
 * @param connection generic connection name (no spaces)
 * @param midlet class name of the MIDlet (no spaces)
 * @param filter filter string (no spaces)
 *
 * @return 0 for success, OUT_OF_MEM_LEN for out of memory,
 * IO_ERROR_LEN if already registered
 */
int midpAddPushEntry(SuiteIdType suiteId,
                     const pcsl_string * connection,
                     const pcsl_string * midlet,
                     const pcsl_string * filter) {
    int opened = 0;
    int status = 0;
    pcsl_string temp = PCSL_STRING_NULL;
    const jbyte* pszTemp = NULL;
    jsize total_len;

    if (NULL == pushlist) {
        /* VM not running */

        /* Tell push open not to start listening to connections. */
        if (pushOpenInternal(0) != 0) {
            return OUT_OF_MEM_LEN;
        }

        opened = 1;
    }

    total_len = pcsl_string_length(connection)
                + pcsl_string_length(midlet)
                + pcsl_string_length(filter)
                + GET_SUITE_ID_LEN(suiteId)
                + PCSL_STRING_LITERAL_LENGTH(COMMA_STRING) * 3;

    pcsl_string_predict_size(&temp, total_len);

    status = OUT_OF_MEM_LEN;
    do {
        if (PCSL_STRING_OK != pcsl_string_append(&temp, connection)) {
            break;
        }

        if (PCSL_STRING_OK != pcsl_string_append(&temp, &COMMA_STRING)) {
            break;
        }

        if (PCSL_STRING_OK != pcsl_string_append(&temp, midlet)) {
            break;
        }

        if (PCSL_STRING_OK != pcsl_string_append(&temp, &COMMA_STRING)) {
            break;
        }

        if (PCSL_STRING_OK != pcsl_string_append(&temp, filter)) {
            break;
        }

        if (PCSL_STRING_OK != pcsl_string_append(&temp, &COMMA_STRING)) {
            break;
        }

        if (PCSL_STRING_OK != pcsl_string_append(&temp,
                                                 midp_suiteid2pcsl_string(suiteId))) {
            break;
        }

        pszTemp = pcsl_string_get_utf8_data(&temp);
        if (NULL == pszTemp) {
            break;
        }

        status = pushadd((char *)pszTemp);
        pcsl_string_release_utf8_data(pszTemp, &temp);
        if (-1 == status) {
            status = IO_ERROR_LEN;
        } else if (-2 == status) {
            status = OUT_OF_MEM_LEN;
        }
    } while (0);

    pcsl_string_free(&temp);

    if (opened) {
        pushclose();
    }

    return status;
}

/**
 * Adds one entry to the push registry.
 * If the entry already exists return an error.
 * On successful registration, write a new copy of the file to disk.
 *
 * @param str A push entry string.
 * @return <tt>0</tt> if successful,
 *         <tt>-1</tt> if the entry already exists,
 *         <tt>-2</tt> if out of memory
 *         <tt>-3</tt> if illegal arguments or unknown connection type
 */
int pushadd(char *str) {
    PushEntry *pe;
    int comma;
    char *cp;
    int ret;

    /* Count the characters up to the first comma. */
    for (comma = 0, cp = str; *cp; comma++, cp++) {
        if (*cp == ',') {
            break;
        }
    }

    /* Check if the entry already exists? */
    for (pe = pushlist; pe != NULL ; pe = pe->next) {
        if (strncmp (str, pe->value, comma) == 0) {
            return -1 ;
        }
    }

    /* Add the new entry. */
    pe = (PushEntry *)midpMalloc(sizeof(PushEntry));
    if (NULL == pe) {
        return -2;
    }

    pe->value = midpStrdup(str);
    pe->storagename = midpStrdup(pushstorage(str, 3));
    pe->filter = pushfilter(str);

    if ((pe->value == NULL) || (pe->storagename == NULL) ||
            (pe->filter == NULL)) {
        midpFree(pe->value);
        midpFree(pe->storagename);
        midpFree(pe->filter);
        midpFree(pe);

        return -2;
    }

    pe->state = AVAILABLE;
    pe->fd = -1;
    pe->fdsock = -1;
    pe->fdAccepted = -1;
    pe->pCachedData = NULL;
    pe->isWMAEntry = KNI_FALSE;
    pe->isWMAMessCached = KNI_FALSE;
    pe->appID = NULL;

#if ENABLE_JSR_180
    pe->isSIPEntry = KNI_FALSE;
#endif

#if ENABLE_JSR_257
    pe->isNFCEntry = KNI_FALSE;
#endif
    /*
#if ENABLE_JSR_82
    bt_push_register_url(str, NULL, 0);
#endif
    */
    ret = pushProcessPort(pe);
    
    if (pe->fd == -1) {
#if ENABLE_JSR_82
        bt_push_unregister_url(str);
#endif
        if (ret == -3) {
            /* 
             * Reject the registration and return error code (-3) for above
             * code to raise IllegalArgumentException exception.
             */
            midpFree(pe->value);
            midpFree(pe->storagename);
            midpFree(pe->filter);
            midpFree(pe);
            return -3;
        }

        /* 
         * Don't reject the registration.
         * Port may be in use by a native application but we can't check this,
         * there is a chance network is just not up yet.
         */
    } else {
        pe->state = CHECKED_IN;
        pushAddNetworkNotifier(pe);
    }

    pe->next = pushlist;
    pushlist = pe;
    pushlength++;

    pushsave();

    return 0;
}

/**
 * Removes one entry from the push registry.
 * If the entry is not registered return an error.
 * On successful deletion, write a new copy of the file to disk.
 *
 * @param str The push entry string to be deleted
 * @param store The MIDletSuite storagename
 * @return <tt>0</tt> if successful, <tt>-2</tt> if the entry belongs
 *         to another suite.
 */
int pushdel(char *str, char *store) {
    PushEntry *p;
    PushEntry **pPrevNext = &pushlist;
    PushEntry *tmp;

    /* Find the entry to remove. */
    for (p = pushlist; p != NULL ; p = tmp) {
        tmp = p->next;
        if (strncmp (str, p->value, strlen(str)) == 0) {
            /* Check if the connection belongs to another suite. */
            if (strcmp(store, p->storagename) != 0) {
                return -2 ;
            }
#if ENABLE_JSR_82
            bt_push_unregister_url(str);
#endif
            pushDeleteEntry(p, pPrevNext);
            pushsave();
            return 0;
        }

        pPrevNext = &p->next;
    }
    return -1;
}

/**
 * Deletes the given entry from the push registry.
 * This function does the actual work.
 *
 * @param p A pointer to the push entry to be removed
 * @param pPrevNext Address of the <tt>Next</tt> field of the
 *        previous push entry
 */
static void pushDeleteEntry(PushEntry *p, PushEntry **pPrevNext) {
    void *context = NULL;
    if (p->fd != -1) {
        /*
         * Cleanup any connections before closing
         * the server socket.
         */
        if (p->state != CHECKED_OUT) {
            /* pushcleanupentry sets the state to CHECKED_IN */
            pushcleanupentry(p);

            /* closing will disconnect any socket notifiers */
            if (pushIsSocketConnection(p->value)) {
#if ENABLE_SERVER_SOCKET
                pcsl_socket_close_start((void*)(p->fd), &context);
                /* Update the resource count */
                if (midpDecResourceCount(RSC_TYPE_TCP_SER, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL, "(Push)TCP Server : Resource limit"
                                " update error");
                }
#endif
            } else if (pushIsDatagramConnection(p->value)) {
                pcsl_datagram_close_start((void *)p->fd, &context);
                /* Update the resource count */
                if (midpDecResourceCount(RSC_TYPE_UDP, 1) == 0) {
                    REPORT_INFO(LC_PROTOCOL, "(Push)Datagram : Resource limit"
                                " update error");
                }
            }
        }
#if ENABLE_JSR_257
    if(p->isNFCEntry) {
        contactless_push_unregister(&p->fd);    
    } 
#endif 
        
#if (ENABLE_JSR_205 || ENABLE_JSR_120)
        /* Check for sms,cbs or mms connection. */
        wmaPushCloseEntry(p->state, p->value, p->port,
                          suiteIdFromChars(p->storagename), p->appID, p->fd);
#endif
        p->fd = -1;
    }

    p->state = AVAILABLE;

    midpFree(p->value);
    p->value = NULL;

    midpFree(p->filter);
    p->filter = NULL;

    midpFree(p->storagename);
    p->storagename = NULL;

    /* Remove the registration entry from the list. */
    *pPrevNext = p->next;

    midpFree(p);

    pushlength--;

}

/**
 * Returns the number of buffered bytes for the given socket.
 *
 * @param fd handle of the socket
 * @return number of cached bytes of -1 if none
 */
int pushcacheddatasize(int fd) {
    PushEntry *p;

    for (p = pushlist; p != NULL ; p = p->next) {
        if ((p->fd == fd && p->pCachedData != NULL) ||
            (p->fdAccepted == fd && p->pCachedData != NULL)) {
            return(p->pCachedData->length - p->pCachedData->offs);
        }
    }

    return -1;
}

/**
 * Fetch the buffered datagram or TCP packet into a buffer.
 *
 * @param fd The handle of the socket from which the packet was received
 * @param ip The ip address of the incoming datagram
 * @param sndport The port from which the data was sent
 * @param buf A pointer to a buffer into which the data should be copied
 * @param len The size of buf
 * @return the length of the returned data if successful, or <tt>-1</tt>
 *         if unsuccessful.
 */
int pushgetcachedpacket(int fd, int *ip, int *sndport, char *buf, int len) {
    PushEntry *p;
    int length = -1;

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL; p = p->next) {
        if (((p->fd == fd && p->fdAccepted == -1) || (p->fdAccepted == fd)) &&
            (p->pCachedData != NULL)) {
            /* Return the cached data. */
            *ip = p->pCachedData->ipAddress;
            *sndport = p->pCachedData->senderport;

            length = p->pCachedData->length - p->pCachedData->offs;
            if (length > 0) {
                if (len < length) {
                    length = len;
                }

                memcpy(buf, &p->pCachedData->buffer[p->pCachedData->offs],
                       length);

                p->pCachedData->offs += length;
            }

            if (p->pCachedData->offs >= p->pCachedData->length) {
                p->fdAccepted = -1;
                /* Destroy the cached entry after it has been read. */
                midpFree(p->pCachedData);
                p->pCachedData = NULL;
            }

            return length;
        }
    }

    return -1;
}

/**
 * Checks out the handle for the requested server socket.
 * @param fd The handle to check out
 * @return the handle to the checked-out server socket
 */
int pushcheckoutaccept(int fd) {
    PushEntry *p;
    int temp;

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = p->next) {
        if (p->fd == fd) {
            temp = p->fdsock;
            p->fdsock = -1;
            return temp;
        }
    }

    return -1;
}

/**
 * Checks out the handle for the requested connection.
 * The CHECKED_OUT token
 * is left in the registry to indicate that an application is
 * actively using the connection.
 *
 * @param protocol The protocol of the connection
 * @param port The port number of the connection
 * @param store The storage name of the requesting Suite
 *
 * @return <tt>-1</tt> if the connection is not found, other wise returns
 * the previously opened file descriptor.
 */
int pushcheckout(char* protocol, int port, char * store) {
    PushEntry *p;
    int fd;
    jboolean wmaProtocol = KNI_FALSE;
    jboolean standardProtocol = KNI_FALSE;
#if ENABLE_JSR_82
    bt_bool_t is_bluetooth = bt_is_bluetooth_url(protocol);
#endif

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = p->next) {
#if ENABLE_JSR_82
        if (is_bluetooth == BT_BOOL_TRUE &&
            !strncmp(p->value, protocol, strlen(protocol))) {
            if (strcmp(store, p->storagename)) {
                return -2;
            }
            return 0;
        }
#endif

#if (ENABLE_JSR_205 || ENABLE_JSR_120)
        wmaProtocol = isWmaProtocol(p->port, p->value,
                                    p->storagename, port, store);
#endif


#if ENABLE_JSR_180
        /*
         * A registered 'sip' connection matches physical 'datagram'
         * connection for UDP transport and 'socket' connection for TCP.
         */
        standardProtocol = (p->port == port &&
                            (!strncmp(p->value, protocol, strlen(protocol)) ||
                             (strncmp(p->value, "sip", 3) == 0 &&
                              ((!strncmp("datagram", protocol, strlen(protocol)) && 
                                pushIsDatagramConnection(p->value)) ||
                               (!strncmp("socket", protocol, strlen(protocol)) && 
                                pushIsSocketConnection(p->value)))
                             )
                            )
                           );
#else
        /* Port and protocol must match before other checks are done. */
        standardProtocol = (p->port == port &&
                            strncmp(p->value, protocol, strlen(protocol)) == 0);
#endif
        if (standardProtocol || wmaProtocol) {
            /* Check if the current suite reserved the port. */
            if (strcmp(store, p->storagename) != 0) {
                return -2;
            }

            fd = p->fd;

            /* The push system should stop monitoring this connection. */
            if (pushIsSocketConnection(p->value)) {
                pcsl_remove_network_notifier((void*)fd, PCSL_NET_CHECK_ACCEPT);
            } else if (pushIsDatagramConnection(p->value)) {
                pcsl_remove_network_notifier((void*)fd, PCSL_NET_CHECK_READ);
            }

            p->state = CHECKED_OUT;

            return fd;
        }
    }

    return -1;
}

/**
 * Checks in the handle for the requested connection.
 * @param fd The handle to be checked in
 * @return <tt>0</tt> if successful, or <tt>-1</tt> on failure to check in the
 * file descriptor to the cached push registry.
 */
int pushcheckin(int fd) {
    PushEntry *p;

    /* Find the entry to check in the open file descriptor. */
    for (p = pushlist; p != NULL ; p = p->next) {
        if (p->fd == fd) {
            if (p->state == CHECKED_OUT) {
                pushcheckinentry(p);
            }

            return 0;
        }
    }

    return -1;
}

/**
 * Checks in all the push connections. Used to cleanup by runMIDletWithArgs.
 */
void pushcheckinall() {
    PushEntry *p;

    for (p = pushlist; p != NULL ; p = p->next) {
        pushcheckinentry(p);
    }
}

/**
 * Checks in the push connection(s) left over from the previous suite
 * that were not opened. Used between VM starts by runMIDletWithArgs.
 * The method does not check in connections that may still be used
 * by the next MIDlet, but leaves them ready to check out.
 *
 * @param nextToRun suiteId of the next suite to run, any connection
 *     that is not checked in and does not belong to the next suite to
 *     to run will be checked in.
 */
void pushcheckinLeftOvers(SuiteIdType nextToRun) {
    PushEntry *p;
    const pcsl_string* strSuiteId = midp_suiteid2pcsl_string(nextToRun);
    const char* pszTemp = (char*)pcsl_string_get_utf8_data(strSuiteId);
    /* IMPL_NOTE: can we do anything meaningful in case of out-of-memory error? */

    for (p = pushlist; p != NULL ; p = p->next) {
        if (p->state == CHECKED_IN) {
            continue;
        }

        if (NULL != pszTemp) {
            if (0 == strcmp(pszTemp, p->storagename)) {
                /* matches next suite to run, do not check in */
                continue;
            }
        }

        pushcheckinentry(p);
    }

    pcsl_string_release_utf8_data((jbyte*)pszTemp, strSuiteId);
}

/**
 * Checks in connections that are in
 * launch pending state for a specific MIDlet.
 *
 * @param suiteId Suite ID of the MIDlet
 * @param pszClassName Class name of the MIDlet
 */
void
pushcheckinbymidlet(SuiteIdType suiteId, char* pszClassName) {
    PushEntry *p;
    char* pTemp;
    int tempLen;
    const pcsl_string* strId = midp_suiteid2pcsl_string(suiteId);
    const char* pszSuiteId = (char*)pcsl_string_get_utf8_data(strId);

    for (p = pushlist; p != NULL ; p = p->next) {
#ifdef ENABLE_JSR_82
        /* IMPL_NOTE: Provide a separate function for this functionality.
         * This function is called when a MIDlet terminates, and we use it to
         * re-activate services closed by the MIDlet. This bears no connection
         * to the launch pending state (see function comment).
         */
        bt_port_t port;
        if (bt_push_parse_url(p->value, &port, NULL) == BT_RESULT_SUCCESS) {
            bt_handle_t handle = bt_push_start_server(&port);
            if (handle != BT_INVALID_HANDLE) {
                p->fd = (int)handle;
            }
            continue;
        }
#endif
        if (p->state != LAUNCH_PENDING) {
            continue;
        }

        if (strcmp(p->storagename, pszSuiteId) != 0) {
            continue;
        }

        pTemp = pushclassname(p->value, &tempLen);
        if (strncmp(pTemp, pszClassName, tempLen) != 0) {
            continue;
        }

        pushcheckinentry(p);
    }

    pcsl_string_release_utf8_data((jbyte*)pszSuiteId, strId);
}

/**
 * Checks in the handle for the requested connection,
 * given the connection name.
 * @param str The connection name
 * @return <tt>0</tt> on success, or <tt>-1</tt> on failure to check in the
 * file descriptor to the cached push registry.
 */
int pushcheckinbyname(char* str) {
    PushEntry *p;

    /* Find the entry to remove */
    for (p = pushlist; p != NULL ; p = p->next) {
        if (strncmp (str, p->value, strlen(str)) == 0) {
            pushcheckinentry(p);
            return 0;
        }
    }

    return -1;
}

/**
 * Checks in the push entry and cleanup anything the MIDlet did not get.
 * @param pe The push entry to check in.
 */
static void pushcheckinentry(PushEntry *pe) {
    pushcleanupentry(pe);

    if (pe->fd != -1) {
        pe->state = CHECKED_IN;
        pushAddNetworkNotifier(pe);
    }
}

/**
 * Cleanup anything cached in the entry the MIDlet did not get.
 * But do not close the server port.
 * @param p The push entry to clean up
 */
static void pushcleanupentry(PushEntry *p) {
#if ENABLE_JSR_82
    if (bt_is_bluetooth_url(p->value)) {
        bt_push_reject(bt_push_find_server((bt_handle_t)p->fd));
        p->fdsock = -1;
        return;
    }
#endif
    /* Close any accepted socket not accessed, yet (if any). */
    if (p->fdsock != -1) {
#if ENABLE_SERVER_SOCKET
        void *context;

        pcsl_socket_close_start((void*)(p->fdsock), &context);
        /*
         * Update the resource count
         * Accepted sockets should be counted against client socket
         * resource limit and not serversocket
         */
        if (midpDecResourceCount(RSC_TYPE_TCP_CLI, 1) == 0) {
            REPORT_INFO(LC_PROTOCOL, "(Push)TCP Client : Resource limit"
                        " update error");
        }
#endif
        p->fdsock = -1;
        p->fdAccepted = -1;
    }

    /* Remove the cached datagram (if any). */
    if (p->pCachedData != NULL) {
        midpFree(p->pCachedData);
        p->pCachedData = NULL;
    }
}

#if ENABLE_JSR_180

/**
 * Checks if SIP header is chached into buffer. End-of-data
 * marker is double CRLF
 * 
 * @param buffer    buffer to check
 * @param len       a length of the buffer
 * 
 * @return KNI_TRUE if SIP header is present at the buffer
 *         wholly, KNI_FALSE otherwise
 */
static jboolean checkForEndOfHeader(char* buffer, int len) {
    while (len--) {
        /* can't just read unsigned double word because some platform has 
           address alignment */
        if ('\r' == *buffer && len > 2) {
            if ('\n' == buffer[1] && '\r' == buffer[2] && '\n' == buffer[3]) {
                return KNI_TRUE;

            }
        }
        buffer++;
    }
    return KNI_FALSE;
}

/**
 * Applies SIP filtering rules on the given push entry.
 * IMPL_NOTE: it should be moved to JSR 180 workspace.
 *
 * @param pushp the push entry to filter
 *
 * @return a midlet suite to launch or NULL
 */
static char* pushApplySipFilter(PushEntry* pushp) {
    unsigned char *sender = NULL;
    unsigned char *acceptcontact_type = NULL;
    unsigned char *required_type = NULL;
    char *p;
    int required_type_len;
    PushEntry* next;
    jboolean found = KNI_FALSE;

    /*
     * SIP Datagram and Socket connections use the SIP
     * "From header URI" filter. First, extract the sender
     * from the cached message, then check for a match
     * with the filter pattern string.
     */
    sender = getSipFromHeaderURI((unsigned char *)
                                 pushp->pCachedData->buffer,
                                 pushp->pCachedData->length);
    next = pushp;
    do {
        if (next->isSIPEntry) {
            if (checksipfilter((unsigned char *)next->filter, sender)) {
                /*
                 * If a media type tag was specified in the connection URI,
                 * then the message is only dispatched if it contains
                 * an Accept-Contact header with a matching media feature
                 * tag.
                 */
                required_type_len = getMIMEType(next->value, &p);
                if (required_type_len) {
                    required_type = (unsigned char *)
                                    pcsl_mem_malloc(required_type_len + 1);

                    if (required_type != NULL) {
                        strncpy((char *)required_type, p, required_type_len);
                        required_type[required_type_len] = '\0';
                    }

                    /*
                     * Extract the message media type.
                     */
                    acceptcontact_type = 
                    getSipAcceptContactType(
                                           (unsigned char *) pushp->pCachedData->buffer,
                                           pushp->pCachedData->length);

                    if (acceptcontact_type != NULL &&
                        midp_strcasecmp((char *)required_type,
                                        (char*)acceptcontact_type) == 0) {
                        REPORT_INFO2(LC_PROTOCOL,
                                     "SIP Push Message Media Type Matched: %s == %s",
                                     required_type,acceptcontact_type);


                        /* Required type matched. */
                        found = KNI_TRUE;
                        break;
                    }

                    REPORT_INFO2(LC_PROTOCOL,
                                 "SIP Push Message Media Type Filtered: %s != %s",
                                 required_type, acceptcontact_type);

                } else {
                    /* No type required. */
                    found = KNI_TRUE;
                    break;
                }
            }
            if (!pushp->isShared || NULL == next->next) {
                /* for dedicated connection 
                   and last entry of shared connections chain */
                pushcheckinentry(next);
                /* and clear chached data for shared connection */
                midpFree(pushp->pCachedData);
                pushp->pCachedData = NULL;
                break;
            } else if (pushp == next) {
                /*  mark first chain element as checked in and look for
                    next element */
                next->state = CHECKED_IN;
            } else {
                /*  keep the state for the rest:
                    non-sip and  intermediate chanin member */
            }
        }
        /* scan for entries shared the same connection */
        next = next->next;
    } while (pushp->isShared);

    midpFree(acceptcontact_type);
    midpFree(required_type);
    midpFree(sender);

    if (found && LAUNCH_PENDING != next->state) {
        next->state = LAUNCH_PENDING;
        return midpStrdup(next->value);
    }

    return NULL;
}
#endif

#if ENABLE_SERVER_SOCKET
/**
 * Accepts the incoming connection.
 *
 * @param pushp push entry with information about the server socket
 * @param prevState state of the push entry before transition to LAUNCH_PENDING
 *
 * @return a midlet suite to launch or NULL
 */
static char* pushAcceptConnection(PushEntry* pushp, int prevState) {
    int status;
    char ipAddress[MAX_HOST_LENGTH];
    void *clientHandle;
    void *context;

    /*
     * accept0() returns a brand new client socket descriptor.
     * So resource check should be done against a client socket
     * limit.
     *
     * IMPL_NOTE : an IOException should be thrown when the resource
     * is not available, but since this is not a shared code, only
     * NULL is returned that would result in returning -1
     * to getMIDlet0().
     */
    if (midpCheckResourceLimit(RSC_TYPE_TCP_CLI, 1) == 0) {
        REPORT_INFO(LC_PROTOCOL, "(Push)Resource limit exceeded for"
                    " TCP client sockets");
        pushp->fdsock = -1;
        pushp->state = prevState;
        return NULL;
    }

#ifdef ENABLE_JSR_180
    if (prevState == RECEIVED_EVENT && pushp->pCachedData != NULL) {
        status = pcsl_socket_read_finish(
                                        (void *)pushp->fdsock, (unsigned char *)pushp->pCachedData->buffer,
                                        MAX_CACHED_DATA_SIZE, &(pushp->pCachedData->length),
                                        &context);

        if (status == PCSL_NET_WOULDBLOCK) {
            /* do nothing
             the case will  be proceeded below */
        } else if (status != PCSL_NET_SUCCESS) {
            /*
             * Receive failed - no data available.
             * Cancel the launch pending.
             */
            REPORT_ERROR1(LC_PUSH,
                          "(Push) Cannot receive data, errno = %d\n",
                          pcsl_network_error((void*)pushp->fdsock));
            pushcheckinentry(pushp);
            return NULL;
        }
        if (checkForEndOfHeader(pushp->pCachedData->buffer, 
                                MAX_CACHED_DATA_SIZE)){
            pushp->fdAccepted = pushp->fdsock;
            pushp->state = prevState;
            return pushApplySipFilter(pushp);
        } else {
            pushp->state = WAITING_DATA;
            /* wait for end of header */
            pcsl_add_network_notifier((void *)pushp->fdsock,
                                      PCSL_NET_CHECK_READ);
        }
        /* wait for end of header */
        return NULL;

    } else
#endif /* ENABLE_JSR_180 */
    {
        /*
         * For a server socket connection, accept the inbound
         * socket connection so the end point filter can be checked.
         */
        status = pcsl_serversocket_accept_start((void*)pushp->fd,
                                                &clientHandle, &context);

        if (status != PCSL_NET_SUCCESS) {
            /*
             * Can't accept the connection.
             * Cancel the launch pending.
             */
            REPORT_ERROR1(LC_PUSH,
                          "(Push) Cannot accept serversocket, errno = %d\n",
                          pcsl_network_error((void*)pushp->fd));
            pushcheckinentry(pushp);
            return NULL;
        }
    }

    /* Update the resource count for client sockets */
    if (midpIncResourceCount(RSC_TYPE_TCP_CLI, 1) == 0) {
        REPORT_INFO(LC_PROTOCOL,
                    "(Push) Resource limit update error");
    }

    pushp->fdsock = (int)clientHandle;

    pcsl_socket_getremoteaddr((void *)pushp->fdsock, ipAddress);

#if ENABLE_JSR_180
    if (pushp->isSIPEntry) {
        unsigned char ipBytes[MAX_ADDR_LENGTH];


        pushp->pCachedData = (PacketEntry*) midpMalloc(sizeof (PacketEntry));
        if (pushp->pCachedData == NULL) {
            pushcheckinentry(pushp);
            return NULL;
        }

        pushp->pCachedData->offs = 0;

        /* Set the raw IP address */
        memcpy(&(pushp->pCachedData->ipAddress), ipBytes, MAX_ADDR_LENGTH);

        memset(ipAddress, '\0', MAX_HOST_LENGTH);
        strcpy(ipAddress, pcsl_inet_ntoa(&ipBytes));

        status = pcsl_socket_read_start(
                                       (void *)pushp->fdsock, (unsigned char *)pushp->pCachedData->buffer,
                                       MAX_CACHED_DATA_SIZE, &(pushp->pCachedData->length), &context);

        if (status == PCSL_NET_SUCCESS) {
            status = pcsl_socket_read_finish(
                                            (void *)pushp->fdsock, (unsigned char *)pushp->pCachedData->buffer,
                                            MAX_CACHED_DATA_SIZE, &(pushp->pCachedData->length),
                                            &context);

            if (status != PCSL_NET_WOULDBLOCK &&
                status != PCSL_NET_SUCCESS) {
                /*
                 * Receive failed - no data available.
                 * Cancel the launch pending.
                 */
                REPORT_ERROR1(LC_PUSH,
                              "(Push) Cannot receive data, errno = %d\n",
                              pcsl_network_error((void*)pushp->fdsock));
                pushcheckinentry(pushp);
                return NULL;
            }
            if (checkForEndOfHeader(pushp->pCachedData->buffer, 
                                    MAX_CACHED_DATA_SIZE)){
                pushp->fdAccepted = pushp->fdsock;
                pushp->state = prevState;
            return pushApplySipFilter(pushp);
            }
            /* notifier will be added below */
        }
        if (status == PCSL_NET_WOULDBLOCK) {
            pushp->state = WAITING_DATA;
            pcsl_add_network_notifier((void *)pushp->fdsock,
                                      PCSL_NET_CHECK_READ);
            return NULL;
        } else {
            pushcheckinentry(pushp);
            return NULL;
        }
    } else
#endif
        /* Datagram and Socket connections use the IP filter. */
        /* SIP has its own filtering mechanism applied above. */
        if (checkfilter(pushp->filter, ipAddress)) {
        return midpStrdup(pushp->value);
    }

    /*
     * Dispose of the filtered push request.
     * Close any accepted socket not accessed, yet.
     */
    pushcheckinentry(pushp);

    return NULL;
}
#endif

/**
 * Lookup the push entry, given a handle.
 *
 * @param fd The handle to a push connection
 *
 * @return The full text push entry from the push list
 */
char *pushfindfd(int fd) {
    PushEntry *pushp;
    PushEntry *pushtmp;
    int temp_state = AVAILABLE;

    AlarmEntry *alarmp;
    AlarmEntry *alarmtmp;
    char *alarmentry = NULL;
    char ipAddress[MAX_HOST_LENGTH];
    int status;
    unsigned char ipBytes[MAX_ADDR_LENGTH];
    void *context = NULL;

    /* Find the entry to pass off the open file descriptor. */
    for (pushp = pushlist; pushp != NULL ; pushp = pushp->next) {
        if ((pushp->fd == (int)fd)) {
            for (pushtmp = pushp; pushtmp != NULL; pushtmp = pushtmp->next) {
                if ((pushtmp->fd == fd) &&
                    (pushtmp->state == LAUNCH_PENDING)) {
                    /*
                     * Some MIDlet is launching and expecting to
                     * read from this fd. Don't steal its traffic.
                     */
                    return NULL;
                }
            }

            temp_state = pushp->state;
            pushp->state = LAUNCH_PENDING;

#ifdef ENABLE_JSR_82
            if (bt_is_bluetooth_url(pushp->value)) {
                bt_pushid_t id = bt_push_find_server((bt_handle_t)fd);
                if (id != BT_INVALID_PUSH_HANDLE) {
                    if (bt_push_accept(id, pushp->filter,
                                       (bt_handle_t *)(void*)&pushp->fdsock)) {
                        return midpStrdup(pushp->value);
                    }
                }
                pushcheckinentry(pushp);
                return NULL;
            }
#endif

            /*
             * Check the push filter, to see if this connection
             * is acceptable.
             */
            if (strncmp(pushp->value, "datagram://:", 12) == 0) {
                /*
                 * Read the datagram and save it til the application reads it.
                 * This is a one datagram message queue.
                 */
                pushp->pCachedData = (PacketEntry*)
                                     midpMalloc(sizeof (PacketEntry));
                if (pushp->pCachedData == NULL) {
                    pushcheckinentry(pushp);
                    return NULL;
                }

                pushp->pCachedData->offs = 0;

                status = pcsl_datagram_read_finish(
                                                  (void *)pushp->fd,
                                                  ipBytes,
                                                  &(pushp->pCachedData->senderport),
                                                  pushp->pCachedData->buffer,
                                                  MAX_CACHED_DATA_SIZE,
                                                  &(pushp->pCachedData->length),
                                                  context);

                if (status != PCSL_NET_SUCCESS) {
                    /*
                     * Receive failed - no data available.
                     * cancel the launch pending
                     * set listening state to CHECKED_IN
                     * to prevent forever loop
                     */
                    pushcheckinentry(pushp);
                    return NULL;
                }

                /* Set the raw IP address */
                memcpy(&(pushp->pCachedData->ipAddress),
                       ipBytes, MAX_ADDR_LENGTH);

                memset(ipAddress, '\0', MAX_HOST_LENGTH);
                strcpy(ipAddress, pcsl_inet_ntoa(&ipBytes));

                /* Datagram and Socket connections use the IP filter. */
                if (checkfilter(pushp->filter, ipAddress)) {
                    return midpStrdup(pushp->value);
                }

                /*
                 * Dispose of the filtered push request.
                 * Release any cached datagrams.
                 */
                pushcheckinentry(pushp);
                return NULL;
#if ENABLE_SERVER_SOCKET
            } else if (pushIsSocketConnection(pushp->value)) {
                return pushAcceptConnection(pushp, temp_state);
#endif
            }
#if ENABLE_JSR_180
            /* Check for JSR180 SIP/SIPS connections (UDP). */
            else if (pushp->isSIPEntry) {

                /* Special case: shared connection. Cached datagram is stored 
                   at first push entry buffer */
                /*
                 * Read the SIP datagram and save it til the
                 * application reads it.
                 * This is a one SIP datagram message queue.
                 */
                pushp->pCachedData = (PacketEntry*)
                                     midpMalloc(sizeof (PacketEntry));
                if (pushp->pCachedData == NULL) {
                    pushcheckinentry(pushp);
                    return NULL;
                }

                pushp->pCachedData->offs = 0;

                status = pcsl_datagram_read_finish(
                                                  (void *)pushp->fd,
                                                  ipBytes,
                                                  &(pushp->pCachedData->senderport),
                                                  pushp->pCachedData->buffer,
                                                  MAX_CACHED_DATA_SIZE,
                                                  &(pushp->pCachedData->length),
                                                  context);

                if (status != PCSL_NET_SUCCESS) {
                    /*
                     * Receive failed - no data available.
                     * cancel the launch pending
                     */
                    pushcheckinentry(pushp);
                    return NULL;
                }

                /* Set the raw IP address */
                memcpy(&(pushp->pCachedData->ipAddress),
                       ipBytes, MAX_ADDR_LENGTH);

                REPORT_INFO1(LC_PROTOCOL,
                             "SIP Push Message: %s",
                             pushp->pCachedData->buffer);
                /* restore state that will be processed separately at 
                   pushApplySipFilter */
                pushp->state = temp_state;

                return pushApplySipFilter(pushp);
            }
#endif

#if ENABLE_JSR_257
            else if(pushp->isNFCEntry) {
                char *entry = pushp->value;
                if(strncmp(entry, "ndef:",5) == 0) {
                    return pcsl_mem_strdup(entry);
                } else {
                    return NULL;
                }
            }        
#endif 

#if (ENABLE_JSR_205 || ENABLE_JSR_120)
            else{
                /*
                 * Return a valid push entry, if the WMA message has been
                 * succesfully received (which for sms, mms includes a
                 * filter check); otherwise return NULL.
                 */
                if (pushp->isWMAMessCached){
                    return getWmaPushEntry(pushp->value);
                } else{
                    pushp->state = temp_state;
                    return NULL;
                }
            }
#endif
            return NULL;
        }
    }

    /*
     * If the file descriptor was not found, it could be
     * an alarm time. If found, clear the entry so it will
     * not fire again.
     */
    for (alarmp = alarmlist; alarmp != NULL; alarmp = alarmtmp){
        alarmtmp = alarmp->next;
        if (alarmp->timerHandle == fd){
            alarmentry = midpStrdup(alarmp->midlet);
            if (alarmentry){
                jlong lastalarm;
                alarmadd(alarmentry, 0, &lastalarm);
                return alarmentry;
            } else{
                destroyTimerHandle(alarmp->timerHandle);

                alarmp->wakeup = 0;
                alarmp->timerHandle = 0;
                alarmp->state = AVAILABLE;
            }
        }
    }

    return NULL;
}

/**
 * Lookup the push entry, given the connection URL.
 *
 * @param str the connection URL
 * @return the full-text push entry from the registry
 */
char *pushfindconn(char *str){
    PushEntry *p;


    /* Find the entry that has matching connection URL. */
    for (p = pushlist; p != NULL ; p = p->next){
        if (strncmp (str, p->value, strlen(str)) == 0){
            return p->value;
        }
    }

    return NULL;
}

/**
 * Given the connection string and port number, look up the
 * push entry and return its filter.
 *
 * @param conn the connection string
 * @param port the port number to match
 * @return the filter from the registry
 */
char *pushgetfilter(char *conn, int port){
    PushEntry *p;

    /* Find the entry that has matching connection and port. */
    for (p = pushlist; p != NULL ; p = p->next){
        if ((strncmp (conn, p->value, strlen(conn)) == 0) &&
            (p->port == port)){
            /* Find the matching filter */
            return pushfilter(p->value);
        }
    }

    return NULL;
}

/**
 * Given the connection string and application ID, look up the
 * push entry and return its filter.
 *
 * @param conn the connection string
 * @param appID The MMS application ID to match
 * @return the filter from the registry
 */
char *pushgetfiltermms(char *conn, char *appID){
    PushEntry *p;

    /* Find the entry that has matching connection and port. */
    for (p = pushlist; p != NULL ; p = p->next){
        if ((strncmp (conn, p->value, strlen(conn)) == 0) &&
            (strcmp(appID, p->appID) == 0)){
            /* Find the matching filter */
            return pushfilter(p->value);
        }
    }

    return NULL;
}

/**
 * To be called when a WMA SMS or CBS message has arrived and has been
 * cached in an inbox. For the given connection string and
 * port number, a push entry is looked up. If one is found,
 * isWMAMessCached flag is set to KNI_TRUE.
 *
 * @param conn the connection string
 * @param port the port number to match
 */
void pushsetcachedflag(char *conn, int port){
    PushEntry *p;

    /* Find the entry that has matching connection and port. */
    for (p = pushlist; p != NULL ; p = p->next){
        if ((strncmp (conn, p->value, strlen(conn)) == 0) &&
            (p->port == port)){
            p->isWMAMessCached = KNI_TRUE;
        }
    }
}

/**
 * To be called when a WMA MMS message has arrived and has been
 * cached in an inbox. For the given connection string and
 * port number, a push entry is looked up. If one is found,
 * isWMAMessCached flag is set to KNI_TRUE.
 *
 * @param conn the connection string
 * @param appId The MMS application ID to match
 */
void pushsetcachedflagmms(char *conn, char *appID){
    PushEntry *p;

    /* Find the entry that has matching connection and port. */
    for (p = pushlist; p != NULL ; p = p->next){
        if ((strncmp (conn, p->value, strlen(conn)) == 0) &&
            (strcmp(appID, p->appID) == 0)){
            p->isWMAMessCached = KNI_TRUE;
        }
    }
}

/**
 * Finds push entries that belong to the given suite. If the available
 * flag is set, return only those entries with live connections (socket)
 * or cached incoming data.
 *
 * @param store The storagename of the suite
 * @param available If set, return only "live" push entries
 *
 * @return A comma delimited list of full-text push entries
 */
char *pushfindsuite(char *store, int available){
    PushEntry *p;
    char *ret = NULL;
    char *ptr;
    int len=0;
    char *connlist = NULL;
    int connlistlen = 0;

    /* Find the entry to pass off the open file descriptor. */
    for (p = pushlist; p != NULL ; p = p->next){
        if (strcmp(store, p->storagename) == 0){
            for (ptr = p->value, len=0; *ptr && (*ptr != ','); ptr++, len++) {
            }

            ret = midpMalloc(len + 1);
            if (ret == NULL) {
                REPORT_ERROR(LC_PUSH, "Out of memory in pushfindsuite");
                return NULL;
            }

            strncpy(ret, p->value, len);
            ret[len] = '\0';

            /*
             * Check if there is pending I/O on the
             * current file descriptor. e.g. an accepted socket
             * or a cache datagram.
             */
            if (available && (p->fd != -1)){
                if ((p->fdsock == -1) && (p->pCachedData == NULL) &&
                    (!p->isWMAMessCached) 
#if ENABLE_JSR_257             
                    && 
                    (! ((p->isNFCEntry) && (p->state == LAUNCH_PENDING)))
#endif                    
                    ) {
                    midpFree(ret);
                    ret = NULL;
                    continue;
                }
            }

            /*
             * Append the entries together in a single list.
             */
            if (connlist == NULL) {
                connlistlen = len;
                connlist = ret;
            } else {
                /* new length is old length plus ret length plus "," length */
                int newconnlistlen = connlistlen + len + 1;
                char *newconnlist = midpRealloc(connlist, newconnlistlen + 1);
                
                if (newconnlist == NULL) {
                    midpFree(ret);
                    midpFree(connlist);

                    REPORT_ERROR(LC_PUSH, "Out of memory in pushfindsuite");
                    return NULL;
                }

                connlistlen = newconnlistlen;
                connlist = newconnlist;
                
                strcat(connlist, ",");
                strcat(connlist, ret);

                midpFree(ret);
            }

            ret = NULL;
        }
    }

    return connlist;
}

/**
 * Parses the persistent push registry from disk into the
 * in memory cache representation.
 *
 * @param pushfd file descriptor for reading
 * @return <tt>0</tt> if successful, <tt>-1</tt> if there was an error opening
 * the push registry file, <tt>-2</tt> if there was a memory allocation failure
 */
static int parsePushList(int pushfd){
    char buffer[MAX_LINE+1];
    char *errStr = NULL;
    PushEntry *pe;

    /* Read a line at a time */
    while (readLine(&errStr, pushfd, buffer, sizeof(buffer)) != 0){

        if (errStr != NULL){
            REPORT_WARN2(LC_PROTOCOL,
                         "Warning: could not read push registration: %s; buffer = '%s'",
                         errStr, buffer);
            storageFreeError(errStr);
            return -1;
        }

        pe = (PushEntry *) midpMalloc (sizeof(PushEntry));

        if (pe == NULL){
            pushListFree();
            return -2;
        }

        pe->next = pushlist;
        pe->value = midpStrdup(buffer);
        pe->storagename = midpStrdup(pushstorage(pe->value, 3));

        if ((pe->value == NULL) || (pe->storagename == NULL)){
            midpFree(pe->value);
            midpFree(pe->storagename);
            midpFree(pe);
            pushListFree();
            return -2;
        } else{
            pe->filter = pushfilter(pe->value);
            pe->fd = -1;
            pe->fdsock = -1;
            pe->fdAccepted = -1;
            pe->state = AVAILABLE;
            pe->pCachedData = NULL;
            pe->isWMAEntry = KNI_FALSE;
            pe->isWMAMessCached = KNI_FALSE;
            pe->appID = NULL;
#if ENABLE_JSR_180
            pe->isSIPEntry = KNI_FALSE;
            pe->isShared = KNI_FALSE;
#endif

#if ENABLE_JSR_257
            pe->isNFCEntry = KNI_FALSE;
#endif

        }

        /*
         * Add the new entry to the top of the push cached
         * list.
         */
        pushlist = pe;
        pushlength++;
    }

    /* This check is required for the case when readLine() didn't put
       any characters into the buffer and while() was not executed */
    if (errStr != NULL){
        REPORT_WARN1(LC_PROTOCOL,
                     "Warning: could not read push registration: %s",
                     errStr);
        storageFreeError(errStr);
        return -1;
    }

    return 0;
}

static void pushStartListening(){
    PushEntry *pe;

    for (pe = pushlist; pe != NULL ; pe = pe->next){
        if (pe->state == AVAILABLE){
            pushProcessPort(pe);
            if (pe->fd != -1){
                pe->state = CHECKED_IN;
                pushAddNetworkNotifier(pe);
            }
        }
    }
}

/**
 * Parses the URL port.
 *
 * buffer A full-text push entry string from the registry ("mms://12345:11")
 * @return port or <tt>-1</tt> on parse error
 */
static int getUrlPort(char* buffer){

    char* p = buffer;
    int colon_found = 0;
    for (; *p != '\0' ; p++){
        if (*p == ':'){
            colon_found++ ;
        }

        if (colon_found == 2){
            p++ ;
            /*
            * Parse the port number from the
            * connection string
            */
            return atoi(p);
        }
    }
    return -1;
}

/**
 * Checks current status of the network.
 *
 * @return <tt>1</tt> if network is enabled, <tt>0</tt> otherwise
 */
static int isNetworkUp(){
    int res, status;

    res = 0;

    switch (networkStatus){
    case NET_STATUS_UP:
        res = 1;
        break;

    case NET_STATUS_GOING_UP:{
            status = pcsl_network_init_finish();

            switch (status){
            case PCSL_NET_SUCCESS:
                networkStatus = NET_STATUS_UP;
                res = 1;
                break;

            case PCSL_NET_WOULDBLOCK:
                /* network is still going up. */
                break;

            default:
                /* network error, try later */
                networkStatus = NET_STATUS_ERROR;
                break;
            }
            break;
        }

    case NET_STATUS_UNKNOWN:
    case NET_STATUS_ERROR:{
            status = pcsl_network_init_start(pcsl_network_initialized);

            switch (status){
            case PCSL_NET_SUCCESS:
                networkStatus = NET_STATUS_UP;
                res = 1;
                break;

            case PCSL_NET_WOULDBLOCK:
                networkStatus = NET_STATUS_GOING_UP;
                break;

            default:
                /* network error, try later */
                networkStatus = NET_STATUS_ERROR;
                break;
            }

            break;
        }

    case NET_STATUS_GOING_DOWN:
    case NET_STATUS_DOWN:
        /* 
         * don't prevent network from going down as well as don't try to
         * bring it up if MIDP has put it down. 
         */
        break;

    default:
        break;
    }

    return res;
}

/**
 * Parses the port number from the connection field
 * and uses it for the connection appropriate open
 * call. The handle will be included in
 * the connection registry until needed by the
 * application level connection open request.
 *
 * @param pe push entry; the following fields are used:
 * <ul>
 * <li>value A full-text push entry string from the registry</li>
 * <li>fd A pointer to a handle. Used to return the open handle</li>
 * <li>port A pointer to a portId. Used to return the port to the caller</li>
 * <li>appID Application ID</li>
 * <li>isWMAEntry set to KNI_TRUE for a WMA connection, KNI_FALSE otherwise</li>
 * <li>isSipEntry set to KNI_TRUE for a SIP/SIPS connection,
                  KNI_FALSE otherwise</li>
 *
 * @return <tt>0</tt> if successful,
 *         <tt>-1</tt> connection error,
 *         <tt>-2</tt> if out of memory
 *         <tt>-3</tt> if illegal arguments or unknown connection type
 * </ul>
 */
static int pushProcessPort(PushEntry* pe){
    char *p, *buffer;
    void *handle;
    void *context = NULL;
    int status;

    if (pe == NULL || pe->value == NULL){
        return -3;
    }

    /* Make sure the network is properly initialized. */
    if (!isNetworkUp()){
        pe->fd = -1;
        return -1;
    }

    /*
     * Open the file descriptor so it can be monitored
     * for inbound connection requests. For MIDP
     * only socket and datagram connections are supported.
     * With WMA 2.0 sms, cbs and mms connections are also
     * allowed.
     */

    p = buffer = pe->value;

    pe->isWMAEntry = KNI_FALSE;
    pe->port = -1;


#if ENABLE_JSR_82
    {
        bt_port_t port;
        if (bt_push_parse_url(buffer, &port, NULL) == BT_RESULT_SUCCESS){
            bt_handle_t handle = bt_push_start_server(&port);
            if (handle == BT_INVALID_HANDLE){
                pe->fd = -1;
                return -1;
            }
            pe->fd = (int)handle;
            return 0;
        }
    }
#endif

#if ENABLE_JSR_180
    /* Check for JSR180 SIP/SIPS connections. */
    if ((strncmp(buffer, "sips:", 5) == 0) ||
        (strncmp(buffer, "sip:", 4) == 0)){
        pe->isSIPEntry = KNI_TRUE;

        if (strncmp(buffer, "sips:", 5) == 0){
            p += 5;
        } else{
            p += 4;
        }

        /*
         * Example JSR180 connection strings
         *   sip:5060
         *   sip:5080;type="application/x-chess"
         *   sip:*;type="application/x-cannons"
         */
        if (*p == '*'){
            PushEntry *pushp;
            /* first conection uses shared port */
            PushEntry* sharedP = NULL;
            char* mime;
            int mime_len;
            /* Get type */
            mime_len = getMIMEType(buffer, &mime);
            if (0 == mime_len){
                /* media must be present */
                REPORT_ERROR(LC_PUSH, "Shared connection must include media.");
                pe->fd = -1;
                return -3;
            }
            /* check if given MIME is registered already */
            if (pushlength > 0 ){
                /* scan up to current record (VM startup case)
                   or until end of records (installation case) */
                for (pushp = pushlist; pushp != pe, pushp != NULL; pushp = pushp->next){
                    if (pushp->isSIPEntry && pushp->isShared){
                        char* tmp;
                        /* compare mime type */
                        if (0 == mime_len - getMIMEType(pushp->value, &tmp)){
                            if (0 == strncmp(mime, tmp, mime_len)){
                                /* there is an application with given mime type */
                                REPORT_ERROR1(LC_PUSH, 
                                              "There is registered application "
                                              "with MIME: ", mime);
                                return -1;
                            }
                        }
                        if (NULL == sharedP){
                            sharedP = pushp;
                        }
                    }
                }
            }
            /* Shared connections must include media. */
            pe->port = 5060;
            /* mark for quick search */
            pe->isShared = KNI_TRUE;
            /* don't create new connection if there is SIP connection 
               shared the same port */
            if (NULL  != sharedP){
                pe->fd = sharedP->fd;
                return 0;
            }


        } else{
            /* Dedicated ports may also include media. */
            pe->port = atoi(p);
        }

        p = buffer;
    }
#endif

#if ENABLE_JSR_257
    if(strncmp(buffer, "ndef:", 5) == 0) {
        pe->isNFCEntry = KNI_TRUE;
        return contactless_push_register(buffer, &pe->fd);
    } else {
        pe->isNFCEntry = KNI_FALSE;
    }
#endif 

    if (pushIsDatagramConnection(buffer)){
        if (pe->port == -1){
            pe->port = getUrlPort(buffer);
        }
        /**
         * Verify that the resource is available well within limit as per
         * the policy in ResourceLimiter
         */
        if (midpCheckResourceLimit(RSC_TYPE_UDP, 1) == 0){
            REPORT_INFO(LC_PROTOCOL, "(Push)Resource limit exceeded for"
                        " datagrams");
            pe->fd = -1;
        } else{
            status = pcsl_datagram_open_start(pe->port,
                                              &handle, &context);

            if (status == PCSL_NET_SUCCESS){
                pe->fd = (int) handle;
                /* Update the resource count  */
                if (midpIncResourceCount(RSC_TYPE_UDP, 1) == 0){
                    REPORT_INFO(LC_PROTOCOL, "(Push)Datagrams: Resource"
                                " limit update error");
                }
            } else{
                pe->fd = -1;
            }
        }
#if ENABLE_SERVER_SOCKET

    } else if (pushIsSocketConnection(buffer)){
        if (pe->port == -1){
            pe->port = getUrlPort(buffer);
        }
        /**
         * Verify that the resource is available well within limit as per
         * the policy in ResourceLimiter
         */
        if (midpCheckResourceLimit(RSC_TYPE_TCP_SER, 1) == 0){
            REPORT_INFO(LC_PROTOCOL, "(Push)Resource limit exceeded"
                        " for TCP server sockets");
            pe->fd = -1;
        } else{
            /* Open the server socket */
            status = pcsl_serversocket_open(pe->port, &handle);

            if (status == PCSL_NET_SUCCESS){
                pe->fd = (int) handle;

                /* Update the resource count  */
                if (midpIncResourceCount(RSC_TYPE_TCP_SER, 1) == 0){
                    REPORT_INFO(LC_PROTOCOL, "(Push)TCP Server: "
                                "Resource limit update error");
                }
            } else {
                /**
                 * pcsl_serversocket_open can never return WOULDBLOCK
                 * thus anything other that PCSL_NET_SUCCESS is an
                 * indication of an error
                 */
                midp_snprintf(get_KNIBuffer(), KNI_BUFFER_SIZE,
                              "IOError in push::serversocket::open = %d\n",
                              pcsl_network_error(handle));
                REPORT_INFO1(LC_PROTOCOL, "%s\n", get_KNIBuffer());

                pe->fd = -1;
            }
        }
#endif
#if (ENABLE_JSR_205 || ENABLE_JSR_120)

    } else if ((strncmp(buffer, "sms://:", 7) == 0) ||
               (strncmp(buffer, "cbs://:", 7) == 0)){

        pe->port = getUrlPort(buffer);

        /* check for sms or cbs connection */
        wmaPushProcessPort(buffer, &pe->fd, pe->port,
                           suiteIdFromChars(pushstorage(buffer, 3)), pe->appID);
        if (pe->fd != -1){
            pe->isWMAEntry = KNI_TRUE;
        }
#endif
#if (ENABLE_JSR_205)

    } else if (strncmp(buffer, "mms://:", 7) == 0){

        pe->appID = getMMSAppID(buffer);
        /* check for mms connection */
        wmaPushProcessPort(buffer, &pe->fd, pe->port,
                           suiteIdFromChars(pushstorage(buffer, 3)), pe->appID);
        if (pe->fd != -1){
            pe->isWMAEntry = KNI_TRUE;
        }
#endif

    } else{
        midp_snprintf(get_KNIBuffer(), KNI_BUFFER_SIZE,
                      "Error in push::serversocket::open: "
                      "unknown connection type.\n");
        REPORT_WARN1(LC_PROTOCOL, "%s\n", get_KNIBuffer());
        return -3;
    }

    return(pe->fd != -1) ? 0 : -1;
}

/**
 * Checks the incoming IP address against the push filter.
 * @param filter The filter string to be used
 * @param ip The incoming ip to be tested by the filter
 * @return <tt>1</tt> if the comparison is successful, <tt>0</tt> if it fails
 */
static int checkfilter(char *filter, char *ip){
    char *p1 = NULL;
    char *p2 = NULL;

    if ((ip == NULL) || (filter == NULL)) return 0;

    /* Filter is exactly "*", then all IP numbers are allowed. */
    if (strcmp(filter, "*") == 0) return 1;

    /*
     * Otherwise walk through the filter string looking for character
     * matches and wildcard matches.
     * The filter pointer is incremented in the main loop and the
     * IP address pointer is incremented as characters and wildcards
     * are matched.
     */
    for (p1=filter, p2=ip; *p1 && *p2; p1++){
        /*
         * For an asterisk, consume all the characters up to
         * a matching next character.
         */
        if (*p1 == '*'){
            /* Initialize the next two filter characters. */
            char f1 = *(p1+1);
            char f2 = '\0';
            if (f1 != '\0'){
                f2 = *(p1+2);
            }

            /* Skip multiple wild cards. */
            if (f1 == '*'){
                continue;
            }

            /*
             * Consume all the characters up to a match of the next
             * character from the filter string. Stop consuming
             * characters, if the ip address is fully consumed.
             */
            while (*p2){
                /* Stop matching at field boundaries. */
                if (*p2 == '.'){
                    p1++;
                    p2++;
                    break;
                }

                /*
                 * When the next character matches, check the second character
                 * from the filter string. If it does not match, continue
                 * consuming characters from the ip address string.
                 */
                if (*p2 == f1 || f1 == '?'){
                    if (*(p2+1) == f2 || f2 == '?' || f2 == '*'){
                        /* Always consume an IP character. */
                        p2++;
                        if (f2 == '*' || *p2 == '.' || *p2 == '\0' ||
                            *(p2+1) == '.' || *(p2+1) == '\0'){
                            /* Also, consume a filter character. */
                            p1++;
                        }
                        break;
                    }
                }
                p2++;
            }
        } else if (*p1 == '?'){
            /* Filter may have upto 3 '?" characters. If coresponding byte in
             * the IPaddress does not have three characters, comparison will
             * fail. Examples : filter = 10.???.32.19 and ipaddress = 10.5.32.19
             */
            if (*p2 != '.'){
                p2 ++;
            }
        } else if (*p1 != *p2){
            /* If characters do not match, filter failed. */
            return 0;
        } else{
            p2 ++;
        }
    }
    if (*p1 != *p2 && *p1 != '*'){
        /* IP address was longer than filter string. */
        return 0;
    }

    return 1;
}

/**
 * Walks through and frees all the alarm entries in the alarm list.
 */
static void alarmListFree(){
    AlarmEntry *alarmp, *alarmtmp;

    /* clean up the list */
    for (alarmp = alarmlist; alarmp != NULL; alarmp = alarmtmp){
        alarmtmp = alarmp->next;

        midpFree(alarmp->midlet);
        midpFree(alarmp->storagename);
        midpFree(alarmp);
    }

    alarmlist = NULL;
}

/**
 * Walks through and frees all the push entries in the push list.
 */
static void pushListFree(){
    PushEntry *pushp;

    /* clean up the list */
    for (pushp = pushlist; pushp != NULL; pushp = pushlist){
        pushDeleteEntry(pushp, &pushlist);
    }
}

/**
 * Find blocking thread for a given socket push handle. Walks through the
 * registry of push entries for a handle that matches the argument. If one is
 * found, its push entry state is set to RECEIVED_EVENT and the handle is
 * returned.
 *
 * @param handle The handle to test for in the push registry
 * @return <tt>0</tt> if no entry is found. Otherwise, <tt>handle</tt> is
 *         returned
 */
int findPushBlockedHandle(int handle){
    PushEntry *pushp, *pushtmp;
    if (pushlength > 0 ){
        for (pushp = pushlist; pushp != NULL; pushp = pushtmp){
            pushtmp = pushp->next;
            if ((handle == pushp->fd &&
                 pushp->state != CHECKED_OUT &&
                 pushp->state != LAUNCH_PENDING) ||
                (handle == pushp->fdsock && pushp->state == WAITING_DATA)){
                pushp->state = RECEIVED_EVENT;
                return handle;
            }
        }
    }
    return 0;
}

/**
 * Find blocking thread for a given timer push handle. Walks through the
 * registry of alarm entries for a handle that matches the argument. If
 * one is found, its entry state is set to RECEIVED_EVENT and the handle is
 * returned.
 *
 * @param handle The handle to test for in the alarm registry
 * @return <tt>0</tt> if no entry is found. Otherwise, <tt>handle</tt> is
 *          returned
 */
int findPushTimerBlockedHandle(int handle){
    AlarmEntry *alarmp;
    AlarmEntry *alarmtmp;

    for (alarmp = alarmlist; alarmp != NULL ; alarmp = alarmtmp){
        alarmtmp = alarmp->next;
        ASSERT((alarmp->state == CHECKED_IN) || (alarmp->state == AVAILABLE));

        /* alarmp->state == AVAILABLE iff timer has been canceled or updated */
        if ((handle == alarmp->timerHandle) && (alarmp->state == CHECKED_IN)){
            alarmp->state = RECEIVED_EVENT;

            return handle;
        }
    }

    return 0;
}

/**
 * Test for incoming events, and block current java thread if none have
 * occurred. The method also initializes any new timers that are needed,
 * and blocks the current Java thread until a push event will wake it up.
 *
 * @return a <tt>handle</tt> to a push entry if an event is pending, or
 *         <tt>-1</tt> if the currently running Java thread is to block.
 */
int pushpoll(){
    int i;
    PushEntry * pe;

    AlarmEntry *alarmp;
    AlarmEntry *alarmtmp;

    /*
     * policy:
     *   1. initialize new timers.
     *   2. check for socket events.
     *   3. check networking events.
     */

    /* Find pending network push. */
    if (pushlength > 0 ){
        for (i = 0, pe = pushlist; i < pushlength && pe != NULL; i++){
            if (pe->state == AVAILABLE){
                /*
                 * When pushopen was called the port for this entry was busy,
                 * so try again.
                 */
                pushProcessPort(pe);
                if (pe->fd != -1){
                    REPORT_INFO1(LC_PUSH,
                                 "Push network signal on descriptor %x", pe->fd);

                    pe->state = CHECKED_IN;
                    pushAddNetworkNotifier(pe);
                }
            }

            if (pe->state == RECEIVED_EVENT){
                return pe->fd;
            }

            pe = pe->next;
        }
    }

    /* Find pending timer push. */
    for (alarmp = alarmlist; alarmp != NULL ; alarmp = alarmtmp){
        alarmtmp = alarmp->next;
        if (alarmp->state == RECEIVED_EVENT){
            REPORT_INFO1(LC_PUSH,
                         "Push timer alarm with handle %x", alarmp->timerHandle);
            return(alarmp->timerHandle);
        }
    }

    /*
     * No push connections are ready or alarms are available,
     * so we are going to sleep for a while. The current thread
     * will be blocked until on of the Push entries change state.
     */
    midp_thread_wait(PUSH_SIGNAL, 0, 0);
    return -1;
}

/**
 * Deletes all of connections for a suite, using the suite ID.
 *
 * @param id The suite ID to be removed from the push registry
 */
void pushdeletesuite(SuiteIdType id){
    if ((pushlist != NULL) || (alarmlist != NULL)){
        pushDeleteSuiteLive(id);
        return;
    }

    if (PCSL_FALSE != pcsl_string_is_null(&pushpathname)){
        pushDeleteSuiteNoVM(id);
    }
}

/**
 * Deletes all of connections for a suite, using the suite ID
 * while the VM is not running MIDP.
 *
 * @param id The suite ID to be removed from the push registry
 */
static void pushDeleteSuiteNoVM(SuiteIdType id){
    /* Tell push open not to start listening to connections. */
    if (pushOpenInternal(0) != 0){
        return;
    }

    pushDeleteSuiteLive(id);

    pushclose();
}

/**
 * Deletes all connections for a suite, using the suite ID
 * when the VM is running MIDP.
 *
 * @param id The suite ID to be removed from the push registry
 */
static void pushDeleteSuiteLive(SuiteIdType id){
    PushEntry *pushp;
    PushEntry **pPrevNext = &pushlist;
    PushEntry *pushnext;

    AlarmEntry *alarmp;
    AlarmEntry **alarmpPrevNext = &alarmlist;
    AlarmEntry *alarmnext;
    const pcsl_string* strId = midp_suiteid2pcsl_string(id);
    const char* pszID = (char*)pcsl_string_get_utf8_data(strId);

    if (pszID == NULL){
        return;
    }

    /* Find all of the entries to remove. */
    for (pushp = pushlist; pushp != NULL; pushp = pushnext){
        pushnext = pushp->next;
        if (strcmp(pszID, pushp->storagename) == 0){
#if ENABLE_JSR_82
            bt_push_unregister_url(pushp->value);
#endif
            pushDeleteEntry(pushp, pPrevNext);
            /* Do not change push prev next */
            continue;
        }

        pPrevNext = &pushp->next;
    }

    pushsave();

    /* Find all of the alarm entries to remove. */
    for (alarmp = alarmlist; alarmp != NULL; alarmp = alarmnext){
        alarmnext = alarmp->next;
        if (strcmp(pszID, alarmp->storagename) == 0){
            *alarmpPrevNext = alarmp->next;

            midpFree(alarmp->midlet);
            alarmp->midlet = NULL;
            midpFree(alarmp->storagename);
            alarmp->storagename = NULL;
            midpFree(alarmp);
            continue;
        }

        alarmpPrevNext = &alarmp->next;
    }

    alarmsave();

    pcsl_string_release_utf8_data((jbyte*)pszID, strId);
}

/**
 * Parses the persistent alarm registry from disk into the
 * in memory cache representation.
 *
 * @param pushfd file descriptor for reading
 * @return <tt>0</tt> if successful, <tt>-1</tt> if an I/O error occurs,
 * <tt>-2</tt> if out of memory
 */
static int parseAlarmList(int pushfd){
    char buffer[MAX_LINE+1];
    char *errStr = NULL;
    jlong alarm  = 0;
    AlarmEntry *pe = NULL;
    char *p;

    /* Read a line at a time. */
    while ( readLine(&errStr, pushfd, buffer, sizeof(buffer)) != 0 ){

        if (errStr != NULL){
            REPORT_WARN2(LC_PROTOCOL,
                         "Warning: could not read alarm registration: %s; buffer = '%s'",
                         errStr, buffer);
            storageFreeError(errStr);
            return -1;
        }

        /* Find the alarm time field. */
        for (p = buffer; *p != 0; p++){
            if (*p == ','){
                p++;
                sscanf(p, PCSL_LLD, &alarm);
                break;
            }
        }

        /*
     * Check if the alarm time field was found
     * and continue if it was not.
     */
        if (*p == 0){
            continue;
        }

        /* Create an alarm registry entry. */
        pe = (AlarmEntry *) midpMalloc (sizeof(AlarmEntry));
        if (pe == NULL){
            alarmListFree();
            return -2;
        }

        pe->next = alarmlist;
        pe->midlet = midpStrdup(buffer);
        alarmstart(pe, alarm);
        pe->storagename = midpStrdup(pushstorage(pe->midlet, 2));

        if ((pe->midlet == NULL) || (pe->storagename == NULL)){
            midpFree(pe->midlet);
            midpFree(pe->storagename);
            midpFree(pe);
            alarmListFree();
            return -2;
        }

        /*
         * Add the new entry to the top of the alarm cached
         * list.
         */
        alarmlist = pe;
    }

    /*
     * This check is required for the case when readLine() didn't put
     * any characters into the buffer and while() was not executed.
     */
    if (errStr != NULL){
        REPORT_WARN1(LC_PROTOCOL,
                     "Warning: could not read alarm registration: %s",
                     errStr);
        storageFreeError(errStr);
        return -1;
    }

    return 0;
}

/**
 * Opens the Alarm Registry file, if it exists and populate
 * an in memory cache of the file contents.
 *
 * @return <tt>0</tt> if successful, <tt>-1</tt> if a memory error occurred
 */
static int alarmopen(){
    int  pushfd;
    int status = 0;

    /* Now read the registered connections. */
    pushfd = storage_open(&errStr, &alarmpathname, OPEN_READ);
    if (errStr == NULL){
        /* Read through the file one line at a time. */
        status = parseAlarmList(pushfd);

        /* Close the storage handle. */
        storageClose (&errStr, pushfd);
        storageFreeError(errStr);
        if (status == -2){
            REPORT_ERROR(LC_PROTOCOL,
                         "Error: alarmopen out of memory when parsing alarm list");
            return -1;
        }


    } else{
        REPORT_WARN1(LC_PROTOCOL,
                     "Warning: could not open alarm registration file: %s",
                     errStr);
        storageFreeError(errStr);
    }

    return 0;
}

/**
 * Saves the in memory cache of alarm registrations to a persistent
 * file for use in subsequent runs.
 */
static void alarmsave(){
    int pushfd;
    AlarmEntry *alarmp;
    AlarmEntry *alarmtmp=NULL;

    pushfd = storage_open(&errStr, &alarmpathname, OPEN_READ_WRITE_TRUNCATE);
    if (errStr == NULL){
        /* Write a new list of push registrations to the persistent file */
        for (alarmp = alarmlist; alarmp != NULL ; alarmp = alarmtmp){
            alarmtmp = alarmp->next;
            storageWrite(&errStr, pushfd, alarmp->midlet,
                         strlen(alarmp->midlet));
            storageWrite(&errStr, pushfd, "\n", 1);
        }

        /* Close the storage handle */
        storageClose (&errStr, pushfd);
    } else{
        REPORT_WARN1(LC_PROTOCOL,
                     "Warning: could not write alarm registration file: %s",
                     errStr);
        storageFreeError(errStr);
        return;
    }
}
/**
 * Starts a timer for a single alarm entry.
 *
 * @param entry A pointer to an AlarmEntry
 * @param  alarm The absolute time at which this alarm is fired
 */
static void alarmstart(AlarmEntry *entry, jlong alarm){
    jlong time;

    entry->wakeup = alarm;
    time = alarm - (jlong)midp_getCurrentTime();

    if (time >= 0){
        /* if not expired, check timer event */
        entry->state = CHECKED_IN;
        entry->timerHandle = createTimerHandle((int)entry, time);
    } else{
        /* if expired, flag the timer as triggered */
        entry->state = RECEIVED_EVENT;
        entry->timerHandle = 0;
    }
}

/**
 * Adds one entry to the alarm registry.
 * If the entry already exists, return previous alarm time.
 * On succesful registration, write a new copy of the file to disk.
 *
 * @param str The alarm entry to add
 * @param alarm The absolute time at which this alarm is fired
 * @param lastalarm The place to return the previous alarm time, if it exists
 *
 * @return <tt>0</tt> if successful, <tt>-2</tt> if there was an error
 *         allocating this alarm.
 */
int alarmadd(char *str, jlong alarm, jlong *lastalarm){
    AlarmEntry *alarmp;
    AlarmEntry *alarmtmp = NULL;
    AlarmEntry *lastp = alarmlist;
    AlarmEntry *pe = NULL;
    char *ptr;
    int len;

    /* Find the length of the midlet field. */
    for (ptr = str, len = 0; *ptr != 0 ; ptr++, len++){
        if (*ptr == ',') break;
    }

    /* Check if the entry already exists? */
    for (alarmp = alarmlist; alarmp != NULL ; alarmp = alarmtmp){
        alarmtmp = alarmp->next;
        if (strncmp (str, alarmp->midlet, len) == 0){
            jlong temp = alarmp->wakeup;
            if (alarm == 0){
                /* Remove an entry. */
                if (alarmp->timerHandle != 0){
                    destroyTimerHandle(alarmp->timerHandle);
                    alarmp->timerHandle = 0;
                }
                if (lastp == alarmlist){
                    alarmlist = alarmp->next;
                } else{
                    lastp->next = alarmp->next;
                }
                midpFree(alarmp->midlet);
                midpFree(alarmp->storagename);
                midpFree(alarmp);
                alarmsave();
            } else{
                /*
         * Replace an entry.
         * State must change first in case timer events occur
         * between destroyTimerHandle calls.
         */
                alarmp->state = AVAILABLE;
                if (alarmp->timerHandle != 0){
                    destroyTimerHandle(alarmp->timerHandle);
                    alarmp->timerHandle = 0;
                }
                /* Update alarm. */
                midpFree(alarmp->midlet);
                alarmp->midlet = midpStrdup(str);
                alarmstart(alarmp,alarm);
                alarmsave();
            }
            *lastalarm = temp;

            return 0;
        }
        lastp = alarmp;
    }

    /* No entry found; last time is zero. */
    *lastalarm = 0;

    /* If no alarm to set; return success. */
    if (alarm == 0){
        return 0;
    }

    /* Add a new entry. */
    if ((pe = (AlarmEntry *) midpMalloc (sizeof(AlarmEntry)))){
        pe->next = alarmlist ;
        pe->midlet = midpStrdup(str);
        pe->storagename = midpStrdup(pushstorage(pe->midlet, 2));
        if ((pe->midlet == NULL) || (pe->storagename == NULL)){
            midpFree(pe->midlet);
            midpFree(pe->storagename);
            midpFree(pe);
            pe = NULL;
        } else{
            alarmstart(pe,alarm);
            alarmlist = pe ;
        }
    }

    if (pe == NULL){
        return -2;
    }

    alarmsave();
    return 0 ;
}

/*
 * Reads from an open file in storage, returning the number of bytes read.
 * May read less than the length of the buffer.
 *
 * If not successful *ppszError will be set to point to an error string,
 * on success it will be set to NULL.
 */
static long
readLine(char** ppszError, int handle, char* buffer, long length){
    long pos = 0;
    char ch;
    jboolean comments_on = KNI_FALSE;

    /* Buffer must be at least 2 bytes long to keep the ending zero. */
    if (length <= 1){
        *ppszError = NULL;
        return 0;
    }

    while ( storageRead(ppszError, handle, &ch, 1) != -1 ){
        if (*ppszError != NULL){
            break;
        }

        /* End of Line. */
        if ( ch == '\n' ){

            /* If the current line is a comment, go to the next one. */
            if (comments_on == KNI_TRUE){
                comments_on = KNI_FALSE;
                continue;
            }

            break;
        }

        /* Ignore carriage returns and skip comment lines. */
        if (ch == '\r' || comments_on == KNI_TRUE){
            continue;
        }

        /* Skip comment lines which begin with '#'. */
        if (ch == '#' && pos == 0){
            comments_on = KNI_TRUE;
            continue;
        }

        buffer[pos] = ch;

        pos++;
        if (pos == length-1){
            break;
        }


    }

    buffer[pos] = 0;

    return pos;
}

/**
 * Wildcard comparing the pattern and the string.
 * @param pattern The pattern that can contain '*' and '?'
 * @param str The string for comparing
 * @return <tt>1</tt> if the comparison is successful, <tt>0</tt> if it fails
 */
int wildComp(const char *pattern, const char *str){
    /* Current compare position of the pattern */
    const char *p1 = NULL;
    /* Current compare position of the string */
    const char *p2 = NULL;
    char tmp, tmp1;
    /* Last position of '*' in pattern */
    const char *posStar = NULL;
    /* Last position of string when '*' was found in the pattern */
    const char *posCmp = NULL;
    int num_quest, i;

    if ((pattern == NULL) || (str == NULL)) return 0;

    /* Filter is exactly "*", then any string is allowed. */
    if (strcmp(pattern, "*") == 0) return 1;

    /*
     * Otherwise walk through the pattern string looking for character
     * matches and wildcard matches.
     * The pattern pointer is incremented in the main loop and the
     * string pointer is incremented as characters and wildcards
     * are matched.
     */
    for (p1=pattern, p2=str; *p1 && *p2; ){
        switch (tmp = *p1){
        case '*' :
            /*
             * For an asterisk, consume all the characters up to
             * a matching next character.
             */
            num_quest = 0;
            posStar = p1; /* Save asterisk position in pattern */
            posCmp = p2;
            posCmp++;    /* Pointer to next position in string */
            do{
                tmp = *++p1;
                if (tmp == '?'){
                    num_quest++; /* number of question symbols */
                }
            } while ((tmp == '*') || (tmp == '?'));
            for (i = 0; i < num_quest; i++){
                if (*p2++ == 0){ /* EOL before questions number was exhausted */
                    return 0;
                }
            }
            if (tmp == 0){ /* end of pattern */
                return 1;
            }
            /* tmp is a next non-wildcard symbol */
            /* search it in the str */
            while (((tmp1 = *p2) != 0) && (tmp1 != tmp)){
                p2++;
            }
            if (tmp1 == 0){ /* no match symbols in str */
                return 0;
            }
            /* symbol found - goto next symbols */
            break;

        case '?' :
            /*
             * Skip a single symbol of str.
             * p1 and p2 points to non-EOL symbol
             */
            p1++;
            p2++;
            break;

        default :
            /*
             * Any other symbol - compare.
             * p1 and p2 points to non-EOL symbol
             */
            if (tmp != *p2){ /* symbol is not match */
                if (posStar == NULL){ /* No previous stars */
                    return 0;
                } else{ /* Return to the previous star position */
                    if (posCmp < p2){
                        p2 = posCmp++;
                    }
                    p1 = posStar;
                }
            } else{ /* match symbol */
                p1++;
                p2++;
            }
            break;
        } /* end if switch */

        if ((*p1 == 0) && (*p2 != 0)){
            if (posStar == NULL){ /* end of pattern */
                return 0;
            } else{
                if (posCmp < p2){
                    p2 = posCmp++;
                }
                p1 = posStar;
            }
        }
    } /* end of loop */

    if (*p2 != 0){ /* symbols remainder in str - not match */
        return 0;
    }

    if (*p1 != 0){ /* symbols remainder in pattern */
        while (*p1++ == '*'){ /* Skip multiple wildcard symbols. */
        }
        if (*--p1 != 0){ /* symbols remainder in pattern - not match */
            return 0;
        }
    }

    return 1;
}

#if ENABLE_I3_TEST
/**
 * API for testing wildComp() function.
 * @param pattern The pattern that can contain '*' and '?'
 * @param str The string for comparing
 * @return <tt>true</tt> if the comparison is successful, <tt>false</tt> if it fails
             */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_i3test_TestCompWildcard_cmpWildCard(){
    const char *pPattern = NULL;
    int patLen = 0, convFltLen;
    const char *pStr = NULL;
    int strLen = 0, convIpLen;
    int ret = KNI_FALSE;
    pcsl_string pcslFilter, pcslIp;
    pcsl_string_status rc1 = PCSL_STRING_ERR;
    pcsl_string_status rc2 = PCSL_STRING_ERR;
    pcsl_string_status rc3 = PCSL_STRING_ERR;
    pcsl_string_status rc4 = PCSL_STRING_ERR;

    KNI_StartHandles(2);
    KNI_DeclareHandle(filter);
    KNI_DeclareHandle(ip);

    KNI_GetParameterAsObject(1, filter);
    KNI_GetParameterAsObject(2, ip);
    rc1 = midp_jstring_to_pcsl_string(filter, &pcslFilter);
    rc2 = midp_jstring_to_pcsl_string(ip, &pcslIp);

    KNI_EndHandles();

    if (PCSL_STRING_OK == rc1 && PCSL_STRING_OK == rc2 ){
        patLen = pcsl_string_utf8_length(&pcslFilter) + 1;
        strLen = pcsl_string_utf8_length(&pcslIp) + 1;
        if ((patLen > 0) && (strLen > 0)){
            pPattern = (char*)midpMalloc(patLen);
            pStr = (char*)midpMalloc(strLen);
        }
    }

    if (pPattern != NULL && pStr != NULL){
        rc3 = pcsl_string_convert_to_utf8(&pcslFilter, (jbyte*)pPattern,
                                          patLen, &convFltLen);
        rc4 = pcsl_string_convert_to_utf8(&pcslIp, (jbyte*)pStr,
                                          strLen, &convIpLen);
    }

    if (PCSL_STRING_OK == rc1){
        pcsl_string_free(&pcslFilter);
    }
    if (PCSL_STRING_OK == rc2){
        pcsl_string_free(&pcslIp);
    }

    if (PCSL_STRING_OK == rc3 && PCSL_STRING_OK == rc4 &&
        patLen == convFltLen + 1 && strLen == convIpLen + 1){
        ret = wildComp(pPattern, pStr);
    }

    if (pPattern != NULL){
        midpFree((jchar*)pPattern);
    }
    if (pStr != NULL){
        midpFree((jchar*)pStr);
    }

    KNI_ReturnBoolean(ret);
}
#endif

/**
 * This function is called when the network initialization or finalization is
 * completed.
 *
 * @param isInit 0 if the network finalization has been finished,
 *               not 0 - if the initialization
 * @param status one of PCSL_NET_* completion codes
 */
static void pcsl_network_initialized(int isInit, int status){
    if (isInit){
        networkStatus = (status == PCSL_NET_SUCCESS) ?
                        NET_STATUS_UP : NET_STATUS_ERROR;
    } else{
        networkStatus = (status == PCSL_NET_SUCCESS) ?
                        NET_STATUS_DOWN : NET_STATUS_ERROR;
    }
}

