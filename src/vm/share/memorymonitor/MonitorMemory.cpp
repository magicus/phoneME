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
// #include <windows.h> 

int MonitorMemory::callStackThread[];
int MonitorMemory::callStackLength[];
Method* MonitorMemory::callStack[MONITOR_CALLSTACK_CNT][MONITOR_CALLSTACK_LEN];
volatile int MonitorMemory::is_flushed = 0;
int MonitorMemory::sendOffset = 0;
int MonitorMemory::numCommands = 0;
int MonitorMemory::lastEnterMethodThread = -1;
char MonitorMemory::sendBuffer[];


/**
 * This function initializes the memory monitor. After the initialization, the
 * other functions can be called.
 */  
void 
MonitorMemory::startup() {
    tty->print_cr("MonitorMemory::startup");
    memset(callStackThread, 0, sizeof(callStackThread));
    // do the platform depended initialization
    MonitorMemoryMd::startup();
}

/**
 * This function frees resources which are necessary for the memory monitor to
 * work properly.
 */  
void 
MonitorMemory::shutdown() {
    tty->print_cr("MonitorMemory::shutdown");
    if (isSocketInitialized()) {
        // stop the memory monitor buffer flushing thread
        MonitorMemoryMd::stopFlushThread();

        MonitorMemoryMd::lock();
        flushBufferInt();
        MonitorMemoryMd::unlock();

        socketDisconnect(); 
    }

    // do the platform depended deinitialization
    MonitorMemoryMd::shutdown();
}

/**
 * This function is called when a new heap is allocated. It happens typically 
 * at the beginning of VM execution.
 * 
 * @param heapSize the allocated size
 */       
void
MonitorMemory::allocateHeap(long heapSize) {
    LimeFunction *f;
    int port;
    tty->print_cr("MonitorMemory::allocateHeap");
    if (!isSocketInitialized()) {
        f = NewLimeFunction(MEMORY_MONITOR_PACKAGE,
                MEMORY_LISTENER_CLASS,
                "startListening");
        f->call(f, &port);
        DeleteLimeFunction(f);

        if (port != -1) {
            socketConnect(port, "127.0.0.1");
            if (isSocketInitialized()) {
                MonitorMemoryMd::startFlushThread();
            } else {
                fprintf(stderr, "Cannot open socket for monitoring events on "
                        "port %i\n", port);
            }
        }
    }
    
    MonitorMemoryMd::lock();
    bufferInit(heapSize);			
    MonitorMemoryMd::unlock();
}

/**
 * Transfers any buffered memory monitor "commands" to the server site of the
 * memory monitor (J2SE GUI).
 */  
void
MonitorMemory::flushBuffer() {
    if (!is_flushed) {
        MonitorMemoryMd::lock();
        flushBufferInt();
		is_flushed = 1;
        MonitorMemoryMd::unlock();
	}
}

void
MonitorMemory::setFlushed(int isFlushed) {
    is_flushed = isFlushed;
}

/**
 * This function is called when a method is entered.
 * 
 * @param m method pointer
 */
void
MonitorMemory::enterMethod(Method* m) {
    if (Thread::current()->obj() == NULL) {
	    return;
	}
	int threadId = (int)(Thread::current()->id());
	threadId = 0;
    int stackIndex = findReserveCallStack(threadId);

    if (stackIndex != -1) {
       int count = callStackLength[stackIndex];
       if (count == MONITOR_CALLSTACK_LEN) {
            // no room for the call
            MonitorMemoryMd::lock();
            flushCallStack(stackIndex);
            MonitorMemoryMd::unlock();
            // reuse the stack
            count = 0;
            callStackThread[stackIndex] = threadId;
		}
        callStack[stackIndex][count++] = m;
        callStackLength[stackIndex] = count;
    } else {
       MonitorMemoryMd::lock();
       bufferEnterMethod(m, threadId);
       MonitorMemoryMd::unlock();
   }
    
   lastEnterMethodThread = threadId;
}  

