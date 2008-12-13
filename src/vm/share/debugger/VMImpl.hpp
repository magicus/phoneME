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

/** \class VMImpl
    Handles commands from the debugger to control the VM as well as returning
    all the classes and all the threads active in the VM.
*/

#if ENABLE_JAVA_DEBUGGER

class VMImpl {
public:
  static void *virtualmachine_cmds[];
  // fix warning in linux arm build
  static void dummy();
private:
  static void virtualmachine_all_classes(COMMAND_ARGS);
  static void virtualmachine_classes_by_signature(COMMAND_ARGS);
  static void virtualmachine_all_threads(COMMAND_ARGS);
  static void virtualmachine_suspend(COMMAND_ARGS);
  static void virtualmachine_resume(COMMAND_ARGS);
  static void virtualmachine_exit(COMMAND_ARGS);
  static void virtualmachine_createstring(COMMAND_ARGS);
  static void virtualmachine_dispose(COMMAND_ARGS);

};
#endif
