/*
 * @(#)shmem_md.c	1.8 06/10/10
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
 *
 */

#include "shmem_md.h"
#include "sysShmem.h"
#include "shmemBase.h"  /* for exitTransportWithError */

/*
 * These functions are not completely universal. For now, they are used 
 * exclusively for Jbug's shared memory transport mechanism. They have
 * been implemented on Win32 only so far, so the abstractions may not be correct
 * yet.
 */
 
static HANDLE memHandle = NULL; 

#ifdef DEBUG
#define sysAssert(expression) {		\
    if (!(expression)) {		\
	    exitTransportWithError \
            ("\"%s\", line %d: assertion failure\n", \
             __FILE__, __DATE__, __LINE__); \
    }					\
}
#else
#define sysAssert(expression) ((void) 0)
#endif

int 
sysSharedMemCreate(const char *name, int length, 
                   sys_shmem_t *mem, void **buffer)
{
    void *mappedMemory;
    HANDLE memHandle;

    sysAssert(buffer);
    sysAssert(name);
    sysAssert(length > 0);

    memHandle  = 
        CreateFileMapping((HANDLE)0xffffffff, /* backed by page file */
                          NULL,               /* no inheritance */
                          PAGE_READWRITE,     
                          0, length,          /* hi, lo order of length */
                          name);
    if (memHandle == NULL) {
        return SYS_ERR;
    } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
        /* If the call above didn't create it, consider it an error */
        CloseHandle(memHandle);
        memHandle = NULL;
        return SYS_INUSE;
    } 

    mappedMemory = 
        MapViewOfFile(memHandle,
                      FILE_MAP_WRITE,       /* read/write */
                      0, 0, 0);             /* map entire "file" */

    if (mappedMemory == NULL) {
        CloseHandle(memHandle);
        memHandle = NULL;
        return SYS_ERR;
    }

    *mem = memHandle;
    *buffer = mappedMemory;
    return SYS_OK;
}

int 
sysSharedMemOpen(const char *name, sys_shmem_t *mem, void **buffer)
{
    void *mappedMemory;
    HANDLE memHandle;

    sysAssert(name);
    sysAssert(buffer);

    memHandle = 
        OpenFileMapping(FILE_MAP_WRITE,     /* read/write */
                        FALSE,              /* no inheritance */
                        name);
    if (memHandle == NULL) {
        return SYS_ERR;
    }

    mappedMemory = 
        MapViewOfFile(memHandle,
                      FILE_MAP_WRITE,       /* read/write */
                      0, 0, 0);             /* map entire "file" */

    if (mappedMemory == NULL) {
        CloseHandle(memHandle);
        memHandle = NULL;
        return SYS_ERR;
    }

    *mem = memHandle;
    *buffer = mappedMemory;
    return SYS_OK;
}

int 
sysSharedMemClose(sys_shmem_t mem, void *buffer)
{
    if (buffer != NULL) {
        if (!UnmapViewOfFile(buffer)) {
            return SYS_ERR;
        }
    }

    if (!CloseHandle(mem)) {
        return SYS_ERR;
    }

    return SYS_OK;
}

int 
sysIPMutexCreate(const char *name, sys_ipmutex_t *mutexPtr)
{
    HANDLE mutex;

    sysAssert(mutexPtr);
    sysAssert(name);

    mutex = CreateMutex(NULL,            /* no inheritance */
                        FALSE,           /* no initial owner */
                        name);
    if (mutex == NULL) {
        return SYS_ERR;
    } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
        /* If the call above didn't create it, consider it an error */
        CloseHandle(mutex);
        return SYS_INUSE;
    } 

    *mutexPtr = mutex;
    return SYS_OK;
}

int 
sysIPMutexOpen(const char *name, sys_ipmutex_t *mutexPtr)
{
    HANDLE mutex;

    sysAssert(mutexPtr);
    sysAssert(name);

    mutex = OpenMutex(SYNCHRONIZE,      /* able to wait/release */
                      FALSE,            /* no inheritance */
                      name);
    if (mutex == NULL) {
        return SYS_ERR;
    }

    *mutexPtr = mutex;
    return SYS_OK;
}

