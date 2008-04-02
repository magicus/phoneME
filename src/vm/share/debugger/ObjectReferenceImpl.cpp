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
# include "incls/_ObjectReferenceImpl.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

void ObjectReferenceImpl::read_value_to_address(PacketInputStream *in, Oop *p, jint field_offset, jbyte type_tag, jboolean is_static) {

  switch(type_tag) { 
    // Static fields are 32 bits long, non-static fields are packed
    // In order for ENDIANness to work right we need to use 'int'
    // accessors for shorter fields
  case JDWP_Tag_BYTE:
    if (is_static) {
      p->int_field_put(field_offset, (unsigned int)in->read_byte());
    } else {
      p->byte_field_put(field_offset, in->read_byte());
    }
    break;
  case JDWP_Tag_BOOLEAN:
    if (is_static) {
      p->int_field_put(field_offset, (unsigned int)in->read_boolean());
    } else {
      p->bool_field_put(field_offset, in->read_boolean());
    }
    break;

  case JDWP_Tag_CHAR:
    if (is_static) {
      p->int_field_put(field_offset, (int)in->read_char());
    } else {
      p->char_field_put(field_offset, in->read_char());
    }
    break;

  case JDWP_Tag_SHORT:
    if (is_static) {
      p->int_field_put(field_offset, (int)in->read_short());
    } else {
      p->short_field_put(field_offset, in->read_short());
    }
    break;

  case JDWP_Tag_LONG:
    p->long_field_put(field_offset, in->read_long());
    break;

  case JDWP_Tag_INT:
    p->int_field_put(field_offset, in->read_int());
    break;

#if ENABLE_FLOAT
  case JDWP_Tag_DOUBLE:
    p->double_field_put(field_offset, in->read_double());
    break;
  case JDWP_Tag_FLOAT:
    p->float_field_put(field_offset, in->read_float());
    break;
#endif

  case JDWP_Tag_VOID: 
    /* do nothing */
    break;

  default:
    {
      Oop::Raw o = in->read_object();
      p->obj_field_put(field_offset, &o);
    }
    break;
  }
}

void
ObjectReferenceImpl::write_value_from_address(PacketOutputStream *out,
                                              Oop *p, jint field_offset,
                                              jbyte tag, bool write_tag,
                                              bool is_static) {

  if (tag == JDWP_Tag_OBJECT || tag == JDWP_Tag_ARRAY) {
    Oop::Raw o = p->obj_field(field_offset);
    tag = JavaDebugger::get_jdwp_tag(&o);
  }
  if (write_tag) { 
    out->write_byte(tag);
  }

  switch(tag) { 
  case JDWP_Tag_LONG:
    out->write_long(p->long_field(field_offset));
    break;

  case JDWP_Tag_BOOLEAN:
    if (is_static) {
      out->write_boolean((jboolean)p->int_field(field_offset));
    } else {
      out->write_boolean(p->bool_field(field_offset));
    }
    break;

  case JDWP_Tag_BYTE:
    if (is_static) {
      out->write_byte((jbyte)p->int_field(field_offset));
    } else {
      out->write_byte(p->byte_field(field_offset));
    }
    break;

  case JDWP_Tag_CHAR:
    if (is_static) {
      out->write_char((jchar)p->int_field(field_offset));
    } else {
      out->write_char(p->char_field(field_offset));
    }
    break;

  case JDWP_Tag_SHORT:
    if (is_static) {
      out->write_short((jshort)p->int_field(field_offset));
    } else {
      out->write_short(p->short_field(field_offset));
    }
    break;
    
  case JDWP_Tag_INT:
    out->write_int(p->int_field(field_offset));
    break;
 
#if ENABLE_FLOAT
  case JDWP_Tag_DOUBLE: 
    out->write_double(p->double_field(field_offset));
    break;
  case JDWP_Tag_FLOAT:
    out->write_float(p->float_field(field_offset));
    break;
#endif

  case JDWP_Tag_VOID:  // happens with function return values
    // write nothing
    break;

  default:
    {
      UsingFastOops fast_oops;

      Oop::Fast o = p->obj_field(field_offset);
      out->write_object(&o);
    }
    break;
  }
}

