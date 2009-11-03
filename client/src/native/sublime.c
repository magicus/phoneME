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

#include "lime.h"
#include "limethread.h"
#include "sublime.h"
#include "sublimeio.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LIME_JNI
#include "jni.h"
#endif

#ifdef WIN32

#include <process.h>

#else /* !WIN32 */ 

#include <pthread.h>
#include <time.h>
#include <sched.h>

#endif /* WIN32 */ 

#define MALLOC(s) malloc(s)
#define FREE(p) free(p)

#define HTON_MEMCPY2(src, dst, tmp)               \
    ((*(uint16_t*)tmp) = htons(*(uint16_t*)src),  \
     (*(char*)dst) = (*(char*)tmp),               \
     (*((char*)dst+1)) = (*((char*)tmp+1)))
#define HTON_MEMCPY4(src, dst, tmp)               \
    ((*(uint32_t*)tmp) = htonl(*(uint32_t*)src),  \
     (*(char*)dst) = (*(char*)tmp),               \
     (*((char*)dst+1)) = (*((char*)tmp+1)),       \
     (*((char*)dst+2)) = (*((char*)tmp+2)),       \
     (*((char*)dst+3)) = (*((char*)tmp+3)))

typedef struct InternalLimeData {
    int32_t functionID;
    int32_t argCount;
    int32_t *argTypes;
    int32_t returnType;
    char *name;
    bool_t trace;
} InternalLimeData;

/* Write the description of a type to a stream */
static void fprintType(FILE *f, int type);

/* Write the description of a method signature to a stream */
static void fprintSignature(FILE *out, LimeFunction *f);

/* Write a value (given its type) to a stream. 'length' is applicable
 * for arrays. 
 */
static void fprintValue(FILE *out, int type, void *pvalue, int length);

/* Read a value of a given type from a shared buffer. Reads the value to
 * pvalue. If an array is read, returns the length. 
 */
static int32_t read_value(SharedBuffer *sb, int type, void *pvalue);

/* Gets a unique identifier for the current thread */
static uint32_t getThreadUniqueId(void);

/* acquire an Event for the current thread. The event is signaled when the 
 * result for this thread is placed in returnSharedBuffer 
 */ 
static EVENT_HANDLE acquireEvent(uint32_t);

/*
 * Shared buffer for communication between 
 * JVM process and CVM process. 
 * callSharedBuffer - used for methods calls 
 * returnSharedBuffer - used for methods returns 
 */             
static SharedBuffer *callSharedBuffer, *returnSharedBuffer; 

static MUTEX_HANDLE callMutex;
static EVENT_HANDLE callRetvalProcessedEvent;

/* synchronized access to threadMap */ 
static MUTEX_HANDLE threadMapMutex; 

/* this thread waits for the return values of the calls (waits for event2) */ 
static LimeThread *listener; 

/* Global debug flag. Also the default trace state for newly created
 * functions 
 */
static int debug = 0;

/* Global synchronicity flag. If this is set, all LIME calls - even those that
 * return no result - are synchronous - not being used */
static int synchronous = 0;

const char *LimeTypes[] = {
    "void", "byte", "short", "int", "long", "char", "boolean", "float", "double"
};

const size_t LimeSizes[] = {
    0, 1, 2, 4, 8, 2, sizeof(int), 8, 8
};

/* Checks if the current machine works on network byteorder */ 
static int networkByteOrder = 0;
static int isNetworkByteorder(){
  long i = 1; 
  return (i == htonl(i));
}

static void swapLong64(long64 * l){
  char* buf = (char*)l; 
  char c; 
  int i; 
  assert(l != NULL);
  for (i = 0; i < 4; i++) {
    c = buf[i]; 
    buf[i] = buf[7-i]; 
    buf[7 - i] = c;
  }
}

#ifdef WIN32 
static ULONG64 getCurrentTime(void) {
    static ULONG64 fileTime_1_1_70 = 0;
    SYSTEMTIME st0;
    FILETIME   ft0;
    
    if (fileTime_1_1_70 == 0) {
        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
        fileTime_1_1_70 =
            (((ULONG64) ft0.dwHighDateTime << 32) | ft0.dwLowDateTime);
    }
    GetSystemTime(&st0);
    SystemTimeToFileTime(&st0, &ft0);
    return ((((ULONG64) ft0.dwHighDateTime << 32) | ft0.dwLowDateTime)
            - fileTime_1_1_70) / 10000;
}

#else /* !WIN32 */ 


// Need to revisit - should compare the logic of 2 implementations 
static ULONG64 getCurrentTime(void) {
  static ULONG64 res = 0;
  struct timespec st0;
  //  if (clock_gettime(CLOCK_REALTIME, &st0) == -1 ){
  //    error("getCurrentTime: clock_gettime failed");
  //  }
  //  res = st0.tv_sec << 32 | (st0.tv_nsec / 1000);
  return res; 
}

#endif /* WIN32 */
    
static int format_string(char *buffer, size_t buflen, char *format, ...) {
    va_list args;

    assert(format != NULL);

    /* prepare buffer */
    buffer[0] = '\0';
    buffer[buflen - 1] = '\0';

    va_start(args, format);

#ifdef WIN32 
    
    _vsnprintf(buffer, buflen - 1, format, args);

#else /* !win32 */ 
    
    vsnprintf(buffer, buflen - 1, format, args);
    
#endif

    va_end(args);
    
    return strlen(buffer);
}

/* Report an error condition and exit */
static void error(char *s, ...) {
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    va_end(args);
    
    EndLime();
    exit(1);
}

/* A buffer type for storing array return types */
typedef struct {
    char *data;
    unsigned int length; /* the size of the buffer */
} ArrayBuffer;

/* Ensure that s buffer is large enough to hold the specified number
   of bytes */
static void ensureBufferCapacity(ArrayBuffer *buffer, size_t size);

/* Obtain a buffer for the sole use of the current thread */
static ArrayBuffer *getArrayBuffer();

/* Obtain a buffer for sublime call arguments for the sole use of the current 
 * thread 
 */
static ArrayBuffer *getArgArrayBuffer();


/* used by the listener thread. wait for the return values of the called methods 
 * and awakes the specific thread that is waiting for this result
 */ 
static void waitForResults(void *param);

/* Obtains an integer value form the environment */
static int getEnvironmentInteger(const char *key, int defaultValue) {
    const char *value = getenv(key);
    if (value == NULL) {
        return defaultValue;
    } else {
        return atoi(value);
    }
}
/* return a pointer to a new alloced string: name+process ID */ 
static char * concatNameProcID(const char *name, const char *procid){
    char *tmp = (char *) malloc(strlen(name) + strlen(procid) + 1);
    if (tmp == NULL) {
        return NULL;
    }

    tmp = strcpy(tmp, name); 
    tmp = strcat(tmp, procid); 
    return tmp; 
}

