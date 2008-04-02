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
# include "incls/_JavaDebugger.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

int JavaDebugger::_loop_count = 0;
bool JavaDebugger::_debugger_option_on = false;
bool JavaDebugger::_debug_isolate_option_on = false;
bool JavaDebugger::_debug_main_option_on = false;
bool JavaDebugger::_suspend_option = true;

int _method_index_base = 0;

// Debugging operations

HANDLE_CHECK(LineVarTable, is_obj_array())
HANDLE_CHECK(LineNumberTable, is_type_array())
HANDLE_CHECK(LocalVariableTable, is_type_array())

#ifdef AZZERT

/** Determine if breakpoint being inserted at legal offset.

    Debug routine to check to see if we are putting a breakpoint at a legal location
*/
bool JavaDebugger::is_legal_offset(Method *m, jlong offset)
{
  if (offset >= (long)m->code_size()) {
    return false;
  }

#if NOT_CURRENTLY_USED
  int index = 0;
  while (index < offset) {
    Bytecodes::Code token = m->bytecode_at(index);
    
    switch(token) {

    case TABLESWITCH: {
      jint low, high;
      index = ((index + 4) & ~3);
      low = get_native_aligned_int(index + 4);
      high = get_native_aligned_int(index + 8);
      index += 12 + (high - low + 1) * 4;
      break;
    }

    case LOOKUPSWITCH: {
      jint pairs;
      index = (index + 4) & ~3;
      pairs = get_native_aligned_int(index + 4);
      index += 8 + pairs * 8;
      break;
    }
    
    case WIDE:
      index += ((m->bytecode_at(index + 1) == Bytecodes::_iinc) ? 6 : 4);
      break;

    default:
      index += Bytecodes::length_for(token);
      break;
    }
  }
  return (index == offset);
#endif
  return true;
}

void JavaDebugger::print_class(JavaClass* jc) {
  UsingFastOops fast_oops;

  Symbol::Fast class_name = jc->name();
  if (class_name.is_null()) {
    tty->print("<Unknown class type>");
  } else {
    class_name().print_symbol_on(tty, true);
  } 
}

void JavaDebugger::print_field(Field* f) {
  UsingFastOops fast_oops;

  Symbol::Fast field_name = f->name();
  if (field_name.is_null()) {
    tty->print("<Unknown field type>");
  } else {
    field_name().print_symbol_on(tty, true);
  } 
}
#endif /* AZZERT */


void JavaDebugger::set_loop_count(int val)
{
  _loop_count += val;
}

int JavaDebugger::get_loop_count()
{
  return _loop_count;
}

int JavaDebugger::_next_seq_num = 1;

void 
JavaDebugger::nop(PacketInputStream *in, PacketOutputStream *out)
{
  unsigned char cmd = in->cmd() /*side effect*/; (void)cmd;

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Unknown Command, %d/%d", in->cmd_set(), in->cmd());
  }
#endif

  out->send_error(JDWP_Error_NOT_IMPLEMENTED);
}

jbyte JavaDebugger::get_jdwp_tag(Oop *p)
{
  if (p == NULL || p->is_null()) {
    return JDWP_Tag_OBJECT;
  } else {
    JavaClass jc = p->blueprint();
    if (p->is_type_array() || p->is_obj_array()) { 
      return JDWP_Tag_ARRAY;
    }
    if (p->is_instance()) {
      if (jc.is_subclass_of(Universe::string_class())) {
        return JDWP_Tag_STRING;
      } else if (jc.is_subclass_of(Universe::thread_class())) { 
        return JDWP_Tag_THREAD;
      } else if (jc.is_subclass_of(Universe::java_lang_Class_class())) { 
        return JDWP_Tag_CLASS_OBJECT;
      }
    } else if (p->is_jvm_thread()) {
      return JDWP_Tag_OBJECT;
    } else if (jc.equals(Universe::string_class())) {
      return JDWP_Tag_STRING;
    } else if (jc.equals(Universe::thread_class())) { 
      return JDWP_Tag_THREAD;
    } else if (jc.equals(Universe::java_lang_Class_class())) { 
      return JDWP_Tag_CLASS_OBJECT;
    }
  }
  return JDWP_Tag_OBJECT;
}

//  Given a JavaClass return one of three JDWP tagtypes

jbyte JavaDebugger::get_jdwp_tagtype(JavaClass* ic) 
{

  if (ic->is_type_array_class() || ic->is_obj_array_class()) {
    return JDWP_TypeTag_ARRAY;
  } else if (ic->access_flags().is_interface()) {
    return JDWP_TypeTag_INTERFACE;
  } else { /* is a class */
    return JDWP_TypeTag_CLASS;
  }
}

// Given a BasicType, return the corresponding signature byte
 
