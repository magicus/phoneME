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

#if ENABLE_REMOTE_TRACER

/**
 * Provide statistical data about heap object distributions
 * over a socket to a remote observation and display tool.
 *
 * See 'src/tools/remotetracer/REAME.html' for the latter.
 */
class RemoteTracer: public AllStatic {
 public:
  static void initialize();
  static void set_heap_size(int value);
  static void set_used_memory(int value);
  static void set_free_memory(int value);
  static void set_compiled_code_size(int value);
  static void set_compiled_method_count(int value);
  static void set_class_size(int value);
  static void set_method_size(int value);
  static void set_constant_pool_size(int value);
  static void set_instance_size(int value);
  static void set_type_array_size(int value);
  static void set_object_array_size(int value);
  static void set_symbol_size(int value);
  static void set_entry_activation_size(int value);
  static void set_execution_stack_size(int value);
  static void set_jvm_thread_size(int value);
  static void set_classparser_state_size(int value);
  static bool tick();
  static void update_stats(OopDesc* obj);
  static void update_stats_from_heap(OopDesc** heap_start, OopDesc** collection_area_start);
  static void freeze_stats();
  static void send_snapshot();
 private:
  static int _socket;
};

extern char* traceHost;

#endif
