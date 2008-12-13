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

/*
 * This file provides a *LIMITED* compatibility layer for porting KVM-style
 * native code to the VM. Please see the file doc/misc/KvmCompat.html
 * for details on using this compatibility layer.
 */

#ifndef _KVM_COMPAT_H_
#define _KVM_COMPAT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Define USE_STRICT_STACK_CHECK=1 if you want strict checks that ensures
 * all parameters are popped before return values are pushed.
 */
#ifndef USE_STRICT_STACK_CHECK
#define USE_STRICT_STACK_CHECK 0
#endif

#ifndef JVM_CHECK
/*
 * Within the VM, these values are declared in GlobalDefinitions.hpp.
 * If we're in here, we are compiling KVM-style native code, which does 
 * not include GlobalDefinitions.hpp.
 */
#define T_BOOLEAN   4
#define T_CHAR      5
#define T_FLOAT     6
#define T_DOUBLE    7
#define T_BYTE      8
#define T_SHORT     9
#define T_INT       10
#define T_LONG      11

typedef enum {
  FALSE = 0,
  TRUE = 1
} bool_t;

#else
typedef bool bool_t;
#endif // JVM_CHECK

#ifndef STRINGBUFFERSIZE
#define STRINGBUFFERSIZE 512
#endif

extern void _kvm_check_stack();

extern int * _kvm_stack_top;
extern int   _kvm_return_value32;
extern int   _kvm_return_type;
extern int   _in_kvm_native_method;

#define getSP() _kvm_stack_top

extern int popStack();
extern void pushStack(int data);

#define popStackAsType(_type_)          ((_type_)popStack())
#define pushStackAsType(_type_, data)   pushStack((int)data)

extern void popDoubleToAddress(void* ptr);
extern void pushDoubleFromAddress(void* ptr);

extern void popLongToAddress(void* ptr);
extern void pushLongFromAddress(void* ptr);

#define popDouble(_lval) popDoubleToAddress((void*)(&_lval))
#define popLong(_lval)   popLongToAddress((void*)(&_lval))

#define pushDouble(_lval) pushDoubleFromAddress((void*)(&_lval))
#define pushLong(_lval)   pushLongFromAddress((void*)(&_lval))

#define oneLess popStack()

typedef void* CLASS;
typedef void* ARRAY_CLASS;
typedef void* INSTANCE;
typedef void* STRING_INSTANCE;

typedef unsigned long cell;
typedef struct arrayStruct* ARRAY;
typedef struct shortArrayStruct* SHORTARRAY;
typedef struct byteArrayStruct* BYTEARRAY;

typedef union cellOrPointer {
    unsigned long cell;
    unsigned long *cellp;
    unsigned long **cellpp;              /* For global roots */

    /* Occasionally needed by GC */
    char *charp;
    char **charpp;
} cellOrPointer;

struct arrayStruct {
  void *klass;
  cell  length;               /* Number of elements */
  cellOrPointer data[1];  
};

struct byteArrayStruct {
  void *klass;
  cell  length;               /* Number of elements */
  signed char bdata[1];
};

struct shortArrayStruct {
  void *klass;
  cell  length;               /* Number of elements */
  short sdata[1];             /* First (zeroeth) data slot of the array */
};

extern void makeGlobalRoot(cell** object);

/* May cause GC. Accepts only names of instance classes, not array classes */
extern CLASS getClass(const char *name);

extern ARRAY_CLASS PrimitiveArrayClasses[];

extern bool_t isAssignableTo(CLASS, CLASS);
extern ARRAY  instantiateArray(ARRAY_CLASS arrayClass, long length);
extern SHORTARRAY createCharArray(const char *utf8stringArg,
                                  int utf8length,
                                  int *unicodelengthP, 
                                  bool_t isPermanent);
extern STRING_INSTANCE instantiateString(const char *stringArg, int utflength);
extern char* getStringContents(STRING_INSTANCE string);
extern char* getStringContentsSafely(STRING_INSTANCE string, char *buf, 
                                     int length);

#define MAXIMUM_TEMPORARY_ROOTS 50
#define MAXIMUM_GLOBAL_ROOTS 20

extern int TemporaryRootsLength;
extern int GlobalRootsLength;

extern union cellOrPointer TemporaryRoots[];
extern union cellOrPointer GlobalRoots[];

#define START_TEMPORARY_ROOTS   { int _tmp_roots_ = TemporaryRootsLength;
#define END_TEMPORARY_ROOTS      TemporaryRootsLength = _tmp_roots_;  }

#ifdef AZZERT
# define VERIFY_INSIDE_TEMPORARY_ROOTS (_tmp_roots_ = _tmp_roots_),
# define VERIFY_TEMPORARY_ROOT_SPACE(size) verifyTemporaryRootSpace(size),
void verifyTemporaryRootSpace(int size);
#else
# define VERIFY_INSIDE_TEMPORARY_ROOTS 
# define VERIFY_TEMPORARY_ROOT_SPACE(size)
#endif

#define IS_TEMPORARY_ROOT(_var_, _value_)                              \
  _var_ = (VERIFY_INSIDE_TEMPORARY_ROOTS                               \
        _var_ = _value_,                                               \
        VERIFY_TEMPORARY_ROOT_SPACE(1)                                 \
        TemporaryRoots[TemporaryRootsLength++].cellp = (cell *)&_var_, \
        _var_)

#define DECLARE_TEMPORARY_ROOT(_type_, _var_, _value_)                 \
     _type_ IS_TEMPORARY_ROOT(_var_, _value_)

extern void garbageCollect(int moreMemory);
extern void fatalError(const char* errorMessage);
extern void raiseException(const char* exceptionClassName);

#ifdef __cplusplus
}
#endif

#endif /* _KVM_COMPAT_H_ */
