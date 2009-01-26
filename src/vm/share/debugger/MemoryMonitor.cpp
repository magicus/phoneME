/*
 *   
 *
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

# include "incls/_precompiled.incl"
# include "incls/_MemoryMonitor.cpp.incl"

#if ENABLE_MEMORY_MONITOR
volatile int MemoryMonitor::is_flushed = 0;
int MemoryMonitor::sendOffset = 0;
int MemoryMonitor::numCommands = 0;
char MemoryMonitor::sendBuffer[];

/**
 * This function initializes the memory monitor. After the initialization, the
 * other functions can be called.
 */  
void 
MemoryMonitor::startup() {
    // do the platform depended initialization
    MemoryMonitorMd::startup();
}

/**
 * This function frees resources which are necessary for the memory monitor to
 * work properly.
 */  
void 
MemoryMonitor::shutdown() {
    if (isSocketInitialized()) {
        // stop the memory monitor buffer flushing thread
        MemoryMonitorMd::stopFlushThread();

        MemoryMonitorMd::lock();
        flushBufferInt();
        MemoryMonitorMd::unlock();

        socketDisconnect(); 
    }

    // do the platform depended deinitialization
    MemoryMonitorMd::shutdown();
}

void MemoryMonitor::notify_heap_created  ( const void* start, const int size ) {
    (void)start;
    startup();
    LimeFunction *f;
    int port;
    if (!isSocketInitialized()) {
        f = NewLimeFunction(MEMORY_MONITOR_PACKAGE,
                MEMORY_LISTENER_CLASS,
                "startListening");
        f->call(f, &port);
        DeleteLimeFunction(f);

        if (port != -1) {
            socketConnect(port, "127.0.0.1");
            if (isSocketInitialized()) {
                MemoryMonitorMd::startFlushThread();
            } else {
                fprintf(stderr, "Cannot open socket for monitoring events on "
                        "port %i\n", port);
            }
        }
    }
    // always flush
    flushBufferInt();
    MemoryMonitorMd::lock();
  
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)INIT);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)size);
    sendOffset += sizeof(u_long);       // heapSize
    ++numCommands;

    MemoryMonitorMd::unlock();
}

void MemoryMonitor::notify_heap_disposed () {
    shutdown();
}

/**
 * Transfers any buffered memory monitor "commands" to the server site of the
 * memory monitor (J2SE GUI).
 */  
void
MemoryMonitor::flushBuffer() {
    if (!is_flushed) {
        MemoryMonitorMd::lock();
        flushBufferInt();
		is_flushed = 1;
        MemoryMonitorMd::unlock();
	}
}

/**
 * Sends all buffered command to the server site of the memory monitor (J2SE
 * GUI). After sending the buffer is emptied.
 */  
