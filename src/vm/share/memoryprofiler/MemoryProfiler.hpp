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

#if ENABLE_MEMORY_PROFILER
#define STD_MEMORY_PROFILER_PORT 3000

class MemoryProfiler {
  public:    
    static void *memory_profiler_cmds[];
    // to satisfy build system
    void dummy_method();

  private:
    static OopDesc** _current_object;
    static PacketOutputStream* _current_out;

    //these varaibles are used for thread enumeration 
    //and link offset calculation
    static OopDesc* _current_stack;
    static int _stack_count;

    //interface functions
    static void get_global_data(PacketInputStream *in, PacketOutputStream *out);
    static void retrieve_all_data(PacketInputStream *in, PacketOutputStream *out);
    static void get_all_classes(PacketInputStream *in, PacketOutputStream *out);
    static void get_roots(PacketInputStream *in, PacketOutputStream *out);
    static void suspend_vm(PacketInputStream *in, PacketOutputStream *out);
    static void resume_vm(PacketInputStream *in, PacketOutputStream *out);
    static void get_stack_trace(PacketInputStream *in, PacketOutputStream *out);
    static void print_stack_trace(PacketOutputStream *out, OopDesc* backtrace);
    static ReturnOop create_stack_trace(Frame frame);
    static void strace_ptr_address_searcher(OopDesc**);
    //heap dump functions 
    static int get_mp_class_id(JavaClass* clazz);
    static void dump_object(Oop*);
    static void link_counter(OopDesc**);
    static void link_dumper(OopDesc**);
    static void stack_link_dumper(OopDesc**);
    static int link_count;

};
#endif