static SharedBuffer* OpenSharedBufferForProcID(const char *prefix, 
                                               const char *procid) {
    SharedBuffer* sharedBuffer;
    char *fullBufferName = concatNameProcID(prefix, procid);
    if (fullBufferName == NULL) {
        return NULL;
    }
    
    sharedBuffer = OpenSharedBuffer(fullBufferName);
    free(fullBufferName);
    
    return sharedBuffer;
}

/* Connect to a SUBLIME server */
LIMEEXPORT void StartLime(void) {
    char *name0, *name1;
    char* prID;

    if (callSharedBuffer != NULL){
        return;
    }

    networkByteOrder = isNetworkByteorder(); 
    /* this environment var was created by Sublime.java */ 
    prID = getenv("SUBLIME_PROC_ID"); 
    if (prID == NULL) {
        error("Could not read environment variable SUBLIME_PROC_ID\n");
    }

    debug = getEnvironmentInteger(LIME_TRACE_FLAG, 0);
    synchronous = getEnvironmentInteger(LIME_SYNCH_FLAG, 0);

    callSharedBuffer = OpenSharedBufferForProcID(SUBLIME_SHARED_BUFFER_NAME0,
                                                 prID);
    if (callSharedBuffer == NULL) {
        error("SUBLIME: Could not open call shared buffer\n");
    }

    returnSharedBuffer = OpenSharedBufferForProcID(SUBLIME_SHARED_BUFFER_NAME1,
                                                   prID);
    if (returnSharedBuffer == NULL) {
        error("SUBLIME: Could not open return shared buffer\n");
    }
    
    callMutex = LimeCreateMutex(NULL, FALSE, NULL);
    if (callMutex == NULL) {
        error("SUBLIME: Could not create call synchronization mutex\n");
    }
    
    callRetvalProcessedEvent = LimeCreateEvent(NULL, FALSE, FALSE, NULL);
    if (callRetvalProcessedEvent == NULL) {
        error("SUBLIME: Could not create call retval event\n");
    }
    
    threadMapMutex = LimeCreateMutex(NULL, FALSE, NULL); 
    if (threadMapMutex == NULL) { 
        error("SUBLIME: Could not create thread map mutex\n"); 
    }

    listener = NewLimeThread(waitForResults, NULL); 
    if (listener == NULL) {
        error("SUBLIME: Could not create listener thread\n"); 
    }

    listener->start(listener); 
}

/* Disconnects from the SUBLIME server */
LIMEEXPORT void EndLime(void) {
    if (callSharedBuffer != NULL) {
        DeleteSharedBuffer(callSharedBuffer); 
        callSharedBuffer = NULL; 
    }
    
    if (returnSharedBuffer != NULL ){
        DeleteSharedBuffer(returnSharedBuffer); 
        returnSharedBuffer = NULL; 
    }
    
    if (callMutex != NULL) {
        LimeDestroyMutex(callMutex);
        callMutex = NULL;
    }
    
    if (callRetvalProcessedEvent != NULL) {
        LimeDestroyEvent(callRetvalProcessedEvent);
        callRetvalProcessedEvent = NULL;
    }

    if (threadMapMutex != NULL) {
        LimeDestroyMutex(threadMapMutex);
        threadMapMutex = NULL;
    } 

    if (listener != NULL) {
        DeleteLimeThread(listener);
        listener = NULL;
    }
}

/*
 * Writes array data to be passed to the buffer.
 *
 * See Notes to the write_nextValue() function for the rational behind using
 * HTON_MEMCPY[124] macros
 */
static void write_array(char **buffer,
                        int type, void *data, int length, bool_t trace) 
{
    int elementSize = LimeSizes[type];
    int i, high, low, value, *tmp;
    long64 value64;
    char *p, *ptmp;
  
    tmp = &value;
  
    if (data == NULL) 
        {
            i = LIME_NULL;
            HTON_MEMCPY4(&i, *buffer, tmp);
            (*buffer) += sizeof(int32_t);
            if (trace) 
                {
                    printf("null");
                }
        } 
    else
        {
            assert(length >= 0);
            HTON_MEMCPY4(&length, *buffer, tmp);
            (*buffer) += sizeof(int);
        
            if (length == 0)
                {
                    return;
                }
        
            p = (char *) data;
        
            /* Convert an array to right byteorder */
            /* No need to convert byte array */
            if (elementSize > 1)
                {
                    for (i = 0; i < length; i++)
                        {
                            switch (elementSize) 
                                {
                                case 2:
                                    HTON_MEMCPY2(p, *buffer, tmp);
                                    break;
                                case 4:
                                    HTON_MEMCPY4(p, *buffer, tmp);
                                    break;
                                case 8:
                                    value64 = *((long64 *) p);
                                    high = (uint32_t) (value64 >> 32);
                                    HTON_MEMCPY4(&high, *buffer, tmp);
                                    ptmp = *buffer + sizeof(int32_t);
                                    low = (uint32_t) (value64 & 0xffffffffUL);
                                    HTON_MEMCPY4(&low, ptmp, tmp);
                                    break;
                                default:
                                    break;
                                }
                            p += elementSize;
                            (*buffer) += elementSize;
                        }
                }
            else {
                memcpy(*buffer, data, length);
                (*buffer) += elementSize * length;
            }
        
            if (trace) {
                fprintValue(stdout, type | LIME_TYPE_ARRAY, data, length);
            }
        }
}

/*
 * Write next parameter value to the array that will be sent to the sublime 
 * server.
 *
 * Parameters:
 *      bufferData         Pointer (to pointer) to the next value
 *      type               Type of the next value
 *      argBufferData      Pointer (to pointer) to the next value source
 *      trace              Flag which enables tracing
 *
 * Side Effects:
 *      bufferData and argBufferData are advanced according to the next value 
 *      sizes in such a way that in the next call it is possible to pass the
 *      same pointers.
 * 
 * Notes:
 *      Macros HTON_MEMCPY[124] copy respected number of bytes one by one
 *      from src to dst after converting the source from host to network 
 *      endiness.
 *      Strainght assigment can not be used because some computer 
 *      architectures systems require address allingment at the time of
 *      memory access. memcpy() is not used in a bid to improve speed by 
 *      saving on function call with such a shord data lengths.
 *
 */
