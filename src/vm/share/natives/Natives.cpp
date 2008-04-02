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
#include "incls/_Natives.cpp.incl"

// Special VM natives

#if ENABLE_DYNAMIC_NATIVE_METHODS || ENABLE_ROM_GENERATOR

ReturnOop Natives::get_native_function_name(Method *method JVM_TRAPS) {
  bool dummy;
  InstanceClass ic = method->holder();
  ConstantPool cp = ic.constants();
  Symbol class_name = ic.original_name();
  Symbol method_name = method->get_original_name(dummy);
  Symbol signature;

  ObjArray methods = ic.methods();
  for (int i=0; i<methods.length(); i++) {
    Method other_method = methods.obj_at(i);
    if (other_method.is_null()) {
      // removed <clinit>
      continue;
    }
    if (!other_method.is_native()) {
      continue;
    }
    if (method->equals(&other_method)) {
      continue;
    }
    Symbol other_name = other_method.get_original_name(dummy);
    if (method_name.matches(&other_name)) {
      // Overloaded native method - need signature
      signature = method->signature();
      break;
    }
  }
  return convert_to_jni_name(&class_name, &method_name, &signature
                                      JVM_NO_CHECK_AT_BOTTOM_0);
}

ReturnOop Natives::convert_to_jni_name(Symbol *class_name,
                                       Symbol *method_name,
                                       Symbol *signature JVM_TRAPS) {
  TypeArray byte_array;
  int max_len = class_name->length() + method_name->length();
  if (!signature->is_null()) {
    max_len += signature->length();
  }
  max_len *= 4; // each character can expand to 4 bytes max
  max_len += 5 + 1 + 2;  // "Java_", "_", "__"

  byte_array = Universe::new_byte_array(max_len+1 JVM_CHECK_0);

  int index = 0;
  append_jni(&byte_array, "Java_", &index);
  append_jni(&byte_array, class_name, 0, 0, &index  JVM_CHECK_0);
  append_jni(&byte_array, "_", &index);
  append_jni(&byte_array, method_name, 0, 0, &index JVM_CHECK_0);

  if (!signature->is_null()) {
    append_jni(&byte_array, "__", &index);
    // Only include the stuff inside the parentheses.
    append_jni(&byte_array, signature, 1, ')', &index JVM_CHECK_0);
  }

  return byte_array;
}

void Natives::append_jni(TypeArray *byte_array, const char *str, int *index) {
  char *data = ((char*)byte_array->data()) + *index;
  char *start = data;

  while (*str) {
    *data++ = *str++;
  }

  *data = 0;
  *index += (data - start);
}

void Natives::append_jni(TypeArray *byte_array, Symbol *symbol,
                         int skip, char end_char, int *index JVM_TRAPS) {
  if (symbol->is_valid_method_signature(NULL)) {
    TypeSymbol::Raw type_symbol = symbol->obj();

    FixedArrayOutputStream faos;
    jvm_memset(faos.array(), 0, 1024);
    type_symbol().print_decoded_on(&faos);

    LiteralStream literal_stream(faos.array());
    append_jni(byte_array, &literal_stream, skip, end_char, index JVM_CHECK);
  } else {
    SymbolStream symbol_stream(symbol);
    append_jni(byte_array, &symbol_stream, skip, end_char, index JVM_CHECK);
  }
}