jbyte JavaDebugger::get_tag_from_type(jint t)
{
  switch (t) {
  case T_BYTE:
    return JVM_SIGNATURE_BYTE;
  case T_CHAR:
    return JVM_SIGNATURE_CHAR;
  case T_DOUBLE:
    return JVM_SIGNATURE_DOUBLE;
  case T_FLOAT:
    return JVM_SIGNATURE_FLOAT;
  case T_INT:
    return JVM_SIGNATURE_INT;
  case T_LONG:
    return JVM_SIGNATURE_LONG;
  case T_SHORT:
    return JVM_SIGNATURE_SHORT;
  case T_BOOLEAN:
    return JVM_SIGNATURE_BOOLEAN;
  case T_VOID:
    return JVM_SIGNATURE_VOID;
  case T_OBJECT:
    return JVM_SIGNATURE_CLASS;
  case T_ARRAY:
    return JVM_SIGNATURE_ARRAY;
  default:
    return (jbyte)-1;
  }
}

// Return the load status of a given JavaClass

jint JavaDebugger::get_jdwp_class_status(JavaClass *jc)
{
  jint ret = 0;

  if (jc->is_array_class() || jc->is_interface()) {
    return JDWP_ClassStatus_INITIALIZED;
  }
  jint is_verified = 0;

#if ENABLE_ISOLATES
  SETUP_ERROR_CHECKER_ARG;
  JavaClassObj::Raw m = jc->get_or_allocate_java_mirror(JVM_SINGLE_ARG_CHECK_0);
  ClassInfo::Raw info = jc->class_info();
  is_verified = info().is_verified() ? JavaClassObj::VERIFIED : 0;
#else
  JavaClassObj::Raw m = jc->java_mirror();
#endif
  jint status = m().status() | is_verified;
  if (status & JavaClassObj::ERROR_FLAG) {
    return JDWP_ClassStatus_ERROR;
  }
  if (status & JavaClassObj::VERIFIED) {
    ret |= (JDWP_ClassStatus_VERIFIED | JDWP_ClassStatus_PREPARED);
  }
  if (status & JavaClassObj::INITIALIZED) {
    ret |= JDWP_ClassStatus_INITIALIZED;
  }
  return ret;
}

// Following routines maintain a mapping between objects and object ID's that
// are returned to the debugger

int JavaDebugger::object_hash_code(Oop *p) {

  unsigned int lastHash = (unsigned int)p->obj();
  int result = 0;

  do {
    lastHash = lastHash * 0xDEECE66DL + 0xB;
    result = lastHash & ~3;
  } while (result == 0);
  return ((unsigned int)result) % JavaDebugger::HASH_SLOT_SIZE;
}

int JavaDebugger::hash_id(int id) {
  return ((unsigned int)id) % JavaDebugger::HASH_SLOT_SIZE;
}

int JavaDebugger::next_seq_num() {
  return _next_seq_num++ ;
}

void JavaDebugger::release_id(int object_id) {

  if (object_id == 0)
    return;
  int index = hash_id(object_id);
  Oop::Raw null_oop;
  ObjArray::Raw refnodes = Universe::objects_by_id_map();
  RefNode::Raw node = refnodes().obj_at(index);
  while (node.not_null() && node().ref_obj() != NULL) {
    if (object_id == node().seq_num()) {
      node().set_seq_num(0);
      node().set_ref_obj(&null_oop);
      return;
   }
    node = node().next_by_id();
  }
}

ReturnOop JavaDebugger::get_thread_by_id(int threadID) {

  ThreadObj::Raw t = get_object_by_id(threadID);
  if (t.is_null()) {
    return NULL;
  } else {
    return (t().thread());
  }
}

int JavaDebugger::get_thread_id_by_ref(Thread *t) {

  UsingFastOops fast_oops;

  if (t->is_null()) {
    return 0;
  }
  ThreadObj::Fast tobj = t->thread_obj();    // get Java thread object
  return get_object_id_by_ref(&tobj);
}

ReturnOop JavaDebugger::get_object_by_id(int objectID) {

  int index = hash_id(objectID);
  ObjArray::Raw refnodes = Universe::objects_by_id_map();
  RefNode::Raw node = refnodes().obj_at(index);
  while (node.not_null()) {
    if (objectID == node().seq_num()) {
      return node().ref_obj();
    }
    node = node().next_by_id();
  }
  return NULL;
}

int JavaDebugger::get_object_id_by_ref_nocreate(Oop *p) {

  ObjArray::Raw refnodes = Universe::objects_by_ref_map();
  int index = object_hash_code(p);
  RefNode::Raw node = refnodes().obj_at(index);
  while (node.not_null()) {
    if (node().ref_obj() == p->obj()) {
      return (node().seq_num());
    }
    node = node().next_by_ref();
  }
  return 0;
}

int JavaDebugger::get_object_id_by_ref(Oop *p) {

  UsingFastOops fast_oops;
  SETUP_ERROR_CHECKER_ARG;

  int index;
  RefNode::Fast node;

  if (p == NULL || p->is_null()) {
    return 0;
  }
  if ((index = get_object_id_by_ref_nocreate(p)) != 0) {
    return index;
  }

  // create new node
  ObjArray::Fast refnodes = Universe::objects_by_ref_map();
  index = object_hash_code(p);
  node = refnodes().obj_at(index);
  {
    TaskAllocationContext tmp(SYSTEM_TASK);
    node = Universe::new_refnode(JVM_SINGLE_ARG_CHECK_0);
  }
  int nextID = JavaDebugger::next_seq_num();
  node().set_seq_num(nextID);
  node().set_ref_obj(p);
  RefNode::Fast rn = refnodes().obj_at(index);
  node().set_next_by_ref(&rn);
  refnodes().obj_at_put(index, &node);
  index = hash_id(nextID);
  refnodes = Universe::objects_by_id_map();
  rn = refnodes().obj_at(index);
  node().set_next_by_id(&rn);
  refnodes().obj_at_put(index, &node);
  return nextID;
}