static void write_nextValue(char **bufferData,
                            int type, char **argBufferData, bool_t trace) {
    long64 value64;
    uint32_t high, low, value, *tmp = &value;
    if (type & LIME_TYPE_ARRAY) {
        void *array;
        int length;
        /* array pointer */
        array =  (void *)*((int*)(*argBufferData));
        (*argBufferData) += sizeof(int);
        /* array length */
        length = *((int*)(*argBufferData));
        (*argBufferData) += sizeof(int);
    
        write_array(bufferData, type & ~LIME_TYPE_ARRAY, array, length,
                    trace);
    } 
    else {
        if (trace)
            {
                fprintValue(stdout, type, *argBufferData, 0);
            }
    
        switch (LimeSizes[type]) 
            {
            case 1:
            case 2:
            case 4:
                HTON_MEMCPY4(*argBufferData, *bufferData, tmp);
                break;
            case 8:
                value64 = *((long64*)*argBufferData);
                high = (uint32_t) (value64 >> 32);
                HTON_MEMCPY4(&high, *bufferData, tmp);
                (*bufferData) += sizeof(int32_t);
                (*argBufferData) += sizeof(int32_t);
                low = (uint32_t) (value64 & 0xffffffffUL);
                HTON_MEMCPY4(&low, *bufferData, tmp);
                break;
            default:
                error("Data type %i not supported by SUBLIME\n", type);
            }
        (*bufferData) += sizeof(int32_t);
        (*argBufferData) += sizeof(int32_t);
    }
}

static int32_t read_array(SharedBuffer *sb, int type, void **pvalue) {
    int32_t rc;
    int32_t arrayLength;
    sb->readInt32(sb, &rc);
    if (rc==LIME_NULL) {
        *pvalue = NULL;
        arrayLength = 0;
    } else {
        ArrayBuffer *buffer;
        size_t elementSize;
        int i;
        char *p;
        int lenBytes;
        elementSize = LimeSizes[type];
        arrayLength = rc;
        lenBytes = elementSize * arrayLength;
        buffer = getArrayBuffer();
        ensureBufferCapacity(buffer, elementSize * arrayLength);
        p = buffer->data;
	   
        sb->readAll(sb, p, lenBytes);
	   
        if (type != LIME_TYPE_BYTE)
            {
                for (i = 0; i<arrayLength; i++) {
                    switch (elementSize) {
                    case 2:
                        *((uint16_t *) p) = ntohs(*(uint16_t *) p);
                        break;
                    case 8:
                        *((uint32_t *) (p + 4)) = ntohl(*(uint32_t *) (p + 4));
                    case 4:
                        *((uint32_t *) p) = ntohl(*(uint32_t *) p);
                        break;
                    default:
                        error("SUBLIME type %i not supported\n", type);
                        break;
                    }
                    p += elementSize;
                }
            }
        *((void **)pvalue) = (void *) buffer->data;
    }
    return arrayLength;
}

static int32_t read_value(SharedBuffer *sb, int type, void *pvalue) {
    int32_t arrayLength = 0;
    if (type & LIME_TYPE_ARRAY) {
        arrayLength = read_array(sb,
                                 type & ~LIME_TYPE_ARRAY, pvalue);
    } else {
        switch (type) {
        case LIME_TYPE_VOID:
            break;
      
        case LIME_TYPE_BYTE:
        case LIME_TYPE_SHORT:
        case LIME_TYPE_CHAR:
        case LIME_TYPE_INT:
        case LIME_TYPE_BOOLEAN: {
            int32_t rc;
            sb->readInt32(sb, &rc);
            if (pvalue != NULL) {
                *((int32_t *) pvalue) = rc;
            }
            break;
        }
        case LIME_TYPE_FLOAT: {
          double rc; 
          float f; 
          sb->readLong64(sb, (long64*)&rc);
          f = (float) rc; 
          if (pvalue != NULL) {
            *((float *) pvalue) = f;
          }
          break;
        }

        case LIME_TYPE_DOUBLE: {
          double rc; 
          sb->readLong64(sb, (long64*)&rc);
          if (pvalue != NULL) {
            *((double *) pvalue) = rc;
          }
          break;
        }
        case LIME_TYPE_LONG: {
      
            long64 rc;
            sb->readLong64(sb, &rc);
            if (pvalue != NULL) {
                *((long64 *) pvalue) = rc;
            }
            break;         
        }
        default:
            error("SUBLIME type %i not supported\n", type);
        }
    }
    return arrayLength;
}

/* 
 * Read sublime call arguments in advance and precalculate required array
 * length required for actual sublime call
 */
static int read_arguments(LimeFunction *f, va_list args, ArrayBuffer *buffer)
{
    int commandLength = 8; /* 8 bytes is the minimum command length */
    char *bufferData;
    int arrayData;
    int i;
    
    ensureBufferCapacity(buffer, f->data->argCount * sizeof(long64));
    bufferData = buffer->data;
    
    for (i=0; i<f->data->argCount; i++) {
        int type = (int) f->data->argTypes[i];
	   
        if (type & LIME_TYPE_ARRAY) {
            /* array */
            arrayData = (*((int *) bufferData)) = va_arg(args, int);
            bufferData += sizeof(int); 
            /* length */
            (*((int *) bufferData)) = va_arg(args, int);
            if (arrayData == 0)
                {
                    (*((int*)bufferData)) = 0;
                }
            commandLength += (*((int *) bufferData)) * 
                LimeSizes[type & ~LIME_TYPE_ARRAY] + sizeof(int);
            bufferData += sizeof(int);
        } else {
            switch (type) {
                /* Note from the man page of stdarg (on Solaris): It is up
                 *  to the calling routine to specify in some manner how
                 *  many arguments there are, since it is not always
                 *  possi- ble to determine the number of arguments from
                 *  the stack frame. For example, execl is passed a zero
                 *  pointer to signal the end of the list. printf can tell
                 *  how many arguments there are by the format. It is
                 *  non-portable to specify a second argument of char,
                 *  short, or float to va_arg, because arguments seen by
                 *  the called function are not char, short, or float. C
                 *  converts char and short arguments to int and converts
                 *  float arguments to double before passing them to a
                 *  function.  
                 */
            case LIME_TYPE_BYTE:
            case LIME_TYPE_CHAR:
            case LIME_TYPE_SHORT:
            case LIME_TYPE_INT:  
            case LIME_TYPE_BOOLEAN:
                (*((int32_t*)bufferData)) = (int32_t) va_arg(args, int32_t);
                bufferData += sizeof(int32_t);
                commandLength += sizeof(int32_t);
                break;
            case LIME_TYPE_LONG:
                (*((long64*) bufferData)) = va_arg(args, long64);
                bufferData += sizeof(long64);
                commandLength += sizeof(long64);
                break;
            case LIME_TYPE_DOUBLE:
            case LIME_TYPE_FLOAT: {
              double d = va_arg(args, double);
              if (networkByteOrder){
                swapLong64((long64*)&d);
              } 
              memcpy(bufferData,(char*)&d, 8);
              bufferData += sizeof(double);
              commandLength += sizeof(double);
              break;
            }            
            default:
                error("Data type %i not supported by SUBLIME\n", type);
            }
        }
    }
    
    return commandLength;
}