void
MemoryMonitor::flushBufferInt() {
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

void
MemoryMonitor::setFlushed(int isFlushed) {
    is_flushed = isFlushed;
}

void MemoryMonitor::notify_object( const OopDesc* obj, const int sizeObject,
                                   const bool create ) {

  Oop::Raw o( (OopDesc*) obj );
  
  FarClassDesc* blueprint = obj->blueprint();
  const int instance_size = blueprint->instance_size_as_jint();
  char type_name[MAX_LEN_CLASS_NAME];
  int classId = UNKNOWN_ID;
  int threadId = 0;
  int length = 0;
  
  #define CASE(kind) case InstanceSize::size_##kind:
  switch( instance_size ) {
    default:                  // Java object
      if( blueprint == Universe::string_class()->obj() ) {
        if (create) {
	      strcpy(type_name, "String");
		}
		classId = STRING_ID;
      } else if( blueprint == Universe::java_lang_Class_class()->obj() ) {
        if (create) {
          strcpy(type_name, "Java class");
        }
		classId = JAVA_LANG_ID;
      } else {
        JavaClass::Raw klass( blueprint );
		classId = (int)klass().class_id();
        if (create) {
          ClassInfo::Raw info = klass().class_info();

          Symbol::Raw name = info().name();
          if( name.equals( Symbols::unknown() ) ) {
            name = ROM::get_original_class_name( &info );
          }
          length = name().length();
		  if (length > MAX_LEN_CLASS_NAME - 1) {
		    length = MAX_LEN_CLASS_NAME - 1;
          }
		  memcpy(type_name, name().base_address(), length);
		  type_name[length] = 0;
        }
      }
      break;
    CASE( obj_array         ) // an array of Java objects
      if (create) {
        strcpy(type_name, "Array of Java objects");
      }
      classId = ARRAY_JAVA_OBJ_ID;
      break;
    CASE( type_array_1      ) // boolean[], byte[]
      if (create) {
        strcpy(type_name, "Arrays of bytes/booleans");
      }
      classId = ARRAY_BYTE_BOOL_ID;
      break;
    CASE( type_array_2      ) // short[], char[]
      if (create) {
        strcpy(type_name, "Arrays of chars/shorts");
      }
      classId = ARRAY_CHAR_SHORT_ID;
      break;
    CASE( type_array_4      ) // int[], float[]
      if (create) {
        strcpy(type_name, "Arrays of ints/floats");
      }
      classId = ARRAY_INT_FLOAT_ID;
      break;
    CASE( type_array_8      ) // long[], double[]    
      if (create) {
        strcpy(type_name, "Arrays of longs/doubles");
      }
      classId = ARRAY_LONG_DOUBLE_ID;
      break;
    CASE( symbol            ) // a symbol
      if (create) {
        strcpy(type_name, "Symbol");
      }
      classId = SYMBOL_ID;
      break;

    CASE( class_info        ) // a class info
    CASE( execution_stack   ) // an execution stack
    CASE( task_mirror       ) // an isolate task mirror
    CASE( instance_class    ) // an instance class
    CASE( obj_array_class   ) // an object array class
    CASE( type_array_class  ) // a typed array class 
    CASE( generic_near      ) // a simplest near
    CASE( java_near         ) // a near for a Java object
    CASE( obj_near          ) // a near for a method
    CASE( far_class         ) // a simple far class
    CASE( mixed_oop         ) // a MixedOop
    CASE( boundary          ) // a Boundary
    CASE( entry_activation  ) // an entry activation
    CASE( method            ) // a method
    CASE( constant_pool     ) // a constant pool
    CASE( compiled_method   ) // compiled method
    CASE( stackmap_list     ) // a stackmap list
    CASE( refnode           ) // debugger mapping from object ID to object
      // ignore
	  return;
  }
  #undef CASE
  if (create) {

    if (length == 0) {
      length = strlen(type_name);
	}

    MemoryMonitorMd::lock();
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // pointer
            sizeof(u_long) +            // classId
            sizeof(u_long) +            // nameLength
            length         +            // className
            sizeof(u_long) +            // allocSize
            sizeof(u_long);             // threadId
    if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
        flushBufferInt();
    }
 
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)ALLOCATE_OBJECT);
    sendOffset += sizeof(u_long);       // command  
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)0);
    sendOffset += sizeof(u_long);       // pointer
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)classId);
    sendOffset += sizeof(u_long);       // classId
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)length);
    sendOffset += sizeof(u_long);       // nameLength
    memcpy(sendBuffer + sendOffset, type_name, length);
    sendOffset += length;           // className
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)sizeObject);
    sendOffset += sizeof(u_long);       // allocSize    
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)threadId);
    sendOffset += sizeof(u_long);       // threadId
 
    ++numCommands;
    MemoryMonitorMd::unlock();
    
  } else { // free object

    MemoryMonitorMd::lock();
    int size = sizeof(u_long) +         // command
            sizeof(u_long) +            // pointer
            sizeof(u_long) +            // classId
            sizeof(u_long);             // allocSize
    if ((sendOffset + size) > MONITOR_BUFFER_SIZE) {
      flushBufferInt();
    }
 
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)FREE_OBJECT);
    sendOffset += sizeof(u_long);         // command  
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m((u_long)0);
    sendOffset += sizeof(u_long);         // pointer
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m(classId);
    sendOffset += sizeof(u_long);         // classId
    *(u_long*)(sendBuffer + sendOffset) = MemoryMonitorMd::htonl_m(sizeObject);
    sendOffset += sizeof(u_long);         // allocSize
 
    ++numCommands;
    MemoryMonitorMd::unlock();
  }

}
#endif
