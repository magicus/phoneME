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

#if ENABLE_MEMORY_MONITOR

#define MEMORY_MONITOR_PACKAGE "com.sun.kvem.memorymon"
#define MEMORY_LISTENER_CLASS "MemoryListener"

/** The size of the memory monitor send buffer. */
#define MONITOR_BUFFER_SIZE 65536
/** The number of calls which can be stored in one call stack. */
#define MONITOR_CALLSTACK_LEN 256
/** The number of call stacks. */
#define MONITOR_CALLSTACK_CNT 8

class MonitorMemory {
public:
    static void allocateHeap(long heapSize);
    static void flushBuffer(void);
    static void startup(void);
    static void shutdown(void);
    static void enterMethod(Method* m);
    static void exitMethod(Method* m);
    static void throwException(void);
    static void allocateObject(Oop* obj);
    static void freeObject(Oop* obj);

private:
/**
 * The client site (this file) of the memory monitor doesn't sent to the server
 * site the enter method and exit method notifications unless some allocation
 * happens between the enter method and the corresponding exit method.
 * 
 * For this the following three arrays are used.
 */
 
/**
 * This array holds thread id for each call stack which is currently used (it
 * has at least one enter method stored).
 */ 
static int callStackThread[MONITOR_CALLSTACK_CNT];
/**
 * This array holds the number of stored enter method notifications for each
 * call stack.
 */  
static int callStackLength[MONITOR_CALLSTACK_CNT];
/**
 * This array holds the methods for which the enter method has been called and
 * no allocate object notification has been received. It's two dimensional 
 * array, one dimension representing different call stacks (belonging to 
 * different threads) and the other methods for the given thread.
 */  
static juint callStack[MONITOR_CALLSTACK_CNT][MONITOR_CALLSTACK_LEN]; 

/** The notification command id for allocateHeap. */
static const int INIT            = 0;
/** The notification command id for allocateObject. */
static const int ALLOCATE_OBJECT = 1;
/** The notification command id for freeObject. */
static const int FREE_OBJECT     = 2;
/** The notification command id for enterMethod. */
static const int ENTER_METHOD    = 3;
/** The notification command id for exitMethod. */
static const int EXIT_METHOD     = 4;

/**
 * The buffer in which notifications (commands) from the VM are stored and
 * which is sent to the server site of the memory monitor when it gets full or
 * when some predefined time elapses.
 */  
static char sendBuffer[MONITOR_BUFFER_SIZE];

static void flushBuffer();
static void flushBufferInt();
static void bufferInit(int heapSize);
static void bufferEnterMethod(Method* m, int threadId);
static void bufferExitMethod(juint id, int threadId);
static void bufferFreeObject(int pointer, int classId, int allocSize);

static int findCallStack(int threadId);
static int findReserveCallStack(int threadId);
static void flushCallStack(int stackIndex);
static int getMethodId(Method* m);

};


#endif