static void lime_call(LimeFunction *f, void *result, ...) {
    va_list args;
    int32_t i;
    int *resultLength; /* For array return types */
    int commandLength;
    int32_t arrayLength;
    void **presult = &result; /* used when aray is returned */
    long64 sendTime;
    ArrayBuffer *buffer, *argBuffer;
    char *bufferData, *argBufferData;
    int threadID = getThreadUniqueId(); 
    uint32_t resultSize;    
    /* this event will be signaled when the return value is placed in returnSharedBuffer */ 
    EVENT_HANDLE event; 
    if (debug) {
        printf("lime_call: %s (threadID:%d)\n",f->data->name,threadID);
    }
    
    assert(f != NULL);
    event = acquireEvent(threadID);
    
    if (event == NULL){
        error("SUBLIME: could not acquire event for \n"); 
    }
    /* Start timing, if tracing is on for this function */
    if (f->data->trace) {
        sendTime = getCurrentTime();
    }
    /* Find out where to store the result */
    va_start(args, result);
    
    if (f->data->returnType & LIME_TYPE_ARRAY) {
        resultLength = va_arg(args, int *);
    }
    argBuffer = getArgArrayBuffer();
        
    /* Read sublime call arguments in advance and precalculate required array
     * length required for actual sublime call
     */
    commandLength = read_arguments(f, args, argBuffer);
    argBufferData = argBuffer->data;
    
    buffer = getArrayBuffer();
    ensureBufferCapacity(buffer, commandLength);
    bufferData = buffer->data;
    
    /* Tell the Lime server that a command is about to be sent */
    i = htonl(LIME_COMMAND); 
    memcpy(bufferData, (char*)&i,sizeof(int32_t));
    bufferData += sizeof(int32_t);
    /* tell the Lime server the ID of the required Lime function */
    i = htonl(f->data->functionID);
    memcpy(bufferData,(char*)&i, sizeof(int32_t));
    bufferData += sizeof(int32_t);
    
    if (f->data->trace) {
        if (f->data->returnType == LIME_TYPE_VOID && !synchronous) {
            printf("SUBLIME: call #%li %s(",
                   (long) f->data->functionID, f->data->name);
        } else {
            printf("SUBLIME: call #%li (%i) %s(",
                   (long) f->data->functionID, f->data->returnType, f->data->name);
        }
    }
    
    /* Prepare array with parameters */
    for (i=0; i<f->data->argCount; i++) {
        int type = (int) f->data->argTypes[i];
        if (f->data->trace && i>0) {
            printf(", ");
        }
        write_nextValue(&bufferData, type, &argBufferData, f->data->trace);
    }
    
    if (f->data->trace) {
        printf(")\n");
    }
    va_end(args);

    WaitForMutex(callMutex);

    if (callSharedBuffer->writeInt32(callSharedBuffer, threadID) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write call threadID\n");
    }
    
    if (callSharedBuffer->writeInt32(callSharedBuffer, commandLength) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write command length\n");
    }
    
    if (callSharedBuffer->writeAll(callSharedBuffer, 
                                   buffer->data, commandLength) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write call data\n");
    }
    
    callSharedBuffer->flush(callSharedBuffer);
    
    LimeReleaseMutex(callMutex);

    /* Wait for the result and read the return data */
    WaitForEvent(event); 

    /* read the result size */
    returnSharedBuffer->readInt32(returnSharedBuffer, &resultSize);

    arrayLength = read_value(returnSharedBuffer, f->data->returnType, result);
    /* process the return data from the SubLime server */
    if (f->data->returnType & LIME_TYPE_ARRAY) {
        presult = (void**) result;
        if (resultLength != NULL) {
            *resultLength = arrayLength;
        }
    }
    if (f->data->trace) {
        long64 endTime = getCurrentTime();
        printf("SUBLIME: call %li to %s returned ", threadID, f->data->name);
        fprintValue(stdout, f->data->returnType, *presult, arrayLength);
        printf("sublime in %i ms\n",
               (int) (endTime - sendTime));
        fflush(stdout);
    }
    
    LimeSetEvent(callRetvalProcessedEvent);
    
    /* context switch - notify the JVM process that returnSharedBuffer is now free to use */ 
    yield();
}

void lime_trace(LimeFunction *f, bool_t trace) {
    f->data->trace = trace;
}