void Natives::append_jni(TypeArray *byte_array, CharacterStream *stream,
                         int skip, char end_char, int *index JVM_TRAPS) {
  String::Raw string = Universe::new_string(stream JVM_CHECK);
  AllocationDisabler raw_pointers_used_below;

  TypeArray::Raw jchar_array = string().value();
  int offset = string().offset();
  int count = string().count();
  char *data = ((char*)byte_array->data()) + *index;
  char *start = data;

  static const char HEX_DIGIT[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                     '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

  for (int i = 0; i < count; i++) {
    if (i < skip) {
      continue;
    }
    jchar ch = jchar_array().char_at(i + offset);
    if (ch == end_char) {
      break;
    }
    if (('A' <= ch && ch <= 'Z') ||
        ('a' <= ch && ch <= 'z') ||
        ('0' <= ch && ch <= '9')) {
      *data++ = (char)ch;
    } else {
      *data++ = '_';

      switch (ch) {
      case '/':
        break; // the _ is all we need
      case '_':
        *data++ = '1';
        break;
      case ';':
        *data++ = '2';
        break;
      case '[':
        *data++ = '3';
        break;
      default:
        *data++ = '0';
        *data++ = HEX_DIGIT[(ch >> 12) & 0xf];
        *data++ = HEX_DIGIT[(ch >> 8)  & 0xf];
        *data++ = HEX_DIGIT[(ch >> 4)  & 0xf];
        *data++ = HEX_DIGIT[ ch        & 0xf];
      }
    }
  }

  *data = 0;
  *index += (data - start);
}

ReturnOop Natives::get_jni_class_name(InstanceClass *klass JVM_TRAPS) {
  Symbol name = klass->original_name();
  int max_len = name.length() * 4; // each character can expand to 4 bytes max
  max_len += 5;                    // "Java_";

  TypeArray byte_array = Universe::new_byte_array(max_len+1 JVM_CHECK_0);

  int index = 0;
  append_jni(&byte_array, "Java_", &index);
  append_jni(&byte_array, &name, 0, 0, &index JVM_CHECK_0);

  return byte_array;
}
#endif // ENABLE_DYNAMIC_NATIVE_METHODS || ENABLE_ROM_GENERATOR

#if ENABLE_DYNAMIC_NATIVE_METHODS

address Natives::load_dynamic_native_code(Method* method JVM_TRAPS) {
  if (Universe::dynamic_lib_count != 0) {
    int i;
    address handle = NULL;
    TypeArray name_array = 
      Natives::get_native_function_name(method JVM_CHECK_0);
    const char * name = (const char *)name_array.data();
    for (i = 0; i < Universe::dynamic_lib_count; i++) {
      handle = (address)Universe::dynamic_lib_handles()->int_at(i);
      address fptr = (address)Os::getSymbol(handle, name);
      if (fptr != NULL) {
        method->set_native_code(fptr);
#if ENABLE_COMPILER
        // The pointer to the old native code is hardcoded 
        // in compiled code. Must recompile.
        method->unlink_compiled_code();
#endif
        return fptr;
      }
    } 
  }

  Throw::unsatisfied_link_error(method JVM_THROW_0);
}

#endif

#if ENABLE_STACK_TRACE
void FrameStream::next() {
  if (_fr.is_java_frame()) {
    _fr.as_JavaFrame().caller_is(_fr);
    _at_end = skip();
  }
}

bool FrameStream::skip() {
  while (_fr.is_entry_frame()) {
    if (_fr.as_EntryFrame().is_first_frame()) {
      return true;
    }
    _fr.as_EntryFrame().caller_is(_fr);
  }
  return false;
}
#endif

extern "C" {

#if ENABLE_DYNAMIC_NATIVE_METHODS

#define DEFINE_JAVA_UNIMPLEMENTED_NON_VOID(jtype, ttype, arg)  \
jtype Java_ ## jtype ## _unimplemented(JVM_SINGLE_ARG_TRAPS) { \
  UsingFastOops fast_oops;                                     \
  JavaFrame frame(Thread::current());                          \
  Method::Fast method = frame.method();                        \
  jtype (*fptr)(void) = ( jtype (*)())                         \
    Natives::load_dynamic_native_code(&method JVM_CHECK_0);    \
  GUARANTEE(fptr != NULL, "Error must have been thrown");      \
                                                               \
  return (*fptr)();                                            \
}

FOR_NON_VOID_TYPES(DEFINE_JAVA_UNIMPLEMENTED_NON_VOID)

#endif // ENABLE_DYNAMIC_NATIVE_METHODS

void Java_void_unimplemented(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaFrame frame(Thread::current());
  Method::Fast method = frame.method();

#if ENABLE_DYNAMIC_NATIVE_METHODS
  void (*fptr)(void) = 
    (void(*)())Natives::load_dynamic_native_code(&method JVM_CHECK);
  GUARANTEE(fptr != NULL, "Error must have been thrown");

  (*fptr)();
#else
  Throw::unsatisfied_link_error(&method JVM_THROW);
#endif
}

void Java_abstract_method_execution(JVM_SINGLE_ARG_TRAPS) {
  Throw::error(abstract_method_error JVM_NO_CHECK_AT_BOTTOM);
}

void Java_incompatible_method_execution(JVM_SINGLE_ARG_TRAPS) {
  Throw::incompatible_class_change_error(empty_message JVM_NO_CHECK_AT_BOTTOM);
}

void Java_illegal_method_execution(JVM_SINGLE_ARG_TRAPS) {
  Throw::error(illegal_access_error JVM_NO_CHECK_AT_BOTTOM);
}

// native public void write(int b)
void Java_com_sun_cldchi_io_ConsoleOutputStream_write() {
  jint value = KNI_GetParameterAsInt(1);
  tty->print("%c", value);
}

  // com.sun.cldchi.jvm natives

void Java_com_sun_cldchi_jvm_JVM_loadLibrary(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_DYNAMIC_NATIVE_METHODS
  void* handle;
  if(Universe::dynamic_lib_handles()->is_null()) {
    const int task = ObjectHeap::start_system_allocation();
    *Universe::dynamic_lib_handles() = Universe::new_int_array(4 JVM_NO_CHECK);
    ObjectHeap::finish_system_allocation(task);
    JVM_DELAYED_CHECK;
  }

  int length = Universe::dynamic_lib_handles()->length(); 
  if(Universe::dynamic_lib_count == length) {
    const int task = ObjectHeap::start_system_allocation();
    TypeArray::Raw new_array = Universe::new_int_array(length + 4 JVM_NO_CHECK);
    ObjectHeap::finish_system_allocation(task);
    JVM_DELAYED_CHECK;
    TypeArray::array_copy(Universe::dynamic_lib_handles(), 0, &new_array, 0, length);
    *Universe::dynamic_lib_handles() = new_array;
    
  }

  UsingFastOops fast_oops;
  String::Fast string = GET_PARAMETER_AS_OOP(1);
  TypeArray::Fast cstring = string().to_cstring(JVM_SINGLE_ARG_NO_CHECK);
  const char *name = (char *) cstring().base_address();
  handle = Os::loadLibrary(name);
  if(!handle) {
    Throw::error(unsatisfied_link_error JVM_THROW);
  }
 
  Universe::dynamic_lib_handles()->int_at_put(Universe::dynamic_lib_count++, 
                                              (int)handle);
#else
  JVM_IGNORE_TRAPS;
  Throw::error(unsatisfied_link_error JVM_THROW);
#endif
}

#if ENABLE_JAVA_DEBUGGER
jlong Java_com_sun_cldchi_jvm_DebuggerInvoke_debuggerInvokeReturn() {
  UsingFastOops fastoops;

  Oop::Fast o = GET_PARAMETER_AS_OOP(1);
  Oop::Fast exc = GET_PARAMETER_AS_OOP(2);
  Oop::Fast t = GET_PARAMETER_AS_OOP(3);
  int id = KNI_GetParameterAsInt(4);
  int options = KNI_GetParameterAsInt(5);
  int return_type = KNI_GetParameterAsInt(6);

  return ObjectReferenceImpl::invoke_return(&o, &exc, &t, id, options, return_type, NULL);
}

OopDesc* Java_com_sun_cldchi_jvm_DebuggerInvoke_debuggerInvokeReturnObj() {
  UsingFastOops fastoops;

  Oop::Fast o = GET_PARAMETER_AS_OOP(1);
  Oop::Fast exc = GET_PARAMETER_AS_OOP(2);
  Oop::Fast t = GET_PARAMETER_AS_OOP(3);
  int id = KNI_GetParameterAsInt(4);
  int options = KNI_GetParameterAsInt(5);
  int return_type = KNI_GetParameterAsInt(6);
  OopDesc *ret_val;

  ObjectReferenceImpl::invoke_return(&o, &exc, &t, id, options, return_type, &ret_val);
  return ret_val;
}

static void debugger_invoke_method(void return_point()) {
  EntryActivation::Raw entry = GET_PARAMETER_AS_OOP(1);
  entry().set_return_point((address) return_point);
  Thread::current()->append_pending_entry(&entry);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeV() {
  debugger_invoke_method(entry_return_void);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeZ() {
  debugger_invoke_method(entry_return_word);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeC() {
  debugger_invoke_method(entry_return_word);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeF() {
  debugger_invoke_method(entry_return_float);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeD() {
  debugger_invoke_method(entry_return_double);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeB() {
  debugger_invoke_method(entry_return_word);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeS() {
  debugger_invoke_method(entry_return_word);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeI() {
  debugger_invoke_method(entry_return_word);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeJ() {
  debugger_invoke_method(entry_return_long);
}

void Java_com_sun_cldchi_jvm_DebuggerInvoke_invokeL() {
  debugger_invoke_method(entry_return_object);
}
#endif

// java.lang.System natives

// private native static String getProperty0(String key);
ReturnOop Java_java_lang_System_getProperty0(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  String::Fast string = GET_PARAMETER_AS_OOP(1);
  GUARANTEE(string.not_null(), "property cannot should have been set")

  TypeArray::Fast cstring = string().to_cstring(JVM_SINGLE_ARG_CHECK_0);

  char *name = (char *) cstring().base_address();
  const char *value = JVMSPI_GetSystemProperty(name);

#if ENABLE_PRODUCT_PRINT_STACK || !defined(PRODUCT)
  if (jvm_strcmp(name, "__debug.only.pss") == 0) {
    pss();
  }
#endif


#ifndef PRODUCT
  if (GenerateROMImage) {
    if (jvm_strcmp(name, "microedition.encoding") == 0 ||
        jvm_strcmp(name, "java.lang.Character.caseConverter") == 0 ||
        jvm_strcmp(name, "com.sun.cldc.i18n.Helper.i18npath") == 0 ||
        jvm_strcmp(name, "ISO8859_1_InternalEncodingName") == 0) {
      // These 4 properties are used when initializing System.out during
      // build time. Please use -D command-line switch to the romizer
      // to choose the desired value for the target platform.
      tty->print_cr("... reading system property: %s", name);
    } else {
      tty->print_cr("WARNING: System.getProperty() should not be called ");
      tty->print_cr("during build-time initialization of romized classes.");
      tty->print_cr("The ROM image may not run.");
      tty->print_cr("property = %s.", name);
      ps();
    }
  }
#endif

  Oop::Fast result;
  bool need_to_free;
  if (value != NULL) {
    need_to_free = true;
  } else {
    need_to_free = false;

    if (jvm_strcmp(name, "microedition.configuration") == 0) {
#if ENABLE_CLDC_111
      value = "CLDC-1.1.1";
#elif ENABLE_CLDC_11
      value = "CLDC-1.1";
#else
      value = "CLDC-1.0";
#endif
    }
    else if (jvm_strcmp(name, "microedition.encoding") == 0) {
      value = "ISO8859_1";
    }
#if ENABLE_CLDC_11
    // These are specified only in 1.1
    else if (jvm_strcmp(name, "microedition.platform") == 0) {
      value = "generic";
    }
    else if (jvm_strcmp(name, "microedition.profiles") == 0) {
      value = "";
    }
#endif
  }

  if (value != NULL) {
    result = Universe::new_string(value, jvm_strlen(value) JVM_CHECK_0);
  }
  if (need_to_free) {
    JVMSPI_FreeSystemProperty((char*)value);
  }
  return result;
}

jlong Java_java_lang_System_currentTimeMillis() {
  return Os::java_time_millis();
}

jint Java_java_lang_System_identityHashCode(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaOop::Fast argument = GET_PARAMETER_AS_OOP(1);
  if (argument.is_null()) {
    return 0;
  }
  return Synchronizer::hash_code(&argument JVM_NO_CHECK_AT_BOTTOM_0);
}

// public static native void arraycopy(Object src, int src_position,
//                                     Object dst, int dst_position,
//                                     int length);
void Java_java_lang_System_arraycopy(JVM_SINGLE_ARG_TRAPS) {
  Array::Raw src = GET_PARAMETER_AS_OOP(1);
  jint src_pos   =   KNI_GetParameterAsInt(2);
  Array::Raw dst = GET_PARAMETER_AS_OOP(3);
  jint dst_pos   =   KNI_GetParameterAsInt(4);
  jint length    =   KNI_GetParameterAsInt(5);

  // Check for null pointers
  if (src.is_null() || dst.is_null()) {
    Throw::null_pointer_exception(arraycopy_null_pointer JVM_THROW);
  }

  // Check for array bounds (exception thrown later)
  bool throw_array_index_out_of_bounds = false;
  // Check is all offsets and lengths are non negative
  if (src_pos < 0 || dst_pos < 0 || length < 0) {
    throw_array_index_out_of_bounds = true;
  }
  // Check if the ranges are valid
  if  ( (((juint) length + (juint) src_pos) > (juint) src().length())
     || (((juint) length + (juint) dst_pos) > (juint) dst().length()) ) {
    throw_array_index_out_of_bounds = true;
  }

  // Dispatch to ObjArray
  if (src.is_obj_array() && dst.is_obj_array()) {
    // Bounds exception must be throw last
    if (throw_array_index_out_of_bounds) {
      Throw::array_index_out_of_bounds_exception(empty_message JVM_THROW);
    }
    // Do the copy
    ObjArray::array_copy((ObjArray*)&src, src_pos, (ObjArray*)&dst, dst_pos,
                         length JVM_NO_CHECK_AT_BOTTOM);
    return;
  }

  // Dispatch to TypeArray
  if (src.is_type_array() && dst.is_type_array()) {
    // Check element type, must be done before bounds checks
    TypeArrayClass::Raw src_class = src.blueprint();
    TypeArrayClass::Raw dst_class = dst.blueprint();
    if (src_class().type() != dst_class().type()) {
      Throw::array_store_exception(arraycopy_incompatible_types JVM_THROW);
    }
    // Bounds exception must be throw last
    if (throw_array_index_out_of_bounds) {
      Throw::array_index_out_of_bounds_exception(empty_message JVM_THROW);
    }
    // Do the copy
    TypeArray::array_copy((TypeArray*)&src, src_pos, (TypeArray*)&dst, dst_pos,
                          length);
    return;
  }

  Throw::array_store_exception(arraycopy_incompatible_types
                               JVM_NO_CHECK_AT_BOTTOM);
}

void Java_java_lang_System_quickNativeThrow() {
  GUARANTEE(_jvm_quick_native_exception, 
            "must have pending exception by quick native method");
  GUARANTEE(!_jvm_in_quick_native_method, 
            "this method must not be quick native");
  KNI_ThrowNew(_jvm_quick_native_exception, NULL);
  _jvm_quick_native_exception = NULL;
}

void Java_com_sun_cldchi_jvm_JVM_createSysImage(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_ROM_GENERATOR && USE_SOURCE_IMAGE_GENERATOR
  CurrentSourceROMWriter rom_writer;

  bool suspended = rom_writer.execute(JVM_SINGLE_ARG_NO_CHECK);
  if (suspended && USE_SOURCE_IMAGE_GENERATOR) {
    // This is a temporary fix for source romizer -- we want to invoke
    // JVM.generateRomImage() in a loop, so that we can run the pending 
    // entries (created as part of class initialization) in between calls
    // to JVM.generateRomImage(). However, during source romizations,
    // constants pools and bytecodes are rewritten, it's not safe to be
    // in the middle of an executing Java method that makes a loop. Instead,
    // we use this approach to create pending entries so that generateRomImage()
    // is called again.
    //
    // We don't have such a problem during binary romization, so
    // we can affort to have a loop inside JVM.createAppImage().

    UsingFastOops fast_oops;
    InstanceClass::Fast klass = Universe::jvm_class()->obj();

    // Find the generateROMImage() method to invoke
    Method::Fast method =
      klass().lookup_method(Symbols::create_sys_image_name(),
                            Symbols::void_signature());
    GUARANTEE(method.not_null() && method().is_static(), "sanity");

    // The return type must be void for this method to work. Otherwise you'd
    // see the romizer error after romization is completed.
    Signature::Fast sig = method().signature();
    GUARANTEE_R(sig().return_type() == T_VOID, "must be void");

    // Create a delayed activation for the generateROMImage() method
    EntryActivation::Fast entry =
        Universe::new_entry_activation(&method, 0 JVM_CHECK);
    Thread::current()->append_pending_entry(&entry);
  }
#else
  Throw::error(romizer_not_supported JVM_THROW);
#endif
}

void Java_com_sun_cldchi_jvm_JVM_startAppImage(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_ROM_GENERATOR && USE_BINARY_IMAGE_GENERATOR
  UsingFastOops fast_oops;
  FilePath::Fast input  = GET_PARAMETER_AS_OOP(1);
  FilePath::Fast output = GET_PARAMETER_AS_OOP(2);
  int flags = KNI_GetParameterAsInt(3);

  BinaryROMWriter rom_writer;
  GenerateROMImage ++;
  rom_writer.start(&input, &output, flags JVM_NO_CHECK);
  GenerateROMImage --;
#else
  Throw::error(romizer_not_supported JVM_THROW);
#endif
}

bool Java_com_sun_cldchi_jvm_JVM_createAppImage0(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_ROM_GENERATOR && USE_BINARY_IMAGE_GENERATOR
  BinaryROMWriter rom_writer;
  GenerateROMImage ++;
  bool suspended = rom_writer.execute(JVM_SINGLE_ARG_NO_CHECK);
  GenerateROMImage --;
  return suspended;
#else
  Throw::error(romizer_not_supported JVM_THROW_0);
#endif
}

int Java_com_sun_cldchi_jvm_JVM_getAppImageProgress(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_ROM_GENERATOR && USE_BINARY_IMAGE_GENERATOR
  JVM_IGNORE_TRAPS;
  return BinaryROMWriter::get_progress();
#else
  Throw::error(romizer_not_supported JVM_THROW_0);
#endif
}

void Java_com_sun_cldchi_jvm_JVM_cancelImageCreation(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_ROM_GENERATOR && USE_BINARY_IMAGE_GENERATOR
  JVM_IGNORE_TRAPS;
  BinaryROMWriter::cancel();
#else
  Throw::error(romizer_not_supported JVM_THROW);
#endif
}

// java.lang.Runtime natives

// private native void exitInternal(int status);
void Java_java_lang_Runtime_exitInternal(JVM_SINGLE_ARG_TRAPS) {
  if (VerifyGC) {
    ObjectHeap::verify();
  }

  jint code = KNI_GetParameterAsInt(1);

  if (JVMSPI_CheckExit() == KNI_FALSE) {
    Throw::allocate_and_throw(Symbols::java_lang_SecurityException(),
                              exit_not_allowed JVM_NO_CHECK_AT_BOTTOM);
  } else {
#if ENABLE_ISOLATES
    Task::current()->forward_stop(code, Task::SELF_EXIT JVM_NO_CHECK_AT_BOTTOM);
#else
    JVM::stop(code);
#endif
  }
}

void Java_java_lang_Runtime_gc(JVM_SINGLE_ARG_TRAPS) {
  ObjectHeap::full_collect(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

jlong Java_java_lang_Runtime_freeMemory() {
  return ObjectHeap::available_for_current_task();
}

jlong Java_java_lang_Runtime_totalMemory() {
  return ObjectHeap::total_memory();
}

// java.lang.Class natives

// public native String getName();
OopDesc* Java_java_lang_Class_getName(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaClassObj::Raw receiver = GET_PARAMETER_AS_OOP(0);

#if ENABLE_REFLECTION
  if (receiver().is_primitive()) {
    const char * name = 
      ParsedTypeSymbol::type_name_for(receiver().type_symbol());
    GUARANTEE(name != NULL, "sanity");
    return Universe::new_string(name, jvm_strlen(name) JVM_NO_CHECK_AT_BOTTOM);
  }
#endif

  JavaClass::Fast c = receiver().java_class();

  int dimension = 0;
  int instance_pad = 0;
  Symbol::Fast name;
  String::Fast result;

  if (c().is_array_class()) {
    for (;;) {
      if (c().is_type_array_class()) {
        static const char table[] = {
          /* T_BOOLEAN */ 'Z',
          /* T_CHAR    */ 'C',
          /* T_FLOAT   */ 'F',
          /* T_DOUBLE  */ 'D',
          /* T_BYTE    */ 'B',
          /* T_SHORT   */ 'S',
          /* T_INT     */ 'I',
          /* T_LONG    */ 'J',
        };

        dimension++;
        TypeArrayClass::Raw tac = c.obj();
        int type = (int)tac().type();
        GUARANTEE(T_BOOLEAN <= type && type <= T_LONG, "sanity");

        LiteralStream ls((char*)(table + type - T_BOOLEAN), 0, 1);
        result = Universe::new_string(&ls, dimension, 0 JVM_CHECK_0);
        break;
      } else if (c().is_instance_class()) {
        InstanceClass::Raw ic = c.obj();
        name = ic().name();
        instance_pad = 1;
        break;
      } else if (c().is_obj_array_class()) {
        dimension++;
        ObjArrayClass::Raw oac = c.obj();
        c = oac().element_class();
      }
    }
  } else {
    name = c().name();
  }

  if (name.not_null()) {
    SymbolStream us(&name);
    us.set_translation('/', '.');
    result = Universe::new_string(&us, dimension+instance_pad, instance_pad
                                  JVM_CHECK_0);
  } else {
    GUARANTEE(result.not_null(), "should have been set");
  }

  {
    TypeArray::Raw t = result().value();
    if (instance_pad) {
      t().char_at_put(dimension, 'L');
      t().char_at_put(t().length()-1, ';');
    }
    for (int i=0; i<dimension; i++) {
      t().char_at_put(i, '[');
    }
  }

  return result;
}

// public static native Class forName(String className)
ReturnOop Java_java_lang_Class_forName(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  String::Fast external_class_name = GET_PARAMETER_AS_OOP(1);

  if (external_class_name.is_null()) {
    Throw::null_pointer_exception(empty_message JVM_THROW_0);
  }

  Symbol::Fast class_name =
      SymbolTable::slashified_symbol_for(&external_class_name JVM_CHECK_0);
  if (FieldType::is_array(&class_name)) {
    class_name = TypeSymbol::parse_array_class_name(&class_name JVM_CHECK_0);
  }

  JavaClass::Fast cl = SystemDictionary::resolve(&class_name,
                                                 ExceptionOnFailure
                                                 JVM_CHECK_0);

  AZZERT_ONLY(Symbol::Fast actual_name = cl().name());
  GUARANTEE(actual_name().matches(&class_name),
            "Inconsistent class name lookup result");

  // For hidden classes we throw ClassNotFoundException if lookup
  // is directly performed by user's code.
  // Also note that we have to test 2 frames, as first one is always one
  // containing Class.forName() call, so if you'll write additional 
  // wrapper for forName() increase argument to has_user_frames_until()
  if (cl().is_hidden() && Thread::current()->has_user_frames_until(2)) {
    Throw::class_not_found(&class_name, ErrorOnFailure JVM_THROW_0);
  } 

#if ENABLE_ISOLATES
  JavaClassObj::Fast result =
    cl().get_or_allocate_java_mirror(JVM_SINGLE_ARG_CHECK_0);
#else
  JavaClassObj::Fast result = cl().java_mirror();
#endif

  GUARANTEE(!result.is_null(), "mirror must not be null");

  if (cl().is_instance_class()) {
    ((InstanceClass*)&cl)->initialize(JVM_SINGLE_ARG_CHECK_0);
  }
  return result.obj();
}

OopDesc* Java_java_lang_Class_newInstance(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  Thread *thread = Thread::current();
  JavaFrame frame(thread);
  JavaClassObj::Fast receiver = GET_PARAMETER_AS_OOP(0);
#if ENABLE_REFLECTION
  if (receiver().is_primitive()) {
    Throw::instantiation(ExceptionOnFailure JVM_THROW_0);
  }
#endif
  JavaClass::Fast receiver_class = receiver().java_class();
  frame.caller_is(frame);
  Method::Fast  method = frame.method(); // the method which called
                                          // java.lang.Class.newInstance()
  InstanceClass::Fast sender_class = method().holder();

  return receiver_class().new_initialized_instance(&sender_class, thread 
                                                   JVM_NO_CHECK_AT_BOTTOM_0);
}

// public native boolean isInstance(Object obj);
jint Java_java_lang_Class_isInstance() {
  JavaClassObj::Raw receiver = GET_PARAMETER_AS_OOP(0);
#if ENABLE_REFLECTION
  if (receiver().is_primitive()) {
    return false;
  }
#endif
  Oop::Raw argument = GET_PARAMETER_AS_OOP(1);
  if (argument.is_null()) {
    return false;
  }

  // Fetch the real class and ask the question
  JavaClass::Raw c = receiver().java_class();
  JavaClass::Raw other = argument().blueprint();
  return other().is_subtype_of(&c);
}

// public native boolean isAssignableFrom(Class cls);
jint Java_java_lang_Class_isAssignableFrom(JVM_SINGLE_ARG_TRAPS) {
  JavaClassObj::Raw receiver = GET_PARAMETER_AS_OOP(0);
  JavaClassObj::Raw argument = GET_PARAMETER_AS_OOP(1);
  if (argument.is_null()) {
    Throw::null_pointer_exception(empty_message JVM_THROW_0);
  }
  
#if ENABLE_REFLECTION
  if (receiver().is_primitive()) {
    return receiver.equals(&argument);
  }
#endif

  // Fetch the real class and ask the question
  JavaClass::Raw c = receiver().java_class();
  JavaClass::Raw other = argument().java_class();
  return other().is_subtype_of(&c);
}

// public native boolean isInterface();
jint Java_java_lang_Class_isInterface() {
  JavaClassObj::Raw receiver = GET_PARAMETER_AS_OOP(0);

  // Fetch the real class and ask the question
  JavaClass::Raw c = receiver().java_class();
  return c().is_instance_class() && ((InstanceClass*)&c)->is_interface();
}

// public native boolean isArray();
jint Java_java_lang_Class_isArray() {
  JavaClassObj::Raw receiver = GET_PARAMETER_AS_OOP(0);
  // Fetch the real class and ask the question
  JavaClass::Raw c = receiver().java_class();
  return c().is_type_array_class() || c().is_obj_array_class();
}

// private native void invoke_verify();
void Java_java_lang_Class_invoke_1verify(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaClassObj::Raw receiver = GET_PARAMETER_AS_OOP(0);
  JavaClass::Fast c = receiver().java_class();
  if (get_UseVerifier() && c().is_instance_class()) {
    InstanceClass* ic = (InstanceClass*)&c;
    ic->verify(JVM_SINGLE_ARG_CHECK);
  }
}

// private native void invoke_clinit();
void Java_java_lang_Class_invoke_1clinit(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaClassObj::Raw receiver = GET_PARAMETER_AS_OOP(0);
  JavaClass::Fast c = receiver().java_class();
  if (c().is_instance_class()) {
    InstanceClass* ic = (InstanceClass*)&c;
    ic->clinit(JVM_SINGLE_ARG_CHECK);
  }
}

// private native void init9();
void Java_java_lang_Class_init9(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaClassObj::Raw receiver = GET_PARAMETER_AS_OOP(0);
  JavaClass::Fast c = receiver().java_class();
  if (c().is_instance_class()) {
    InstanceClass* ic = (InstanceClass*)&c;
#if ENABLE_ISOLATES
    // Remove class initialization barrier.
    ic->set_initialized();
#else
    ic->remove_clinit();
#endif
#if USE_EMBEDDED_VTABLE_BITMAP
    ic->update_vtable_bitmaps(JVM_SINGLE_ARG_CHECK);
#endif
  }
}

// private native Class getSuperclass();
ReturnOop Java_java_lang_Class_getSuperclass(JVM_SINGLE_ARG_TRAPS) {
#ifndef PRODUCT
  {
    // Change to doneit = 0 for studying stack frames as discussed
    // in doc/misc/TechFAQ.html.
    static int doneit = 1;
    if (!doneit) {
      doneit = 1;
      PrintExtraLongFrames = 1;
      ps();
    }
  }
#endif
  // IMPL_NOTE: why does java_mirror() need to allocate in this case? super class
  // and this class should already have been initialized.
  // Possibly because we are in Class.initialize() and we are in the process
  // of initializing the derived class.
  UsingFastOops fast_oops;
  JavaClassObj::Fast receiver = GET_PARAMETER_AS_OOP(0);
#if ENABLE_REFLECTION
  if (receiver().is_primitive()) {
    return NULL;
  }
#endif
  JavaClass::Fast c = receiver().java_class();
  JavaClass::Fast s = c().super();
#if ENABLE_ISOLATES
  return s.is_null() ? NULL :
    s().get_or_allocate_java_mirror(JVM_SINGLE_ARG_NO_CHECK);
#else
  JVM_IGNORE_TRAPS;
  return s.is_null() ? NULL : s().java_mirror();
#endif
}


// java.lang.Object natives

// public final native Class getClass();
ReturnOop Java_java_lang_Object_getClass(JVM_SINGLE_ARG_TRAPS) {
  // IMPL_NOTE: why does java_mirror() need to allocate in this case?
  // This class should already have been initialized.
  UsingFastOops fast_oops;

  JavaOop::Fast receiver = GET_PARAMETER_AS_OOP(0);
  JavaClass::Fast cl = receiver().blueprint();
#if ENABLE_ISOLATES
  return cl().get_or_allocate_java_mirror(JVM_SINGLE_ARG_NO_CHECK);
#else
  JVM_IGNORE_TRAPS;
  return cl().java_mirror();
#endif
}

// public int hashCode();
jint Java_java_lang_Object_hashCode(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaOop::Fast thisObj = GET_PARAMETER_AS_OOP(0);
  return Synchronizer::hash_code(&thisObj JVM_NO_CHECK_AT_BOTTOM_0);
}

void Java_java_lang_Object_notify(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaOop::Fast receiver = GET_PARAMETER_AS_OOP(0);
  Scheduler::notify(&receiver, false JVM_NO_CHECK_AT_BOTTOM);
}

void Java_java_lang_Object_notifyAll(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaOop::Fast receiver = GET_PARAMETER_AS_OOP(0);
  Scheduler::notify(&receiver, true JVM_NO_CHECK_AT_BOTTOM);
}

// public final native void wait(long timeout) throws InterruptedException;
void Java_java_lang_Object_wait(JVM_SINGLE_ARG_TRAPS) {
  if (Universe::is_stopping()) {
    Throw::uncatchable(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  } else {
    UsingFastOops fast_oops;
    JavaOop::Fast receiver = GET_PARAMETER_AS_OOP(0);
    jlong millis = KNI_GetParameterAsLong(1);

    if (millis < 0L) {
      Throw::throw_exception(Symbols::java_lang_IllegalArgumentException()
                             JVM_THROW);
    } else {
      Scheduler::wait(&receiver, millis JVM_NO_CHECK_AT_BOTTOM);
    }
  }
}

jint Java_java_lang_String_hashCode() {
  String::Raw this_obj = GET_PARAMETER_AS_OOP(0);

  return this_obj().hash();
}

jint Java_java_lang_String_lastIndexOf__I() {
  String::Raw this_obj = GET_PARAMETER_AS_OOP(0);
  jint ch = KNI_GetParameterAsInt(1);

  if ((ch & 0xFFFF0000) != 0) {
    return -1;
  }

  // Pass the largest positive integer as fromIndex.
  return this_obj().last_index_of((jchar)ch, 0x7FFFFFFF);
}

jint Java_java_lang_String_lastIndexOf__II() {
  String::Raw this_obj = GET_PARAMETER_AS_OOP(0);
  jint ch = KNI_GetParameterAsInt(1);
  jint fromIndex = KNI_GetParameterAsInt(2);

  if ((ch & 0xFFFF0000) != 0) {
    return -1;
  }

  return this_obj().last_index_of((jchar)ch, fromIndex);
}
// java.lang.Thread natives

ReturnOop Java_java_lang_Thread_currentThread(){
  return Thread::current()->thread_obj();
}

void Java_java_lang_Thread_yield() {
  Scheduler::yield();
}

// public static native void sleep(long millis) throws InterruptedException;
void Java_java_lang_Thread_sleep(JVM_SINGLE_ARG_TRAPS) {
  jlong millis = KNI_GetParameterAsLong(1);
  if (millis < 0L) {
    Throw::throw_exception(Symbols::java_lang_IllegalArgumentException()
                           JVM_THROW);
  } else {
#if ENABLE_CLDC_11
    if (Thread::current()->is_pending_interrupt()) {
      Scheduler::handle_pending_interrupt(JVM_SINGLE_ARG_NO_CHECK);
    } else
#endif
    if (Universe::is_stopping()) {
      Throw::uncatchable(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
    } else {
      Scheduler::sleep_current_thread(millis);
    }
  }
}

// private native void start0();
void Java_java_lang_Thread_start0(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ThreadObj::Fast receiver = GET_PARAMETER_AS_OOP(0);
  if (!receiver().is_unstarted()) {
    Throw::illegal_thread_state_exception(already_started JVM_THROW);
  }
  receiver().set_stillborn();
  Thread::Fast new_thread = Thread::allocate(JVM_SINGLE_ARG_CHECK);
  new_thread().set_thread_obj(&receiver);
#if ENABLE_ISOLATES
  {
    UsingFastOops fast_oops_inside;
    Thread *thread = Thread::current();
    int tid = thread->task_id();
    Task::Fast t = thread->task_for_thread();
    GUARANTEE(!t.is_null(), "Null task");
    new_thread().set_task_id(tid);
    if (t().status() == Task::TASK_STOPPING) {
      Thread::set_current_pending_exception(Task::get_termination_object());
      return;
    }
    new_thread().start(JVM_SINGLE_ARG_CHECK);
    t().add_thread();
  }
#else
  new_thread().start(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
#endif
}

// private synchronized native void internalExit();
void Java_java_lang_Thread_internalExit() {
  Thread::finish();
}

// public final native boolean isAlive();
jint Java_java_lang_Thread_isAlive() {
  ThreadObj::Raw receiver = GET_PARAMETER_AS_OOP(0);
  return receiver().is_alive();
}

jint Java_java_lang_Thread_activeCount() {
  return Scheduler::active_count();
}

// private native void setPriority0(int oldPriority, int newPriority);
void Java_java_lang_Thread_setPriority0() {
  ThreadObj::Raw receiver = GET_PARAMETER_AS_OOP(0);
  int old_priority = KNI_GetParameterAsInt(1);
  int new_priority = KNI_GetParameterAsInt(2);
  receiver().set_priority(new_priority);
  if (receiver().is_alive()) {
    Thread::Raw t = receiver().thread();
    Scheduler::set_priority(&t, old_priority);
  }
}

#if ENABLE_CLDC_11
void Java_java_lang_Thread_interrupt0(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ThreadObj::Fast receiver = GET_PARAMETER_AS_OOP(0);
  Thread::Fast t;
  if (receiver().is_alive()) {
    t = receiver().thread();
    Scheduler::interrupt_thread(&t JVM_NO_CHECK_AT_BOTTOM);
  }
}
#endif

#if ENABLE_STACK_TRACE

void Java_java_lang_Throwable_fillInStackTrace(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JavaFrame frame(Thread::current());
  Frame pass1(frame);
  Frame pass2(frame);
  Throwable::Fast throwable = GET_PARAMETER_AS_OOP(0);
  Method::Fast method;
  Symbol::Fast name;
  InstanceClass::Raw holder, exception_class;

  int skip_size = 0;
  int stack_size = 0;

  {
    FrameStream st(pass1);

    // skip fillInStackTrace
    if (!st.at_end()) {
      method = st.method();
      name = method().name();
      if (name.equals(Symbols::fillInStackTrace_name())) {
        st.next();
        skip_size++;
      }
    }

    // skip <init> methods of the exceptions klass. If there is <init>
    // methods that belongs to a superclass of the exception we are
    // going to skipping them in the stack trace. This is similar to
    // classic VM.

    while (!st.at_end()) {
      method = st.method();
      name = method().name();
      holder = method().holder();
      exception_class = throwable.blueprint();
      if (!(name.equals(Symbols::object_initializer_name()) &&
            exception_class().is_subtype_of(&holder))) {
        break;
      }
      st.next();
      skip_size++;
    }

    stack_size = 0;
    while (!st.at_end()) {
      st.next();
      stack_size++;
    }
  }

  // Allocate the trace
  ObjArray::Fast methods = Universe::new_obj_array(stack_size JVM_CHECK);
  TypeArray::Fast offsets = Universe::new_int_array(stack_size JVM_CHECK);

  {
    ObjArray::Raw trace = Universe::new_obj_array(2 JVM_CHECK);
    trace().obj_at_put(0, &methods);
    trace().obj_at_put(1, &offsets);
    throwable().set_backtrace(&trace);
  }

  // Fill in the trace
  FrameStream st(pass2);
  int index;
  for (index = 0; index < skip_size; index++) {
    st.next();
  }
  for (index = 0; index < stack_size; index++) {
    method = st.method();
    methods().obj_at_put(index, &method);
    offsets().int_at_put(index, st.bci());
    st.next();
  }
}

#else
// !ENABLE_STACK_TRACE
void Java_java_lang_Throwable_fillInStackTrace() {
}
#endif

#if ENABLE_CLDC_111
jobject Java_java_lang_Throwable_obtainBackTrace() {
  UsingFastOops fast;
  Throwable::Fast throwable = GET_PARAMETER_AS_OOP(0);  
  ObjArray::Fast backtrace;  
#if ENABLE_STACK_TRACE
  SETUP_ERROR_CHECKER_ARG;
  ObjArray::Fast compr_backtrace = throwable().backtrace();
  if (compr_backtrace.is_null()) {    
    return NULL;
  }
  
  TypeArray::Fast offsets = compr_backtrace().obj_at(1);      
  if (offsets.is_null()) {
    return NULL;
  }
  int length = offsets().length();
  
  backtrace = Universe::new_obj_array(3 JVM_CHECK_0);
  ObjArray::Fast class_names = Universe::new_obj_array(length JVM_CHECK_0);
  ObjArray::Fast method_names = Universe::new_obj_array(length JVM_CHECK_0);
  //copy offsets to new backtrace
  backtrace().obj_at_put(0, &class_names);
  backtrace().obj_at_put(1, &method_names);
  backtrace().obj_at_put(2, &offsets);
  
  ObjArray::Fast methods = compr_backtrace().obj_at(0);
  Method::Fast m;
  Symbol::Fast method_name;
  Symbol::Fast class_name;
  if (!methods.is_null() && !offsets.is_null()) {
    int i;      
    for (i=0; i<methods().length(); i++) {
      m = methods().obj_at(i);
      if (m.is_null()) {          break;        }
       method_name = m().name();
#ifndef PRODUCT        
      // Non-public methods in a romized image may be renamed to
      // .unknown. to save space. In non-product mode, to aid
      // debugging, we retrieve the original name using
      // ROM::get_original_method_name().
      if (method_name().equals(Symbols::unknown())) {
        method_name = ROM::get_original_method_name(&m);
      }
#endif
      String::Raw method_name_str = Universe::new_string(&method_name JVM_CHECK_0);      
      method_names().obj_at_put(i, &method_name_str);
      
      InstanceClass::Raw ic = m().holder();
      class_name = ic().name();
      String::Raw class_name_str = Universe::new_string(&class_name JVM_CHECK_0);      
      class_names().obj_at_put(i, &class_name_str);      
    }
  }
#endif
  return (jobject)backtrace.obj();
}
#else
// public void printStackTrace();
void Java_java_lang_Throwable_printStackTrace() {
  Throwable::Raw throwable = GET_PARAMETER_AS_OOP(0);
  throwable().print_stack_trace();
}
#endif

// static native Object open(String name);
ReturnOop Java_com_sun_cldc_io_ResourceInputStream_open(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  String::Fast j_filename = GET_PARAMETER_AS_OOP(1);
  Symbol::Fast c_filename = SymbolTable::symbol_for(&j_filename JVM_CHECK_0);
  Buffer::Fast resource_data;

  // Scan for romized resource with the given name
  int length = Universe::resource_names()->length();
  for (int i = 0; i < length; i++) {
    if (Universe::resource_names()->obj_at(i) == c_filename) {
      resource_data = Universe::resource_data()->obj_at(i);
      int resource_size = Universe::resource_size()->int_at(i);
      int flags = resource_size < 0 ? LAST_BLOCK : INCREMENTAL_INFLATE;
      Inflater::Raw inflater = Inflater::allocate(NULL, 0, resource_size, -1,
                                                  0, flags JVM_CHECK_0);
      if (resource_size < 0) { // uncompressed resource
        resource_size = resource_data().length();
        inflater().set_out_buffer(&resource_data);
        inflater().set_out_offset(resource_size);
        inflater().set_file_size(resource_size);
        inflater().set_bytes_remain(resource_size);
      } else {                 // inflated resource
        inflater().set_in_buffer(&resource_data);
      }
      return inflater;
    }
  }

  // Open resource from a file
  return ClassPathAccess::open_entry(&c_filename, false
                                     JVM_NO_CHECK_AT_BOTTOM_0);
}

// static native int bytesRemain(Object fileDecoder);
int Java_com_sun_cldc_io_ResourceInputStream_bytesRemain() {
  FileDecoder::Raw fd = GET_PARAMETER_AS_OOP(1);
  return fd().bytes_remain();
}

// static native int readByte(Object fileDecoder);
int Java_com_sun_cldc_io_ResourceInputStream_readByte(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  unsigned char result;
  ArrayPointer destination(&result);  
  FileDecoder::Fast fd = GET_PARAMETER_AS_OOP(1);
  if (fd().bytes_remain() <= 0) {
    return -1;
  }
  fd().get_bytes(&destination, 1 JVM_CHECK_0);
  return result;
}

// static native int readBytes(Object fileDecoder, byte b[], int off, int len);
int Java_com_sun_cldc_io_ResourceInputStream_readBytes(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  FileDecoder::Fast fd = GET_PARAMETER_AS_OOP(1);
  TypeArray::Fast b = GET_PARAMETER_AS_OOP(2);
  int len = KNI_GetParameterAsInt(4);
  ArrayPointer destination(&b, KNI_GetParameterAsInt(3)); 
  if (fd().bytes_remain() <= 0) {
    return -1;
  }
  return fd().get_bytes(&destination, len JVM_NO_CHECK_AT_BOTTOM_0);
}

// static native Object clone(Object source);
ReturnOop Java_com_sun_cldc_io_ResourceInputStream_clone(JVM_SINGLE_ARG_TRAPS) {
  return ObjectHeap::clone(GET_PARAMETER_AS_OOP(1) JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_CLDC_11

// native void initializeWeakReference(Object referent);
void
Java_java_lang_ref_WeakReference_initializeWeakReference(JVM_SINGLE_ARG_TRAPS)
{
  UsingFastOops fast_oops;
  WeakReference::Fast thisObj = GET_PARAMETER_AS_OOP(0);
  Oop::Fast referent = GET_PARAMETER_AS_OOP(1);

  thisObj().set_referent_index(-1); // in case of failure, or if referent==NULL

  if (referent.not_null()) {
    const ObjectHeap::ReferenceType type = ObjectHeap::WEAK;
    const int refIndex = ObjectHeap::register_global_ref_object(&referent,
      type JVM_MUST_SUCCEED);
    if( refIndex < 0 ) {
      Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
    }
    thisObj().set_referent_index(refIndex);
  }
}

// native Object get();
ReturnOop Java_java_lang_ref_WeakReference_get() {
  WeakReference::Raw thisObj = GET_PARAMETER_AS_OOP(0);
  jint refIndex = thisObj().referent_index();
  if (refIndex < 0) {
    return NULL;
  } else {
    return ObjectHeap::get_global_ref_object(refIndex);
  }
}

// native void clear();
void Java_java_lang_ref_WeakReference_clear() {
  WeakReference::Raw thisObj = GET_PARAMETER_AS_OOP(0);
  jint refIndex = thisObj().referent_index();
  if (refIndex >= 0) {
    ObjectHeap::unregister_global_ref_object(refIndex);
    thisObj().set_referent_index(-1);
  }
}

// native void finalize();
void Java_java_lang_ref_WeakReference_finalize() {
  WeakReference::Raw thisObj = GET_PARAMETER_AS_OOP(0);
  jint refIndex = thisObj().referent_index();
  if (refIndex >= 0) {
    ObjectHeap::unregister_global_ref_object(refIndex);
    thisObj().set_referent_index(-1); // just for sanity
  }
}

ReturnOop Java_java_lang_String_intern(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  String::Fast thisObj = GET_PARAMETER_AS_OOP(0);
  return Universe::interned_string_for(&thisObj JVM_NO_CHECK_AT_BOTTOM_0);
}

#endif  // ENABLE_CLDC_11

#if ENABLE_SEMAPHORE
void Java_com_sun_cldc_util_SemaphoreLock_release() {
  UsingFastOops fast_oops;
  SemaphoreLock::Fast thisObj = GET_PARAMETER_AS_OOP(0);
  thisObj().release();
}

void Java_com_sun_cldc_util_SemaphoreLock_acquire() {
  UsingFastOops fast_oops;
  SemaphoreLock::Fast thisObj = GET_PARAMETER_AS_OOP(0);
  thisObj().acquire();
}
#endif

jint Java_com_sun_cldchi_jvm_JVM_verifyNextChunk(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_VERIFY_ONLY
  UsingFastOops fast_oops;
  String::Fast jar_str = GET_PARAMETER_AS_OOP(1);
  jint next_chunk_id = KNI_GetParameterAsInt(2);
  jint chunk_size = KNI_GetParameterAsInt(3);

  GlobalSaver verify_saver(&VerifyOnly);
  VerifyOnly = 1;

#ifndef PRODUCT
  tty->print("Verifying JAR ");
  jar_str().print_string_on(tty);
  tty->print_cr(" chunk_id: %d", next_chunk_id);
#endif

  FilePath::Fast path = FilePath::from_string(&jar_str JVM_CHECK_0);
  return Universe::load_next_and_verify(&path, next_chunk_id,
                                        chunk_size JVM_NO_CHECK_AT_BOTTOM);

#else

  return -1;

#endif
}

} // extern "C"

#if (!ROMIZING) || (!defined(PRODUCT))

void Natives::register_function(InstanceClass* c,
                                const JvmNativeFunction* functions,
                                bool is_native JVM_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast methods = c->methods();
  Symbol::Fast name;
  Signature::Fast signature;
  Method::Fast m;

  for (const JvmNativeFunction* func = functions; func->name != NULL; func++) {
    name      = SymbolTable::symbol_for(func->name JVM_CHECK);
    signature = TypeSymbol::parse(func->signature JVM_CHECK);
    address f = (address)(func->function);
    for (int i=0; i<methods().length(); i++) {
      m = methods().obj_at(i);
      if (m().match(&name, &signature)) {
        if (is_native) {
          GUARANTEE(m().is_native(), "must be native");
          m().set_native_code(f);
        } else {
          m().set_fixed_entry(f);
        }
      }
    }
  }
}

void Natives::register_natives_for(InstanceClass* c JVM_TRAPS) {
  UsingFastOops fast_oops;
  Symbol::Fast class_name = c->name();
  Symbol::Fast name;

#if (!ROMIZING) || (!defined(PRODUCT))
  for (const JvmNativesTable* table= jvm_natives_table; table->name!= NULL;
       table++) {
    name = SymbolTable::symbol_for(table->name JVM_CHECK);
    if (class_name.equals(&name)) {
      if (table->natives != NULL) {
        register_function(c, table->natives, true JVM_CHECK);
      }
      if (table->entries != NULL) {
        register_function(c, table->entries, false JVM_CHECK);
      }
      return;
    }
  }
#endif

  // Ignore classes that are not in the native table
  // This is needed to pass several TCK tests (very very strange)
}

#endif /* !ROMIZING || !PRODUCT */

#if ENABLE_TTY_TRACE
extern "C" {
address _current_native_function;
}

extern "C" void trace_native_call() {
  // IMPL_NOTE: KVM native funcs not traced.
  const JvmNativesTable* table;
  const JvmNativeFunction* native;

  tty->print("native_call: ");

  for (table = jvm_natives_table; table->name!= NULL; table++) {
    if (table->natives == NULL) {
      continue;
    }
    for (native = table->natives; native->name != NULL; native++) {
      if (native->function == _current_native_function) {
        tty->print_cr("%s.%s:%s", table->name, native->name,
                      native->signature);
        return;
      }
    }
  }

  tty->print_cr("(unknown)");
}
#endif

