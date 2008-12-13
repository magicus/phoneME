/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

enum {
  // NOTE: to see a comprehensive listing of all the flags, run
  //          cldc_vm_g -definitions

  // Flags used by classes, methods and fields
  JVM_ACC_SYNTHETIC               = (1 << 13),

  // Flags used by methods
  JVM_ACC_HAS_MONITOR_BYTECODES   = (1 <<  6),     
  JVM_ACC_HAS_INVOKE_BYTECODES    = (1 <<  7),

  JVM_ACC_HAS_COMPRESSED_HEADER   = (1 <<  9),  // if ROM class has its 
                                                // headers compressed
  JVM_ACC_HAS_NO_STACKMAPS        = (1 << 12),  // if method has no stackmaps
  JVM_ACC_HAS_NO_EXCEPTION_TABLE  = (1 << 14),  // if method has no exception
                                                // table
  JVM_ACC_DOUBLE_SIZE             = (1 << 15),  // if method requires double
                                                // compiled code factor

  // Flags used by classes
  JVM_ACC_HIDDEN                  = (1 << 22),  // class is hidden from the 
                                                // loaded classes, and only 
                                                // available to ROMized ones
  JVM_ACC_CONVERTED               = (1 << 23),  // Romized application class
  JVM_ACC_NON_OPTIMIZABLE         = (1 << 24),  // Indicates a class should
                                                // not be optimized by romizer.
  JVM_ACC_HAS_VANILLA_CONSTRUCTOR = (1 << 25),
  JVM_ACC_ARRAY_CLASS             = (1 << 26),  // Used to distinguish between
                                                // InstanceClass and ArrayClass
  JVM_ACC_VERIFIED                = (1 << 27),  // Indicate a class has been
                                                // verified. Used by ISOLATES
                                                // only.
  JVM_ACC_PRELOADED               = (1 << 28),  // Romized system class
  JVM_ACC_FAKE_CLASS              = (1 << 29),  // fake class created during
                                                // signature parsing
  JVM_ACC_HAS_FINALIZER           = (1 << 30)   // True if klass has a 
                                                // non-empty finalize() method
};

/** /class AccessFlags
 * provides wrappers for Java access flags manipulations.
 */
class AccessFlags {
 private:
  jint _flags;

 public:
  // Java access flags
  bool is_public() const {
    return (_flags & JVM_ACC_PUBLIC) != 0;
  }
  bool is_private() const {
    return (_flags & JVM_ACC_PRIVATE) != 0;
  }
  bool is_protected() const {
    return (_flags & JVM_ACC_PROTECTED) != 0;
  }
  bool is_static() const {
    return (_flags & JVM_ACC_STATIC) != 0;
  }
  bool is_final() const {
    return (_flags & JVM_ACC_FINAL) != 0;
  }
  bool is_synchronized() const {
    return (_flags & JVM_ACC_SYNCHRONIZED) != 0;
  }
  bool is_super() const {
    return (_flags & JVM_ACC_SUPER) != 0;
  }
  bool is_volatile() const {
    return (_flags & JVM_ACC_VOLATILE) != 0;
  }
  bool is_transient() const {
    return (_flags & JVM_ACC_TRANSIENT) != 0;
  }
  bool is_native() const {
    return (_flags & JVM_ACC_NATIVE) != 0;
  }
  bool is_interface() const {
    return (_flags & JVM_ACC_INTERFACE) != 0;
  }
  bool is_abstract() const {
    return (_flags & JVM_ACC_ABSTRACT) != 0;
  }
  bool is_strict() const {
    return (_flags & JVM_ACC_STRICT) != 0;
  }  
  
  // Attribute flags
  bool is_synthetic() const {
    return (_flags & JVM_ACC_SYNTHETIC) != 0;
  }

  // Attribute flags
  bool is_double_size() const {
    return (_flags & JVM_ACC_DOUBLE_SIZE) != 0;
  }

  // For access flags of fields/methods
  bool is_package_private() const {
    return (!is_public() && !is_private() && !is_protected());
  }

  // Class flags.
  bool has_finalizer() const {
    return (_flags & JVM_ACC_HAS_FINALIZER) != 0;
  }

  bool has_unresolved_finalizer() const {
    return (_flags & JVM_ACC_HAS_FINALIZER) != 0 &&
      (_flags & JVM_ACC_NATIVE) != 0;
  }

  bool is_array_class() const {
    return (_flags & JVM_ACC_ARRAY_CLASS) != 0;
  }
  bool is_fake_class() const {
    return (_flags & JVM_ACC_FAKE_CLASS) != 0;
  }
  bool is_hidden() const {
    return (_flags & JVM_ACC_HIDDEN) != 0;
  }
  bool has_vanilla_constructor() const {
    return (_flags & JVM_ACC_HAS_VANILLA_CONSTRUCTOR) != 0;
  }
  bool has_monitor_bytecodes() const {
    return (_flags & JVM_ACC_HAS_MONITOR_BYTECODES) != 0;
  }
  bool has_invoke_bytecodes() const {
    return (_flags & JVM_ACC_HAS_INVOKE_BYTECODES) != 0;
  }
  bool has_compressed_header() const {
    return (_flags & JVM_ACC_HAS_COMPRESSED_HEADER) != 0;
  }  
  bool has_no_stackmaps() const {
    return (_flags & JVM_ACC_HAS_NO_STACKMAPS) != 0;
  }  
  bool has_no_exception_table() const {
    return (_flags & JVM_ACC_HAS_NO_EXCEPTION_TABLE) != 0;
  }  
  bool is_preloaded() const {
    return (_flags & JVM_ACC_PRELOADED) != 0;
  }
  bool is_converted() const {
    return (_flags & JVM_ACC_CONVERTED) != 0;
  }
  // Returns true if the class is either a preloaded system class 
  // or a converted application class.
  bool is_romized() const {
    return is_preloaded() || is_converted();
  }
  bool is_verified() const {
    return (_flags & JVM_ACC_VERIFIED) != 0;
  }
  bool is_optimizable() const {
    return (_flags & JVM_ACC_NON_OPTIMIZABLE) == 0;
  }

