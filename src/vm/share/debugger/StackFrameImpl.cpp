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
# include "incls/_StackFrameImpl.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

bool StackFrameImpl::read_local_to_address(PacketInputStream *in,
                                           PacketOutputStream *out,
                                           JavaFrame *fr, jint slot, jbyte tag)
{
  bool ret = true;

  switch(tag) { 
  case JDWP_Tag_BYTE:
  case JDWP_Tag_BOOLEAN:
    *(fr->local_at(slot)->int_addr()) =(int)in->read_byte();
    break;

  case JDWP_Tag_CHAR:
    *(fr->local_at(slot)->int_addr()) =(unsigned int)in->read_char();
    break;
  case JDWP_Tag_SHORT:
    *fr->local_at(slot)->int_addr() = (int)in->read_short();
    break;

  case JDWP_Tag_LONG:
    {
      // we bump up the slot number so we index correctly to the beginning
      // of the long
      jlong l = in->read_long();
      fr->local_at(slot + 1)->set_long(l);
      break;
    }

#if ENABLE_FLOAT
  case JDWP_Tag_DOUBLE:
    {
      // we bump up the slot number so we index correctly to the beginning
      // of the long
      jdouble d = in->read_double();
      fr->local_at(slot + 1)->set_double(d);
      break;
    }
  case JDWP_Tag_FLOAT:
    *(jint *)(fr->local_at(slot)->int_addr()) = in->read_int();
    break;
#endif
  
  case JDWP_Tag_INT:
    *(jint *)(fr->local_at(slot)->int_addr()) = in->read_int();
    break;

  case JDWP_Tag_VOID: 
    /* do nothing */
    break;
  case JDWP_Tag_OBJECT:
    *(Oop *)(fr->local_at(slot)->int_addr()) = in->read_object();
    break;

  default:
    out->set_error(JDWP_Error_INVALID_TAG);
    ret = false;
    break;
  }
  return ret;
}

bool StackFrameImpl::write_local_from_address(PacketOutputStream *out,
                                              JavaFrame *fr, jint slot,
                                              jbyte tag, jboolean write_tag, 
                                              jboolean bogus_object) {

  bool ret = true;

  if (!bogus_object && (tag == JDWP_Tag_OBJECT || tag == JDWP_Tag_ARRAY)) {
    Oop::Raw o = fr->local_at(slot)->as_obj();
    tag = JavaDebugger::get_jdwp_tag(&o);
  }
  if (write_tag) { 
    out->write_byte(tag);
  }
  
  switch(tag) {
#if ENABLE_FLOAT
  case JDWP_Tag_DOUBLE: 
    // we bump up the slot number so we index correctly to the beginning
    // of the long
    out->write_double(fr->local_at(slot + 1)->as_double());
    break;

  case JDWP_Tag_FLOAT:
    out->write_float(fr->local_at(slot)->as_float());
    break;
#endif

  case JDWP_Tag_LONG:
    // we bump up the slot number so we index correctly to the beginning
    // of the long
    out->write_long(fr->local_at(slot + 1)->as_long());
    break;
    
  case JDWP_Tag_BYTE:
    out->write_byte((jbyte)fr->local_at(slot)->as_int());
    break;

  case JDWP_Tag_CHAR:
    out->write_char((jchar)fr->local_at(slot)->as_int());
    break;

  case JDWP_Tag_INT:
    out->write_int(fr->local_at(slot)->as_int());
    break;
            
  case JDWP_Tag_SHORT:
    out->write_short((jshort)(fr->local_at(slot)->as_int()));
    break;

  case JDWP_Tag_BOOLEAN:
    out->write_boolean((jboolean)(fr->local_at(slot)->as_int()));
    break;

  case JDWP_Tag_VOID:  /* happens with function return values */   
    /* write nothing */
    break;

  case JDWP_Tag_OBJECT:
  case JDWP_Tag_ARRAY:
  case JDWP_Tag_STRING:
  case JDWP_Tag_THREAD:
  case JDWP_Tag_CLASS_OBJECT:
    if (bogus_object) {
      Oop::Raw null_obj;
      out->write_object(&null_obj);
    } else {
      out->write_object(fr->local_at(slot)->as_oop());
    }
    break;
  default:
    out->set_error(JDWP_Error_INVALID_TAG);
    ret = false;
    break;
  }

  return ret;
}

#ifdef AZZERT
void
StackFrameImpl::print_local_from_address(JavaFrame *fr, jint slot,
                                         jbyte tag, 
                                         jboolean bogus_object) {

  if (!bogus_object && (tag == JDWP_Tag_OBJECT || tag == JDWP_Tag_ARRAY)) {
    Oop::Raw o = fr->local_at(slot)->as_obj();
    tag = JavaDebugger::get_jdwp_tag(&o);
  }
  
  tty->print("Tag: %c", tag);
  
  switch(tag) {
#if ENABLE_FLOAT
  case JDWP_Tag_DOUBLE: 
    // we bump up the slot number so we index correctly to the beginning
    // of the long
    tty->print(" Data: %lf", fr->local_at(slot + 1)->as_double());
    break;
  case JDWP_Tag_FLOAT:
    tty->print(" Data: %f", jvm_f2d(fr->local_at(slot)->as_float()));
    break;
#endif
  case JDWP_Tag_LONG:
    // we bump up the slot number so we index correctly to the beginning
    // of the long
    tty->print(" Data: %ld", fr->local_at(slot + 1)->as_long());
    break;
    
  case JDWP_Tag_BYTE:
    tty->print(" Data: %c", (jbyte)fr->local_at(slot)->as_int());
    break;

  case JDWP_Tag_CHAR:
    tty->print(" Data: %c", (jchar)fr->local_at(slot)->as_int());
    break;

  case JDWP_Tag_INT:
    tty->print(" Data: %d", fr->local_at(slot)->as_int());
    break;
  case JDWP_Tag_SHORT:
    tty->print(" Data: %hu", (jshort)(fr->local_at(slot)->as_int()));
    break;

  case JDWP_Tag_BOOLEAN:
    if ((fr->local_at(slot)->as_int()) == true) {
      tty->print(" Data: True");
    } else {
      tty->print(" Data: False");
    }
    break;

  case JDWP_Tag_VOID:  /* happens with function return values */   
    /* write nothing */
    break;

  case JDWP_Tag_OBJECT:
  case JDWP_Tag_ARRAY:
  case JDWP_Tag_STRING:
  case JDWP_Tag_THREAD:
  case JDWP_Tag_CLASS_OBJECT: {
    UsingFastOops fast_oops;
    Oop::Fast o;
    if (!bogus_object) {
      o = fr->local_at(slot)->as_oop();
    }
    tty->print(" Data: 0x%x", o.obj());
    break;
    }
  }
  return;
}
#endif

