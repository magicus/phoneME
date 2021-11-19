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

#ifndef __sublimeio_h
#define __sublimeio_h

#include "sublimeLinuxWin32Bridge.h"
#ifdef WIN32

#include <windows.h>


typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 long64;

#else /* !WIN32 */ 

#include <sys/types.h>
#include <stdint.h>
#ifndef LONG64_DEFINED
#define LONG64_DEFINED

typedef u_int64_t long64;
typedef u_int64_t ULONG64;

#endif /* LONG64_DEFINED */ 
#endif /* WIN32 */

#ifdef SUBLIME_BUFFER_SIZE
#define BUFFER_SIZE SUBLIME_BUFFER_SIZE
#else /* SUBLIME_BUFFER_SIZE */
/* 8 MB minus 12 bytes for the header length */ 
#define BUFFER_SIZE 8388596
#endif /* SUBLIME_BUFFER_SIZE */

/*
 * the data buffer structure: 
 * 0 to 3   : data (only the data!) buffer size (w/o header) (host byte order) 
 * 4 to 7   : thread unique ID (host byte order)
 * 8 to 11  : read/write protocol - the message length (network byte order)
 * 12 to bufferSize+12 : data buffer
 */ 
typedef struct DataBufferStruct{
  int32_t size;
  int32_t threadID;
  int32_t dataLength;
  char data;
} DataBuffer;

#ifdef WIN32 

typedef struct InternalSharedBufferDataStruct {
    DataBuffer* dataBuffer;
    HANDLE hMapFile;
    HANDLE mutex;
    // pointers point to the specific byte to be written/read
    int write_pointer, read_pointer;
} InternalSharedBufferData;

#else /* !WIN32 */ 

typedef struct InternalSharedBufferDataStruct {
  DataBuffer* dataBuffer;
  int* hMapFile;
  pthread_mutex_t *mutex;
  // pointers point to the specific byte to be written/read
  int write_pointer, read_pointer;
} InternalSharedBufferData;

#endif


typedef struct SharedBufferStruct { 
    InternalSharedBufferData *data;
    // read/write from/to the shared buffer 
    int (*write) (struct SharedBufferStruct* sb, char * buffer, int numOfBytes); 
    int (*read) (struct SharedBufferStruct* sb, char * targetBuffer, int32_t bufSize, int *byte);
    int (*readInt32) (struct SharedBufferStruct *sb, int32_t *result);
    int (*readLong64)(struct SharedBufferStruct *sb, long64 *result) ;
    int (*writeInt32)(struct SharedBufferStruct *sb, int32_t value) ;
    int (*printf) (struct SharedBufferStruct *sb, char *format, ...);
    void (*lock) (struct SharedBufferStruct* sb); 
    void (*unlock) (struct SharedBufferStruct* sb);
    int (*reset) (struct SharedBufferStruct* sb);
    void (*stampID) (struct SharedBufferStruct* sb);
    void (*setID) (struct SharedBufferStruct* sb, uint32_t id);
    int32_t (*getBufferSize) (struct SharedBufferStruct* sb);
    int32_t (*getMsgLength) (struct SharedBufferStruct* sb);
    int32_t (*getThreadID) (struct SharedBufferStruct* sb);
} SharedBuffer; 

SharedBuffer* CreateNewSharedBuffer(char* name);
SharedBuffer* OpenSharedBuffer(char* name);
void DeleteSharedBuffer(SharedBuffer *s);


#endif
