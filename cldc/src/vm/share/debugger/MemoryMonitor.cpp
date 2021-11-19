/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
# include "incls/_MemoryMonitor.cpp.incl"

#if ENABLE_MEMORY_MONITOR
void MemoryMonitor::notify_object( const OopDesc* obj, const int size,
                                   const bool create ) {
  FixedArrayOutputStream stream;
  stream.print( "%c 0x%x %d ", create ? '+' : '-', obj, size );

  Oop::Raw o( (OopDesc*) obj );
  
  FarClassDesc* blueprint = obj->blueprint();
  const int instance_size = blueprint->instance_size_as_jint();

  #define CASE(kind) case InstanceSize::size_##kind:
  switch( instance_size ) {
    default:                  // Java object
#if 0
      if( blueprint == Universe::string_class()->obj() ) {
        type_name = "String";
      } else if( blueprint == Universe::java_lang_Class_class()->obj() ) {
        type_name = "Java class";
      } else {
        JavaClass::Raw klass( blueprint );
        ClassInfo::Raw info = klass().class_info();

        Symbol::Raw name = info().name();
        if( name.equals( Symbols::unknown() ) ) {
          name = ROM::get_original_class_name( &info );
        }
      }
      break;
#endif
    CASE( obj_array         ) // an array of Java objects
    CASE( type_array_1      ) // boolean[], byte[]
    CASE( type_array_2      ) // short[], char[]
    CASE( type_array_4      ) // int[], float[]
    CASE( type_array_8      ) // long[], double[]    
    CASE( symbol            ) // a symbol
      o.print_value_on( &stream );
      break;

    CASE( class_info        ) // a class info
    CASE( execution_stack   ) // an execution stack
    CASE( task_mirror       ) // an isolate task mirror
    CASE( instance_class    ) // an instance class
    CASE( obj_array_class   ) // an object array class
    CASE( type_array_class  ) // a typed array class 
    CASE( generic_near      ) // a simplest near
    CASE( java_near         ) // a near for a Java object
    CASE( obj_near          ) // a near for a method
    CASE( far_class         ) // a simple far class
    CASE( mixed_oop         ) // a MixedOop
    CASE( boundary          ) // a Boundary
    CASE( entry_activation  ) // an entry activation
    CASE( method            ) // a method
    CASE( constant_pool     ) // a constant pool
    CASE( compiled_method   ) // compiled method
    CASE( stackmap_list     ) // a stackmap list
    CASE( refnode           ) // debugger mapping from object ID to object
      return;
  }
  #undef CASE
  tty->print_cr( stream.array() );
}
#endif
