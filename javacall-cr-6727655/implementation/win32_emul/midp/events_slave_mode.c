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

#include "javacall_events.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "lcd.h"

typedef struct EventMessage_ {
    struct EventMessage_* next;
    unsigned char* data;
    int dataLen;
}EventMessage;


/*
    Storage for interprocess communication.
    It will contains a list of null-terminated strings.
    The end of the list is empty string.
*/
typedef struct EventSharedList_ {
    char   data[4000];
} EventSharedList;

static BOOL event_initialized = FALSE;

HANDLE           events_mutex     =NULL;
HANDLE           events_handle    =NULL;
EventMessage*    head             =NULL;
HANDLE           events_sharefile =NULL;
EventSharedList* events_shared    =NULL;
javacall_bool    events_secondary =JAVACALL_FALSE;

#define EVENT_SHARED_NAME    "Meadlet_SharedSpacE"
#define EVENT_MUTEX_NAME     "Meadlet_EventListMuteX"
#define EVENT_EVENT_NAME     "Meadlet_EventNewEvenT"

#define EVENT_QUEUE_ACQUIRE  (WaitForSingleObject(events_mutex, 300) == WAIT_OBJECT_0)
#define EVENT_QUEUE_RELEASE  ReleaseMutex(events_mutex)

javacall_bool javacall_events_init(void) {

    if (events_sharefile==NULL) {
        events_sharefile = CreateFileMapping(
            INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            0, sizeof(EventSharedList), EVENT_SHARED_NAME);

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            events_secondary = JAVACALL_TRUE;
        }

        if (events_sharefile != NULL) {
            events_shared = (EventSharedList*) MapViewOfFile(
                events_sharefile, FILE_MAP_WRITE, 0, 0,
                sizeof(EventSharedList));
        }

        if (!events_secondary) {
            events_shared->data[0] = 0;
        }
    }


    if (events_mutex==NULL) {
        events_mutex=CreateMutex(
            NULL,       // default security attributes
            FALSE,      // initially not owned
            EVENT_MUTEX_NAME);      // named object
    }

    if (events_handle==NULL) {
        events_handle = CreateEvent(
            NULL,       // default security attributes
            TRUE,       // manual-reset event
            FALSE,      // initial state is signaled
            EVENT_EVENT_NAME        // named object
        );
    }

    event_initialized = TRUE;

    return (events_mutex != NULL) && (events_handle != NULL) &&
        (events_sharefile != NULL) && (events_shared != NULL);
}

javacall_bool javacall_events_finalize(void) {

    if (events_shared!=NULL) {
        UnmapViewOfFile(events_shared);
        events_shared=NULL;
    }
    if (events_sharefile!=NULL) {
        CloseHandle(events_sharefile);
        events_sharefile=NULL;
    }
    if (events_mutex!=NULL) {
        CloseHandle(events_mutex);
        events_mutex=NULL;
    }
    if (events_handle!=NULL) {
        CloseHandle(events_handle);
        events_handle=NULL;
    }

    event_initialized = FALSE;

	return JAVACALL_OK;
}

javacall_bool isSecondaryInstance(void)
{
    return events_secondary;
}

void enqueueInterprocessMessage(int argc, char *argv[])
{
    if (EVENT_QUEUE_ACQUIRE)
    {
        int i;
        char *dest = events_shared->data;
        for(i = 0; (i < argc) && argv[i]; i++)
        {
            int length = strlen(argv[i]) + 1;
            memcpy(dest, argv[i], length);
            dest += length;
        }
        *dest = 0;
        EVENT_QUEUE_RELEASE;
    }
}

int dequeueInterprocessMessage(char*** argv)
{
    int argc = 0;
    if (EVENT_QUEUE_ACQUIRE)
    {
        int lengthTotal = 1;
        char *src = events_shared->data;

        while(*src) {
            argc++;
            lengthTotal += strlen(src) + 1;
            src += strlen(src) + 1;
        }

        if (argc) {
            *argv = javacall_malloc(lengthTotal + argc * sizeof(char*));
            src = (char*) ((*argv) + argc);
            memcpy(src, events_shared->data, lengthTotal);
            argc = 0;
            while(*src) {
                (*argv)[argc++] = src;
                src += strlen(src) + 1;
            }
            events_shared->data[0] = 0;
            events_shared->data[1] = 0;
        }
        EVENT_QUEUE_RELEASE;
    }
    return argc;
}

void enqueueEventMessage(unsigned char* data,int dataLen)
{
    EventMessage** iter;
    EventMessage* elem=(EventMessage*)javacall_malloc(sizeof(EventMessage));
    elem->data      =javacall_malloc(dataLen);
    elem->dataLen   =dataLen;
    elem->next      =NULL;
    memcpy(elem->data, data, dataLen);

    for(iter=&head; *iter!=NULL; iter=&((*iter)->next) )
        ;
    *iter=elem;
}

int dequeueEventMessage(unsigned char* data,int dataLen)
{
    EventMessage* root;
    if (head==NULL) {
        return 0;
    }
    dataLen=min(dataLen, head->dataLen);
    memcpy(data, head->data, dataLen);
    root=head->next;
    javacall_free(head->data);
    javacall_free(head);
    head=root;

    return dataLen;
}