LIMEEXPORT LimeFunction *NewLimeFunction(const char *packageName,
                              const char *className,
                              const char *methodName) {
    LimeFunction *f  = (LimeFunction *) MALLOC( sizeof(LimeFunction) );
    InternalLimeData *d = (InternalLimeData *)
        MALLOC( sizeof(InternalLimeData) );
    int threadID = getThreadUniqueId(); 
    int lookupLength;
    uint32_t resultSize;    
    char lookupBuffer[1024];
    
    /* this event will be signaled when the return value is placed in returnSharedBuffer */ 
    EVENT_HANDLE event; 
    assert(packageName != NULL);
    assert(className != NULL);
    assert(methodName != NULL);
   
    StartLime();
    
    if (!f || !d) {
        if (f) {
            FREE(f);
        }
        error("Cannot allocate memory for new SUBLIME object\n");
    } else {
        f->data = d;
    }
    
    event = acquireEvent(threadID); 
    
    if (event == NULL){
        error("SUBLIEME: could not access shared buffer\n"); 
    }
    
    d->name = strdup(methodName);
    
    if (debug) {
        printf("SUBLIME: lookup %s.%s.%s\n",
               packageName, className, methodName);
        fflush(stdout);
    } 

    lookupLength = format_string(lookupBuffer, 
                                 sizeof(lookupBuffer) / sizeof(*lookupBuffer), 
                                 "%s\n%s\n%s\n", 
                                 packageName, className, methodName);

    WaitForMutex(callMutex);

    if (callSharedBuffer->writeInt32(callSharedBuffer, threadID) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write call threadID\n");
    }
    
    if (callSharedBuffer->writeInt32(callSharedBuffer, 
                                     lookupLength + sizeof(int32_t)) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write method lookup length\n");
    }
    
    if (callSharedBuffer->writeInt32(callSharedBuffer, LIME_LOOKUP) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write LIME_LOOKUP request type\n");
    }
    
    if (callSharedBuffer->writeAll(callSharedBuffer, 
                                   lookupBuffer, lookupLength) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write lookup data\n");
    }

    callSharedBuffer->flush(callSharedBuffer);
    
    LimeReleaseMutex(callMutex);

    /* Wait for the result and read the return data */
    WaitForEvent(event); 

    /* read the result size */
    returnSharedBuffer->readInt32(returnSharedBuffer, &resultSize);

    /* the target buffer is now returnSharedBuffer */ 
    returnSharedBuffer->readInt32(returnSharedBuffer, &d->functionID);
    if (debug) {
        printf("SUBLIME: getting data for function #%li\n", d->functionID);
    }
    returnSharedBuffer->readInt32(returnSharedBuffer, &d->argCount);
    d->argTypes = (int32_t *) MALLOC( sizeof(int32_t) * d->argCount);
    if (d->argTypes==0) {
        error("Cannot allocate memory for new SUBLIME object\n");
    } else {
        int i;
        
        for (i=0; i<d->argCount; i++) {
            returnSharedBuffer->readInt32(returnSharedBuffer, d->argTypes + i);
        }
    }
    
    returnSharedBuffer->readInt32(returnSharedBuffer, &d->returnType);
    
    LimeSetEvent(callRetvalProcessedEvent);
    
    /* context switch - notify the JVM process that returnSharedBuffer is now free to use */ 
    //    sleep(0); 
    yield();
    f->call = lime_call;
    f->trace = lime_trace;
    if (debug) {
        printf("SUBLIME: #%li = ", d->functionID);
        fprintSignature(stdout, f);
        printf("\n");
        fflush(stdout);
    }
    /* The default value of the "trace" field is the same as that of the
     * global "debug" variable. */
    d->trace = debug;
    
    return f;
}

LIMEEXPORT void DeleteLimeFunction(LimeFunction *f) {
    assert(f != NULL);
    assert(f->data != NULL);
    if (f->data->argTypes) {
        FREE (f->data->argTypes);
    }
    FREE(f->data->name); 
    FREE(f->data);
    FREE(f);
}

/* empty implementation */ 
LIMEEXPORT int LimeCheckEvent(int *flag) {
    *flag = 1;
    return 0;
}

LIMEEXPORT int LimeDecrementEventCount() {
    return 0;
}


/* Prints out a description of a type */
static void fprintType(FILE *f, int type) {
    int baseType = (type & LIME_TYPE_ARRAY)
        ? (type & ~LIME_TYPE_ARRAY)
        : type;
    fprintf(f, "%s", LimeTypes[baseType]);
    if (type & LIME_TYPE_ARRAY) {
        fprintf(f, "[]");
    }
}


/* Prints out a function's signature in human-readable form */
static void fprintSignature(FILE *out, LimeFunction *f) {
    int i;
    fprintf(out, "(");
    for (i=0; i<f->data->argCount; i++) {
        if (i>0) {
            fprintf(out, ", ");
        }
        fprintType(out, f->data->argTypes[i]);
    }
    fprintf(out, ") : ");
    fprintType(out, f->data->returnType);
}

/* Prints out a sublime parameter or return value */
static void fprintValue(FILE *out, int type, void *pvalue, int length) {
    int baseType = (type & LIME_TYPE_ARRAY)
        ? (type & ~LIME_TYPE_ARRAY)
        : type;
    if (baseType==type) {
        int32_t i;
        uint16_t c;
        long64 l;
        float f; 
        double d; 
        switch (type){
        case LIME_TYPE_BYTE:
            i = (int32_t) * ( (char*) pvalue);
            fprintf(out, "0x%.2x", i & 0xff);
            break;
        case LIME_TYPE_SHORT:
            i = (int32_t) * ( (signed short*) pvalue);
            fprintf(out, "%li", i);
            break;
        case LIME_TYPE_CHAR:
            c = (uint16_t) * ( (uint16_t*) pvalue);
            if (c < 32 || c >= 127)
                {
                    fprintf(out, "\\u%.4x", (int32_t) c);
                }
            else
                {
                    fprintf(out, "%c", (char)c);
                }
            break;
        case LIME_TYPE_INT:
            i = * ( (uint32_t*) pvalue);
            fprintf(out, "%li", i);
            break;
        case LIME_TYPE_FLOAT:
          f = (float) * ( (float*) pvalue);
          fprintf(out, "%10.5f", f);
          break;
        case LIME_TYPE_BOOLEAN:
            i = (int32_t) * ( (bool_t*) pvalue);
            fprintf(out, "%li", i);
            break;
        case LIME_TYPE_LONG:
            l = * ( (long64*) pvalue);
            fprintf(out, "%lli", l);
            break;
        case LIME_TYPE_VOID:
            fprintf(out, "void");
            break;
        case LIME_TYPE_DOUBLE: 
            d = * ( (double*) pvalue);
            fprintf(out, "%10.5d", d);
            break;         
        default:
            error("SUBLIME type %i not supported\n", type);
        }
    } else {
        if (pvalue==NULL) {
            fprintf(out, "null");
        } else {
            int i;
            char* p = (char *) pvalue;
            size_t elementSize = LimeSizes[baseType];
            if (baseType == LIME_TYPE_CHAR)
                {
                    fprintf(out, "\"");
                }
            else
                {
                    fprintf(out, "[%li: ", length);
                }
            for (i=0; i<length; i++) {
                if (i>0 && baseType != LIME_TYPE_CHAR) {
                    fprintf(out, ", ");
                }
			 
                if (length > (baseType == LIME_TYPE_CHAR ? 4000 : 16) && i == 6) {
                    fprintf(out, "...");
                    i = length - 3;
                    continue;
                }
        
                fprintValue(out, baseType, p, 0);
                p += elementSize;
            }
            if (baseType == LIME_TYPE_CHAR)
                {
                    fprintf(out, "\"");
                }
            else
                {
                    fprintf(out, "]");
                }
        }
    }
}

/* === Variables and types for shared buffer and buffers === */

/* structure for storing a (thread ID, event) pair */
typedef struct {
    uint32_t thread;
    EVENT_HANDLE event;
    ArrayBuffer buffer;  /* a buffer for storing array return types */
    ArrayBuffer argBuffer; /* a buffet for storing sublime_call arguments  */
} ThreadMapping;