/**
 * This function is called when a method is exited.
 * 
 * @param m method instance
 */
void
MonitorMemory::exitMethod(Method* m) {
    if (Thread::current()->obj() == NULL) {
	    return;
	}
	int threadId = (int)(Thread::current()->id());
	threadId = 0;
	int methodId = getMethodId(m);
    int stackIndex = findCallStack(threadId);
     if (stackIndex != -1) {
        int last = callStackLength[stackIndex] - 1; // last >= 0
        if (getMethodId(callStack[stackIndex][last]) == methodId) {
            // there has been no allocation in that method, remove it from the
            // call stack
            callStackLength[stackIndex] = last;
            if (last == 0) {
                // no method remained in the call stack, unreserve it
                callStackThread[stackIndex] = 0;
            }
        } else {
            MonitorMemoryMd::lock();
            flushCallStack(stackIndex);
            bufferExitMethod(methodId, threadId);
            MonitorMemoryMd::unlock();
        }        
    } else {
        MonitorMemoryMd::lock();
        bufferExitMethod(methodId, threadId);
        MonitorMemoryMd::unlock();
    }
}

/**
 * This function is called when an exception is thrown.
 */ 
void 
MonitorMemory::throwException() {
    if (Thread::current()->obj() == NULL) {
	    return;
	}
    /* A unique number that identifies the thread */
	int threadId = (int)(Thread::current()->id());
	threadId = 0;
    int stackIndex = findCallStack(threadId);
    
    if (stackIndex != -1) {
        MonitorMemoryMd::lock();
        flushCallStack(stackIndex);
        MonitorMemoryMd::unlock();
    }    
}

/**
 * Sends all buffered command to the server site of the memory monitor (J2SE
 * GUI). After sending the buffer is emptied.
 */  
void
MonitorMemory::flushBufferInt() {
    if (numCommands == 0) {
        // nothing to send
        is_flushed = 1;
        return;
    }

    socketSendCommandData(numCommands, sendOffset, sendBuffer);
  
    sendOffset = 0;
    numCommands = 0;
    is_flushed = 1;
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
    flushBufferInt();
  
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)INIT);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)heapSize);
    sendOffset += sizeof(u_long);       // heapSize
  
    ++numCommands;
} 

int MonitorMemory::getMethodId(Method* m) {
    UsingFastOops fast_oops;
    Symbol::Fast method_name = m->name();
    InstanceClass::Fast holder = m->holder();
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
    InstanceClass::Fast holder = m->holder();
    Symbol::Fast class_name = holder().name();
	int lengthClassName = class_name().length();
	int lengthMetodName = method_name().length();
	int length = lengthClassName + 1 + lengthMetodName; // class.method
    tty->print_cr("MonitorMemory::bufferEnterMethod methodId = %d threadId = %d", getMethodId(m), threadId);
	
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // methodId
            sizeof(u_long) +            // nameLength
            length + // methodName
            sizeof(u_long);             // threadId
    if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
        flushBufferInt();
    }
  
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)ENTER_METHOD);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)getMethodId(m));
    sendOffset += sizeof(u_long);       // methodId
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)length);
    sendOffset += sizeof(u_long);       // nameLength
    class_name().string_copy(sendBuffer + sendOffset, lengthClassName);
    sendOffset += lengthClassName;      // className
	*(char*)(sendBuffer + sendOffset) = '.';
	sendOffset++;
    method_name().string_copy(sendBuffer + sendOffset, lengthMetodName);
    sendOffset += lengthMetodName;      // methodName
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)threadId);
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
MonitorMemory::bufferExitMethod(juint id, int threadId) {
    tty->print_cr("MonitorMemory::bufferExitMethod methodId = %d threadId = %d", id, threadId);
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // methodId
            sizeof(u_long);             // threadId 
    if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
        flushBufferInt();
    }
  
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)EXIT_METHOD);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)id);
    sendOffset += sizeof(u_long);       // methodId
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)threadId);
    sendOffset += sizeof(u_long);       // threadId
  
    ++numCommands;
}

