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

#if ENABLE_PROFILER

/** \class Profiler
     This profiler collects statistical execution time information.
     At timer interrupts the active thread is stopped and information is 
     gathered. Calling print causes the collected information to be printed.
*/
class Profiler : public AllStatic {
 public:
   /// Initialize resources needed by the profiler
   static void initialize();

   /// Activate the profiler
   static void engage();

   /// Deactivate the profiler - it may be activated again
   static void disengage();

   /// Free all resources needed by the profiler
   static void dispose();

   /// Dumps all profile data related with specified task to stream
   static void dump_and_clear_profile_data(int id);

   /// Called when a receiving a timer interrupt
   static void tick(int delay_time_is_ms);

   /// Check if the profiler is ready to update records
   static bool is_ready();

   /// Prints the collected profiling information
   static void print(Stream* out, int id);

   /// Garbage collection support
   static void oops_do(void do_oop(OopDesc**));

   // Profile the current method
   static void profile_method(Method* method, bool is_compiled);

   /// Gets default stream for profile data output
   static Stream* get_default_output_stream();
};

#endif