static javacall_bool checkForEvents(long timeout) {
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        /* Dispatching the message will call WndProc below. */
        if (msg.message == WM_QUIT) {
            return JAVACALL_FALSE;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (WaitForSingleObject(events_handle, 0)==WAIT_OBJECT_0) {
        /* We got signal to unblock a Java thread. */
        return JAVACALL_TRUE;
    }
    return JAVACALL_FALSE; /* time out */
}

/**
 * Waits for an incoming event message and copies it to user supplied
 * data buffer
 * @param waitForever indicate if the function should block forever
 * @param timeTowaitInMillisec max number of seconds to wait
 *              if waitForever is false
 * @param binaryBuffer user-supplied buffer to copy event to
 * @param binaryBufferMaxLen maximum buffer size that an event can be
 *              copied to.
 *              If an event exceeds the binaryBufferMaxLen, then the first
 *              binaryBufferMaxLen bytes of the events will be copied
 *              to user-supplied binaryBuffer, and JAVACALL_OUT_OF_MEMORY will
 *              be returned
 * @param outEventLen user-supplied pointer to variable that will hold actual
 *              event size received
 *              Platform is responsible to set this value on success to the
 *              size of the event received, or 0 on failure.
 *              If outEventLen is NULL, the event size is not returned.
 * @return <tt>JAVACALL_OK</tt> if an event successfully received,
 *         <tt>JAVACALL_FAIL</tt> or if failed or no messages are avaialable
 */
javacall_result javacall_event_receive(
                            long                    timeTowaitInMillisec,
                            /*OUT*/ unsigned char*  binaryBuffer,
                            /*IN*/  int             binaryBufferMaxLen,
                            /*OUT*/ int*            outEventLen)
{
    javacall_bool ok;
    int totalRead=0;

    if (!event_initialized) {
        javacall_events_init();
    }

    ok=(binaryBuffer!=NULL) && (binaryBufferMaxLen>0);

    if (ok) {
        ok = WaitForSingleObject(events_mutex, 0)==WAIT_OBJECT_0;
    }
    if (ok) {
        totalRead = dequeueEventMessage(binaryBuffer,binaryBufferMaxLen);
        if (head == NULL) {
            ResetEvent(events_handle);
        }
        ok = ReleaseMutex(events_mutex);
    }

    ok= ok && (totalRead!=0);
    if (outEventLen!=NULL) {
        *outEventLen = ok ? totalRead : 0;
    }
    return ok ? JAVACALL_OK : JAVACALL_FAIL;
}

 /**
 * copies a user supplied event message to a queue of messages
 *
 * @param binaryBuffer a pointer to binary event buffer to send
 *        The platform should make a private copy of this buffer as
 *        access to it is not allowed after the function call.
 * @param binaryBufferLen size of binary event buffer to send
 * @return <tt>JAVACALL_OK</tt> if an event successfully sent,
 *         <tt>JAVACALL_FAIL</tt> or negative value if failed
 */
javacall_result javacall_event_send(unsigned char* binaryBuffer,
                                    int binaryBufferLen){
    javacall_bool ok;

    if (!event_initialized) {
        javacall_events_init();
    }

    ok=(binaryBuffer!=NULL) && (binaryBufferLen>0);

    if (ok) {
        ok=WaitForSingleObject(events_mutex, 500)==WAIT_OBJECT_0;
    }
    if (ok) {
        ok=(binaryBuffer!=NULL) && (binaryBufferLen>0);
    }
    if (ok) {
        enqueueEventMessage(binaryBuffer,binaryBufferLen);
        SetEvent(events_handle);
        ok=ReleaseMutex(events_mutex);
    }
    return ok ? JAVACALL_OK : JAVACALL_FAIL;
}


#define EVENT_LOOP_TIMER_ID 2112

void javacall_schedule_vm_timeslice(void) {
    PostMessage(midpGetWindowHandle(), WM_TIMER, (WPARAM)EVENT_LOOP_TIMER_ID, (LPARAM)NULL);
}

/**
 * Sample event loop.
 */
void midp_slavemode_port_event_loop(void) {
    MSG msg;
    javacall_int64 ms;

    /*
     * Init part from midpRunVm, should be placed to javanotify_start():
     * midp_thread_set_timeslice_proc(midp_slavemode_port_schedule_vm_timeslice);
     * JVM_SetConfig(JVM_CONFIG_SLAVE_MODE, KNI_TRUE);
     * JVM_Start(classPath, mainClass, argc, argv);
     */

    javacall_schedule_vm_timeslice();

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_QUIT) {
            return;
        }

        if (msg.message == WM_TIMER && msg.wParam == EVENT_LOOP_TIMER_ID) {
            KillTimer(midpGetWindowHandle(), EVENT_LOOP_TIMER_ID);

            ms = javanotify_vm_timeslice();
            if (ms == 0) {
                //PostMessage(midpGetWindowHandle(), WM_TIMER, EVENT_LOOP_TIMER_ID, NULL); // Impl note: to bypass timer mechanism
                SetTimer(midpGetWindowHandle(), EVENT_LOOP_TIMER_ID, ms, NULL);
            } else if (ms > 0) {
                SetTimer(midpGetWindowHandle(), EVENT_LOOP_TIMER_ID, ms, NULL);
            } else if (ms == -2) {
                return;
            } else /*if (ms == -1)*/ {
                /* wait for event */
            }
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            javanotify_inform_event();
	}
    }

    /*
     * Cleanup:
     * JVM_CleanUp();
     */
}


#ifdef __cplusplus
}
#endif