#ifdef AZZERT
void
ObjectReferenceImpl::print_value_from_address(Oop *p, jint field_offset, jbyte tag)
{

  if (tag == JDWP_Tag_OBJECT || tag == JDWP_Tag_ARRAY) {
    Oop::Raw o = p->obj_field(field_offset);
    tag = JavaDebugger::get_jdwp_tag(&o);
  }

  tty->print("Tag: %c", tag);

  switch(tag) { 
  case JDWP_Tag_DOUBLE: 
    tty->print(" Data: %lf", p->double_field(field_offset));  
    break;
  case JDWP_Tag_LONG:
    tty->print(" Data: %ld", p->long_field(field_offset));
    break;

  case JDWP_Tag_BYTE:
    tty->print(" Data: %c", p->byte_field(field_offset));
    break;

  case JDWP_Tag_CHAR:
    tty->print(" Data: 0x%hx", p->char_field(field_offset));
    break;

  case JDWP_Tag_INT:
    tty->print(" Data: %d", p->int_field(field_offset));
    break;
  case JDWP_Tag_FLOAT:
    tty->print(" Data: %f", jvm_f2d(p->float_field(field_offset)));
    break;
 
  case JDWP_Tag_SHORT:
    tty->print(" Data: %hu", p->short_field(field_offset));
    break;
    
  case JDWP_Tag_BOOLEAN:
    if (p->bool_field(field_offset) == true) { 
      tty->print(" Data: True");
    } else {
      tty->print(" Data: False");
    }
    break;

  case JDWP_Tag_VOID:  // happens with function return values
    // write nothing
    break;

  default:
    {
      UsingFastOops fast_oops;

      Oop::Fast o = p->obj_field(field_offset);
      tty->print(" Data: 0x%x", o.obj());
    }
    break;
  }
}
#endif /* AZZERT */

// Given an object, return the javaclass that this object is a type of

void
ObjectReferenceImpl::object_reference_reference_type(PacketInputStream *in, 
                     PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  Oop::Fast object = in->read_object();
  JavaClass::Fast ref_class;

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Object Reference Type: ObjectID=%ld, Object=0x%lx", 
                   JavaDebugger::get_object_id_by_ref_nocreate(&object), 
                   object.obj());
  }
#endif 
  if (object.not_null()) {
    ref_class = object().blueprint();
    if (object.is_jvm_thread()) {
      // this is a VM thread object that's in the java.lang.Thread object
      // User really doesn't want to know about this as such
      ref_class = Universe::object_class();
    }
    out->write_byte(JavaDebugger::get_jdwp_tagtype(&ref_class));
    out->write_class(&ref_class);
#ifdef AZZERT
    if (TraceDebugger) { 
      tty->print_cr("    Type is 0x%x ID = %ld", ref_class.obj(),
                    JavaDebugger::get_object_id_by_ref_nocreate(&ref_class));
    }
#endif
  } else {
    out->set_error(JDWP_Error_INVALID_OBJECT);
  }
  out->send_packet();
}

// Return instance values
 
void ObjectReferenceImpl::object_field_getter_setter(PacketInputStream *in, 
                         PacketOutputStream *out, 
                         bool is_setter)
{
  UsingFastOops fast_oops;
  int i;

  Oop::Fast object = in->read_object();
  jint num = in->read_int();
  if (object.is_null()) {
    out->send_error(JDWP_Error_INVALID_OBJECT);
    return;
  }

  // Create a buffered output stream so we can asynchronously send an error
  // Calculate the size based on half of the items being 'longs'
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Object %s, objectID=%ld, object=0x%lx, count=%ld",
                  (is_setter ? "Set" : "Get"), 
                  JavaDebugger::get_object_id_by_ref_nocreate(&object), 
                  object.obj(), num);
  }