// Flush and RefNodes that point to dead objects

void JavaDebugger::flush_refnodes()
{
  GUARANTEE(ObjectHeap::is_gc_active(),
            "flush_refnodes Must be called during GC");

  ObjArrayDesc *refMap =
    (ObjArrayDesc*)(Universe::objects_by_ref_map()->obj());

  RefNodeDesc *node;
  OopDesc **p;
  int i;
  int len;

  OopDesc ** refnodes = (OopDesc **)((address)refMap + sizeof(ArrayDesc));
  len = refMap->_length;
  for (i = 0; i < len; i++) {
    node = (RefNodeDesc *)(refnodes[i]);
    while (node != NULL && node->_ref_obj != NULL) {
      p = (OopDesc **)(node->_ref_obj);
      if (ObjectHeap::in_collection_area(p) && !ObjectHeap::test_bit_for(p)) {
        node->_ref_obj = NULL;
      }
      node = (RefNodeDesc *)(node->_next_by_ref);
    }
  }
  ObjArrayDesc *IDMap =
    (ObjArrayDesc*)(Universe::objects_by_id_map()->obj());
  OopDesc ** IDnodes = (OopDesc **)((address)(IDMap) + sizeof(ArrayDesc));
  len = IDMap->_length;
  for (i = 0; i < len; i++) {
    node = (RefNodeDesc *)(IDnodes[i]);
    while (node != NULL && node->_ref_obj != NULL) {
      p = (OopDesc **)(node->_ref_obj);
      if (ObjectHeap::in_collection_area(p) && !ObjectHeap::test_bit_for(p)) {
        node->_ref_obj = NULL;
      }
      node = (RefNodeDesc *)(node->_next_by_id);
    }
  }
}

// removes any RefNode in a linked list of nodes that have a null
// ref_obj.  I.e. the object that this RefNode was mapping has
// been collected by GC so the debugger doesn't need to know about it anymore
ReturnOop JavaDebugger::compress_chain(RefNode *node, bool by_ref) {
  //Find the first non-null node to use as the head
  Oop::Raw ref_obj = node->ref_obj();
  RefNode::Raw head_node = node->obj();
  while(ref_obj.is_null()) {
    head_node = head_node().next(by_ref);
    if (head_node.is_null()) {
      // no active nodes in this chain
      return head_node;
    }
    ref_obj = head_node().ref_obj();
  }
  RefNode::Raw prevNode = head_node.obj();
  RefNode::Raw nextNode = head_node().next(by_ref);
  while(!nextNode.is_null()) {
    ref_obj = nextNode().ref_obj();
    if (ref_obj.is_null()) {
      nextNode = nextNode().next(by_ref);
      prevNode().set_next(&nextNode, by_ref);
    } else {
      prevNode = nextNode.obj();
      nextNode = nextNode().next(by_ref);
    }
  }
  return head_node;
}

void JavaDebugger::rehash() {

  // Presumably at this point, all the buckets and objects they point
  // to have been properly collected and updated.  Now we need to step through
  // each entry in the maps and re-hash based on the new object address.

  UsingFastOops fast_oops;
  ObjArray::Fast refMap = Universe::objects_by_ref_map();
  RefNode::Fast head_node, node, prevNode, nextNode;
  Oop::Raw ref_obj;
  int len = refMap().length();
  int i;
  int index;

  // First we cycle through the maps and remove any RefNodes
  // with a ref_obj == NULL.  Theses were set to null above

  for (i = 0; i < len; i++) {
    node = refMap().obj_at(i);
    if (node.is_null()) {
      continue;
    }
    head_node = compress_chain(&node, true);
    refMap().obj_at_put(i, &head_node);
  }
  refMap = Universe::objects_by_id_map();
  for (i = 0; i < len; i++) {
    node = refMap().obj_at(i);
    if (node.is_null()) {
      continue;
    }
    head_node = compress_chain(&node, false);
    refMap().obj_at_put(i, &head_node);
  }

  // At this point there are no 'null' ref_obj entries in the maps
  // we can now rehash the object references
  refMap = Universe::objects_by_ref_map();
  for (i = 0; i < len; i++) {
    // The first entry is a special case, we iterate until we either run out of
    // nodes or we get a node that hashes to this same index.
    node = refMap().obj_at(i);
    do {
      if (node.not_null()) {
        Oop o = node().ref_obj();
        index = object_hash_code(&o);
        if (index != i) {
          // This node doesn't hash to this entry.
          // First, remove this node from this chain
          nextNode = node().next_by_ref();
          refMap().obj_at_put(i, &nextNode);
          // Now set the next pointer in this node to point to the head of the
          // chain it is going to be inserted in.
          nextNode = refMap().obj_at(index);
          node().set_next_by_ref(&nextNode);
          // Finally, put this node into its new chain
          refMap().obj_at_put(index, &node);
          // The head of the chain is the next one to look at since we
          // replaced it 3 code lines above.
          node = refMap().obj_at(i);
        } else {
          // this node maps to this chain, break out and continue checking
          // the rest of the chain
          break;
        }
      }
    } while (node.not_null());
    
    if (node.not_null()) {
      // If we got here then the top node in the chain is set so we start
      // with the next one
      prevNode = node;
      node = node().next_by_ref();
      while (node.not_null()) {
        Oop o = node().ref_obj();
        index = object_hash_code(&o);
        if (index == i) {
          // We're lucky, it hashes to the same entry
          prevNode = node;
          node = node().next_by_ref();
        } else {
          // unlink this node from this chain
          nextNode = node().next_by_ref();
          prevNode().set_next_by_ref(&nextNode);
          // insert node into the new chain
          nextNode = refMap().obj_at(index);
          node().set_next_by_ref(&nextNode);
          refMap().obj_at_put(index, &node);
          // get the next node in the original chain. prevNode is ok as it
          // points to the node before the one just removed.
          node = prevNode().next_by_ref();
        }
      }
    }
  }
}

