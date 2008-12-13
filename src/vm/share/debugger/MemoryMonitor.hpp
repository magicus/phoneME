/*
 *   
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

class MemoryMonitor: AllStatic {
  public:
   static void notify_object( const OopDesc* obj, const int sizeObject,
                              const bool create );
   static void notify_object_moved  ( const void* dst, const void* src ) {}
   static void notify_heap_created  ( const void* start, const int size );
   static void notify_heap_disposed ( void );
   static void flushBuffer(void);
   static void setFlushed(int);

  private:

   /** The notification command id for allocateHeap. */
   static const int INIT            = 0;
   /** The notification command id for allocateObject. */
   static const int ALLOCATE_OBJECT = 1;
   /** The notification command id for freeObject. */
   static const int FREE_OBJECT     = 2;

   static const int MAX_LEN_CLASS_NAME = 256;

   /** Non-java class IDs. */
   static const int UNKNOWN_ID     = -1;
   static const int STRING_ID      = -2;
   static const int JAVA_LANG_ID   = -3;
   static const int SYMBOL_ID      = -4;
   static const int ARRAY_JAVA_OBJ_ID      = -5;
   static const int ARRAY_BYTE_BOOL_ID     = -6;
   static const int ARRAY_CHAR_SHORT_ID    = -7;
   static const int ARRAY_INT_FLOAT_ID     = -8;
   static const int ARRAY_LONG_DOUBLE_ID   = -9;

   /**
    * A flag which indicates if the memory monitor bufer has been flushed from the
    * last time this flag was cleared. It's used in the memory monitor flushing 
    * thread (see memMonitor_md.c).
    */   
   static volatile int is_flushed;
   
   /**
    * The offset to the send buffer, where the next command will be placed.
    */ 
   static int sendOffset;
   
   /**
    * The number of commands stored in the send buffer.
    */ 
   static int numCommands;

   /**
    * The buffer in which notifications (commands) from the VM are stored and
    * which is sent to the server site of the memory monitor when it gets full or
    * when some predefined time elapses.
    */  
   static char sendBuffer[MONITOR_BUFFER_SIZE];
   static void flushBufferInt();
   static void bufferFreeObject(int pointer, int classId, int allocSize);
   static void startup(void);
   static void shutdown(void);
};
#endif