int 
sysIPMutexEnter(sys_ipmutex_t mutex)
{
    DWORD rc;
    sysAssert(mutex);
    rc = WaitForSingleObject(mutex, INFINITE);
    return (rc == WAIT_OBJECT_0) ? SYS_OK : SYS_ERR;
}

int 
sysIPMutexExit(sys_ipmutex_t mutex)
{
    sysAssert(mutex);
    return ReleaseMutex(mutex) ? SYS_OK : SYS_ERR;
}

int 
sysIPMutexClose(sys_ipmutex_t mutex)
{
    return CloseHandle(mutex) ? SYS_OK : SYS_ERR;
}

int 
sysEventCreate(const char *name, sys_event_t *eventPtr)
{
    HANDLE event;

    sysAssert(eventPtr);
    sysAssert(name);

    event = CreateEvent(NULL,            /* no inheritance */
                        FALSE,           /* auto-reset */
                        FALSE,           /* initially, not signalled */
                        name);
    if (event == NULL) {
        return SYS_ERR;
    } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
        /* If the call above didn't create it, consider it an error */
        CloseHandle(event);
        return SYS_INUSE;
    } 

    *eventPtr = event;
    return SYS_OK;
}

int 
sysEventOpen(const char *name, sys_event_t *eventPtr)
{
    HANDLE event;

    sysAssert(eventPtr);
    sysAssert(name);

    event = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE,
                                        /* able to wait/signal */
                      FALSE,            /* no inheritance */
                      name);
    if (event == NULL) {
        return SYS_ERR;
    }

    *eventPtr = event;
    return SYS_OK;
}

int 
sysEventWait(sys_process_t otherProcess, sys_event_t event)
{
    HANDLE handles[2];        /* process, event */
    DWORD rc;
    int count;
    int i;

    /*
     * If the signalling process is specified, and it dies while we wait, 
     * detect it and return an error. 
     */
    sysAssert(event);

    handles[0] = event;
    handles[1] = otherProcess;

    count = (otherProcess == NULL) ? 1 : 2;

    /*
     * The WaitForMulitple... call below sometimes acts strangely under Win95.
     * For some reason, it intermittently returns with an ERROR_INVALID_HANDLE
     * even though both handles are fine. A subsequent call with the same
     * arguments seems to work perfectly all the time, so until we can 
     * figure out what, if anything, we're doing wrong, the loop exists.
     */
    for (i = 0; i < 2; i++) {
        rc = WaitForMultipleObjects(count, handles,
                                    FALSE,        /* wait for either, not both */
                                    INFINITE);
        if (rc == WAIT_OBJECT_0) {
            /* Signalled, return success */
            return SYS_OK;
        } else if (rc == WAIT_OBJECT_0 + 1) {
            /* Other process died, return error */
            return SYS_ERR;
        } else {
            /* Some other error */
            /* Should just return SYS_ERR here, but need repeat in Win95 (see above) */
            /* printf("sysEventWait failed, rc = %d, last err = %d\n", rc, GetLastError()); */
        } 
    }

    /*
     * If we *still* have the ERROR_INVALID_HANDLE problem, we want to know
     * about it. 
     */
    sysAssert(GetLastError() != ERROR_INVALID_HANDLE);

    return SYS_ERR;
}

int 
sysEventSignal(sys_event_t event)
{
    sysAssert(event);
    return SetEvent(event) ? SYS_OK : SYS_ERR;
}

int 
sysEventClose(sys_event_t event)
{
    return CloseHandle(event) ? SYS_OK : SYS_ERR;
}

jlong 
sysProcessGetID()
{
    return GetCurrentProcessId();
}

int 
sysProcessOpen(jlong processID, sys_process_t *processPtr)
{
    HANDLE process;

    sysAssert(processPtr);

    process = OpenProcess(SYNCHRONIZE,    /* able to wait on death */
                          FALSE,          /* no inheritance */
                          (DWORD)processID);
    if (process == NULL) {
        return SYS_ERR;
    }

    *processPtr = process;
    return SYS_OK;
}

int 
sysProcessClose(sys_process_t *process)
{
    return CloseHandle(process) ? SYS_OK : SYS_ERR;
}