/* a table mapping thread ids to events */
static ThreadMapping *threadMap = NULL;
static int threadMapLength = 0;
static int threadMapCapacity = 0;

/* === Buffer functions === */

static void ensureBufferCapacity(ArrayBuffer *buffer, size_t size) {
    /* Even a buffer of size 0 needs to be allocated. realloc() won't do size 0,
     * so allocate a buffer of size 1.
     */
    size = (size < 1) ? 1 : size;
  
    if (buffer->length < size) {
        char *newBufferData = realloc(buffer->data, size);
        if (newBufferData==NULL) {
            error("SUBLIME out of memory: cannot allocate %li bytes",
                  (long) size);
        } else {
            if (debug) {
                printf("SUBLIME: Expanded buffer from %i bytes to %i bytes\n",
                       buffer->length, size);
                fflush(stdout);
            }
            buffer->data = newBufferData;
            buffer->length = size;
        }
    }
}

/* Obtain a buffer for the sole use of the current thread */
static ArrayBuffer *getArrayBuffer() {
    int i;
    uint32_t currentThread = getThreadUniqueId();
    for (i=0; i<threadMapLength; i++) {
        if (threadMap[i].thread == currentThread) {
            ArrayBuffer *buffer = &threadMap[i].buffer;
            assert(buffer != NULL);
            return buffer;
        }
    }
    /* This point should never be reached */
    assert(FALSE);
    return NULL;
}

/* Obtain a buffer for sublime call arguments for the sole use of the current 
 * thread 
 */
static ArrayBuffer *getArgArrayBuffer() {
    int i;
    uint32_t currentThread = getThreadUniqueId();
    for (i=0; i<threadMapLength; i++) {
        if (threadMap[i].thread == currentThread) {
            ArrayBuffer *buffer = &threadMap[i].argBuffer;
            assert(buffer != NULL);
            return buffer;
        }
    }
    /* This point should never be reached */
    assert(FALSE);
    return NULL;
}

/* Obtain an event for notifying the thread that the return value is 
 * placed in returnSharedBuffer, and also match an entry in the threadMap table for 
 * this thread. 
 */
static EVENT_HANDLE acquireEvent(uint32_t tID) {
    int i;
    EVENT_HANDLE event = NULL;  
    ThreadMapping *availableMapping = NULL; 
    uint32_t currentThread = tID;
    WaitForMutex(threadMapMutex);
    for (i=0; i<threadMapLength; i++) {
        if (threadMap[i].thread == currentThread) {
            event = threadMap[i].event;
            break;
        }
        else {
            if (!availableMapping && !threadMap[i].thread) {
                availableMapping = threadMap + i;
            }
        }
    }
    if (event == NULL) {
        if (availableMapping) {
            availableMapping->thread = currentThread;
            event = availableMapping->event;
        } else {
            /* Create a new event */
            event = LimeCreateEvent(NULL, FALSE, FALSE, NULL); 
            if ( event == NULL){
                error("Cannot create new event for SUBSUBLIME current thread\n");
            }
            if (threadMapLength == threadMapCapacity) {
                /* enlarge the threadMap table */ 
                int newThreadMapCapacity = 1 + threadMapCapacity * 2;
                ThreadMapping *newThreadMap =
                    realloc(threadMap,
                            newThreadMapCapacity * sizeof(ThreadMapping));
                if (newThreadMap == NULL) {
                    newThreadMapCapacity = threadMapCapacity + 1;
                    newThreadMap = realloc(threadMap,
                                           newThreadMapCapacity * sizeof(ThreadMapping));
                    if (newThreadMap == NULL) {
                        error("SUBLIME out of memory: cannot create a new event "
                              "(%i already existing)\n", threadMapLength);
                    }
                }
                threadMap = newThreadMap;
                threadMapCapacity = newThreadMapCapacity;
            }
            threadMap[threadMapLength].thread = currentThread;
            threadMap[threadMapLength].buffer.data = NULL;
            threadMap[threadMapLength].buffer.length = 0;
            threadMap[threadMapLength].argBuffer.data = NULL;
            threadMap[threadMapLength].argBuffer.length = 0;
            threadMap[threadMapLength].event = event; 
            threadMapLength++;
        }
    }
    LimeReleaseMutex(threadMapMutex); 
    return event;
}


static void releaseEvent(SharedBuffer *sb) {
    int i;
    int current = getThreadUniqueId(); 
    WaitForMutex(threadMapMutex);
    for (i=0; i<threadMapLength; i++) {
        if (threadMap[i].thread == current) {
            threadMap[i].thread = 0;
            break;
        }
    }
    LimeReleaseMutex(threadMapMutex); 
}

/* === Native functions === */

#ifdef WIN32

static HANDLE lockObject;

static uint32_t getThreadUniqueId(void) {
    return(GetCurrentThreadId());
}

#else

static pthread_mutex_t lockObject;

static uint32_t getThreadUniqueId(void) {
    return(pthread_self());
}

#endif

LIMEEXPORT void LimeLock(void) {
    //  lock();
}

LIMEEXPORT void LimeUnlock(void) {
    //  unlock();
}


void waitForResults(void *p){
    EVENT_HANDLE event; 
    int32_t tID; 
    while (1) {
        if (returnSharedBuffer->readInt32(returnSharedBuffer, &tID) != 0) {
            break;
        }

        event = acquireEvent(tID); 
        /* set the event for the specific thread 
           that is waiting for this result */ 
        LimeSetEvent(event);
        WaitForEvent(callRetvalProcessedEvent);
    }
}

#ifdef LIME_JNI

static void write_nextValue_jni(JNIEnv *env, char **bufferData,
                            int type, jobject arg, bool_t trace);
static int read_arguments_jni(
        JNIEnv *env, LimeFunction *f, jobjectArray args);

static jclass class_Boolean = NULL;
static jclass class_Byte = NULL;
static jclass class_Character = NULL;
static jclass class_Short = NULL;
static jclass class_Integer = NULL;
static jclass class_Long = NULL;
static jclass class_Float = NULL;
static jclass class_Double = NULL;
static jmethodID method_getBoolean = NULL;
static jmethodID method_getByte = NULL;
static jmethodID method_getChar = NULL;
static jmethodID method_getShort = NULL;
static jmethodID method_getInt = NULL;
static jmethodID method_getLong = NULL;
static jmethodID method_getFloat = NULL;
static jmethodID method_getDouble = NULL;
static jmethodID cons_Boolean = NULL;
static jmethodID cons_Byte = NULL;
static jmethodID cons_Char = NULL;
static jmethodID cons_Short = NULL;
static jmethodID cons_Integer = NULL;
static jmethodID cons_Long = NULL;
static jmethodID cons_Float = NULL;
static jmethodID cons_Double = NULL;

