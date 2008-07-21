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
 *
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef javacall_print
#undef javacall_print
#endif
#define javacall_print printf

#include "javacall_events.h"

#include <windows.h>

typedef struct EventMessage_ {
    struct EventMessage_* next;
    unsigned char* data;
    int dataLen;
} EventMessage;

typedef struct _JSROP_EVENT_QUEUE {
    int              subsystem_id;
    HANDLE           events_mutex;
    HANDLE           events_handle;
    EventMessage*    head;
    BOOL             closing;
    struct _JSROP_EVENT_QUEUE *next;
} EventQueue;

#define EVENT_LIST_MUTEX_NAME L"JC_EQListMuteX"
#define EVENT_MUTEX_NAME      L"JC_EventMuteX"
#define EVENT_EVENT_NAME      L"JC_EventNewEvenT"
#define MUTEX_TIMEOUT         1000

static EventQueue *eq_list_head = NULL;
static HANDLE eq_list_mutex = NULL;

static TCHAR *make_object_name(TCHAR *buf, TCHAR *prefix, int no);
static javacall_result create_event_queue(EventQueue **handle,int subsystem_id);
static EventQueue *find_event_queue(int subsystem_id);
static void enqueueEventMessage(EventQueue *q, unsigned char* data, int dataLen);
static int dequeueEventMessage(EventQueue *q, unsigned char* data, int dataLen);
static javacall_bool checkForEvents(EventQueue *q, long timeout);

/**
 * The function is called during Java VM startup, allowing the
 * platform to perform specific initializations. It is called in the same
 * process as javacall_event_receive() and javacall_events_finalize().
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_events_init() {
    if (eq_list_mutex != NULL) { // already init'ed
        return JAVACALL_OK;
    }
    eq_list_mutex = CreateMutex(
        NULL,       // default security attributes
        FALSE,      // initially not owned
        EVENT_LIST_MUTEX_NAME); // named object
    
    if (eq_list_mutex == NULL) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * The function is called during Java VM shutdown, allowing the platform to
 * perform specific events-related shutdown operations. It is called in the same
 * process as javacall_events_init() and javacall_event_receive().
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result javacall_events_finalize() {
    EventQueue *q, *q0;
    
    if (eq_list_mutex == NULL) { // already closed
        return JAVACALL_FAIL;
    }
    
    // try to set the lock to prevent creating new queues
    if (WaitForSingleObject(eq_list_mutex, MUTEX_TIMEOUT) != WAIT_OBJECT_0) {
        return JAVACALL_FAIL;
    }
    if (eq_list_head != NULL) { // the queue list is not empty
        for (q = eq_list_head; q != NULL; q = q->next) {
            q->closing = TRUE;
            if (q->events_mutex != NULL) {
                CloseHandle(q->events_mutex);
                q->events_mutex = NULL;
            }
            if (q->events_handle != NULL) {
                CloseHandle(q->events_handle);
                q->events_handle = NULL;
            }
        }
        CloseHandle(eq_list_mutex);
        eq_list_mutex = NULL;
        // give time for receiving errors
        Sleep(500);
        // free memory
        for (q = eq_list_head; q != NULL; q = q0) {
            q0 = q->next;
            if (q->head != NULL) {
                EventMessage *h, *p;
                
                for (h = q->head; h != NULL; h = p) {
                    p = h->next;
                    free(h->data);
                    free(h);
                }
            }
            free(q);
        }
    } else {
        CloseHandle(eq_list_mutex);
        eq_list_mutex = NULL;
    }
    return JAVACALL_OK;
}

/**
 * Waits for an incoming event in the queue with the given ID and copies it to
 * a user supplied data buffer.
 *
 * @param queueId identifier of an event queue, typically a JSR number.
 * @param binaryBuffer user-supplied buffer to copy event to.
 * @param binaryBufferMaxLen maximum buffer size that an event can be
 *              copied to.
 * @param outEventLen user-supplied pointer to variable that will hold actual 
 *              event size received, or desired buffer size if
 *              binaryBufferMaxLen is insufficient.
 *              If outEventLen is NULL, the event size is not returned.
 * @return <tt>JAVACALL_OK</tt> if an event successfully received,
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> if event size exceeds
 *         binaryBufferMaxLen,
 *         <tt>JAVACALL_FAIL</tt> on any other error.
 */
