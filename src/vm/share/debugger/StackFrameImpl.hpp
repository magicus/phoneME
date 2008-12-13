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

/** \class StackFrameImpl
    Handles commands to read/write local variables on the execution stack

*/
#if ENABLE_JAVA_DEBUGGER

class StackFrameImpl {
public:
  static void *stack_frame_cmds[]; 
  static void dummy();
  static jboolean get_frame(Frame& fr, jint);
private:
  static void stack_frame_get_values(COMMAND_ARGS);
  static void stack_frame_set_values(COMMAND_ARGS);

  static void stack_frame_getter_setter(COMMAND_ARGS, bool);
  static bool read_local_to_address(PacketInputStream *, PacketOutputStream *,
                                    JavaFrame *, jint, jbyte);
  static bool write_local_from_address(PacketOutputStream *,
                                       JavaFrame *,
                                       jint, jbyte, jboolean write_tag, 
                                       jboolean bogus_object);
#ifdef AZZERT
  static void print_local_from_address(JavaFrame *, jint, jbyte, jboolean);
#endif
};
#endif
