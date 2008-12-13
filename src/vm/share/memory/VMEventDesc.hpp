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

class VMEventDesc : public MixedOopDesc {
public:
  ReturnOop _next;
  ReturnOop _send_next;     // chain of events to send to debugger
  ReturnOop _queue_next;
  ReturnOop _mods;
  ReturnOop _transport;

  /* All oops must go before here.  If you change the number of oops, be
   * sure to change pointer_count()
   */

  jbyte _event_kind;
  jbyte _suspend_policy;
  jbyte _padding1;
  jbyte _padding2;
  jint _num_modifiers;
  jint _event_id;
  jint _task_id;

private:
  static size_t allocation_size() {
    return align_allocation_size(sizeof(VMEventDesc));
  }
  static int pointer_count() { return 5; }

  friend class OopDesc;
  friend class VMEvent;
  friend class Universe;
  friend class FarClassDesc;
};