javacall_result javacall_event_receive_cvm(int queueId,
    /*OUT*/ unsigned char *binaryBuffer, int binaryBufferMaxLen,
    /*OUT*/ int *outEventLen) {

    javacall_bool ok = JAVACALL_TRUE;
    EventQueue *q;
    int totalRead = 0;

    if (binaryBuffer == NULL || binaryBufferMaxLen <= sizeof (javacall_int32)) {
        return JAVACALL_FAIL;
    }
    q = find_event_queue(queueId);
    if (q == NULL) {
        return JAVACALL_FAIL;
    }

    while (ok) {
        if (ok) {
            ok = WaitForSingleObject(q->events_mutex, MUTEX_TIMEOUT) == WAIT_OBJECT_0;
        }
        if (q->closing) {
            return JAVACALL_FAIL;
        }
        if (ok) {
            totalRead = dequeueEventMessage(q, binaryBuffer,
                    binaryBufferMaxLen - sizeof (javacall_int32));
            if (q->head == NULL) {
                ResetEvent(q->events_handle);
            }
            ok = ReleaseMutex(q->events_mutex);
        }
        if (0 == totalRead) {
            ok = checkForEvents(q, -1);
        } else {
            break;
        }
    }
    
    ok= ok && (totalRead != 0);
    if (outEventLen != NULL) {
        *outEventLen = ok ? totalRead + sizeof (javacall_int32) : 0;
    }
    return ok ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Copies a user supplied event message to a queue of messages and wakes up the
 * thread that is waiting for events on the queue with the given id.
 *
 * @param queueId identifier of an event queue, typically a JSR number.
 * @param binaryBuffer a pointer to binary event buffer to send. The first int
 *        is the event id.
 * @param binaryBufferLen size of binary event buffer to send.
 * @return <tt>JAVACALL_OK</tt> if an event has been successfully sent,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_event_send_cvm(int queueId,
    unsigned char* binaryBuffer, int binaryBufferLen) {

    javacall_bool ok;
    EventQueue *q;

    if (binaryBuffer == NULL || binaryBufferLen <= sizeof (javacall_int32)) {
        return JAVACALL_FAIL;
    }
    q = find_event_queue(queueId);
    if (q == NULL) {
        return JAVACALL_FAIL;
    }
    // lock the queue
    ok = WaitForSingleObject(q->events_mutex, MUTEX_TIMEOUT) == WAIT_OBJECT_0;
    if (q->closing) {
        return JAVACALL_FAIL;
    }
    if (ok) {
        enqueueEventMessage(q, binaryBuffer,
                binaryBufferLen - sizeof (javacall_int32));
        ok = ReleaseMutex(q->events_mutex);
    }
    if (ok) {
        ok = SetEvent(q->events_handle);
    }
    return ok ? JAVACALL_OK : JAVACALL_FAIL;
}

/* creates a name of a Windows object using prefix and number */
static TCHAR *make_object_name(TCHAR *buf, TCHAR *prefix, int no) {
    char b[20] = "", *p = b;
    TCHAR *dst = buf;
    
    while (*prefix) {
        *dst++ = *prefix++;
    }
    sprintf(b, "%d", no);
    while (*p) {
        *dst++ = (TCHAR) *p++;
    }
    *dst++ = 0;
    return buf;
}

/* creates new queue for specified subsystem (JSR) */
static javacall_result create_event_queue(EventQueue **handle, 
                                          int subsystem_id) {
    EventQueue *q;
    TCHAR object_name[100];
    
    q = malloc(sizeof *q);
    if (q == NULL) {
        return JAVACALL_FAIL;
    }
    memset((char*)q, 0, sizeof *q);
    
    q->events_mutex = CreateMutex(
        NULL,       // default security attributes
        FALSE,      // initially not owned
        make_object_name(object_name, EVENT_MUTEX_NAME, subsystem_id)
    );
    if (q->events_mutex == NULL) {
        free(q);
        return JAVACALL_FAIL;
    }

    q->events_handle = CreateEvent(
        NULL,       // default security attributes
        TRUE,       // manual-reset event
        FALSE,      // initial state is signaled
        make_object_name(object_name, EVENT_EVENT_NAME, subsystem_id)
    );
    if (q->events_mutex == NULL) {
        CloseHandle(q->events_mutex);
        free(q);
        return JAVACALL_FAIL;
    }
    q->closing = FALSE;
    q->subsystem_id = subsystem_id;
    q->next = NULL;
    *handle = q;
    return JAVACALL_OK;
}

/* Finds existing queue or creates new one. */
static EventQueue *find_event_queue(int subsystem_id) {
    EventQueue *q;
    /* save the head because somebody can add new queue 
       while we were waiting for the lock */
    EventQueue *saved_head = eq_list_head;
    
    for (q = eq_list_head; q != NULL; q = q->next) {
        if (q->subsystem_id == subsystem_id) {
            break;
        }
    }
    if (q == NULL) { // needed queue wasn't found. Try to add
        
        // set the lock
        if (WaitForSingleObject(eq_list_mutex, MUTEX_TIMEOUT) != WAIT_OBJECT_0) {
            return NULL;
        }
        if (saved_head != eq_list_head) {
            // if a new queue was added then search again
            for (q = eq_list_head; q != NULL; q = q->next) {
                if (q->subsystem_id == subsystem_id) {
                    break;
                }
            }
        }
        if (q == NULL) {
            if (create_event_queue(&q, subsystem_id) != JAVACALL_OK) {
                ReleaseMutex(eq_list_mutex);
                return NULL;
            }
            q->next = eq_list_head;
            eq_list_head = q;
        }
        if (!ReleaseMutex(eq_list_mutex)) {
            return NULL;
        }
    }
    return q;
}

static void enqueueEventMessage(EventQueue *q, unsigned char* data, int dataLen) {
    EventMessage** iter;
    EventMessage* elem=(EventMessage*)malloc(sizeof(EventMessage));
    if (elem == NULL) {
        javacall_print("enqueueEventMessage: No memory\n");
        return;
    }
    elem->data      =malloc(dataLen);
    if (elem->data == NULL) {
        free(elem);
        javacall_print("enqueueEventMessage: No memory\n");
        return;
    }
    elem->dataLen   =dataLen;
    elem->next      =NULL;
    memcpy(elem->data, data, dataLen);

    for(iter=&q->head; *iter!=NULL; iter=&((*iter)->next) )
        ;
    *iter=elem;
}

static int dequeueEventMessage(EventQueue *q, unsigned char* data, int dataLen) {
    EventMessage* root;
    if (q->head==NULL) {
        return 0;
    }
    dataLen=min(dataLen, q->head->dataLen);
    memcpy(data, q->head->data, dataLen);
    root=q->head->next;
    free(q->head->data);
    free(q->head);
    q->head=root;

    return dataLen;
}

static javacall_bool checkForEvents(EventQueue *q, long timeout) {
    MSG msg;
    unsigned long before = 0;
    unsigned long after  = 0;
    javacall_bool forever = JAVACALL_FALSE;
    HANDLE handles[] = { q->events_handle };

    if (timeout > 0) {
        before = (unsigned long)GetTickCount();
    } else if (-1 == timeout) {
        forever = JAVACALL_TRUE;
    }

    do {
        /*
         * Process any pending messages regardless of being seen or not
         */
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (q->closing || msg.message == WM_QUIT) {
                return JAVACALL_FALSE;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        /*
         * Check if timed out when processing messages.
         */
        if (!forever && timeout > 0) {
            after = (unsigned long)GetTickCount();
            if (after >= before) {
                timeout -= (long)(after - before);
            } else {
                /* The tick count has wrapped. (happens every 49.7 days) */
                timeout -= (long)((unsigned long)0xFFFFFFFF - before + after);
            }
        }

        before = after;

        if (timeout > 0 || forever) {
            /*
             * Wait for events, new messages or timeout
             */
            switch (MsgWaitForMultipleObjects(
                        1, handles, FALSE, 
                        forever ? INFINITE : timeout, QS_ALLEVENTS)) {

            case WAIT_OBJECT_0:
                /*
                 * We got signal to unblock a Java thread.
                 */
                return JAVACALL_TRUE;

            case WAIT_OBJECT_0+1:
                /*
                 * New message, fallback to PeekMessage.
                 */
                break;

            case WAIT_TIMEOUT:
                /*
                 * Timed out.
                 */
                return JAVACALL_FALSE;
                
            default:
                if (q->closing) {
                    return JAVACALL_FALSE;
                }
                break;
            }
        }
        
    } while ( (timeout > 0) || forever );

    return JAVACALL_FALSE; /* time out */
}

#ifdef __cplusplus
}
#endif



