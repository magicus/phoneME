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

// The class Symbols is a name spaces for fast lookup of 
// symbols commonly used in the VM.

#define VM_SYMBOLS_DO_GENERIC(template, sig_template) \
  /* ---- commonly used class names ---- */                                 \
  template(java_lang_Object,       "java/lang/Object")                      \
  template(java_lang_Class,        "java/lang/Class")                       \
  template(java_lang_String,       "java/lang/String")                      \
  template(java_lang_System,       "java/lang/System")                      \
  template(java_lang_Thread,       "java/lang/Thread")                      \
  template(java_lang_Throwable,    "java/lang/Throwable")                   \
  template(java_lang_Math,         "java/lang/Math")                        \
  template(com_sun_cldchi_jvm_JVM, "com/sun/cldchi/jvm/JVM")                \
  template(com_sun_cldchi_jvm_FileDescriptor,                               \
                                   "com/sun/cldchi/jvm/FileDescriptor")     \
  /* ---- class file format tags ---- */                                    \
  template(tag_source_file,        "SourceFile")                            \
  template(tag_inner_classes,      "InnerClasses")                          \
  template(tag_constant_value,     "ConstantValue")                         \
  template(tag_code,               "Code")                                  \
  template(tag_line_number_table,  "LineNumberTable")                       \
  template(tag_local_var_table,    "LocalVariableTable")                    \
  template(tag_exceptions,         "Exceptions")                            \
  template(tag_stackmap,           "StackMap")                              \
  template(tag_synthetic,          "Synthetic")                             \
  /* ---- Exception classes: all exceptions thrown by the VM ---- */        \
  template(java_lang_Error,        "java/lang/Error")                       \
  template(java_lang_RuntimeException,                                      \
                                   "java/lang/RuntimeException")            \
  template(java_lang_OutOfMemoryError,                                      \
                                   "java/lang/OutOfMemoryError")            \
  template(java_lang_ClassNotFoundException,                                \
                                   "java/lang/ClassNotFoundException")      \
  template(java_lang_NullPointerException,                                  \
                                   "java/lang/NullPointerException")        \
  template(java_lang_ArrayStoreException,                                   \
                                   "java/lang/ArrayStoreException")         \
  template(java_lang_ArrayIndexOutOfBoundsException,                        \
                                "java/lang/ArrayIndexOutOfBoundsException") \
  template(java_lang_NegativeArraySizeException,                            \
                                   "java/lang/NegativeArraySizeException")  \
  template(java_lang_ClassCastException,                                    \
                                   "java/lang/ClassCastException")          \
  template(java_lang_IllegalAccessException,                                \
                                   "java/lang/IllegalAccessException")      \
  template(java_lang_IllegalMonitorStateException,                          \
                                   "java/lang/IllegalMonitorStateException")\
  template(java_lang_IllegalArgumentException,                              \
                                   "java/lang/IllegalArgumentException")    \
  template(java_lang_IllegalThreadStateException,                           \
                                   "java/lang/IllegalThreadStateException") \
  template(java_lang_ArithmeticException,                                   \
                                   "java/lang/ArithmeticException")         \
  template(java_lang_InstantiationException,                                \
                                   "java/lang/InstantiationException")      \
  template(java_lang_SecurityException,                                     \
                                   "java/lang/SecurityException")           \
  /* ---- common method names ---- */                                       \
  template(initialize_name,        "initialize")                            \
  template(object_initializer_name,"<init>")                                \
  template(class_initializer_name, "<clinit>")                              \
  template(unknown,                ".unknown.")                             \
  template(finalize_name,          "finalize")                              \
  template(main_name,              "main")                                  \
  template(run_name,               "run")                                   \
  template(min_name,               "min")                                   \
  template(max_name,               "max")                                   \
  template(abs_name,               "abs")                                   \
  template(internal_exit_name,     "internalExit")                          \
  template(println_name,           "println")                               \
  template(printStackTrace_name,   "printStackTrace")                       \
  template(fillInStackTrace_name,  "fillInStackTrace")                      \
  template(backtrace_name,         "backtrace")                             \
  template(wait_cleanup_name,      "waitCleanup")                           \
  template(enter_lock_object_name, "enterLockObject")                       \
  template(create_sys_image_name,  "createSysImage")                        \
  template(create_app_image_name,  "createAppImage")                        \
  template(arraycopy_name,         "arraycopy")                             \
  template(unchecked_byte_arraycopy_name,  "unchecked_byte_arraycopy")      \
  template(unchecked_char_arraycopy_name,  "unchecked_char_arraycopy")      \
  template(unchecked_int_arraycopy_name,   "unchecked_int_arraycopy")       \
  template(unchecked_obj_arraycopy_name,   "unchecked_obj_arraycopy")       \
  template(unchecked_long_arraycopy_name,  "unchecked_long_arraycopy")      \
  /* ---- common signatures ---- */                                         \
  sig_template(java_lang_Object_signature,                                  \
                                   "Ljava/lang/Object;")                    \
  sig_template(java_lang_Object_Object_void_signature,                      \
                                  "(Ljava/lang/Object;Ljava/lang/Object;)V")\
  sig_template(string_array_void_signature,                                 \
                                   "([Ljava/lang/String;)V")                \
  sig_template(string_void_signature,                                       \
                                   "(Ljava/lang/String;)V")                 \
  sig_template(char_array_char_array_int_void_signature,                    \
                                "([C[CI)V") \
  sig_template(void_signature,     "()V")