#endif
  if (!is_setter) { 
    out->write_int(num);
  }

  for (i = 0; i < num; i++) {
    UsingFastOops fast_oops_2;

    InstanceClass::Fast fieldClazz = in->read_class_ref();
    TypeArray::Fast fields;  
    jint field_id = in->read_int();

    if (fieldClazz.is_null()) {
      out->send_error(JDWP_Error_INVALID_FIELDID);
      return;
    }
    fields = fieldClazz().fields();
    if (fields.is_null()) {
      out->send_error(JDWP_Error_INVALID_FIELDID);
      return;
    }
    if (field_id >=fields().length() / Field::NUMBER_OF_SLOTS) {
      out->send_error(JDWP_Error_INVALID_FIELDID);
      return;
    }
    Field field(&fieldClazz, field_id * Field::NUMBER_OF_SLOTS);
    char type_tag = (field.type() < T_OBJECT) 
      ? JavaDebugger::get_tag_from_type(field.type()) 
      : JDWP_Tag_OBJECT;
    if (is_setter) { 
      read_value_to_address(in, &object, field.offset(), type_tag, false);
    } else { 
      write_value_from_address(out, &object, field.offset(), type_tag, true, false);
    }
#ifdef AZZERT
    if (TraceDebugger) {
      //AccessFlags field_access_flags = field.access_flags();
      tty->print("    ");  
      JavaDebugger::print_class(&fieldClazz), 
        tty->print(".");
      JavaDebugger::print_field(&field);
      tty->print(": ");
      print_value_from_address(&object, field.offset(), type_tag);
      tty->print_cr("");
    }
#endif
  }
  out->send_packet();
}

void ObjectReferenceImpl::object_reference_get_values(PacketInputStream *in, 
                                             PacketOutputStream *out)
{
    object_field_getter_setter(in, out, false);
}

void ObjectReferenceImpl::object_reference_set_values(PacketInputStream *in, 
                                             PacketOutputStream *out)
{
    object_field_getter_setter(in, out, true);
}

void ObjectReferenceImpl::object_reference_is_collected(PacketInputStream *in, 
                                             PacketOutputStream *out)
{
  Oop::Raw p = in->read_object();
#ifdef AZZERT
    if (TraceDebugger) {
      tty->print_cr("Iscollected: obj = 0x%x, collected = 0x%x", p.obj(),
                    p.is_null() ? 1 : 0);
    }
#endif
  out->write_boolean(p.is_null());
  out->send_packet();
}

void ObjectReferenceImpl::common_invoke_method(PacketInputStream *in,
                                                     PacketOutputStream *out,
                                                     bool is_static)
{
  UsingFastOops fastoops;

  SETUP_ERROR_CHECKER_ARG;
  Oop::Fast this_object;
  JavaClass::Fast this_klass;
  Thread::Fast invoke_thread;
  InstanceClass::Fast ic;
  Method::Fast m, sync_method;
  int arg_count;
  jbyte type_tag, modified_tag;
  EntryActivation::Fast entry, sync_entry;
  int arg_index = 0;
  int options;

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("ObjectReferenceImpl: invoke %s method starting",
                  (is_static ? "static" : "instance"));
  }
#endif
  if (!is_static) {
    this_object = in->read_object();
    if (this_object.is_null()) {
      out->send_error(JDWP_Error_INVALID_OBJECT);
      return;
    }
    invoke_thread = in->read_thread_ref();
    ic = in->read_class_ref();
  } else {
    // Why couldn't they use the same order as static invoke?
    ic = in->read_class_ref();
    invoke_thread = in->read_thread_ref();
  }
  if (invoke_thread.is_null()) {
    out->send_error(JDWP_Error_INVALID_THREAD);
    return;
  }
  if ((invoke_thread().status() & THREAD_DBG_SUSPENDED_BY_EVENT) == 0) {
    if (TraceDebugger) {
      tty->print_cr("ObjectReferenceImpl: Thread not suspended by event");
    }
    out->send_error(JDWP_Error_THREAD_NOT_SUSPENDED);
    return;
  }
  if (ic.is_null()) {
    out->send_error(JDWP_Error_INVALID_CLASS);
    return;
  }
  m = JavaDebugger::get_method_by_id(&ic, in->read_long());

  if (m.is_null()) {
    out->send_error(JDWP_Error_INVALID_METHODID);
    return;
  }
#ifdef AZZERT
  if (TraceDebugger) {
      tty->print("ObjectReferenceImpl: invoke method ");
      m().print_value_on(tty);
      tty->cr();
      tty->print("   class ref ");
      ic().print_name_on(tty);
      tty->cr();
  }
