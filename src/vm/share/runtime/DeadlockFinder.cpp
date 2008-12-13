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

#include "incls/_precompiled.incl"
#include "incls/_DeadlockFinder.cpp.incl"

#ifndef PRODUCT

#define STACK_LOCK_SIZE (StackLock::size() + 4)

// as those routines can be called from the signal handler,
// (i.e. by pressing Ctrl-\) allocation routines used here
// should be signal safe
static inline void* sigsafe_malloc(size_t size) {
  return GlobalObj::malloc_bytes(size);
}
static inline void sigsafe_free(void* ptr) {
  GlobalObj::free_bytes(ptr);
}

// This class describes what is the thread waiting for;
// we store raw pointers as all those structures are kept in the native heap
// for short time without GC
class BlockedThread {
public:
  BlockedThread(OopDesc* thread, StackLock* sl) : thread(thread), sl(sl) {
    marked_by = 0;
    next = list;
    list = this;
  }

  void* operator new(size_t size){
    return sigsafe_malloc(size);
   }
  
  void operator delete(void* p) {
    sigsafe_free(p);
  }

  BlockedThread* waits_for() {
    for (BlockedThread* bt = list; bt != NULL; bt = bt->next) {
      if (bt->thread == sl->thread()) {
        return bt;
      }
    }
    return NULL;
  }
  
  OopDesc* thread;     // Thread that is blocked
  StackLock* sl;       // StackLock object which has thread among its waiters
  int marked_by;       // The number of component/loop the thread belongs to
  BlockedThread* next; // To keep list structure

  static BlockedThread* list;
};

BlockedThread* BlockedThread::list = NULL;

static void process_frame(JavaFrame* jf) {
  for (int i = 0; i < jf->stack_lock_length(); i += STACK_LOCK_SIZE) {
    StackLock* lock = jf->stack_lock_at(i);

    if (lock->owner() == NULL) {
      continue;
    }
    
    int raw_near = 
      *(int*)((address)lock + 
              StackLock::copied_near_offset() + 
              JavaNear::raw_value_offset());
    // only if has meaningful waiters
    if (raw_near & 2) {
      Thread::Raw wt;
      wt = lock->waiters();
      while (!wt.is_null()) {
        new BlockedThread(wt.obj(), lock);
        wt = wt().next();
      }
    }
  }
}

/* XXXX I don't think this works correctly.  What if we have this:
 *   synchronized(Y) {
 *     synchronized(X) {
 *         X.wait();
 *     }
 *   }
 *
 * Seems like the thread waiting for X owns Y lock.  Suppose another 
 * thread is waiting for Y lock like:
 *   synchronized (X) {
 *     synchronized(Y) {
 *      ...
 *
 * Seems like deadlock to me but this loop doesn't process this thread since
 * it appears on the list of threads 'waiting'
 */

static void process_thread(Thread* t) {
  if (t->last_java_sp() == 0) {
    // not executing in VM
    return;
  }
  Thread::Raw wt, thr;
  // if thread is waiting on condition it cannot be part of the deadlock
  // and waiters field has different sematics, so we ignore such threads
  wt = Universe::scheduler_waiting();
  while (!wt.is_null()) {
    thr = wt.obj();
    while (!thr.is_null()) {
      if (thr.obj() == t->obj()) {
        return;
      }
      thr = thr().next();
    }
    wt = wt().next_waiting();
  }

  Frame fr(t);
  while (true) {
    if (fr.is_entry_frame()) {
      if (fr.as_EntryFrame().is_first_frame()) {
        break;
      }
      fr.as_EntryFrame().caller_is(fr);
    } else {
      JavaFrame jf = fr.as_JavaFrame();
      process_frame(&jf);
      jf.caller_is(fr);
    }
  }
}

static void print_loop(BlockedThread* start, Stream* out) {
  BlockedThread* bt = start;
  out->cr();
  out->print_cr("DEADLOCK:");
  do {
    Thread::Raw thread = bt->thread;
    out->print("  ");
    thread().print_value_on(out);
    Oop::Raw o = bt->sl->owner();
    out->print_cr(" trying to lock object = 0x%x, ", o.obj());    
    out->print("    [");
    o().print_value_on(out);       
    out->print("]");
    bt = bt->waits_for();
    if (bt != start) {
      out->print(" which is held by "); 
    } else {
      out->print(" held by thread %p", bt->thread);
    }
    out->cr();
    GUARANTEE(bt->marked_by == start->marked_by,
              "Should belong to the same deadlock loop");
  } while (bt != start);
}

static int follow_blocked_thread(BlockedThread* bt, int marker, Stream* out) {
  for (; bt != NULL; bt = bt->waits_for()) {
    if (bt->marked_by == marker) {
      print_loop(bt, out);
      return 1; // deadlock found
    }
    bt->marked_by = marker;
  }
  return 0; // no loop found
}

int DeadlockFinder::find_and_print_deadlocks(Stream* out) {
  static int in_progress = 0;

  if (in_progress) {
    return 0;
  }
  in_progress = 1;
    
  // Create a graph of blocked threads with corresponding StackLocks
  for (Thread::Raw t = Universe::global_threadlist(); t.not_null();
       t = t().global_next()) {
    process_thread(&t);
  }

  // Walk through the graph searching for connected components
  int deadlock_count = 0;
  for (int marker = 1; BlockedThread::list != NULL; marker++) {
    BlockedThread* bt = BlockedThread::list;
    if (bt->marked_by == 0) {
      deadlock_count += follow_blocked_thread(bt, marker, out);
    }
    BlockedThread::list = bt->next;
    delete bt; // each node is visited only once, therefore no longer needed
  }

  in_progress = 0;
  return deadlock_count;
}

#undef STACK_LOCK_SIZE

#endif
