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

/*
 * the data buffer structure (all field values are in network byte order): 
 * 0 to 3   : capacity - the real size of data field in bytes 
 * 4 to 7   : closed - closed flag
 * 8 to 11  : readIndex - the index at which to start read
 * 12 to 15 : byteCount - the number of bytes which are valid in the buffer,
 *                        valid data start at [data + readIndex] up to 
 *                        [data + (readIndex + byteCount) % capacity - 1]
 * 16 to capacity + 16 : the buffer
 */ 
typedef struct DataBufferStruct{
  int32_t capacity;
  int32_t closed;
  int32_t readIndex;
  int32_t byteCount;
  char data[4];
} DataBuffer;

/*  capacity + closed flag + read index + byte count */
#define SUBLIME_FIELDS_LENGTH (sizeof(DataBuffer) - 4 * sizeof(char))

#ifndef SUBLIME_BUFFER_SIZE
#define SUBLIME_BUFFER_SIZE (8 * 1024 * 1024)
#endif
 
#define SUBLIME_USABLE_BUFFER_SIZE (SUBLIME_BUFFER_SIZE - SUBLIME_FIELDS_LENGTH)

#ifdef WIN32 

typedef struct InternalSharedBufferDataStruct {
    DataBuffer* dataBuffer;
    HANDLE hMapFile;
    MUTEX_HANDLE mutex;
    EVENT_HANDLE bufferReadyEvent;
    EVENT_HANDLE dataReadyEvent;
} InternalSharedBufferData;

#else /* !WIN32 */ 

typedef struct InternalSharedBufferDataStruct {
    DataBuffer* dataBuffer;
    int* hMapFile;
    MUTEX_HANDLE mutex;
    EVENT_HANDLE bufferReadyEvent;
    EVENT_HANDLE dataReadyEvent;
} InternalSharedBufferData;

#endif


typedef struct SharedBufferStruct { 
    // read/write from/to the shared buffer 
    int (*write)(struct SharedBufferStruct *sb, 
                 const void *buffer, int32_t numOfBytes);
    int (*writeAll)(struct SharedBufferStruct *sb, 
                    const void *buffer, int32_t numOfBytes);
    int (*writeInt32)(struct SharedBufferStruct *sb, int32_t value);
    void (*flush)(struct SharedBufferStruct *sb);
    int (*read)(struct SharedBufferStruct *sb, 
                void *buffer, int32_t numOfBytes);
    int (*readAll)(struct SharedBufferStruct *sb, 
                   void *buffer, int32_t numOfBytes);
    int (*readInt32)(struct SharedBufferStruct *sb, int32_t *result);
    int (*readLong64)(struct SharedBufferStruct *sb, long64 *result);

    InternalSharedBufferData data;
} SharedBuffer; 

SharedBuffer* CreateNewSharedBuffer(char* name);
SharedBuffer* OpenSharedBuffer(char* name);
void DeleteSharedBuffer(SharedBuffer *s);


#endif