#endif
  arg_count = in->read_int();
  arg_count += (!is_static ? 1 : 0);
  entry = Universe::new_entry_activation(&m, arg_count JVM_NO_CHECK);
  Signature::Fast sig = m().signature();
  SignatureStream ss(&sig, is_static);

  // If it's an instance method we need to push the 'this_object' rather
  // than one of the arguments sitting in the input stream.
  if (!is_static) {
    entry().obj_at_put(arg_index++, &this_object);
    ss.next();  // point at arg just past 'this'
  }

  for (;
       !ss.eos() && arg_index < arg_count && !ss.is_return_type();
       ss.next()) {
    type_tag = in->read_byte();
    modified_tag = type_tag;
    if (type_tag == JDWP_Tag_STRING ||
        type_tag == JDWP_Tag_THREAD ||
        type_tag == JDWP_Tag_CLASS_OBJECT) {
      modified_tag = JDWP_Tag_OBJECT;
    }
    if (JavaDebugger::get_tag_from_type(ss.type()) != modified_tag) {
      if (TraceDebugger) {
        tty->print_cr("ObjectReferenceImpl: invoke method: illegal arg, type %d, expected %d ", type_tag, ss.type());
      }
      out->send_error(JDWP_Error_ILLEGAL_ARGUMENT);
      return;
    } 
    switch(type_tag) {
    case JDWP_Tag_INT:
      entry().int_at_put(arg_index++, in->read_int());
      break;
    case JDWP_Tag_CHAR:
      entry().int_at_put(arg_index++, (int)in->read_char());
      break;
    case JDWP_Tag_SHORT:
      entry().int_at_put(arg_index++, (int)in->read_short());
      break;
    case JDWP_Tag_DOUBLE:
      entry().double_at_put(arg_index++, in->read_double());
      break;
    case JDWP_Tag_LONG:
      entry().long_at_put(arg_index++, in->read_long());
      break;
    case JDWP_Tag_BOOLEAN:
      entry().int_at_put(arg_index++, (int)in->read_boolean());
      break;
    case JDWP_Tag_OBJECT:
    case JDWP_Tag_ARRAY:
    case JDWP_Tag_STRING:
    case JDWP_Tag_THREAD:
    case JDWP_Tag_CLASS_OBJECT:
      {
        UsingFastOops fastoops2;
        Oop::Fast o = in->read_object();
        JavaClass::Fast o_klass = o().blueprint();
        JavaClass::Fast sig_klass = ss.type_klass();
        if (!sig_klass().is_subtype_of(&o_klass)) {
          out->send_error(JDWP_Error_ILLEGAL_ARGUMENT);
          return;
        }
        entry().obj_at_put(arg_index++, &o);
        break;
      }
    default:
#ifdef AZZERT
      tty->print_cr("Unknown argument type %d", type_tag);
      out->send_error(JDWP_Error_ILLEGAL_ARGUMENT);
      return;
#endif
      break;
    }
  }
  if (arg_index < arg_count || !ss.is_return_type()) {
    // if arg_index < arg_count we must have reached eos
    // if !return_type then the actual method has more args than the debugger
    // thinks
    out->send_error(JDWP_Error_ILLEGAL_ARGUMENT);
    return;
  }
  options = in->read_int();
  if (options & JDWP_InvokeOptions_INVOKE_SINGLE_THREADED) {
    ThreadReferenceImpl::resume_specific_thread(&invoke_thread, -1);
  } else {
    ThreadReferenceImpl::resume_all_threads(-1);
  }
  sync_method =
    Universe::dbg_class()->find_local_method(Symbols::debugger_sync_name(),
                               Symbols::obj_obj_int_int_int_int_void_signature());
  if (sync_method.is_null()) {
    out->send_error(JDWP_Error_ILLEGAL_ARGUMENT);
    return;
  }
#ifdef AZZERT
  Frame fr(&invoke_thread);
  GUARANTEE(fr.is_java_frame(), "Must have java frame before invoking method");
