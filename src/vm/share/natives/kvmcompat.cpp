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
#include "incls/_kvmcompat.cpp.incl"


extern "C" {
#ifdef AZZERT
int _in_kvm_native_method = 0;
#endif

#if ENABLE_KVM_COMPAT
/*
 * For a KVM native function with parameters p0, p1, ... pn, where
 *    - for an instance method, p0 is <this>
 *    - for a static method, p0 is the first parameter
 *
 * _kvm_stack_top    => [pn]
 *                      ...
 *                      [p1]   
 *                      [p0]   <= _kni_parameter_base (instance method only)
 * _kvm_stack_bottom => [xx]   <= _kni_parameter_base (static method only)
 *
 * A popStack() operation returns the value currently pointed to by
 * _kvm_stack_top, and then move _kvm_stack_top one element down.
 *
 * After all the parameters are popped, _kvm_stack_top will equal to
 * _kvm_stack_bottom.
 */
int * _kvm_stack_top;
int   _kvm_return_value32;
jlong _kvm_return_value64;
int   _kvm_return_type;

#ifdef AZZERT
int   _kvm_pushed;
int * _kvm_stack_bottom;
#endif

int   TemporaryRootsLength;
int   GlobalRootsLength;
union cellOrPointer TemporaryRoots[MAXIMUM_TEMPORARY_ROOTS];
union cellOrPointer GlobalRoots[MAXIMUM_GLOBAL_ROOTS];
ARRAY_CLASS PrimitiveArrayClasses[T_LONG+1];
static char str_buffer[STRINGBUFFERSIZE];

// Initialize the kvmcompat module. This function must be called by
// Universe.cpp after all the primitive array classes have been loaded.
void kvmcompat_initialize() {
#ifdef AZZERT
  jvm_memset(PrimitiveArrayClasses, 0xff, sizeof(PrimitiveArrayClasses));
  _in_kvm_native_method = 0;
#endif

  ARRAY_CLASS *p = PrimitiveArrayClasses;

  p[T_BOOLEAN] = (ARRAY_CLASS)Universe::  bool_array_class()->java_mirror();
  p[T_CHAR]    = (ARRAY_CLASS)Universe::  char_array_class()->java_mirror();
  p[T_BYTE]    = (ARRAY_CLASS)Universe::  byte_array_class()->java_mirror();
  p[T_SHORT]   = (ARRAY_CLASS)Universe:: short_array_class()->java_mirror();
  p[T_INT]     = (ARRAY_CLASS)Universe::   int_array_class()->java_mirror();
  p[T_LONG]    = (ARRAY_CLASS)Universe::  long_array_class()->java_mirror();
#if ENABLE_FLOAT
  p[T_FLOAT]   = (ARRAY_CLASS)Universe:: float_array_class()->java_mirror();
  p[T_DOUBLE]  = (ARRAY_CLASS)Universe::double_array_class()->java_mirror();
#endif
  TemporaryRootsLength = 0;
  GlobalRootsLength = 0;
}

void kvmcompat_oops_do(void do_oop(OopDesc**)) {
  ARRAY_CLASS *pac = PrimitiveArrayClasses;

  do_oop((OopDesc**)(&pac[T_BOOLEAN]));
  do_oop((OopDesc**)(&pac[T_CHAR]));
#if ENABLE_FLOAT
  do_oop((OopDesc**)(&pac[T_FLOAT]));
  do_oop((OopDesc**)(&pac[T_DOUBLE]));
#endif
  do_oop((OopDesc**)(&pac[T_BYTE]));
  do_oop((OopDesc**)(&pac[T_SHORT]));
  do_oop((OopDesc**)(&pac[T_INT]));
  do_oop((OopDesc**)(&pac[T_LONG]));

  cell *p, *end;

  p   = (cell*)(&GlobalRoots[0]);
  end = (cell*)(&GlobalRoots[GlobalRootsLength]);
  for (; p<end; p++) {
    do_oop((OopDesc**)(*p));
  }

  p   = (cell*)(&TemporaryRoots[0]);
  end = (cell*)(&TemporaryRoots[TemporaryRootsLength]);
  for (; p<end; p++) {
    do_oop((OopDesc**)(*p));
  }

  if (_kvm_return_type == T_OBJECT || _kvm_return_type == T_ARRAY) {
    do_oop((OopDesc**)(&_kvm_return_value32));
  }
}

void makeGlobalRoot(cell** object) {
  GUARANTEE(_in_kvm_native_method, "sanity");

  int index = GlobalRootsLength;
  if (index >= MAXIMUM_GLOBAL_ROOTS) {
    fatalError("Global roots overflow");
  }
  GlobalRoots[index].cellpp = object;
  GlobalRootsLength = index + 1;
}

#ifdef AZZERT
void _kvm_check_stack() {
  GUARANTEE(_in_kvm_native_method, "sanity");

#if USE_STRICT_STACK_CHECK
  GUARANTEE(_kvm_stack_top == _kvm_stack_bottom, "all stacks must be popped");
#endif
}
#endif

int popStack() {
  GUARANTEE(_in_kvm_native_method, "sanity");

  if (JavaStackDirection > 0) {
    GUARANTEE(_kvm_stack_top > _kvm_stack_bottom, "stack is empty");
  } else {
    GUARANTEE(_kvm_stack_top < _kvm_stack_bottom, "stack is empty");
  }
  int value = *_kvm_stack_top;
  _kvm_stack_top -= JavaStackDirection;
  return value;
}

void pushStack(int data) {
  GUARANTEE(_in_kvm_native_method, "sanity");
#if USE_STRICT_STACK_CHECK
  GUARANTEE(_kvm_stack_top == _kvm_stack_bottom, 
            "must have an empty stack before pushing");
#endif
  GUARANTEE(!_kvm_pushed, "pushStack can be called at most once");
  _kvm_return_value32 = data;
}

void popLongToAddress(void* ptr) {
  GUARANTEE(_in_kvm_native_method, "sanity");

  int BB = *_kvm_stack_top;
  _kvm_stack_top -= JavaStackDirection;
  int AA = *_kvm_stack_top;
  _kvm_stack_top -= JavaStackDirection;
  jlong value;

  if (JavaStackDirection < 0) {
    value = jlong_from_low_high(BB, AA);
  } else {
    value = jlong_from_low_high(AA, BB);
  }

  *((jlong*)ptr) = value;
}

void pushLongFromAddress(void* ptr) {
  GUARANTEE(_in_kvm_native_method, "sanity");
#if USE_STRICT_STACK_CHECK
  GUARANTEE(_kvm_stack_top == _kvm_stack_bottom, 
            "must have an empty stack before pushing");
#endif
  GUARANTEE(!_kvm_pushed, "pushStack can be called at most once");
  _kvm_return_value64 = *((jlong*)ptr);
}

void popDoubleToAddress(void* ptr) {
  GUARANTEE(_in_kvm_native_method, "sanity");
  popLongToAddress(ptr);
}

void pushDoubleFromAddress(void* ptr) {
  GUARANTEE(_in_kvm_native_method, "sanity");
  pushLongFromAddress(ptr);
}

CLASS getClass(const char *name) {
  GUARANTEE(_in_kvm_native_method, "sanity");

  OopDesc *mirror_obj = NULL;
  KNI_FindClass(name, (jclass)&mirror_obj);
  return mirror_obj;
}

ARRAY instantiateArray(ARRAY_CLASS arrayClass, long len) {
  GUARANTEE(_in_kvm_native_method, "sanity");
  ARRAY_CLASS *p = PrimitiveArrayClasses;

  SETUP_ERROR_CHECKER_ARG;
  if (arrayClass == p[T_BOOLEAN]) {
    return (ARRAY)Universe::  new_bool_array((int)len JVM_NO_CHECK_AT_BOTTOM);
  } else if (arrayClass == p[T_CHAR]) {
    return (ARRAY)Universe::  new_char_array((int)len JVM_NO_CHECK_AT_BOTTOM);
#if ENABLE_FLOAT
  } else if (arrayClass == p[T_FLOAT]) {
    return (ARRAY)Universe:: new_float_array((int)len JVM_NO_CHECK_AT_BOTTOM);
  } else if (arrayClass == p[T_DOUBLE]) {
    return (ARRAY)Universe::new_double_array((int)len JVM_NO_CHECK_AT_BOTTOM);
#endif
  } else if (arrayClass == p[T_BYTE]) {
    return (ARRAY)Universe::  new_byte_array((int)len JVM_NO_CHECK_AT_BOTTOM);
  } else if (arrayClass == p[T_SHORT]) {
    return (ARRAY)Universe:: new_short_array((int)len JVM_NO_CHECK_AT_BOTTOM);
  } else if (arrayClass == p[T_INT]) {
    return (ARRAY)Universe::   new_int_array((int)len JVM_NO_CHECK_AT_BOTTOM);
  } else if (arrayClass == p[T_LONG]) {
    return (ARRAY)Universe::  new_long_array((int)len JVM_NO_CHECK_AT_BOTTOM);
  } else {
    // Only primitive arrays are supported. If you KVM code needs to allocate
    // object arrays, please convert it to use KNI/SNI code.
    SHOULD_NOT_REACH_HERE();
    return NULL;
  }
}

SHORTARRAY createCharArray(const char *utf8stringArg,
                           int utf8length,
                           int *unicodelengthP, 
                           bool_t isPermanent) {
  GUARANTEE(_in_kvm_native_method, "sanity");
  GUARANTEE(!isPermanent, "Permanent char arrays not supported");
  (void)isPermanent;

  LiteralStream ls((char*)utf8stringArg, 0, utf8length);
  int num_chars = ls.length();
  if (num_chars >= 0) {
    SETUP_ERROR_CHECKER_ARG;
    TypeArray::Raw array = Universe::new_char_array(num_chars JVM_CHECK_0);
    for (int i=0; i<num_chars; i++) {
      array().char_at_put(i, ls.read());
    }
    *unicodelengthP = num_chars;
    return (SHORTARRAY)(array().obj());
  } else {
    // Bad UTF8 string in utf8stringArg
    SHOULD_NOT_REACH_HERE();
    return NULL;
  }
}

/*=========================================================================
 * FUNCTION:      instantiateString()
 * TYPE:          constructor
 * OVERVIEW:      Create an initialized Java-level string object.
 * INTERFACE:
 *   parameters:  UTF8 String containing the value of the new string object
 *                number of bytes in the UTF8 string
 *   returns:     pointer to array object instance
 * NOTE:          String arrays are not intended to be manipulated
 *                directly; they are manipulated from within instances
 *                of 'java.lang.String'.
 *=======================================================================*/

STRING_INSTANCE
instantiateString(const char *stringArg, int utflength)
{
  GUARANTEE(_in_kvm_native_method, "sanity");
  SETUP_ERROR_CHECKER_ARG;
  return (STRING_INSTANCE)Universe::new_string(stringArg, utflength
                                               JVM_NO_CHECK_AT_BOTTOM);
}

/*=========================================================================
 * FUNCTION:      getStringContents()
 * TYPE:          internal operation on string objects
 * OVERVIEW:      Get the contents of a string object in C/C++
 *                string format.
 * INTERFACE:
 *   parameters:  String object pointer
 *   returns:     char* to the string in C/C++ format
 *
 * NOTE:          This operation uses an internal string buffer
 *                that is shared and has a fixed length.  Thus,
 *                the contents of the returned string may (will)
 *                change over time.
 *=======================================================================*/

char* getStringContents(STRING_INSTANCE string){
  GUARANTEE(_in_kvm_native_method, "sanity");
  return getStringContentsSafely(string, str_buffer, STRINGBUFFERSIZE);
}

/*=========================================================================
 * FUNCTION:      getStringContentsSafely()
 * TYPE:          internal operation on string objects
 * OVERVIEW:      Get the contents of a string object in C/C++
 *                string format.
 * INTERFACE:
 *   parameters:  String object pointer
 *   returns:     char* to the string in C/C++ format
 *=======================================================================*/

char* getStringContentsSafely(STRING_INSTANCE string, char *buf, int bufLen) {
  GUARANTEE(_in_kvm_native_method, "sanity");

  String::Raw str((OopDesc*)string);
  TypeArray::Raw t = str().value();
  int offset = str().offset();
  int length = str().length();
  if (length > bufLen-1) {
    length = bufLen-1;
  }

  int i = 0;
  for (; i<length; i++) {
    buf[i] = (char)(t().char_at(i + offset));
  }
  buf[i] = 0;

  return buf;
}

/*=========================================================================
 * FUNCTION:      isAssignableTo()
 * TYPE:          public instance-level operation
 * OVERVIEW:      Check if the class 'fromClass' can be assigned
 *                to a variable of type 'toClass'.
 * INTERFACE:
 *   parameters:  fromClass, toClass: class pointers
 *   returns:     true if is assignable, false otherwise
 * 
 * NOTE:          This code now implements all the hairy checks
 *                required by Java Language Specification.
 *=======================================================================*/

bool_t isAssignableTo(CLASS fromClass, CLASS toClass) {
  GUARANTEE(_in_kvm_native_method, "sanity");

  JavaClassObj::Raw fm((OopDesc*)fromClass);
  JavaClassObj::Raw tm((OopDesc*)toClass);

  JavaClass::Raw f = fm().java_class();
  JavaClass::Raw t = tm().java_class();

  return f().compute_is_subtype_of(&t);
}


void verifyTemporaryRootSpace(int size) {
  GUARANTEE(_in_kvm_native_method, "sanity");

  if (TemporaryRootsLength + size > MAXIMUM_TEMPORARY_ROOTS) {
    fatalError("temporary root overflow");
  }
}

/*=========================================================================
 * FUNCTION:      garbageCollect
 * TYPE:          public garbage collection function
 * OVERVIEW:      Perform garbage collection (produces same result as
 *                the java.lang.Runtime.gc() method).
 * INTERFACE:
 *   parameters:  int moreMemory: we will attempt to make available
 *                                <moreMemory> bytes of free space inside
 *                                the heap. However, when the heap is very
 *                                full, the request may not be satisfied.
 *   returns:     <nothing>
 *=======================================================================*/

void garbageCollect(int moreMemory) {
  GUARANTEE(_in_kvm_native_method, "sanity");

  SETUP_ERROR_CHECKER_ARG;
  ObjectHeap::collect(moreMemory JVM_NO_CHECK);
  ObjectHeap::clear_inline_allocation_area();
  Thread::clear_current_pending_exception();
}

void fatalError(const char* errorMessage) {
  GUARANTEE(_in_kvm_native_method, "sanity");
  KNI_FatalError(errorMessage);
}

void raiseException(const char* exceptionClassName) {
  GUARANTEE(_in_kvm_native_method, "sanity");
  KNI_ThrowNew(exceptionClassName, NULL);
}
#endif // ENABLE_KVM_COMPAT

} // extern "C"
