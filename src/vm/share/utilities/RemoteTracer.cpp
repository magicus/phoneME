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

# include "incls/_precompiled.incl"
# include "incls/_RemoteTracer.cpp.incl"

#if ENABLE_REMOTE_TRACER

bool RemoteTracer::_socket = 0;
char *traceHost = "localhost";

extern "C" {
extern int open_socket(char* hostname, int port, int mode, jboolean nonblock);
extern int socket_send(int sock_fd, char *buf, int size, int offset);
};
  
struct rt_stats {
  int heap_size;
  int used_memory;
  int free_memory;
  int code_size;
  int compiled_method_count;
  
  int class_size;
  int method_size;
  int constant_pool_size;
  int instance_size;
  int type_array_size;
  int object_array_size;
  int symbol_size;
  int entry_activation_size;
  int execution_stack_size;
  int jvm_thread_size;
  int classparser_state_size;
};

static struct rt_stats stats;
static struct rt_stats tmp_stats;

void RemoteTracer::initialize() {
  _socket = open_socket(traceHost, RemoteTracePort, 0, 0);
  if (_socket < 0) {
    tty->print_cr("*** could not connect to tracer");
    JVM::exit(-1);
  }

  tty->print_cr("connected to tracer");

  stats.heap_size              = 0;
  stats.used_memory            = 0;
  stats.free_memory            = 0;
  stats.code_size              = 0;
  stats.compiled_method_count  = 0;
  stats.class_size             = 0;
  stats.method_size            = 0;
  stats.constant_pool_size     = 0;
  stats.instance_size          = 0;
  stats.type_array_size        = 0;
  stats.object_array_size      = 0;
  stats.symbol_size            = 0;
  stats.entry_activation_size  = 0;
  stats.execution_stack_size   = 0;
  stats.jvm_thread_size        = 0;
  stats.classparser_state_size = 0;
}

void RemoteTracer::set_heap_size(int value) {
  stats.heap_size = value;
}

void RemoteTracer::set_used_memory(int value) {
  stats.used_memory = value;
}

void RemoteTracer::set_free_memory(int value) {
  stats.free_memory = value;
}

void RemoteTracer::set_compiled_code_size(int value) {
  stats.code_size = value;
}

void RemoteTracer::set_compiled_method_count(int value) {
  stats.compiled_method_count = value;
}

void RemoteTracer::set_class_size(int value) {
  stats.class_size = value;
}

void RemoteTracer::set_method_size(int value) {
  stats.method_size = value;
}

void RemoteTracer::set_constant_pool_size(int value) {
  stats.constant_pool_size = value;
}

void RemoteTracer::set_instance_size(int value) {
  stats.instance_size = value;
}

void RemoteTracer::set_type_array_size(int value) {
  stats.type_array_size = value;
}

void RemoteTracer::set_object_array_size(int value) {
  stats.object_array_size = value;
}

void RemoteTracer::set_symbol_size(int value) {
  stats.symbol_size = value;
}

void RemoteTracer::set_entry_activation_size(int value) {
  stats.entry_activation_size = value;
}

void RemoteTracer::set_execution_stack_size(int value) {
  stats.execution_stack_size = value;
}

void RemoteTracer::set_jvm_thread_size(int value) {
  stats.jvm_thread_size = value;
}

void RemoteTracer::set_classparser_state_size(int value) {
  stats.classparser_state_size = value;
}

bool RemoteTracer::tick() {
  static int count = 0;

  if (count > 0) {
    count--;
    return false;
  } else {
    count = 50;
    return true;
  }
}

void RemoteTracer::update_stats(OopDesc* obj) {
  int object_size = obj->object_size();

  if (obj->is_compiled_method()) {
    tmp_stats.code_size += object_size;
    tmp_stats.compiled_method_count++;
  }

  if (obj->is_method()) {
    tmp_stats.method_size += object_size;
  }

  if (obj->is_constant_pool()) {
    tmp_stats.constant_pool_size += object_size;
  }

  if (obj->is_instance()) {
    tmp_stats.instance_size += object_size;
  }

  if (obj->is_instance_class()) {
    tmp_stats.class_size += object_size;
  }

  if (obj->is_type_array()) {
    tmp_stats.type_array_size += object_size;
  }

  if (obj->is_obj_array()) {
    tmp_stats.object_array_size += object_size;
  }

  if (obj->is_symbol()) {
    tmp_stats.symbol_size += object_size;
  }

  if (obj->is_entry_activation()) {
    tmp_stats.entry_activation_size += object_size;
  }

  if (obj->is_execution_stack()) {
    tmp_stats.execution_stack_size += object_size;
  }

  if (obj->is_jvm_thread()) {
    tmp_stats.jvm_thread_size += object_size;
  }

  if (obj->is_class_parser_state()) {
    tmp_stats.classparser_state_size += object_size;
  }
}

void RemoteTracer::update_stats_from_heap(OopDesc** heap_start, OopDesc** collection_area_start) {
  jvm_memset(&tmp_stats, 0, sizeof(tmp_stats));
  
  OopDesc** p = heap_start;
  while (p < collection_area_start) {
    OopDesc* obj = (OopDesc*) p;
    update_stats(obj);
    p += obj->object_size() / BytesPerWord;
  }
}

void RemoteTracer::freeze_stats() {
#if ENABLE_COMPILER
  CompiledMethodDesc::total_code = tmp_stats.code_size;
  CompiledMethodDesc::total_items = tmp_stats.compiled_method_count;
#endif

  jvm_memmove(&stats, &tmp_stats, sizeof(stats));
}

void RemoteTracer::send_snapshot() {
  if (_socket < 0) return;
  socket_send(_socket, (char *)&stats, sizeof(stats), 0);
}

#endif // ENABLE_REMOTE_TRACER
