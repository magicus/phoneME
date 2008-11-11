/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

# include "incls/_precompiled.incl"
# include "incls/_MonitorMemory.cpp.incl"

#if ENABLE_MEMORY_MONITOR

#ifdef __cplusplus
extern "C" {
#endif

/* The IP address of the server */
static const char *server = "127.0.0.1";

/**
 * The offset to the send buffer, where the next command will be placed.
 */ 
static int sendOffset = 0;

/**
 * The number of commands stored in the send buffer.
 */ 
static int numCommands = 0;

static int lastEnterMethodThread = -1;

/**
 * A flag which indicates if the memory monitor bufer has been flushed from the
 * last time this flag was cleared. It's used in the memory monitor flushing 
 * thread (see memMonitor_md.c).
 */   
volatile int memmonitor_flushed = 0;

#ifdef __cplusplus
}
#endif

/**
 * This function initializes the memory monitor. After the initialization, the
 * other memmonitor_* functions can be called.
 */  
void 
MonitorMemory::memmonitor_startup() {
    memset(callStackThread, 0, sizeof(callStackThread));
    // do the platform depended initialization
    memmonitor_md_startup();
}

/**
 * This function frees resources which are necessary for the memory monitor to
 * work properly.
 */  
void 
MonitorMemory::memmonitor_shutdown() {
    if (isSocketInitialized()) {
        // stop the memory monitor buffer flushing thread
        memmonitor_md_stopFlushThread();

        memmonitor_lock();
        flushBuffer();
        memmonitor_unlock();

        socketDisconnect(); 
    }

    // do the platform depended deinitialization
    memmonitor_md_shutdown();
}

/**
 * This function is called when a new heap is allocated. It happens typically 
 * at the beginning of VM execution.
 * 
 * @param heapSize the allocated size
 */       
void
MonitorMemory::memmonitor_allocateHeap(long heapSize) {
    LimeFunction *f;
    int port;

    if (!isSocketInitialized()) {
        f = NewLimeFunction(MEMORY_MONITOR_PACKAGE,
                MEMORY_LISTENER_CLASS,
                "startListening");
        f->call(f, &port);
        DeleteLimeFunction(f);

        if (port != -1) {
            socketConnect(port, server);
            if (isSocketInitialized()) {
                memmonitor_md_startFlushThread();
            } else {
                fprintf(stderr, "Cannot open socket for monitoring events on "
                        "port %i\n", port);
            }
        }
    }
    
    memmonitor_lock();
    bufferInit(heapSize);			
    memmonitor_unlock();
}

/**
 * Transfers any buffered memory monitor "commands" to the server site of the
 * memory monitor (J2SE GUI).
 */  
void
MonitorMemory::memmonitor_flushBuffer() {
    memmonitor_lock();
    flushBuffer();
    memmonitor_unlock();
}

/**
 * This function is called when a method is entered.
 * 
 * @param m method pointer
 */
void
MonitorMemory::memmonitor_enterMethod(Method* m) {
	int threadId = (int)(Thread::current()->id());
	int methodId = getMethodId(m);
    int stackIndex = findReserveCallStack(threadId);

    if (stackIndex != -1) {
       int count = callStackLength[stackIndex];
       if (count == MONITOR_CALLSTACK_LEN) {
            // no room for the call
            memmonitor_lock();
            flushCallStack(stackIndex);
            memmonitor_unlock();
            // reuse the stack
            count = 0;
            callStackThread[stackIndex] = threadId;
		}
        callStack[stackIndex][count++] = methodId;
        callStackLength[stackIndex] = count;
    } else {
       memmonitor_lock();
       bufferEnterMethod(m, threadId);
       memmonitor_unlock();
   }
    
   lastEnterMethodThread = threadId;
}  

/**
 * This function is called when a method is exited.
 * 
 * @param m method instance
 */
void
MonitorMemory::memmonitor_exitMethod(Method* m) {
	int threadId = (int)(Thread::current()->id());
	int methodId = getMethodId(m);
    int stackIndex = findCallStack(threadId);
     if (stackIndex != -1) {
        int last = callStackLength[stackIndex] - 1; // last >= 0
        if (callStack[stackIndex][last] == threadId) {
            // there has been no allocation in that method, remove it from the
            // call stack
            callStackLength[stackIndex] = last;
            if (last == 0) {
                // no method remained in the call stack, unreserve it
                callStackThread[stackIndex] = 0;
            }
        } else {
            memmonitor_lock();
            flushCallStack(stackIndex);
            bufferExitMethod(methodId, threadId);
            memmonitor_unlock();
        }        
    } else {
        memmonitor_lock();
        bufferExitMethod(methodId, threadId);
        memmonitor_unlock();
    }
}