  // Initialization
  void set_flags(jint flags) {
    _flags = flags;
  }

  // Conversion
  jshort as_short() const {
    return (jshort)_flags;
  }
  jint as_int() const {
    return _flags;
  }

  // Atomic update of flags
  void atomic_set_bits(jint bits) {
    // This is only atomic because we have cooperative scheduling.
    _flags = as_int() | bits;
  }

  void atomic_clear_bits(jint bits) {
    // This is only atomic because we have cooperative scheduling.
    _flags = as_int() & ~bits;
  }

  // Printing/debugging
#if USE_DEBUG_PRINTING
  void print_to_buffer(char* /*buff*/, int /*type*/) const;
  void p() const;
  static void print_definitions();
  static void print_definitions(int /*type*/);
#else
  void print_to_buffer(char* /*buff*/, int /*type*/) const PRODUCT_RETURN;
  void p() const PRODUCT_RETURN;
  static void print_definitions() PRODUCT_RETURN;
  static void print_definitions(int /*type*/) PRODUCT_RETURN;
#endif

  enum {
      CLASS_FLAGS  = 0x01,
      METHOD_FLAGS = 0x02,
      FIELD_FLAGS  = 0x04,

      C   = CLASS_FLAGS,
      M   = METHOD_FLAGS,
      F   = FIELD_FLAGS,
      CM  = (CLASS_FLAGS  | METHOD_FLAGS),
      MF  = (METHOD_FLAGS | FIELD_FLAGS),
      CMF = (CLASS_FLAGS  | METHOD_FLAGS | FIELD_FLAGS)
  };

 private:
  friend class ClassFileParser;
  friend class Method;
  friend class MethodDesc;
  friend class JavaClass;
  friend class InstanceClass;
  friend class ClassInfo;
  friend class ClassInfoDesc;
  friend class ROMWriter;
  friend class ROMOptimizer;
  friend class SystemDictionary;
  friend class JVM;
  friend class ConstantPool;
  friend class Universe;

  // The functions below should only be called on the _access_flags
  // instance variables directly, otherwise they are just changing a
  // copy of the flags.

  void set_is_final() {
    atomic_set_bits(JVM_ACC_FINAL);
  }
  void set_is_synthetic() {
    atomic_set_bits(JVM_ACC_SYNTHETIC);
  }
  void set_double_size() {
    atomic_set_bits(JVM_ACC_DOUBLE_SIZE);
  }
  void set_has_finalizer() {
    atomic_set_bits(JVM_ACC_HAS_FINALIZER);
  } 
  void set_has_unresolved_finalizer() {
    atomic_set_bits(JVM_ACC_HAS_FINALIZER | JVM_ACC_NATIVE);
  } 
  void set_has_vanilla_constructor() {
    atomic_set_bits(JVM_ACC_HAS_VANILLA_CONSTRUCTOR);
  }
  void set_has_monitor_bytecodes() {
    atomic_set_bits(JVM_ACC_HAS_MONITOR_BYTECODES);
  }
  void set_has_invoke_bytecodes() {
    atomic_set_bits(JVM_ACC_HAS_INVOKE_BYTECODES);
  }
  void set_is_preloaded() {
    atomic_set_bits(JVM_ACC_PRELOADED);
  }
  void set_is_converted() {
    atomic_set_bits(JVM_ACC_CONVERTED);
  }
  void set_is_fake_class() {
    atomic_set_bits(JVM_ACC_FAKE_CLASS);
  }
  void set_has_compressed_header() {
    atomic_set_bits(JVM_ACC_HAS_COMPRESSED_HEADER);
  }
  void set_has_no_stackmaps() {
    atomic_set_bits(JVM_ACC_HAS_NO_STACKMAPS);
  }
  void set_has_no_exception_table() {
    atomic_set_bits(JVM_ACC_HAS_NO_EXCEPTION_TABLE);
  }
  void set_is_verified() {
    atomic_set_bits(JVM_ACC_VERIFIED);
  }
  void set_is_non_optimizable() {
    atomic_set_bits(JVM_ACC_NON_OPTIMIZABLE);
  }
  void set_is_hidden() {
    atomic_set_bits(JVM_ACC_HIDDEN);
  }

  void clear_has_monitor_bytecodes()   {
    atomic_clear_bits(JVM_ACC_HAS_MONITOR_BYTECODES);
  }

  void clear_has_unresolved_finalizer()   {
    GUARANTEE(has_unresolved_finalizer(), "No need to clear");
    atomic_clear_bits(JVM_ACC_NATIVE);
  }

  /// This structure is used in non-product mode to print out the
  /// definitions of the access-flags bits.
  struct FlagInfo {
    juint flag;
    juint type;
    const char *name;
  };

  static FlagInfo flag_info[];
};
