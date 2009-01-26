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

#if ENABLE_COMPILER
class CompiledMethodDependency: public CompilerObject {
 private:
  CompiledMethodDependency* _next;
  MethodDesc* _method;

 public:
  CompiledMethodDependency* next ( void ) const {
    return _next;
  }
  void set_next ( CompiledMethodDependency* next ) {
    _next = next;
  }

  MethodDesc* method ( void ) const {
    return _method;
  }
  void set_method ( MethodDesc* method ) {
    _method = method;
  }
     
  static const CompiledMethodDependency*
  find( const CompiledMethodDependency* p, const MethodDesc* method ) {
    for( ; p && p->method() != method; p = p->next() ) {
    }
    return p;
  }

  static void oops_do( CompiledMethodDependency* p, void do_oop(OopDesc**) ) {
    for( ; p; p = p->next() ) {    
      do_oop( (OopDesc**) &p->_method );
    }
  }
};
#endif