void JavaDebugger::process_suspend_policy(jbyte policy, Thread *thread,
                                          int task_id, jboolean forceWait) {

  switch(policy) {
  case JDWP_SuspendPolicy_NONE:
    /* do nothing */
    dispatch(0);
    break;
  case JDWP_SuspendPolicy_EVENT_THREAD:
    GUARANTEE(thread != NULL, "Thread must be specified");
#if ENABLE_ISOLATES
    task_id = thread->task_id();
#endif
    ThreadReferenceImpl::suspend_specific_thread(thread, task_id, true);
    dispatch(0);
    break;
  case JDWP_SuspendPolicy_ALL:
    ThreadReferenceImpl::suspend_all_threads(task_id, true);
    if (forceWait) {
      process_command_loop();
    }
    break;
  }
}

int JavaDebugger::get_method_index(const Method *method) {

  jlong methodID = get_method_id(method);
  return (int)(methodID & 0xFFFFFFFF);
}

jlong JavaDebugger::get_method_id(const Method *method) {

  InstanceClass::Raw clazz = method->holder();
  return (get_method_id(&clazz, method));
}

jlong JavaDebugger::get_method_id(InstanceClass *ic, const Method *method) {

  ObjArray::Raw methods = ic->methods();
  int numMethods = methods().length();
  int i;
  for (i = 0; i < numMethods; i++) {
    if (*method == methods().obj_at(i))
      break;
  }
  if (i == numMethods) {
    // This may be a rom_debug_method, iterate through all the event requests
    // to see if we can find it
    VMEventStream es;
    VMEvent::Raw ep;
    LocationModifier::Raw loc;
    Method::Raw dm;

    while (!es.at_end()) {
      ep = es.next_by_kind(JDWP_EventKind_BREAKPOINT);
      if (ep.is_null()) {
        return -1;
      }
      loc = VMEvent::get_modifier(&ep,
         JDWP_EventRequest_Set_Out_modifiers_Modifier_LocationOnly);
      if (loc.not_null()) {
        dm = loc().rom_debug_method();  // get potential rom method
        if (!dm.is_null() && dm.obj() == method->obj()) {
          return loc().method_id();
        }
      }
      // no match, try next event
    }
    return -1;
  } else {
    return ((jlong)((jlong)get_object_id_by_ref(ic) << 32) | (jlong)(i + _method_index_base));
  }
}

ReturnOop JavaDebugger::get_method_by_id(InstanceClass *clazz, jlong methodID) {

  UsingFastOops fastoops;

  if (clazz->is_null()) {
    return NULL;
  }
  InstanceClass::Fast ic =
    get_object_by_id((int)((methodID >> 32) & 0xFFFFFFFF));
  int index = (int)(methodID & 0xFFFFFFFF);
  ObjArray::Raw methods = ic().methods();
  if (methods.is_null() ||
      (index - _method_index_base) >= methods().length()) {
    return NULL;
  }
  GUARANTEE(clazz->is_subclass_of(&ic), "Method holder not subclass");
  return methods().obj_at(index - _method_index_base);
}

void JavaDebugger::send_all_class_prepares()
{
#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("Class Prepare ... All Classes");
  }
#endif

  if (NOTIFY_WANTED(Dbg_EventKind_CLASS_PREPARE)) {
    UsingFastOops fast_oops;
    // check for any queued up events 
    /*
    TypeArray::Fast queue = Universe::vmevent_class_prepare_queue();
    JavaClass::Fast jc;
    for (int i = 0; i < queue().length; i++) {
      jc = Universe::class_from_id(i);
      if (!jc().is_fake_class()) {
        VMEvent::new_class_prepare_event(&jc);
      }
    }
    */
    int num_classes = Universe::number_of_java_classes();
    int rom_classes = ROM::number_of_system_classes();
    JavaClass::Fast jc;
    for (int i = rom_classes; i < num_classes; i++) {
      jc = Universe::class_from_id(i);
      if (!jc().is_fake_class()) {
                VMEvent::class_prepare_event(&jc);
      }
    }
  }
}