#endif
  address* stack_pointer = (address*)invoke_thread().stack_pointer();
  address ret_addr =
      stack_pointer[-1 * JavaStackDirection];
  int is_object_return = 0;
  if (ret_addr == (address)shared_call_vm_oop_return) {
    is_object_return = 1;
  }
  sync_entry = Universe::new_entry_activation(&sync_method, 6 JVM_NO_CHECK);
  sync_entry().obj_at_put(0, &entry);
  sync_entry().obj_at_put(1, in->transport());
  sync_entry().int_at_put(2, in->id());
  sync_entry().int_at_put(3, options);
  sync_entry().int_at_put(4, ss.type());
  sync_entry().int_at_put(5, is_object_return);
  invoke_thread().append_pending_entry(&sync_entry);
}


void ObjectReferenceImpl::object_reference_invoke_method(PacketInputStream *in,
                                                     PacketOutputStream *out)
{
  common_invoke_method(in, out, false);
}

jlong ObjectReferenceImpl::invoke_return(Oop *o, Oop *exc, Oop *transport,
                                         int id, int options, int return_type,
                                         OopDesc **obj_ret_val) {

  UsingFastOops fastoops;
  Oop::Fast ret_obj;
  Transport *t = (Transport *)transport;
  PacketOutputStream out(t);
  out.init_reply(id);

  if (!exc->is_null()) {
    // have an exception condition, return the exception object
    out.write_byte(JDWP_Tag_OBJECT);
    out.write_object(exc);
  } else {
    if (o->is_null()) {
      out.write_byte(JDWP_Tag_VOID);
    } else if (o->is_obj_array()) {
      ObjArray::Raw oa = o->obj();
      ret_obj = oa().obj_at(0);
      jbyte tag = JavaDebugger::get_jdwp_tag(&ret_obj);
      out.write_byte(tag);
      out.write_object(&ret_obj);
    } else {
      GUARANTEE(o->is_type_array(), "Invalid return array type");
      TypeArray::Raw ta = o->obj();
      switch(return_type) {
      case T_BOOLEAN:
        out.write_byte(JDWP_Tag_BOOLEAN);
        out.write_boolean((jboolean)ta().int_at(0));
        break;
      case T_BYTE:
        out.write_byte(JDWP_Tag_BYTE);
        out.write_byte((jboolean)ta().int_at(0));
        break;
      case T_SHORT:
        out.write_byte(JDWP_Tag_SHORT);
        out.write_short((jshort)ta().int_at(0));
        break;
      case T_CHAR:
        out.write_byte(JDWP_Tag_CHAR);
        out.write_char((jchar)ta().int_at(0));
        break;
      case T_INT:
        out.write_byte(JDWP_Tag_INT);
        out.write_int(ta().int_at(0));
        break;
      case T_LONG:
        out.write_byte(JDWP_Tag_LONG);
        out.write_long((jlong)ta().long_at(0));
        break;
      case T_DOUBLE:
        out.write_byte(JDWP_Tag_DOUBLE);
        out.write_double(ta().double_at(0));
        break;
      case T_FLOAT:
        out.write_byte(JDWP_Tag_FLOAT);
        out.write_float(ta().float_at(0));
        break;
      }
    }
    out.write_byte(JDWP_Tag_OBJECT);
    out.write_int(0);
  }
  if (options & JDWP_InvokeOptions_INVOKE_SINGLE_THREADED) {
    ThreadReferenceImpl::suspend_specific_thread(Thread::current(), -1, true);
  } else {
    ThreadReferenceImpl::suspend_all_threads(-1, true);
  }
  out.send_packet();
  // At this point the stack looks like:
  //  [ 0] com/sun/cldchi/jvm/JVM.debuggerInvokeReturn
  //  [ 1] com/sun/cldchi/jvm/JVM.debuggerInvoke
  //  [ 2] EntryFrame
  //  [ 3] test/Testx.main
  // Need to remove the two JavaFrames and the entry frame.
  // The stack will look like it did before we did the invoke
  Thread *thread = Thread::current();
  Frame fr(thread);
  GUARANTEE(fr.is_java_frame(), "Frame must be java frame");
  fr.as_JavaFrame().caller_is(fr);
  GUARANTEE(fr.is_java_frame(), "Frame must be java frame");
  fr.as_JavaFrame().caller_is(fr);
  GUARANTEE(fr.is_entry_frame(), "Frame must be entry frame");
  // We want to pass the original return value from the call VM (that got
  // saved in the EntryFrame) back to call_vm so it looks like this
  // invoke never happened.
  jlong ret_val = 0;
  if (obj_ret_val != NULL) {
    *obj_ret_val = fr.as_EntryFrame().stored_obj_value();
  } else {
    ret_val = jlong_from_low_high(fr.as_EntryFrame().stored_int_value2(),
                                  fr.as_EntryFrame().stored_int_value1());
  }
  fr.as_EntryFrame().caller_is(fr);
  GUARANTEE(fr.is_java_frame(), "Frame must be java frame");
  // point to just after pc_addr
  address empty_stack = fr.sp() + (JavaStackDirection * BytesPerStackElement);
  // place to store original fp and return into call_vm code
  address *two_word_stack =
    (address *)(empty_stack + JavaStackDirection * BytesPerStackElement * 2);
  address* stack_pointer = (address*)thread->stack_pointer();
  // original return address
  address ret_addr =
      stack_pointer[-1 * JavaStackDirection];
  two_word_stack[-1*JavaStackDirection] = ret_addr;
  two_word_stack[0] = fr.fp();
  thread->set_last_java_fp(fr.fp());
  thread->set_last_java_sp(fr.sp());
  thread->set_stack_pointer((jint)two_word_stack);
  return ret_val;
}