static void init(JNIEnv *env) {
    if (cons_Double != NULL) {
        return;
    }
    class_Boolean = (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Boolean"));
    class_Byte = (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Byte"));
    class_Character = (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Character"));
    class_Short = (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Short"));
    class_Integer = (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Integer"));
    class_Long = (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Long"));
    class_Float = (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Float"));
    class_Double = (*env)->NewGlobalRef(env, (*env)->FindClass(env, "java/lang/Double"));
    method_getBoolean = (*env)->GetMethodID(env, class_Boolean, "booleanValue", "()Z");
    method_getByte = (*env)->GetMethodID(env, class_Byte, "byteValue", "()B");
    method_getChar = (*env)->GetMethodID(env, class_Character, "charValue", "()C");
    method_getShort = (*env)->GetMethodID(env, class_Short, "shortValue", "()S");
    method_getInt = (*env)->GetMethodID(env, class_Integer, "intValue", "()I");
    method_getLong = (*env)->GetMethodID(env, class_Long, "longValue", "()J");
    method_getFloat = (*env)->GetMethodID(env, class_Float, "floatValue", "()F");
    method_getDouble = (*env)->GetMethodID(env, class_Double, "doubleValue", "()D");
    cons_Boolean = (*env)->GetMethodID(env, class_Boolean, "<init>", "(Z)V");
    cons_Byte = (*env)->GetMethodID(env, class_Byte, "<init>", "(B)V");
    cons_Char = (*env)->GetMethodID(env, class_Character, "<init>", "(C)V");
    cons_Short = (*env)->GetMethodID(env, class_Short, "<init>", "(S)V");
    cons_Integer = (*env)->GetMethodID(env, class_Integer, "<init>", "(I)V");
    cons_Long = (*env)->GetMethodID(env, class_Long, "<init>", "(J)V");
    cons_Float = (*env)->GetMethodID(env, class_Float, "<init>", "(F)V");
    cons_Double = (*env)->GetMethodID(env, class_Double, "<init>", "(D)V");
}

JNIEXPORT jlong JNICALL
Java_com_sun_kvem_lime_LimeFunction_newLimeFunction(
        JNIEnv *env, jclass cls,
        jstring packageName, jstring className, jstring methodName) {
    const jsize packageNameUTFLength = (*env)->GetStringUTFLength(env, packageName);
    const jsize classNameUTFLength = (*env)->GetStringUTFLength(env, className);
    const jsize methodNameUTFLength = (*env)->GetStringUTFLength(env, methodName);
    const jsize packageNameLength = (*env)->GetStringLength(env, packageName);
    const jsize classNameLength = (*env)->GetStringLength(env, className);
    const jsize methodNameLength = (*env)->GetStringLength(env, methodName);
    char *packageNameChars = malloc(packageNameUTFLength + 1);
    char *classNameChars = malloc(classNameUTFLength + 1);
    char *methodNameChars = malloc(methodNameUTFLength + 1);
    LimeFunction *f;
    assert(packageNameChars != 0);
    assert(classNameChars != 0);
    assert(methodNameChars != 0);
    packageNameChars[packageNameUTFLength] = '\000';
    classNameChars[classNameUTFLength] = '\000';
    methodNameChars[methodNameUTFLength] = '\000';
    (*env)->GetStringUTFRegion(env, packageName, 0, packageNameLength, packageNameChars);
    (*env)->GetStringUTFRegion(env, className, 0, classNameLength, classNameChars);
    (*env)->GetStringUTFRegion(env, methodName, 0, methodNameLength, methodNameChars);
    
    f = NewLimeFunction(packageNameChars, classNameChars, methodNameChars);
    init(env);
    free(packageNameChars);
    free(classNameChars);
    free(methodNameChars);
    return (jlong) f;
}

JNIEXPORT void JNICALL
Java_com_sun_kvem_lime_LimeFunction_deleteLimeFunction(
        JNIEnv *env, jclass cls, jlong f) {
    DeleteLimeFunction((LimeFunction *) f);
}

JNIEXPORT jobject JNICALL
Java_com_sun_kvem_lime_LimeFunction_invokeLimeFunction(
        JNIEnv *env, jclass cls, jlong limeFunction, jobjectArray args) {
    int32_t i;
    int commandLength;
    int32_t arrayLength;
    jobject result = NULL;
    long64 sendTime;
    ArrayBuffer *buffer;
    char *bufferData, *argBufferData;
    uint32_t threadID = getThreadUniqueId(); 
    uint32_t resultSize;    
    /* this event will be signaled when the return value is placed in returnSharedBuffer */ 
    EVENT_HANDLE event; 
    LimeFunction *f = (LimeFunction *) limeFunction;
    
    assert(f != NULL);
    assert((*env)->GetArrayLength(env, args) == f->data->argCount);
    event = acquireEvent(threadID);
    
    if (event == NULL){
        error("SUBLIME: could not acquire event for \n"); 
    }
    /* Start timing, if tracing is on for this function */
    if (f->data->trace) {
        sendTime = getCurrentTime();
    }
    
    /* Read sublime call arguments in advance and precalculate required array
     * length required for actual sublime call
     */
    commandLength = read_arguments_jni(env, f, args);
    
    buffer = getArrayBuffer();
    ensureBufferCapacity(buffer, commandLength);
    bufferData = buffer->data;
    
    /* Tell the Lime server that a command is about to be sent */
    i = htonl(LIME_COMMAND);
    memcpy(bufferData, (char*)&i,sizeof(int32_t));
    bufferData += sizeof(int32_t);
    /* tell the Lime server the ID of the required Lime function */
    i = htonl(f->data->functionID);
    memcpy(bufferData,(char*)&i, sizeof(int32_t));
    bufferData += sizeof(int32_t);
    
    if (f->data->trace) {
        if (f->data->returnType == LIME_TYPE_VOID && !synchronous) {
            printf("SUBLIME: call #%li %s(",
                   (long) f->data->functionID, f->data->name);
        } else {
            printf("SUBLIME: call #%li %s(",
                   (long) f->data->functionID,
                   f->data->name);
        }
    }
    
    /* Prepare array with parameters */
    for (i = 0; i < f->data->argCount; i++) {
        int type = (int) f->data->argTypes[i];
        if (f->data->trace && i>0) {
            printf(", ");
        }
        write_nextValue_jni(env, &bufferData, type,
                (*env)->GetObjectArrayElement(env, args, i),
                f->data->trace);
    }
    
    if (f->data->trace) {
        printf(")\n");
    }

    
    WaitForMutex(callMutex);

    if (callSharedBuffer->writeInt32(callSharedBuffer, threadID) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write call threadID\n");
    }

    if (callSharedBuffer->writeInt32(callSharedBuffer, commandLength) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write command length\n");
    }
    
    if (callSharedBuffer->writeAll(callSharedBuffer, 
                                   buffer->data, commandLength) != 0) {
        LimeReleaseMutex(callMutex);
        error("SUBLIME: Failed to write call data\n");
    }
    
    callSharedBuffer->flush(callSharedBuffer);
    
    LimeReleaseMutex(callMutex);

    /* Wait for the result and read the return data */
    WaitForEvent(event); 

    /* read the result size */
    returnSharedBuffer->readInt32(returnSharedBuffer, &resultSize);

    /* process the return data from the SubLime server */
    if (f->data->returnType & LIME_TYPE_ARRAY) {
        void *arrayData;
        jsize arrayLength = read_value(returnSharedBuffer, f->data->returnType, &arrayData);
        if (f->data->trace) {
            long64 endTime = getCurrentTime();
            printf("SUBLIME: call to %s returned ", f->data->name);
            fprintValue(stdout, f->data->returnType, arrayData, arrayLength);
            printf(" in %i ms\n",
                   (int) (endTime - sendTime));
            fflush(stdout);
        }
        if (arrayData != NULL) {
            switch (f->data->returnType & ~LIME_TYPE_ARRAY) {
                case LIME_TYPE_INT: {
                    jintArray resultArray = (*env)->NewIntArray(env, arrayLength);
                    (*env)->SetIntArrayRegion(env, resultArray, 0, arrayLength, (jint *) arrayData);
                    result = resultArray;
                    break;
                }
            }
        }
    } else {
        jlong longValue;
        read_value(returnSharedBuffer, f->data->returnType, &longValue);
        switch (f->data->returnType) {
            case LIME_TYPE_BOOLEAN: {
                result = (*env)->NewObject(env, class_Boolean, cons_Boolean, *((jboolean *) &longValue));
                break;
            }
            case LIME_TYPE_INT: {
                result = (*env)->NewObject(env, class_Integer, cons_Integer, *((jint *) &longValue));
                break;
            }
        }
        if (f->data->trace) {
            long64 endTime = getCurrentTime();
            printf("SUBLIME: call to %s returned ", f->data->name);
            fprintValue(stdout, f->data->returnType, &longValue, 0);
            printf(" in %i ms\n",
                   (int) (endTime - sendTime));
            fflush(stdout);
        }
    }
    
    LimeSetEvent(callRetvalProcessedEvent);

    /* context switch - notify the JVM process that returnSharedBuffer is now free to use */ 
    yield();
    return result;
}

static int read_arguments_jni(
        JNIEnv *env, LimeFunction *f, jobjectArray args)
{
    int commandLength = 8; /* 8 bytes is the minimum command length */
    int i;
    
    for (i = 0; i < f->data->argCount; i++) {
        int type = (int) f->data->argTypes[i];
	   
        if (type & LIME_TYPE_ARRAY) {
            /* array */
            jarray array = (jarray) (*env)->GetObjectArrayElement(env, args, i);
            commandLength += (*env)->GetArrayLength(env, array) * 
                LimeSizes[type & ~LIME_TYPE_ARRAY] + sizeof(int);
        } else {
            switch (type) {
            case LIME_TYPE_BYTE:
            case LIME_TYPE_CHAR:
            case LIME_TYPE_SHORT:
            case LIME_TYPE_INT:  
            case LIME_TYPE_BOOLEAN:
                commandLength += sizeof(int32_t);
                break;
            case LIME_TYPE_LONG:
                commandLength += sizeof(long64);
                break;
            default:
                error("Data type %i not supported by SUBLIME\n", type);
            }
        }
    }
    
    return commandLength;
}

static void write_nextValue_jni(JNIEnv *env, char **bufferData,
                            int type, jobject arg, bool_t trace) {
    long64 value64;
    uint32_t high, low, value, *tmp = &value;
    if (type & LIME_TYPE_ARRAY) {
        void *array = (*env)->GetPrimitiveArrayCritical(env, (jarray) arg, 0);
        int length = (*env)->GetArrayLength(env, (jarray) arg);
        write_array(bufferData, type & ~LIME_TYPE_ARRAY, array, length,
                    trace);
        (*env)->ReleasePrimitiveArrayCritical(env, (jarray) arg, array, 0);
    } 
    else {
        switch (LimeSizes[type]) 
            {
            case 1:
            case 2:
            case 4: {
                jint i;
                switch (type) {
                    case LIME_TYPE_BOOLEAN: {
                        i = (jint) (*env)->CallBooleanMethod(env, 
                                arg, method_getBoolean);
                        break;
                    }
                    case LIME_TYPE_BYTE: {
                        i = (jint) (*env)->CallByteMethod(env, 
                                arg, method_getByte);
                        break;
                    }
                    case LIME_TYPE_CHAR: {
                        i = (jint) (*env)->CallCharMethod(env, 
                                arg, method_getChar);
                        break;
                    }
                    case LIME_TYPE_SHORT: {
                        i = (jint) (*env)->CallShortMethod(env, 
                                arg, method_getShort);
                        break;
                    }
                    case LIME_TYPE_INT: {
                        i = (jint) (*env)->CallIntMethod(env, 
                                arg, method_getInt);
                        break;
                    }
                    case LIME_TYPE_LONG: {
                        i = (jint) (*env)->CallLongMethod(env, 
                                arg, method_getLong);
                        break;
                    }
                    default: {
                        error("Data type %i not supported by Lime\n", type);
                    }
                }
                if (trace) {
                    fprintValue(stdout, type, &i, 0);
                }
                HTON_MEMCPY4(&i, *bufferData, tmp);
                break;
            }
            case 8: {
                value64 = (*env)->CallLongMethod(env, arg, method_getLong);
                if (trace) {
                    fprintValue(stdout, type, &value64, 0);
                }
                high = (uint32_t) (value64 >> 32);
                HTON_MEMCPY4(&high, *bufferData, tmp);
                (*bufferData) += sizeof(int32_t);
                low = (uint32_t) (value64 & 0xffffffffUL);
                HTON_MEMCPY4(&low, *bufferData, tmp);
                break;
            }
        }
        (*bufferData) += sizeof(int32_t);
    }
}

#endif