// Get/Set the value of a stack variable

void StackFrameImpl::stack_frame_getter_setter(PacketInputStream *in, 
                        PacketOutputStream *out, 
                        bool is_setter)
{
  UsingFastOops fast_oops;
  Thread::Fast thread;
  thread = in->read_thread_ref();
  jint frame_id = in->read_int();
  jint num_values = in->read_int();
  int i;

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Stack Frame %s: threadID=%ld, frame=%ld, count=%ld",
                  (is_setter ? "Set" : "Get"),
                   JavaDebugger::get_thread_id_by_ref(&thread), 
                   frame_id, num_values);
  }
#endif

  if (!JavaDebugger::is_valid_thread(&thread) ||
      !(thread().status() & THREAD_DBG_SUSPENDED) ||
      !thread().last_java_frame_exists()) {
    out->send_error(JDWP_Error_INVALID_THREAD);
    return;
  }

  Frame fr(&thread);

  if (get_frame(fr, frame_id) == false) {
    out->send_error(JDWP_Error_INVALID_FRAMEID);
    return;
  }
  if (fr.is_entry_frame()) {
    // can't be, must be a javaframe at this point
    out->send_error(JDWP_Error_INVALID_FRAMEID);
    return;
  }
  // Create a buffered output stream so we can asynchronously send an error
  // Calculate the size based on half of the items being 'longs'
  if (!is_setter) { 
    out->write_int(num_values);
  }
        
  for (i=0; i < num_values; i++) {
    bool ret;
    jint slot = in->read_int();
    char tag  = in->read_byte();
    jboolean bogus_object = false;
    Frame fr(&thread);
    get_frame(fr, frame_id);
    JavaFrame jf = fr.as_JavaFrame();
    if (is_setter) {
      ret = read_local_to_address(in, out, &jf, slot, tag);
    } else {
      switch (tag) {
      case JDWP_Tag_OBJECT:
      case JDWP_Tag_ARRAY:
      case JDWP_Tag_STRING:
      case JDWP_Tag_THREAD:
      case JDWP_Tag_CLASS_OBJECT:
        {
#if ENABLE_ISOLATES
          TaskGCContext tmp(thread().task_id());
#endif
          int map_length;
          TypeArray::Raw map = jf.generate_stack_map(map_length);
          int t = map().byte_at(slot);
          if (t != obj_tag) {
            // The Debugger thinks there is an objects in this slots, but the
            // verifier maps tell us it contains a bogus value (and may have been 
            // clobbered during GC, so we can only treat it as an int when writing
            // the value back to the Debugger.
            //
            // Note that this is not necessary in read_local_to_address() --
            // any "object" written to such bogus slots would be treated as
            // ints by the VM.
            //
            // tty->print_cr("[%d] expected obj_tag (%d) but got %d", slot,obj_tag,t);
            bogus_object = true;
          }
        }
        break;
      }

      ret = write_local_from_address(out, &jf, slot, tag, true, bogus_object);
    }

#ifdef AZZERT            
    if (TraceDebugger) { 
        UsingFastOops fast_oops;
        //Method::Fast m  = jf.method();
        //InstanceClass::Fast holder = m().holder(&thread);
        jvm_fprintf(stdout, "    Local #%d = ", (jint)slot);
        StackFrameImpl::print_local_from_address(&jf, slot, tag, bogus_object);
        tty->print_cr("");
    }
#endif
    if (!ret) {
      // Had an error of some sort, send the error packet
      break;
    }
  }
  out->send_packet();
}

void StackFrameImpl::stack_frame_get_values(PacketInputStream *in, 
                                 PacketOutputStream *out)
{
    stack_frame_getter_setter(in, out, false);
}

void StackFrameImpl::stack_frame_set_values(PacketInputStream *in, 
                                 PacketOutputStream *out)
{
    stack_frame_getter_setter(in, out, true);
}

jboolean StackFrameImpl::get_frame(Frame& fr, jint frame_id) {

  while (true) {
    if (fr.is_entry_frame()) {
      EntryFrame e = fr.as_EntryFrame();
      if (e.is_first_frame()) {
        return false;
      }
      e.caller_is(fr);
    } else if (fr.is_java_frame()) {
      JavaFrame jf = fr.as_JavaFrame();
      // frame id's are 1 based
      if (--frame_id == 0) {
        return true;
      }
      jf.caller_is(fr);
    }
  }
  return false;
}

void *StackFrameImpl::stack_frame_cmds[] = { 
  (void *)0x3
  ,(void *)StackFrameImpl::stack_frame_get_values
  ,(void *)StackFrameImpl::stack_frame_set_values
  ,(void *)JavaDebugger::nop  // StackFrame_ThisObject
};

#endif