#if ENABLE_CLDC_11
#define VM_SYMBOLS_DO_CLDC11(template, sig_template) \
  template(java_lang_ref_WeakReference,                                     \
                                   "java/lang/ref/WeakReference")           \
  template(java_lang_InterruptedException,                                  \
                                   "java/lang/InterruptedException")        \
  template(java_lang_NoClassDefFoundError,                                  \
                                   "java/lang/NoClassDefFoundError")        
#else
#define VM_SYMBOLS_DO_CLDC11(template, sig_template)
#endif

#if ENABLE_ISOLATES
#define VM_SYMBOLS_DO_ISOLATE(template, sig_template) \
  template(com_sun_cldc_isolate_IsolateStartupException,                    \
                   "com/sun/cldc/isolate/IsolateStartupException")          \
  template(com_sun_cldc_isolate_llegalIsolateStateException,                \
                   "com/sun/cldc/isolate/IllegalIsolateStateException")     \
  template(com_sun_cldc_isolate_Isolate,                                    \
                                   "com/sun/cldc/isolate/Isolate")          \
  template(stop_name,              "stop")                                  \
  template(com_sun_cldc_isolate_IsolateResourceError,                       \
                        "com/sun/cldc/isolate/IsolateResourceError")        \
  sig_template(int_obj_array_void_signature,                                \
                                   "(I[Ljava/lang/Object;)V")               \
  sig_template(obj_array_void_signature,                                    \
                                   "([Ljava/lang/Object;)V")                \
  sig_template(int_int_void_signature,                                      \
                                   "(II)V")                                 \

#else
#define VM_SYMBOLS_DO_ISOLATE(template, sig_template)
#endif

#if ENABLE_REFLECTION
#define VM_SYMBOLS_DO_REFLECTION(template, sig_template) \
  template(java_lang_reflect_Constructor,                                   \
                                   "java/lang/reflect/Constructor")         \
  template(java_lang_reflect_Method,                                        \
                                   "java/lang/reflect/Method")              \
  template(java_lang_reflect_Field,                                         \
                                   "java/lang/reflect/Field")               \
  template(no_such_field_exception,                                         \
                                   "java/lang/NoSuchFieldException")        \
  template(no_such_method_exception,                                        \
                                   "java/lang/NoSuchMethodException")       \
  template(java_lang_Boolean,      "java/lang/Boolean")                     \
  template(java_lang_Character,    "java/lang/Character")                   \
  template(java_lang_Float,        "java/lang/Float")                       \
  template(java_lang_Double,       "java/lang/Double")                      \
  template(java_lang_Byte,         "java/lang/Byte")                        \
  template(java_lang_Short,        "java/lang/Short")                       \
  template(java_lang_Integer,      "java/lang/Integer")                     \
  template(java_lang_Long,         "java/lang/Long")                        \
  template(java_lang_Void,         "java/lang/Void")                        \

#else
#define VM_SYMBOLS_DO_REFLECTION(template, sig_template)
#endif

#if ENABLE_JAVA_DEBUGGER
#define VM_SYMBOLS_DO_DEBUGGER(template, sig_template) \
  template(com_sun_cldchi_jvm_DBG, "com/sun/cldchi/jvm/DebuggerInvoke") \
  template(debugger_sync_name,       "debuggerInvoke")                  \
  sig_template(obj_obj_int_int_int_int_void_signature,                      \
               "(Ljava/lang/Object;Ljava/lang/Object;IIII)V")            \

#else
#define VM_SYMBOLS_DO_DEBUGGER(template, sig_template)
#endif

#define VM_SYMBOLS_DO(template, sig_template) \
        VM_SYMBOLS_DO_GENERIC(template, sig_template) \
        VM_SYMBOLS_DO_CLDC11(template, sig_template)  \
        VM_SYMBOLS_DO_ISOLATE(template, sig_template) \
        VM_SYMBOLS_DO_REFLECTION(template, sig_template) \
        VM_SYMBOLS_DO_DEBUGGER(template, sig_template)

// Mapping function names to values. New entries should be added below.
#define VM_SYMBOL_ACCESS(name, string) \
  static Symbol* name() { return at(name##_index); }

// Exception classes that are not defined in CLDC:
#define NON_CLDC_EXCEPTION(name) \
  static Symbol* name() { return java_lang_Error(); }

class Symbols: public AllStatic {
 private:
  static Symbol* at(int index) {
    return (Symbol*) &system_symbols[index];
  }

 public:
  enum {
   VM_SYMBOLS_DO(UNIVERSE_HANDLES_DECLARE, UNIVERSE_HANDLES_DECLARE)
   __number_of_system_symbols
  };

  static int number_of_system_symbols() {
    return __number_of_system_symbols;
  }

#if !ROMIZED_PRODUCT
  static void initialize(JVM_SINGLE_ARG_TRAPS);
  static void oops_do(oop_doer /*do_oop*/);
#else
  static void initialize(JVM_SINGLE_ARG_TRAPS) PRODUCT_RETURN;
  static void oops_do(oop_doer /*do_oop*/) PRODUCT_RETURN;
#endif

  VM_SYMBOLS_DO(VM_SYMBOL_ACCESS, VM_SYMBOL_ACCESS)

  friend class Throw;

  NON_CLDC_EXCEPTION(java_lang_ClassFormatError)
  NON_CLDC_EXCEPTION(java_lang_VerifyError)
  NON_CLDC_EXCEPTION(java_lang_IncompatibleClassChangeError)
  NON_CLDC_EXCEPTION(java_lang_UnsatisfiedLinkError)

#if !ENABLE_CLDC_11
  NON_CLDC_EXCEPTION(java_lang_NoClassDefFoundError)
#endif

  static bool is_system_symbol(Symbol* /*symbol*/) PRODUCT_RETURN0;
};

#undef VM_SYMBOL_ACCESS
