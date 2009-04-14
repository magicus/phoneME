/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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


#ifndef SUBLIMELINUXWIN32BRIDGE_H
#define SUBLIMELINUXWIN32BRIDGE_H

#ifndef WIN32

#include <pthread.h>

#define MUTEX_HANDLE pthread_mutex_t*
#define EVENT_HANDLE sublimeEvent*
#define FALSE 0
#define TRUE 1
#define EVENT_CHAR 's' 
#define EVENT_NAME_LENGTH 8


/* The implementation of event is done with FIFO. 
   setting an event is done by pushing a char to the stream, 
   and waiting for event is done by blocking read of one char */ 

typedef struct sublimeEventStruct { 
  int pipeDescriptor; 
  void (*waitForEvent) (struct sublimeEventStruct *es); 
  void (*setEvent) (struct sublimeEventStruct *es);
} sublimeEvent;  


/* security attributes are void * because the windows version 
   uses this argument as null, so the type does not matter */ 
EVENT_HANDLE CreateEvent(void* sa,  int manualReset, int initialState, char * name); 
MUTEX_HANDLE CreateMutex(void* sa, int initialState, char * name); 

void SetEvent(EVENT_HANDLE event);
void ReleaseMutex(MUTEX_HANDLE fd); 

int GetCurrentProcessId(void); 

void itoa(int num, char *buf, int radix);

int GetLastError(); 

char* getTempDirLocation(void);

/* The path of the files created by events/sharedBuffers */  

#else /* WIN32 */ 
#include <windows.h>

#define MUTEX_HANDLE HANDLE
#define EVENT_HANDLE HANDLE



#endif 

void WaitForMutex(MUTEX_HANDLE m);
void WaitForEvent(EVENT_HANDLE e);
void yield(void); 
#endif /* SUBLIMELINUXWIN32BRIDGE_H */