/**
 * Stores the ALLOCATE_OBJECT command (allocateObject) into the send buffer. 
 * Flushes the buffer if necessary. 
 * 
 * @param obj the object pointer
 */    
void 
MonitorMemory::allocateObject(Oop* referent JVM_TRAPS) {

	if (!referent->obj()->is_instance() && !referent->obj()->is_array()) {
	  return;
	}
    if (Thread::current()->obj() == NULL) {
	    return;
	}
	JavaClass jc = referent->obj()->blueprint();
    u_long classId = (u_long)jc.class_id();
    UsingFastOops fast;
    String::Fast class_name = jc.getStringName(JVM_SINGLE_ARG_CHECK);
	TypeArray::Fast cstring_name = class_name().to_cstring(JVM_SINGLE_ARG_NO_CHECK);
	char *className = (char *) cstring_name().base_address();
	u_long threadId = (u_long)(Thread::current()->id());
    threadId = 0;
	u_long nameLength = (u_long)(class_name().length());
	if (!nameLength) {
	    return;
	}
	className[nameLength] = (char)0;
    u_long pointer = (u_long)(referent->obj());
	u_long allocSize = (u_long)(referent->obj()->object_size());
    tty->print_cr("MonitorMemory::allocateObject classId = %d threadId = %d className = %s", classId, threadId, className);

    MonitorMemoryMd::lock();
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // pointer
            sizeof(u_long) +            // classId
            sizeof(u_long) +            // nameLength
            nameLength +                // className
            sizeof(u_long) +            // allocSize
            sizeof(u_long);             // threadId
    if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
        flushBufferInt();
    }
 
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)ALLOCATE_OBJECT);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m(pointer);
    sendOffset += sizeof(u_long);       // pointer
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m(classId);
    sendOffset += sizeof(u_long);       // classId
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m(nameLength);
    sendOffset += sizeof(u_long);       // nameLength
    memcpy(sendBuffer + sendOffset, className, nameLength);
    sendOffset += nameLength;           // className
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m(allocSize);
    sendOffset += sizeof(u_long);       // allocSize    
    *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m(threadId);
    sendOffset += sizeof(u_long);       // threadId
 
    ++numCommands;
    MonitorMemoryMd::unlock();
    // ::Sleep(2000);
}

/**
 * Stores the FREE_OBJECT command (freeObject) into the send buffer. Flushes 
 * the buffer if necessary. 
 * 
 * @param obj the object pointer
 */    
void 
MonitorMemory::freeObject(Oop* referent) {

	if (!referent->obj()->is_instance() && !referent->obj()->is_array()) {
	  return;
	}
    u_long pointer = (u_long)(referent->obj());
	u_long allocSize = (u_long)(referent->obj()->object_size());
	JavaClass jc = referent->obj()->blueprint();
    u_long classId = (u_long)jc.class_id();
    tty->print_cr("MonitorMemory::freeObject classId = %d", classId);
    MonitorMemoryMd::lock();
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // pointer
            sizeof(u_long) +            // classId
            sizeof(u_long);             // allocSize
  if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
    flushBufferInt();
  }
 
  *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m((u_long)FREE_OBJECT);
  sendOffset += sizeof(u_long);         // command  
  *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m(pointer);
  sendOffset += sizeof(u_long);         // pointer
  *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m(classId);
  sendOffset += sizeof(u_long);         // classId
  *(u_long*)(sendBuffer + sendOffset) = MonitorMemoryMd::htonl_m(allocSize);
  sendOffset += sizeof(u_long);         // allocSize
 
  ++numCommands;
  MonitorMemoryMd::unlock();
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