// Call back to the debug agent to get information on the next possible
// line to single step to

void JavaDebugger::vendor_get_stepping_info(PacketInputStream *in, 
                            PacketOutputStream *out)
{
  UsingFastOops fast_oops;

  StepModifier *sm = (StepModifier *)(in->read_int());

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("KVM Commands Get Stepping Info", NULL);
  }
#endif

  sm->set_step_target_offset(in->read_long());
  sm->set_dup_current_line_offset(in->read_long());
  sm->set_post_dup_line_offset(in->read_long());
  out->send_packet();
  set_loop_count(-1);
}

// Initialization hand-shake from/to debug agent

void 
JavaDebugger::vendor_hand_shake(PacketInputStream *in,
                                PacketOutputStream *out) {

  String versionString;
  int minor;
  const char *kvmString = "KVM Reference Debugger Interface";

#ifdef AZZERT
  if (TraceDebugger) {
    tty->print_cr("KVM Commands Handshake");
  }
#endif

  versionString =  in->read_string();
  /*major=*/ in->read_byte();
  minor  =   in->read_byte();
  {
    Transport::Raw t = in->transport();
    if (minor < KDP_REQUIRED_MINOR /* 4 */) {
      close_java_debugger(&t);
      tty->print_cr("VM requires newer Debug Agent; minor version >= 4");
    } else {
      t().set_connection_confirmed(1);
    }
  }
  out->write_raw_string(kvmString);
  int base_mode = 0x7fff;
  // this version of debug agent indexes methods starting at 1
  _method_index_base = 1;
  base_mode |= KVM_METHOD_BASE_ONE;
  // NOTE: bit 16-15 == 0x10, method indexes start at 1
  // Debug agent can handle VM that contains line number table
  // Inform agent of same.
  // NOTE: bits 18-17 == 0x10, VM has line number table flag
  base_mode |= KVM_VERSION_3;
  out->write_int(base_mode);  // old debug agent, index starts at 0
  out->send_packet();
}

void
JavaDebugger::get_line_num_table(PacketInputStream *in,
                                 PacketOutputStream *out) {
#if ENABLE_ROM_JAVA_DEBUGGER
  UsingFastOops fast_oops;
  InstanceClass::Fast ic = in->read_class_ref();
  Method::Fast m = get_method_by_id(&ic, in->read_long());
  if (m.is_null() || (ROM::system_contains(m.obj()) && !_rom_is_debuggable)) {
    out->write_int(0);
    out->send_packet();
    return;
  }
  jushort code_size = m().code_size();
  LineVarTable::Fast lvt =  m().line_var_table();
  LineNumberTable::Fast line_num_table;
  if (!lvt.is_null()) {
    line_num_table = lvt().line_number_table();
  }

  if (lvt.is_null() || line_num_table.is_null()) {
    out->write_int(0);
    out->send_packet();
    return;
  }
  // each entry is two shorts
  int num_entries = line_num_table().count();
  // each entry is two shorts

  out->write_int(num_entries);   // number of line_number_table entries
  out->write_long(0);  // start of code
  out->write_long(jlong(code_size) - 1); // last code offset
  for (int i = 0; i < num_entries; i++) {
    out->write_short(line_num_table().pc(i));
    out->write_short(line_num_table().line_number(i));
  }
  out->send_packet();
#else
  out->write_int(0);
  out->send_packet();
  return;
#endif
}

void JavaDebugger::get_local_var_table(PacketInputStream *in,
                                       PacketOutputStream *out) {
#if ENABLE_ROM_JAVA_DEBUGGER
  UsingFastOops fast_oops;
  InstanceClass::Fast ic = in->read_class_ref();
  Method::Fast m = get_method_by_id(&ic, in->read_long());
  if (m.is_null() || (ROM::system_contains(m.obj()) && !_rom_is_debuggable)) {
    out->write_int(0);
    out->send_packet();
    return;
  }
  LineVarTable::Fast lvt =  m().line_var_table();
  LocalVariableTable::Fast local_var_table;
  if (!lvt.is_null()) {
    local_var_table = lvt().local_variable_table();
  }
  if (lvt.is_null() || local_var_table.is_null()) {
    out->write_int(0);
    out->send_packet();
    return;
  }
  // each entry is three shorts
  int num_entries = local_var_table().count();
  out->write_int(num_entries);   // number of local_var_table entries
  for (int i = 0; i < num_entries; i++) {
    out->write_short(local_var_table().start_pc(i));
    out->write_short(local_var_table().code_length(i));
    out->write_short(local_var_table().slot(i));
  }
  out->send_packet();
#else
  out->write_int(0);
  out->send_packet();
  return;
#endif
}

// Main command processing loop which checks for data on a transport
// and vectors off to the appropriate command processing function

