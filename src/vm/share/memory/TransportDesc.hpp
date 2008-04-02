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

class TransportDesc : public MixedOopDesc {
#if ENABLE_JAVA_DEBUGGER
public:
  ReturnOop            _next;
  address              _ops;
  int                  _task_id;
  int                  _flags;
  int                  _connection_confirmed;
#endif
protected:
  static jint header_size() { return sizeof(TransportDesc); }
  static int pointer_count() { return 1;}

private:
  static size_t allocation_size() {
    return align_allocation_size(header_size());
  }

  // Initializes the object after allocation
  void initialize(OopDesc* klass) {
    OopDesc::initialize(klass);
  }

  friend class Transport;
  friend class FarClassDesc;
  friend class Universe;
  friend class OopDesc;
};