/**
 * This function is called when an exception is thrown.
 */ 
void 
memmonitor_throwException() {
    /* A unique number that identifies the thread */
	int threadId = (int)(Thread::current()->id());
    int stackIndex = findCallStack(threadId);
    
    if (stackIndex != -1) {
        memmonitor_lock();
        flushCallStack(stackIndex);
        memmonitor_unlock();
    }    
}

/**
 * Sends all buffered command to the server site of the memory monitor (J2SE
 * GUI). After sending the buffer is emptied.
 */  
void
MonitorMemory::flushBuffer() {
    if (numCommands == 0) {
        // nothing to send
        memmonitor_flushed = 1;
        return;
    }

    socketSendCommandData(numCommands, sendOffset, sendBuffer);
  
    sendOffset = 0;
    numCommands = 0;
    memmonitor_flushed = 1;
}

/**
 * Stores the INIT command (allocateHeap) into the send buffer. Flushes the
 * buffer before storing. 
 * 
 * @param heapSize the total heap size
 */    
void 
MonitorMemory::bufferInit(int heapSize) {
    // always flush
    flushBuffer();
  
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)INIT);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)heapSize);
    sendOffset += sizeof(u_long);       // heapSize
  
    ++numCommands;
} 

int MonitorMemory::getMethodId(Method* m) {
    UsingFastOops fast_oops;
    Symbol::Fast method_name = m->name();
    InstanceClass::Fast holder = m().holder();
    Symbol::Fast class_name = holder().name();
    return (int)(method_name().hash() + class_name().hash());
}

/**
 * Stores the ENTER_METHOD command (enterMethod) into the send buffer. Flushes 
 * the buffer if necessary. 
 * 
 * @param m method pointer
 * @param threadId thread id
 */    
void 
MonitorMemory::bufferEnterMethod(Method* m, int threadId) {
    UsingFastOops fast_oops;
    Symbol::Fast method_name = m->name();
    InstanceClass::Fast holder = m().holder();
    Symbol::Fast class_name = holder().name();
	int lengthClassName = class_name().length();
	int lengthMetodName = method_name().length();
	int length = lengthClassName + 1 + lengthMetodName; // class.method
	
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // methodId
            sizeof(u_long) +            // nameLength
            length + // methodName
            sizeof(u_long);             // threadId
    if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
        flushBuffer();
    }
  
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)ENTER_METHOD);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)getMethodId(m));
    sendOffset += sizeof(u_long);       // methodId
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)length);
    sendOffset += sizeof(u_long);       // nameLength
    class_name().string_copy(sendBuffer + sendOffset, lengthClassName);
    sendOffset += lengthClassName;      // className
	*(char*)(sendBuffer + sendOffset) = '.';
	sendOffset++;
    method_name().string_copy(sendBuffer + sendOffset, lengthMetodName);
    sendOffset += lengthMetodName;      // methodName
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)threadId);
    sendOffset += sizeof(u_long);       // threadId    
 
    ++numCommands;
}

/**
 * Stores the EXIT_METHOD command (exitMethod) into the send buffer. Flushes 
 * the buffer if necessary. 
 * 
 * @param id method id
 * @param threadId thread id
 */    
void 
MonitorMemory::bufferExitMethod(int id, int threadId) {
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // methodId
            sizeof(u_long);             // threadId 
    if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
        flushBuffer();
    }
  
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)EXIT_METHOD);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)id);
    sendOffset += sizeof(u_long);       // methodId
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)threadId);
    sendOffset += sizeof(u_long);       // threadId
  
    ++numCommands;
}

/**
 * Stores the ALLOCATE_OBJECT command (allocateObject) into the send buffer. 
 * Flushes the buffer if necessary. 
 * 
 * @param obj the object pointer
 */    