bool JavaDebugger::dispatch(int timeout)
{
  UsingFastOops fastoops;
  void **f2Array;
  command_handler func;
  Transport::Fast t, next_t;
  Transport::transport_op_def_t *ops;
  Oop::Fast current_exception;
  bool processed_cmd = false;

  t = Universe::transport_head();
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    // We don't want the debugger code to be concerned with 
    // any exception that may have been posted.  If we're out of
    // memory totally then we may not be able to communicate
    // with the debugger.  That should be rare.  Trying to debug a Java
    // program with the heap at capacity is probably a 'bad thing to do'
    // anyway.  
    current_exception = Thread::current_pending_exception();
    Thread::clear_current_pending_exception();
  }
  if (t.is_null() &&_debugger_option_on &&
      (!is_debug_isolate_option_on() || is_debug_main_option_on())) {
    SETUP_ERROR_CHECKER_ARG;
    // No existing transport but option is on, we probably disconnected
    // check to see if another connection attempt happened
    bool connected = initialize_java_debugger(JVM_SINGLE_ARG_NO_CHECK);
    if (connected) {
      t = Universe::transport_head();
    } else {
        // No connection yet.
        return false;
    }
  }
  while (!t.is_null())  {
    UsingFastOops fastoops2;
    next_t = t().next();
    ops = t().ops();
    if (_debugger_option_on && !ops->initialized(&t)) {
#if ENABLE_ISOLATES
      // Only want to connect if current thread is correct one for this 
      // transport
      if (t().task_id() != Thread::current()->task_id()) {
        // Try next one
        t = next_t.obj();
        continue;
      }
#endif
      // VM started with -nosuspend and we haven't connected with a debugger
      // on this transport.  Try to see if a connection attempt has come in
      bool connected = ops->connect_transport(&t, Transport::SERVER, 0);
      if (connected) {
        sync_debugger(&t);
      } else {
        t = next_t.obj();
        // No connection yet.  Don't check any subsequent transports
        // since we always want to connect the first transport to the first
        // debugger, not the first debugger to the second (or later) transport.
        break;
      }
    }
#ifdef AZZERT
    if (TraceDebugger) {
      if (!ops->char_avail(&t, 0)) {
        //        tty->print_cr("dispatch: no char avail");
      }
    }
#endif
    while(ops->char_avail(&t, timeout)) {
      PacketInputStream in(&t);
      int bytes_avail;
      int dummy;
      bytes_avail = ops->peek_bytes(&t, &dummy, sizeof(dummy));
      if (bytes_avail == 0) {
        // other side closed connection?
        close_java_debugger(&t);
        break;
      }
      bool result = in.receive_packet(); 
      if (result == false) {
        break;
      } 

      if (in.flags() & FLAGS_REPLY) {
        // It's a reply, just drop it
        break;
      }

      if (in.cmd_set() == KVM_CMDSET) {
        f2Array = (void **)(func_array[0]);
      } else {
        if (in.cmd_set() == 0 || in.cmd_set() > JDWP_HIGHEST_COMMAND_SET) {
          if (TraceDebugger) {
            jvm_fprintf(stderr, "Unknown KDWP command set");
          }
          break;
        }
        f2Array = (void **)(func_array[in.cmd_set()]);
      }
      if (f2Array == NULL || in.cmd() > (int)f2Array[0]) {
        if (TraceDebugger) {
          jvm_fprintf(stderr, "Unknown KDWP command");
        }
        break;
      }
      func = (command_handler)(f2Array[in.cmd()]);
#ifdef AZZERT
      if (TraceDebugger) {
        //        tty->print_cr("dispatch: xprt = 0x%x, id = %d, %d/%d",
        //                      (int)t().obj(), in.id(), in.cmd_set(), in.cmd());
      }
#endif
      int data_len = 0;
#if ENABLE_MEMORY_PROFILER 
      if (in.cmd_set() == 18) { 
        data_len = 3900; 
      }
#endif
      PacketOutputStream out(&t, data_len);
      out.init_reply(in.id());

      //    waitOnSuspend = false;
      func(&in, &out);
      processed_cmd = true;

      if (in.error()) {
        out.send_error(in.error());
      }
      // try to empty the queue if any but don't block
      timeout = 0;
    }
    t = next_t.obj();
  }
  if (!current_exception.is_null()) {
    // restore the saved exception.
    Thread::set_current_pending_exception(&current_exception);
  }
  return processed_cmd;
}

// Loop continuously looking for commands from the debugger.

void JavaDebugger::process_command_loop()
{
  int loop_count = get_loop_count();

  /* wait for some command to come from the debugger */
  while (_debugger_active) {
    JavaDebugger::dispatch(-1);   /* wait forever */
    if (get_loop_count() != loop_count)
      return;
  }
}

bool JavaDebugger::initialize_java_debugger_main(JVM_SINGLE_ARG_TRAPS) {
  // Called to initialize the debugger from JVM.cpp
  bool retval = true;
  if (is_debugger_option_on()) {
    Transport::init_all_transports(JVM_SINGLE_ARG_CHECK_0);
  }
  if (is_debugger_option_on() &&
      (!is_debug_isolate_option_on() || is_debug_main_option_on())) {
    retval = initialize_java_debugger(JVM_SINGLE_ARG_NO_CHECK);
    if (!retval) {
      // init failed, debugger could have killed us during initialization
      // If so need to pass back exit code sent with VM_Exit command
      if (CURRENT_HAS_PENDING_EXCEPTION) {
        tty->print("Uncaught Exception during Debugger initialization: ");
        //        Thread::print_current_pending_exception_stack_trace();
      }
    }
  }
  return retval;
}

