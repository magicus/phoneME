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

/** \class ObjectReferenceImpl
    Handles commands to read/write instance variables as well as determining the
    underlying class reference type of objects.
*/
#if ENABLE_JAVA_DEBUGGER

class ObjectReferenceImpl {
public:
  static void *object_reference_cmds[]; 
  static void read_value_to_address(PacketInputStream *, Oop *, jint,
                                    jbyte, jboolean);
  static void write_value_from_address(PacketOutputStream *, Oop *,
                                       jint, jbyte, bool, bool);
#ifdef AZZERT
  static void print_value_from_address(Oop *p, jint field_id, jbyte tag ); 
#endif
  static void common_invoke_method(COMMAND_ARGS, bool);
  static jlong invoke_return(Oop *o, Oop *exc, Oop *t, int id, int options,
                             int return_type, OopDesc **ret_val);
private:
  static void object_reference_reference_type(COMMAND_ARGS);
  static void object_reference_get_values(COMMAND_ARGS);
  static void object_reference_set_values(COMMAND_ARGS);

  static void object_field_getter_setter(COMMAND_ARGS, bool);
  static void object_reference_is_collected(COMMAND_ARGS);
  static void object_reference_invoke_method(COMMAND_ARGS);
};

/** \class ClassObjectReferenceImpl
    Returns the class mirrored by this instance of java.lang.Class.

 */
class ClassObjectReferenceImpl {
public:
  static void *class_object_reference_cmds[];
  // fix warning in linux arm build
  static void dummy();
private:
  static void class_object_reflected_type(COMMAND_ARGS);
};

/** \class StringReferenceImpl
    Returns the value of a string object.

 */
class StringReferenceImpl {
public:
  static void *string_reference_cmds[];
  // fix warning in linux arm build
  static void dummy();
private:
  static void string_reference_value(COMMAND_ARGS);
};

#endif
