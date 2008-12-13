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
# include "incls/_ArrayReferenceImpl.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

void ArrayReferenceImpl::array_reference_length(PacketInputStream *in,
                                                PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  Oop::Fast obj = in->read_object();
#ifdef AZZERT
  if (TraceDebugger) {
    int obj_id = JavaDebugger::get_object_id_by_ref_nocreate(&obj);
    tty->print_cr("ArrayLength: ArrayID=%ld, Array=0x%lx",
                  obj_id, obj.obj());
  }
#endif

  if (obj.not_null()) {
    if (obj.is_type_array() || obj.is_obj_array()) {
      UsingFastOops fast_oops_2;
      Array::Fast array = obj.obj();

#ifdef AZZERT
      if (TraceDebugger) { 
        tty->print_cr("    Length = %ld\n", array().length());
      }
#endif
      out->write_int(array().length());
    } else {
      out->write_int(0);
    }
  }
  out->send_packet();
}

void
ArrayReferenceImpl::array_getter_setter(PacketInputStream *in,
                                        PacketOutputStream *out,
                                        bool is_setter)
{ 
  UsingFastOops fast_oops;

  ObjArrayClass::Fast oc;
  jbyte element_type_tag;
  int i;

  Oop::Raw oop = in->read_object();
  if (oop.is_null() || !oop().is_array()) {
    out->send_error(JDWP_Error_INVALID_OBJECT);
    return;
  }
  Array::Fast array = oop.obj();
  TypeArray::Fast ta;
  ObjArray::Fast oa;
  jint first_index = in->read_int();
  jint length = in->read_int();
  int last_index = 
    (!is_setter && (length == -1)) ? array().length() : first_index + length;
  if ((last_index - first_index) > array().length()) {
    out->send_error(JDWP_Error_INVALID_LENGTH);
    return;
  }
#ifdef AZZERT
  if (TraceDebugger) {
    int obj_id = JavaDebugger::get_object_id_by_ref_nocreate(&array);
    tty->print_cr("Array %s: arrayID=%ld, array=0x%x, first=%ld, length=%ld",
                  (is_setter ? "Set" : "Get"),
                  obj_id, array.obj(), first_index, length);
  }
#endif

  if (array.not_null()) {
    UsingFastOops fast_oops_2;
    ArrayClass::Fast ac = array().blueprint();
    if (ac().is_obj_array_class()) {
      oa = array();
      element_type_tag = JDWP_Tag_OBJECT;
    } else {
      // not an array of objects, just primitives
      ta = array();
      TypeArrayClass::Raw tc = ac.obj();
      element_type_tag = (jbyte)JavaDebugger::get_tag_from_type(tc().type());
    }
    if (!is_setter) {
      if (ac().is_obj_array_class()) {

        // Ok so it's an array of objects.  If the object is itself an array
        // then we need to send the JDWP_Tag_ARRAY back to the debugger
        Oop::Raw element = oa().obj_at(first_index);
        if (element.is_null()) {
          // this is the best we can do if the object array element is
          // not allocated
          out->write_byte(JDWP_Tag_OBJECT);
        } else {
          JavaClass::Raw ec = element().blueprint();
          if (ec().is_type_array_class() || ec().is_obj_array_class()) {
            // since the element itself is an array set the tag correctly
            out->write_byte(JDWP_Tag_ARRAY);
          } else {
            out->write_byte(JDWP_Tag_OBJECT);
          }
        }
      } else {
        out->write_byte(element_type_tag);
      }

      out->write_int(last_index - first_index);

      for (i=first_index; i < last_index; i++) {
        switch(element_type_tag) {
        case JDWP_Tag_LONG:
          out->write_long(ta().long_at(i));
          break;
#if ENABLE_FLOAT
        case JDWP_Tag_DOUBLE:
          out->write_double(ta().double_at(i));
          break;
        case JDWP_Tag_FLOAT:
          out->write_float(ta().float_at(i));
          break;
#endif
        case JDWP_Tag_BYTE:
          out->write_byte(ta().byte_at(i));
          break;
        case JDWP_Tag_BOOLEAN: 
          out->write_byte(ta().bool_at(i));
          break;
        case JDWP_Tag_CHAR:
          out->write_short(ta().char_at(i));
          break;
        case JDWP_Tag_SHORT: 
          out->write_short(ta().short_at(i));
          break;
        case JDWP_Tag_INT:
          out->write_int(ta().int_at(i));
          break;
        case JDWP_Tag_OBJECT:
          // This is the only case in which we write out the tag
          {
            UsingFastOops fast_oops_6;

            Oop::Fast p = oa().obj_at(i);
            out->write_byte(JavaDebugger::get_jdwp_tag(&p));
            out->write_object(&p);
          }
          break;
        default: 
          out->send_error(JDWP_Error_INVALID_TAG);
          return;
        }
      }
      out->send_packet();
    } else {
      // is_setter
      for (i=first_index; i < last_index; i++) {
        switch (element_type_tag) {
        case JDWP_Tag_INT:
          ta().int_at_put(i, in->read_int());
          break;
#if ENABLE_FLOAT
        case JDWP_Tag_FLOAT:
          ta().float_at_put(i, in->read_float());
          break;
        case JDWP_Tag_DOUBLE:
          ta().double_at_put(i, in->read_double());
          break;
#endif
        case JDWP_Tag_LONG:
          ta().long_at_put(i, in->read_long());
          break;
        case JDWP_Tag_BYTE:
          ta().byte_at_put(i, in->read_byte());
          break;
        case JDWP_Tag_BOOLEAN: 
          ta().bool_at_put(i, in->read_byte());
          break;
        
        case JDWP_Tag_CHAR:
          ta().char_at_put(i, in->read_short());
          break;
        case JDWP_Tag_SHORT: 
          ta().short_at_put(i, in->read_short());
          break;
        case JDWP_Tag_OBJECT:
          {
            Oop::Raw p = in->read_object();
            oa().obj_at_put(i, &p);
          }
          break;
        default:
          out->send_error(JDWP_Error_INVALID_TAG);
          return;
        }
      }
      out->send_packet();
    }
#ifdef AZZERT
    if (TraceDebugger) { 
      for (i=first_index; i < last_index; i++) {
        tty->print("    array[%d] = ", i);
        tty->print("Tag: %c", element_type_tag);

        switch(element_type_tag) { 
#if ENABLE_FLOAT
        case JDWP_Tag_DOUBLE: 
          tty->print(" Data: %lf", ta().double_at(i));  
          break;
        case JDWP_Tag_FLOAT:
          tty->print(" Data: %f", jvm_f2d(ta().float_at(i)));
          break;
#endif
        case JDWP_Tag_LONG:
          tty->print(" Data: 0x%lx", ta().long_at(i));
          break;
        
        case JDWP_Tag_BYTE:
          tty->print(" Data: %c", ta().byte_at(i));
          break;

        case JDWP_Tag_CHAR:
          tty->print(" Data: 0x%hx", ta().char_at(i));
          break;

        case JDWP_Tag_INT:
          tty->print(" Data: 0x%x", ta().int_at(i));
          break;
 
        case JDWP_Tag_SHORT:
          tty->print(" Data: 0x%hx", ta().short_at(i));
          break;
    
        case JDWP_Tag_BOOLEAN:
          if (ta().bool_at(i) == true) { 
            tty->print(" Data: True");
          } else {
            tty->print(" Data: False");
          }
          break;

        case JDWP_Tag_VOID:  // happens with function return values
          // write nothing
          break;

        case JDWP_Tag_OBJECT:
          {
            UsingFastOops fast_oops_7;
            
            Oop::Fast o = oa().obj_at(i);
            tty->print(" Data: 0x%x, id = 0x%x", o.obj(),
                       JavaDebugger::get_object_id_by_ref_nocreate(&o));
          }
          break;
        default:
          break;
        }
        tty->print_cr("");
      }
    }
#endif /* AZZERT */
  }
  out->send_packet();
}

void ArrayReferenceImpl::array_reference_get_values(PacketInputStream *in, 
                         PacketOutputStream *out)
{ 
    array_getter_setter(in, out, false);
}

void ArrayReferenceImpl::array_reference_set_values(PacketInputStream *in, 
                         PacketOutputStream *out)
{ 
    array_getter_setter(in, out, true);
}

void *ArrayReferenceImpl::array_reference_cmds[] = { 
  (void *)0x3
  ,(void *)ArrayReferenceImpl::array_reference_length
  ,(void *)ArrayReferenceImpl::array_reference_get_values
  ,(void *)ArrayReferenceImpl::array_reference_set_values
};

#endif
