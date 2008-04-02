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

// Utility class for registering and handling finalizers.

class FinalizerConsDesc: public ObjArrayDesc {
 private:
  enum {
    ReferentOffset = 0,
    NextOffset     = 1,
    Size           = 2
  };

  OopDesc** obj_at_addr(int index) {
    OopDesc** base = (OopDesc**)((address)this + ArrayDesc::header_size());
    return base + index;
  }

  // Raw functions during GC
  OopDesc** referent_addr()           { return obj_at_addr(ReferentOffset); }
  FinalizerConsDesc** next_addr()     { return (FinalizerConsDesc**) obj_at_addr(NextOffset); }

  // Referent (finalizer reachable object)
  OopDesc* referent()                 { return *referent_addr(); }
  void set_referent(OopDesc* r)       { oop_write_barrier(referent_addr(), r); }

  // Link to next element in list
  FinalizerConsDesc* next()           { return *next_addr(); }
  void set_next(FinalizerConsDesc* n) { oop_write_barrier((OopDesc**) next_addr(), n); }

  void run_finalizer(void);

  friend class ObjectHeap;
};