// Given a mirror object, return the javaclass that this object is a mirror of

void
ClassObjectReferenceImpl::class_object_reflected_type(PacketInputStream *in, 
                     PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  JavaClassObj::Fast mirror = in->read_object();
  JavaClass::Fast ref_class;

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Class Object Reflected Type: ObjectID=%ld, Object=0x%lx", 
                   JavaDebugger::get_object_id_by_ref_nocreate(&mirror), 
                   mirror.obj());
  }
#endif 
  if (mirror.not_null()) {
    ref_class = mirror().java_class();
    out->write_byte(JavaDebugger::get_jdwp_tagtype(&ref_class));
    out->write_class(&ref_class);
#ifdef AZZERT
    if (TraceDebugger) { 
      tty->print_cr("    Type is 0x%x ID = %ld", ref_class.obj(),
                    JavaDebugger::get_object_id_by_ref_nocreate(&ref_class));
    }
#endif
  } else {
    out->set_error(JDWP_Error_INVALID_OBJECT);
  }
  out->send_packet();
}

void StringReferenceImpl::string_reference_value(PacketInputStream *in, 
                      PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  String::Fast s = in->read_object();
  out->write_string(&s);

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print("String: stringID=%ld, string=0x%lx, value = ", 
                  JavaDebugger::get_object_id_by_ref_nocreate(&s), s.obj());
    TypeArray::Raw t = s().value();
    for (int i = 0; i < s().count(); i++) {
      tty->print("%c", (char)t().char_at(i + s().offset()));
    }
    tty->print_cr("");
  }
#endif
  out->send_packet();
}

void *ObjectReferenceImpl::object_reference_cmds[] = { 

  (void *)0x9
  ,(void *)ObjectReferenceImpl::object_reference_reference_type
  ,(void *)ObjectReferenceImpl::object_reference_get_values
  ,(void *)ObjectReferenceImpl::object_reference_set_values
  ,(void *)NULL               // no longer used
  ,(void *)JavaDebugger::nop  // monitorInfo
  ,(void *)ObjectReferenceImpl::object_reference_invoke_method
  ,(void *)JavaDebugger::nop  // disableCollection
  ,(void *)JavaDebugger::nop  // enableCollection
  ,(void *)ObjectReferenceImpl::object_reference_is_collected
};

void *StringReferenceImpl::string_reference_cmds[] = { 
  (void *)0x1
  ,(void *)string_reference_value
};

void *ClassObjectReferenceImpl::class_object_reference_cmds[] = { 
  (void *)1
  ,(void *)ClassObjectReferenceImpl::class_object_reflected_type
};

#endif