static void 
memmonitor_allocateObject(Oop* obj) {

    if (obj == NULL) {
	  return;
	}
	if (!obj->is_instance() && !obj->is_array()) {
	  return;
	}
	JavaClass jc = obj->blueprint();
    u_long classId = (u_long)jc.class_id();
    UsingFastOops fast;
    String::Fast class_name = jc.getStringName();
	TypeArray::Fast cstring_name = class_name().to_cstring(JVM_SINGLE_ARG_NO_CHECK);
	const char *className = (char *) cstring_name().base_address();
	u_long nameLength = (u_long)(class_name().length());
    u_long pointer = (u_long)(obj->obj());
	u_long threadId = (u_long)(Thread::current()->id());
	u_long allocSize = (u_long)(obj->object_size());

    memmonitor_lock();
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // pointer
            sizeof(u_long) +            // classId
            sizeof(u_long) +            // nameLength
            nameLength +                // className
            sizeof(u_long) +            // allocSize
            sizeof(u_long);             // threadId
    if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
        flushBuffer();
    }
 
    *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)ALLOCATE_OBJECT);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = htonl_m(pointer);
    sendOffset += sizeof(u_long);       // pointer
    *(u_long*)(sendBuffer + sendOffset) = htonl_m(classId);
    sendOffset += sizeof(u_long);       // classId
    *(u_long*)(sendBuffer + sendOffset) = htonl_m(nameLength);
    sendOffset += sizeof(u_long);       // nameLength
    memcpy(sendBuffer + sendOffset, className, nameLength);
    sendOffset += nameLength;           // className
    *(u_long*)(sendBuffer + sendOffset) = htonl_m(allocSize);
    sendOffset += sizeof(u_long);       // allocSize    
    *(u_long*)(sendBuffer + sendOffset) = htonl_m(threadId);
    sendOffset += sizeof(u_long);       // threadId
 
    ++numCommands;
    memmonitor_unlock();
}

/**
 * Stores the FREE_OBJECT command (freeObject) into the send buffer. Flushes 
 * the buffer if necessary. 
 * 
 * @param obj the object pointer
 */    
static void 
memmonitor_freeObject(Oop* obj) {

    if (obj == NULL) {
	  return;
	}
	if (!obj->is_instance() && !obj->is_array()) {
	  return;
	}
    u_long pointer = (u_long)(obj->obj());
	u_long allocSize = (u_long)(obj->object_size());
	JavaClass jc = obj->blueprint();
    u_long classId = (u_long)jc.class_id();
    memmonitor_lock();
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // pointer
            sizeof(u_long) +            // classId
            sizeof(u_long);             // allocSize
  if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
    flushBuffer();
  }
 
  *(u_long*)(sendBuffer + sendOffset) = htonl_m((u_long)FREE_OBJECT);
  sendOffset += sizeof(u_long);         // command  
  *(u_long*)(sendBuffer + sendOffset) = htonl_m(pointer);
  sendOffset += sizeof(u_long);         // pointer
  *(u_long*)(sendBuffer + sendOffset) = htonl_m(classId);
  sendOffset += sizeof(u_long);         // classId
  *(u_long*)(sendBuffer + sendOffset) = htonl_m(allocSize);
  sendOffset += sizeof(u_long);         // allocSize
 
  ++numCommands;
  memmonitor_unlock();
}

/**
 * Returns the index of the corresponding call stack for the given thread id or
 * -1 if no such call stack exists.
 * 
 * @param threadId the thread id 
 * @return index of the call stack for the given threadId or -1 if no such call
 *      stack exists
 */  
int
MonitorMemory::findCallStack(int threadId) {
    int i;

    if (threadId == 0) {
        return -1;
    }

    for (i = 0; i < MONITOR_CALLSTACK_CNT; ++i) {
        if (callStackThread[i] == threadId) {
            return i;
        }
    }
    
    return -1;
}

/**
 * Returns the index of the corresponding call stack for the given thread id. If
 * no call stack can be found and there is some call stack unassigned, it 
 * assigns it to the thread and returns its index. If all call stacks have been
 * assigned the return value is -1.  
 * 
 * @param threadId the thread id 
 * @return index of the call stack found or reserved for the given threadId or 
 *      -1 if no such call stack can be found or reserved
 */
int
MonitorMemory::findReserveCallStack(int threadId) {
    int i;
    int firstFree = -1;
    
    if (threadId == 0) {
        return -1;
    }
    
    for (i = 0; i < MONITOR_CALLSTACK_CNT; ++i) {
        if (callStackThread[i] == threadId) {
            return i;
        }
        if ((firstFree == -1) && (callStackThread[i] == 0)) {
            firstFree = i;
        }
    }
    
    if (firstFree != -1) {
        callStackLength[firstFree] = 0;
        callStackThread[firstFree] = threadId;
    }
    
    return firstFree;
}

/**
 * For each method from the call stack of the given index this function stores
 * ENTER_METHOD command in the send buffer and then empties the call stack and
 * removes its association to the thread.
 * 
 * @param stackIndex the stack index
 */     
void
MonitorMemory::flushCallStack(int stackIndex) {
    int i;
    int count = callStackLength[stackIndex];
    long threadId = callStackThread[stackIndex];

    for (i = 0; i < count; ++i) {
        bufferEnterMethod(callStack[stackIndex][i], threadId);
    }
    
    callStackLength[stackIndex] = 0;
    callStackThread[stackIndex] = 0;
}

#endif //ENABLE_MEMORY_MONITOR
