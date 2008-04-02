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

/** \class ReferenceImpl
    Handles commands from the debugger to read class variables 

*/
#if ENABLE_JAVA_DEBUGGER

class ReferenceTypeImpl {
public:
  static void *reference_type_cmds[]; 
private:
  static void reference_type_signature(COMMAND_ARGS);
  static void reference_type_get_values(COMMAND_ARGS);
  static void static_field_getter_setter(COMMAND_ARGS, bool);

  friend class ClassTypeImpl;
};

/** \class ClassTypeImpl
    Handles commands from the debugger to write class variables, why it's not
    in ReferenceTypeImpl I don't know.  Also handles superclass command.

*/

class ClassTypeImpl {
public:
  static void *class_type_cmds[];
  // fix compiler warning on linux arm build
  static void dummy();
private:
  static void class_type_super_class(COMMAND_ARGS);
  static void class_type_set_values(COMMAND_ARGS);
  static void class_type_invoke_method(COMMAND_ARGS);

};

#endif