void JavaDebugger::initialize_java_debugger_task(JVM_SINGLE_ARG_TRAPS) {

  if (is_debugger_option_on()) {
    (void) JavaDebugger::initialize_java_debugger(JVM_SINGLE_ARG_CHECK);
  }
}

bool JavaDebugger::initialize_java_debugger(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fastoops;
  ObjArray::Fast plist;

  Transport::Fast t;
  if (!CURRENT_HAS_PENDING_EXCEPTION && _debugger_option_on) {
    // Determine which transport to use via command line args, for now
    // only socket
    {
      TaskAllocationContext tmp(SYSTEM_TASK);
      t = Transport::new_transport("socket" JVM_CHECK_0);
    }
    if (!t.is_null()) {
      t().set_task_id(-1);
#if ENABLE_ISOLATES
      {
        // We may be shutting down this task so don't create a transport
        int task_id = TaskContext::current_task_id();
        if (task_id == -1) {
          return false;
        }
        // set transport in the task
        Task::Raw task = Task::current();
        task().set_transport(&t);
        t().set_task_id(task_id);
      }
#endif
      // append to list of transports
      Transport::Raw t_next = Universe::transport_head();
      if (t_next().is_null()) {
        // first one
        *Universe::transport_head() = t.obj();
      } else {
        GUARANTEE(ENABLE_ISOLATES, "More than one transport");
        while (!t_next.is_null()) {
          if (t_next().next() == NULL) {
            break;
          }
          t_next = t_next().next();
        }
        GUARANTEE(!t_next.is_null(), "Null transport pointer");
        t_next().set_next(&t);
      }
      Transport::transport_op_def_t *ops = t().ops();
#ifdef AZZERT
      if (TraceDebugger) {
        tty->print_cr("JavaDebugger: init: transport 0x%x", (int)t().obj());
      }
#endif
      // Setup some free packet buffers
      plist = Universe::packet_buffer_list();
      if (plist.is_null()) {
        TaskAllocationContext tmp(SYSTEM_TASK);
        UsingFastOops fastoops2;
#if ENABLE_MEMORY_PROFILER
        *Universe::packet_buffer_list() =
           Universe::new_obj_array(PacketStream::NUM_PACKET_BUFS + 1 JVM_NO_CHECK);
        //this is special buffer for MemoryProfiler
        Oop::Raw mp_buffer = Universe::new_byte_array(4000 JVM_NO_CHECK);
        Universe::packet_buffer_list()->obj_at_put(PacketStream::NUM_PACKET_BUFS, mp_buffer.obj());
#else
        // no buffers yet, allocate some
        *Universe::packet_buffer_list() =
          Universe::new_obj_array(PacketStream::NUM_PACKET_BUFS JVM_NO_CHECK);
#endif
        if (CURRENT_HAS_PENDING_EXCEPTION) {
          // ran out of memory this soon?  Oh well,
          close_java_debugger(&t);
          return false;
        }
        plist = Universe::packet_buffer_list();
        GUARANTEE(!plist.is_null(), "Null packet buffer list");
        for (int i = 0; i < PacketStream::NUM_PACKET_BUFS; i++) {
          TypeArray::Raw data_buffer =
            Universe::new_byte_array(PacketStream::INITIAL_SEGMENT_SIZE JVM_NO_CHECK);
          if (CURRENT_HAS_PENDING_EXCEPTION) {
            // ran out of memory this soon?  Oh well,
            close_java_debugger(&t);
            return false;
          }
          plist().obj_at_put(i, &data_buffer);
        }
      }
      if (is_suspend()) {
        // Wait for debugger to connect
        ops->connect_transport(&t, Transport::SERVER, -1);
      } else {
        // Don't wait just check
        ops->connect_transport(&t, Transport::SERVER, 0);
      }
      if (sync_debugger(&t) == false) {
        return false;
      }
      return true;
    }
  }
  return false;
}


bool JavaDebugger::sync_debugger(Transport *t)
{
  Transport::transport_op_def_t *ops = t->ops();

  if (!ops->initialized(t)) {
    if (is_suspend()) {
      // Should have made a connection if in suspend mode.  Something failed.
      return false;
    }
    // In no_suspend mode so just return true, we'll connect later;
    // Have your debugger call my debugger to set something up.
    return true;
  }
  _debugger_active = (_debugger_active + 2) | DEBUGGER_ACTIVE;
  // Get the handshake from the proxy
  //  JavaDebugger::dispatch(-1 JVM_CHECK_0);
  VMEvent::vminit(t);
  if (!_debugger_active) {
    // We must have gotten a VM_Exit command during init, return false
    return false;
  }
  return true;
}

