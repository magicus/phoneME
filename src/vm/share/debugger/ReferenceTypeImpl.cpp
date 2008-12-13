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
# include "incls/_ReferenceTypeImpl.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

// Get static variables
 
void ReferenceTypeImpl::static_field_getter_setter(PacketInputStream *in, 
                         PacketOutputStream *out,
                         bool is_setter)
{
  UsingFastOops fast_oops;

  InstanceClass::Fast clazz = in->read_class_ref(); 
  jint num = in->read_int();
  int i;

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Static %s: class=0x%lx, count=%ld",
                  (is_setter ? "Set" : "Get"), 
                  clazz.obj(), num);
  }
#endif

  // Prevent compiler warnings
  clazz = clazz;

  // Create a buffered output stream so we can asynchronously send an error
  // Calculate the size based on half of the items being 'longs'
  if (!is_setter) { 
    out->write_int(num);
  }

  for (i = 0; i < num; i++) {
    UsingFastOops fast_oops_2;

    InstanceClass::Fast field_clazz = in->read_class_ref();
    TypeArray::Fast fields;
    jint field_id = in->read_int();
    char type_tag;

    if (field_clazz.is_null()) {
      out->send_error(JDWP_Error_INVALID_FIELDID);
      return;
    }
    fields = field_clazz().fields();
    if (fields.is_null()) {
      out->send_error(JDWP_Error_INVALID_FIELDID);
      return;
    }
    if (field_id >= fields().length() / Field::NUMBER_OF_SLOTS) {
      out->send_error(JDWP_Error_INVALID_FIELDID);
      return;
    }
    Field field(&field_clazz, field_id * Field::NUMBER_OF_SLOTS);
    if (!field.is_static() || (is_setter && field.is_final())) { 
      out->send_error(JDWP_Error_INVALID_FIELDID);
      return;
    }

    type_tag = (field.type() < T_OBJECT ) 
      ? JavaDebugger::get_tag_from_type(field.type())
      : JDWP_Tag_OBJECT;
    Oop::Fast p;
#if ENABLE_ISOLATES
    // Statics don't live at the end of the JavaClassDesc, we need to
    // find the correct task mirror for this task
    {
      SETUP_ERROR_CHECKER_ARG;
      TaskGCContext tmp(in->transport()->task_id());
      TaskMirror::Fast tm = field_clazz().task_mirror_no_check();
      if (!TaskMirrorDesc::is_initialized_mirror((TaskMirrorDesc*)tm.obj())) {
        GUARANTEE(field_clazz().is_interface(),
                  "Non-interface class not initialized");
        if (field_clazz().is_interface()) {
          SAVE_CURRENT_EXCEPTION;
          // If an interface contains only non-object data then
          // it will not get initialized since javac will only
          // create a reference to the interface class if you
          // are accessing some static object like a static array
          // We need to at least initialized the statics
          tm = task_barrier(Thread::current(), field_clazz.obj() JVM_NO_CHECK);
          if (tm.is_null()) {
            // oome
            RESTORE_CURRENT_EXCEPTION;
            out->send_error(JDWP_Error_OUT_OF_MEMORY);
            return;
          }
          RESTORE_CURRENT_EXCEPTION;
        }
      }
      p = tm();
    }
#else
    p = field_clazz();
#endif
    if (is_setter) {
      ObjectReferenceImpl::read_value_to_address(in, &p,
                                                 field.offset(),
                                                 type_tag, true);
    } else { 
      ObjectReferenceImpl::write_value_from_address(out, &p,
                                                    field.offset(),
                                                    type_tag, true, true);
    }
#ifdef AZZERT
    if (TraceDebugger) { 
      tty->print("    ");     
      JavaDebugger::print_class(&field_clazz);
      tty->print(".");  
      JavaDebugger::print_field(&field);
      tty->print(": ");
      ObjectReferenceImpl::print_value_from_address(&p, 
                                                    field.offset(),
                                                    type_tag);
      tty->print_cr("");
    }
#endif
  }
  out->send_packet();
}

void ReferenceTypeImpl::reference_type_signature(PacketInputStream *in, 
                        PacketOutputStream *out)
{
  UsingFastOops fastoops;
  JavaClass::Fast clazz = in->read_class_ref();

  if (clazz.is_null() || clazz().is_interface()) { 
    out->send_error(JDWP_Error_INVALID_CLASS);
    return;
  } else {
#ifdef AZZERT
    if (TraceDebugger) {
      tty->print("Signature of class: ");
      JavaDebugger::print_class(&clazz);
      tty->print_cr("");
    }
#endif
    out->write_class_name(&clazz);
    out->send_packet();
  }
}

void ReferenceTypeImpl::reference_type_get_values(PacketInputStream *in, 
                        PacketOutputStream *out) 
{ 
    static_field_getter_setter(in, out, false);
}

// return superclass

void ClassTypeImpl::class_type_super_class(PacketInputStream *in, 
                     PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  InstanceClass::Fast clazz = in->read_class_ref();
  InstanceClass::Fast superclass;

  if (clazz.is_null() || clazz().is_interface()) { 
    out->write_int(0);
  } else {
#ifdef AZZERT
    if (TraceDebugger) {
      tty->print("Superclass of class: ");
      JavaDebugger::print_class(&clazz);
      tty->print_cr("");
    }
#endif
    superclass = clazz().super();
    if (superclass.is_null()) {
      out->write_int(0);
    } else {
#ifdef AZZERT
      if (TraceDebugger) {
        tty->print("SuperClass = ");
        JavaDebugger::print_class(&superclass);
        tty->print_cr(", id = 0x%x", JavaDebugger::get_object_id_by_ref_nocreate(&superclass));
      }
#endif
      out->write_class(&superclass);
    }
  }
  out->send_packet();
}

// Set static values

void ClassTypeImpl::class_type_set_values(PacketInputStream *in, 
                                PacketOutputStream *out)
{
  // I have no idea why the command to get statics is in one category, and
  // the command to set them is in another.
  // I'll combine them anyway.
  //
  ReferenceTypeImpl::static_field_getter_setter(in, out, true);
}

void ClassTypeImpl::class_type_invoke_method(PacketInputStream *in,
                                  PacketOutputStream *out)
{
  ObjectReferenceImpl::common_invoke_method(in, out, true);
}

void *ReferenceTypeImpl::reference_type_cmds[] = { 
  (void *)11
  ,(void *)ReferenceTypeImpl::reference_type_signature
  ,(void *)JavaDebugger::nop        // ReferenceType_ClassLoader
  ,(void *)JavaDebugger::nop        // ReferenceType_Modifiers
  ,(void *)JavaDebugger::nop        // ReferenceType_Fields
  ,(void *)JavaDebugger::nop        // ReferenceType_Methods
  ,(void *)ReferenceTypeImpl::reference_type_get_values
  ,(void *)JavaDebugger::nop        // JavaDebugger_SourceFile
  ,(void *)JavaDebugger::nop        // nestedTypes
  ,(void *)JavaDebugger::nop        // ReferenceType_Status
  ,(void *)JavaDebugger::nop        // ReferenceType_Interfaces
  ,(void *)JavaDebugger::nop        // classObject
};

void *ClassTypeImpl::class_type_cmds[] = { 
  (void *)0x4
  ,(void *)ClassTypeImpl::class_type_super_class
  ,(void *)ClassTypeImpl::class_type_set_values
  ,(void *)ClassTypeImpl::class_type_invoke_method
  ,(void *)JavaDebugger::nop        // newInstance
};

#endif
