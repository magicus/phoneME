/*
 *   
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
# include "incls/_JavaDebuggerContext.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

ReturnOop JavaDebuggerContext::allocate(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaDebuggerContext::Fast context = 
    Universe::new_mixed_oop(MixedOopDesc::Type_JavaDebuggerContext,
                            JavaDebuggerContext::allocation_size(),
                            JavaDebuggerContext::pointer_count()
                            JVM_CHECK_0);

  ObjArray::Raw array;
  array = Universe::new_obj_array(JavaDebuggerContext::HASH_SLOT_SIZE 
                                  JVM_OZCHECK(array));
  context().set_objects_by_id_map(&array);
  array = Universe::new_obj_array(JavaDebuggerContext::HASH_SLOT_SIZE 
                                  JVM_OZCHECK(array));
  context().set_objects_by_ref_map(&array);
  
  return context.obj();
}

ReturnOop JavaDebuggerContext::get_object_by_id(int objectID) {

  int index = hash_id(objectID);
  ObjArray::Raw refnodes = objects_by_id_map();
  RefNode::Raw node = refnodes().obj_at(index);
  while (node.not_null()) {
    if (objectID == node().seq_num()) {
      return node().ref_obj();
    }
    node = node().next_by_id();
  }
  return NULL;
}

int JavaDebuggerContext::get_object_id_by_ref_nocreate(Oop *p) {

  ObjArray::Raw refnodes = objects_by_ref_map();
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

int JavaDebuggerContext::get_object_id_by_ref(Oop *p) {

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
  ObjArray::Fast refnodes = objects_by_ref_map();
  index = object_hash_code(p);
  node = Universe::new_refnode(JVM_SINGLE_ARG_CHECK_0);
  int nextID = JavaDebugger::next_seq_num();
  node().set_seq_num(nextID);
  node().set_ref_obj(p);
  RefNode::Fast rn = refnodes().obj_at(index);
  node().set_next_by_ref(&rn);
  refnodes().obj_at_put(index, &node);
  index = hash_id(nextID);
  refnodes = objects_by_id_map();
  rn = refnodes().obj_at(index);
  node().set_next_by_id(&rn);
  refnodes().obj_at_put(index, &node);
  return nextID;
}

// Flush and RefNodes that point to dead objects

void JavaDebuggerContext::flush_refnodes()
{
  GUARANTEE(ObjectHeap::is_gc_active(),
            "flush_refnodes Must be called during GC");

  ObjArray::Raw refMap = objects_by_ref_map();

  RefNodeDesc *node;
  OopDesc **p;
  int i;
  int len;

  OopDesc ** refnodes = (OopDesc **)refMap().base_address();
  len = refMap().length();
  for (i = 0; i < len; i++) {
    node = (RefNodeDesc *)(refnodes[i]);
    while (node != NULL && node->_ref_obj != NULL) {
      p = (OopDesc **)(node->_ref_obj);
      if( ObjectHeap::in_collection_area_unmarked(p) ) {
        node->_ref_obj = NULL;
      }
      node = (RefNodeDesc *)(node->_next_by_ref);
    }
  }
  ObjArray::Raw IDMap = objects_by_id_map();
  OopDesc ** IDnodes = (OopDesc **)IDMap().base_address();
  len = IDMap().length();
  for (i = 0; i < len; i++) {
    node = (RefNodeDesc *)(IDnodes[i]);
    while (node != NULL && node->_ref_obj != NULL) {
      p = (OopDesc **)(node->_ref_obj);
      if( ObjectHeap::in_collection_area_unmarked(p) ) {
        node->_ref_obj = NULL;
      }
      node = (RefNodeDesc *)(node->_next_by_id);
    }
  }
}

void JavaDebuggerContext::rehash() {

  // Presumably at this point, all the buckets and objects they point
  // to have been properly collected and updated.  Now we need to step through
  // each entry in the maps and re-hash based on the new object address.

  UsingFastOops fast_oops;
  ObjArray::Fast refMap = objects_by_ref_map();
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
  refMap = objects_by_id_map();
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
  refMap = objects_by_ref_map();
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

int JavaDebuggerContext::object_hash_code(Oop *p) {

  unsigned int lastHash = (unsigned int)p->obj();
  int result = 0;

  do {
    lastHash = lastHash * 0xDEECE66DL + 0xB;
    result = lastHash & ~3;
  } while (result == 0);
  return ((unsigned int)result) % JavaDebuggerContext::HASH_SLOT_SIZE;
}

int JavaDebuggerContext::hash_id(int id) {
  return ((unsigned int)id) % JavaDebuggerContext::HASH_SLOT_SIZE;
}

// removes any RefNode in a linked list of nodes that have a null
// ref_obj.  I.e. the object that this RefNode was mapping has
// been collected by GC so the debugger doesn't need to know about it anymore
ReturnOop JavaDebuggerContext::compress_chain(RefNode *node, bool by_ref) {
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

#endif
