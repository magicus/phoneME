/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

class CompilationQueueElementDesc: public MixedOopDesc { 
 private:
  static size_t allocation_size() { 
    return align_allocation_size(sizeof(CompilationQueueElementDesc));
  }
  static int pointer_count() { return 2; }

 private:
  // Link to next element in queue.
  CompilationQueueElementDesc* _next;
  // The virtual stack frame
  VirtualStackFrameDesc*       _frame;

  /* All oops must go before here.  If you change the number of oops, be
   * sure to change pointer_count()
   */

  // The type of this queue element.
  int                          _type;
  // The bytecode index
  int                          _bci;
  // Labels.
  int                          _entry_label;
  int                          _return_label;
  // Registers.
  int                          _register_0;
  int                          _register_1;
  // A spare location
  int                          _info;
  // Used by CompilationContinuation: code size before compiling this element.
  int                          _code_size_before;
  // true if this is a CompilationContinuation and it has been suspended.
  jboolean                     _is_suspended;
  jboolean                     _entry_has_been_bound;

  friend class Compiler;
  friend class CompilationQueueElement;
  friend class Universe;
  friend class OopDesc;
  friend class BinaryCompiler;
};