void JavaDebugger::close_java_debugger(Transport *t) {

  Transport::transport_op_def_t *ops;

  if (_debugger_active) {
    UsingFastOops fastoops;

    VMEvent::clear_all_events(t);
    VMEvent::vmdeath(t);
    Thread::Fast thread = Universe::global_threadlist();
    int task_id = t->task_id();

    while (thread.not_null()) {
      if (is_valid_thread(&thread)) {
        if (task_id == -1 || thread().task_id() == task_id) {
          thread().clear_dbg_suspended();
        }
      }
      thread = thread().global_next();
    }

    int is_stepping = _debugger_active & DEBUGGER_STEPPING;
    _debugger_active = ((_debugger_active &
                         ~(DEBUGGER_STEPPING | DEBUGGER_ACTIVE)) - 2);
    if ((_debugger_active & ~DEBUGGER_STEPPING) > 0) {
      _debugger_active |= (DEBUGGER_ACTIVE | is_stepping);
    }
    ops = t->ops();
    if (ops->initialized(t)) {
      set_suspend(false);
      ops->disconnect_transport(t);
    }
      
#if ENABLE_ISOLATES
    Transport::Fast transport = Universe::transport_head();
    Transport::Fast prev;
    GUARANTEE(!transport.is_null(), "No debugger transports");
    if (t->equals(transport)) {
      *Universe::transport_head() = transport().next();
    } else {
      while (!transport.is_null() && !t->equals(transport)) {
        prev = transport;
        transport = transport().next();
      }
      GUARANTEE(!transport.is_null(), "can't find correct debugger transport");

      Transport::Fast next = transport().next();
      prev().set_next(&next);
    }
  {
    // clear transport in the task
    Oop::Raw null_oop;
    Task::Raw task = Task::get_task(t->task_id());
    task().set_transport(&null_oop);
  }
#endif
    JVMSPI_DebuggerNotification(KNI_FALSE);
  }
}
  
static const char *_null_transport = "";

extern "C" char *JVM_GetDebuggerTransport() {
  Transport::Raw t = Universe::transport_head();
  if (t.is_null()) {
    return (char*)_null_transport;
  } else {
    Transport::transport_op_def_t *ops = t().ops();
    return ops->name();
  }
}

extern "C" void JVM_ProcessDebuggerCmds() {
  JavaDebugger::dispatch(0);
}

extern "C" jboolean JVM_IsDebuggerActive() {
  return (_debugger_active != 0);
}

extern "C" jboolean JVM_IsDebuggerOptionOn() {
  return JavaDebugger::is_debugger_option_on();
}

// These command sets are not supported or are supported in the debug agent.
// Hence we just vector to the 'nop' command

 void *JavaDebugger::arraytype_cmds[] = { 
  (void *)0x1
  ,(void *)JavaDebugger::nop // newInstance
};

void *JavaDebugger::field_cmds[] = { (void *)0x0 };

void *JavaDebugger::method_cmds[] = { 
  (void *)0x3
  ,(void *)JavaDebugger::nop // Method_LineTable
  ,(void *)JavaDebugger::nop // variable Table
  ,(void *)JavaDebugger::nop // bytecodes
};

void *JavaDebugger::threadgroupreference_cmds[] = { 
  (void *)3
  ,(void *)JavaDebugger::nop  // ThreadGroupReference_Name
  ,(void *)JavaDebugger::nop  // ThreadGroupReference_Parent
  ,(void *)JavaDebugger::nop  // ThreadGroupReference_Children
};

void *JavaDebugger::classloader_reference_cmds[] = { 
  (void *)0x1
  ,(void *)JavaDebugger::nop  // visibleClasses
};

void *JavaDebugger::vendor_commands[] = {
  (void *)5
  ,(void *)JavaDebugger::vendor_hand_shake  // vendor_commands_hand_shake
  ,(void *)JavaDebugger::vendor_get_stepping_info
  ,(void *)JavaDebugger::nop  // command from VM to debug agent
                                // vendor_commands_Stepping_info
  ,(void *)JavaDebugger::get_local_var_table
  ,(void *)JavaDebugger::get_line_num_table
};

// Table of command sets

void **JavaDebugger::func_array[] = { 
  // We use slot 0 for the vendor_commands.  Normally, 0 isn't allowed
  (void **)vendor_commands,
  (void **)VMImpl::virtualmachine_cmds,                 // 1
  (void **)ReferenceTypeImpl::reference_type_cmds,      // 2
  (void **)ClassTypeImpl::class_type_cmds,              // 3
  (void **)arraytype_cmds,                              // 4
  NULL,                                                 // 5
  (void **)method_cmds,                                 // 6
  NULL,                                                 // 7
  (void **)field_cmds,                                  // 8
  (void **)ObjectReferenceImpl::object_reference_cmds,  // 9
  (void **)StringReferenceImpl::string_reference_cmds,  // 10
  (void **)ThreadReferenceImpl::thread_reference_cmds,  // 11
  (void **)threadgroupreference_cmds,                   // 12
  (void **)ArrayReferenceImpl::array_reference_cmds,    // 13
  (void **)classloader_reference_cmds,                  // 14
  (void **)VMEvent::event_request_cmds,                 // 15
  (void **)StackFrameImpl::stack_frame_cmds,            // 16
  (void **)ClassObjectReferenceImpl::class_object_reference_cmds, // 17
#if ENABLE_MEMORY_PROFILER
  (void **)MemoryProfiler::memory_profiler_cmds         // 18
#endif
};
#else //ENABLE_JAVA_DEBUGGER

extern "C" void JVM_ProcessDebuggerCmds() {}

#endif
