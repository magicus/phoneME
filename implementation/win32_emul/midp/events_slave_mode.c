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


#ifdef __cplusplus
extern "C" {
#endif

#include "lime.h"

#include "javacall_events.h"

#ifdef WIN32
#include <windows.h>
#endif

#include "javacall_memory.h"
#include "javacall_lifecycle.h"
#include "javacall_socket.h"
#include "javacall_datagram.h"

#include "lcd.h"

#if ENABLE_JSR_120
extern javacall_result try_process_wma_emulator(javacall_handle handle);
#endif

static HWND hwnd;


typedef struct EventMessage_ {
    struct EventMessage_* next;
    unsigned char* data;
    int dataLen;
} EventMessage;


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

/**
 * Initialize events mechanism
 *
 */
javacall_bool javacall_events_init(void) {

    if (events_sharefile == NULL) {
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


    if (events_mutex == NULL) {
        events_mutex = CreateMutex(
            NULL,       // default security attributes
            FALSE,      // initially not owned
            EVENT_MUTEX_NAME);      // named object
    }

    if (events_handle ==NULL) {
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

/**
 * Finalize events mechanism
 *
 */
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


/**
 * Adds message to the end of events queue.  Performs memory allocation for
 * the message.
 *
 * @param data buffer containing the message to be added
 * @param dataLen buffer size
 *
 * @return <tt><JAVACALL_OK></tt> on success
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> on memory allocation error
 */
javacall_result enqueueEventMessage(unsigned char* data, int dataLen){
    EventMessage** iter;
    EventMessage* elem;

    /* allocate memory */
    if (NULL == (elem =(EventMessage*)javacall_malloc(sizeof(EventMessage)))) {
        return JAVACALL_OUT_OF_MEMORY;
    }
    if (NULL == (elem->data = javacall_malloc(dataLen))) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    /* initialize fields */
    elem->dataLen = dataLen;
    elem->next = NULL;
    memcpy(elem->data, data, dataLen);

    /* iterate to the end of the list */
    for(iter=&head; *iter!=NULL; iter=&((*iter)->next));
    *iter=elem;
    return JAVACALL_OK;
}

/**
 * Copies message from the head of events queue to the buffer and afterwards
 * removes the message from the head, deallocating memory on the way.
 *
 * @param data buffer to be copied to
 * @param dataLen buffer size
 *
 * @return number of copied characters, or 0 if no character copied
 */
javacall_result dequeueEventMessage(unsigned char* data, int dataLen){
    EventMessage* root;

    if (head == NULL) {
        return 0;
    }

    dataLen = min(dataLen, head->dataLen);
    memcpy(data, head->data, dataLen);
    root = head->next;
    javacall_free(head->data);
    javacall_free(head);
    head = root;

    return dataLen;
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

    ok = (binaryBuffer != NULL) && (binaryBufferMaxLen > 0);

    if (ok) {
        ok = WaitForSingleObject(events_mutex, 0) == WAIT_OBJECT_0;
    }
    if (ok) {
        totalRead = dequeueEventMessage(binaryBuffer, binaryBufferMaxLen);
        if (head == NULL) {
            ResetEvent(events_handle);
        }
        ok = ReleaseMutex(events_mutex);
    }

    ok = ok && (totalRead!=0);
    if (outEventLen!=NULL) {
        *outEventLen = ok ? totalRead : 0;
    }
    return ok ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Copies a user supplied event message to a queue of messages
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

    ok =(binaryBuffer != NULL) && (binaryBufferLen > 0);

    if (ok) {
        ok = WaitForSingleObject(events_mutex, 500) == WAIT_OBJECT_0;
    }
    if (ok) {
        ok = (binaryBuffer!=NULL) && (binaryBufferLen>0);
    }
    if (ok) {
        enqueueEventMessage(binaryBuffer, binaryBufferLen);
        SetEvent(events_handle);
        ok = ReleaseMutex(events_mutex);
    }
    return ok ? JAVACALL_OK : JAVACALL_FAIL;
}


#define EVENT_LOOP_TIMER_ID 2112

/**
 * Signals the platform that JVM needs to execute one timeslice.
 * Used in slave mode only.
 */
void javacall_schedule_vm_timeslice(void) {
    PostMessage(midpGetWindowHandle(), WM_TIMER, (WPARAM)EVENT_LOOP_TIMER_ID, (LPARAM)NULL);
}

/**
 * Platform-specific event processing loop
 */
void javacall_slavemode_handle_events(void) {
    MSG msg;

    /* signal the platform that SJWC needs to execute one timeslice */
    javacall_schedule_vm_timeslice();

    while (GetMessage(&msg, NULL, 0, 0)) {
        /* Dispatch the message */
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void FinalizeLimeEvents() {
    EndLime();
}

int IsJavaRunning = 0;

#define LIME_PACKAGE "com.sun.kvem.midp"
#define LIME_GRAPHICS_CLASS "GraphicsBridge"
#define LIME_EVENT_CLASS "EventBridge"

/* Thread which checks if LIME events are pending */
static DWORD WINAPI
DispatcherThread(void* pArg) {

	extern void CheckLimeEvent();

    //Make sure lime was properly initialized
    Sleep(200);
	while(IsJavaRunning) {
    	CheckLimeEvent();
    }

    return 0;
}

static LRESULT CALLBACK
WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
    //check if udp or tcp
    int level = SOL_SOCKET;
    int opttarget;
    int optname;
    int optsize = sizeof(optname);
    javacall_int64 ms;	
    
	switch (iMsg) {
    case WM_TIMER:
        if (wParam == EVENT_LOOP_TIMER_ID) {
            KillTimer(midpGetWindowHandle(), EVENT_LOOP_TIMER_ID);
            /* execute one timeslice */
            ms = javanotify_vm_timeslice();
            if (ms >= 0) {
                /* JVM nas not exited and at least one of the threads is unblocked */
                SetTimer(midpGetWindowHandle(), EVENT_LOOP_TIMER_ID, (javacall_int32)ms, NULL);
            }
            else if (ms == -1){
                /* all ot the threads are blocked, wait for event */
            }
            else if (ms == -2) {
                /* JVM has exited */
                PostQuitMessage(0);
                return 0;
            }
        }
        return 0;

    case WM_NETWORK:

#ifdef ENABLE_NETWORK_TRACING
        fprintf(stderr, "Got WM_NETWORK(");
        fprintf(stderr, "descriptor = %d, ", (int)wParam);
        fprintf(stderr, "status = %d, ", WSAGETSELECTERROR(lParam));
#endif

        optname = SO_TYPE;
        if (0 != getsockopt((SOCKET)wParam, SOL_SOCKET,  optname,(char*)&opttarget, &optsize)) {
#ifdef ENABLE_NETWORK_TRACING
            fprintf(stderr, "getsocketopt error)\n");
#endif
        }

	if(opttarget == SOCK_STREAM) {
	        switch (WSAGETSELECTEVENT(lParam)) {
	        case FD_CONNECT:
	            /* Change this to a write. */
	            javanotify_socket_event(
	                    JAVACALL_EVENT_SOCKET_OPEN_COMPLETED,
	                    (javacall_handle)wParam,
	                    (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[TCP] FD_CONNECT)\n");
#endif
	            return 0;

	        case FD_WRITE:
	            javanotify_socket_event(
	                    JAVACALL_EVENT_SOCKET_SEND,
	                    (javacall_handle)wParam,
	                    (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
	            javanotify_socket_event(
	                    JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED,
	                    (javacall_handle)wParam,
	                    (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[TCP] FD_WRITE)\n");
#endif
	            return 0;

	        case FD_ACCEPT:
		      javanotify_server_socket_event(
	                    JAVACALL_EVENT_SERVER_SOCKET_ACCEPT_COMPLETED,
	                    (javacall_handle)wParam,
						(javacall_handle)wParam,
	                    (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[TCP] FD_ACCEPT, ");
#endif
	        case FD_READ:
	            javanotify_socket_event(
	                    JAVACALL_EVENT_SOCKET_RECEIVE,
	                    (javacall_handle)wParam,
	                    (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[TCP] FD_READ)\n");
#endif
	            return 0;

	        case FD_CLOSE:
                        javanotify_socket_event(
                                JAVACALL_EVENT_SOCKET_CLOSE_COMPLETED,
	                    (javacall_handle)wParam,
	                    (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[TCP] FD_CLOSE)\n");
#endif
	            return 0;

	        default:
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[TCP] unsolicited event %d)\n",
	                    WSAGETSELECTEVENT(lParam));
#endif
	            break;
	        }//end switch
	}//SOCK_STREAM
	else if (opttarget == SOCK_DGRAM) {
		 switch (WSAGETSELECTEVENT(lParam)) {

	        case FD_WRITE:
	            javanotify_datagram_event(
	                    JAVACALL_EVENT_DATAGRAM_SENDTO_COMPLETED,
	                    (javacall_handle)wParam,
	                    (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[UDP] FD_WRITE)\n");
#endif
	            return 0;
	        case FD_READ:
#if ENABLE_JSR_120
				if (JAVACALL_OK == try_process_wma_emulator((javacall_handle)wParam)) {
                   return 0;
				}
#endif
				javanotify_datagram_event(
						   JAVACALL_EVENT_DATAGRAM_RECVFROM_COMPLETED,
						   (javacall_handle)wParam,
						   (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);

#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[UDP] FD_READ)\n");
#endif
	            return 0;
	        case FD_CLOSE:
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[UDP] FD_CLOSE)\n");
#endif
	            return 0;
	        default:
#ifdef ENABLE_NETWORK_TRACING
	            fprintf(stderr, "[UDP] unsolicited event %d)\n",
	                    WSAGETSELECTEVENT(lParam));
#endif
	            break;
               }//end switch
	}//end UDP
       return 0;

    case WM_HOST_RESOLVED:
#ifdef ENABLE_TRACE_NETWORKING
        fprintf(stderr, "Got Windows event WM_HOST_RESOLVED \n");
#endif
        javanotify_socket_event(
                JAVACALL_EVENT_NETWORK_GETHOSTBYNAME_COMPLETED,
                (javacall_handle)wParam,
                (WSAGETSELECTERROR(lParam) == 0) ? JAVACALL_OK : JAVACALL_FAIL);
        return 0;

    }

    return DefWindowProc (hwnd, iMsg, wParam, lParam);
}


static void InitializePhantomWindow() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    char szAppName[] = "no_window";
    WNDCLASSEX  wndclass;


    wndclass.cbSize        = sizeof (wndclass);
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = NULL;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm       = NULL;

    RegisterClassEx (&wndclass);

    hwnd = CreateWindow (szAppName,               /* window class name       */
                         "MIDP",                  /* window caption          */
                         WS_DISABLED,             /* window style; disable   */
                         CW_USEDEFAULT,           /* initial x position      */
                         CW_USEDEFAULT,           /* initial y position      */
                         0,                       /* initial x size          */
                         0,                       /* initial y size          */
                         NULL,                    /* parent window handle    */
                         NULL,                    /* window menu handle      */
                         hInstance,               /* program instance handle */
                         NULL);                   /* creation parameters     */

}

HWND midpGetWindowHandle() {

    if (hwnd == NULL) {
            InitializePhantomWindow();
    }

    return hwnd;
}

void InitializeLimeEvents(){
    static int initialized = 0;

    IsJavaRunning = 1;

    if (!initialized) {
        LimeFunction *f;
        initialized = TRUE;
        f = NewLimeFunction(LIME_PACKAGE,
                            LIME_GRAPHICS_CLASS,
                            "initialize");
        f->call(f, NULL);
        DeleteLimeFunction(f);

        /* initialize the event bridge */
        f = NewLimeFunction(LIME_PACKAGE,
                            LIME_EVENT_CLASS,
                            "initialize");
        f->call(f, NULL);
        DeleteLimeFunction(f);

    }

    CreateThread(NULL, 0, DispatcherThread, (void *)2000, 0, NULL);
}



#ifdef __cplusplus
}
#endif



