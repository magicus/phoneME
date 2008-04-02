
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
 * Generator.cpp: Assembly code generator
 *
 * This is the general assembly code generator. It generates
 * assembly code, such as the byte code interpreter and stubs,
 * that can be compiled and linked into the runtime system.
 *
 * Automatic generation of assembly code makes the maintenance
 * of the byte code interpreter and other machine-specific
 * parts of the system significantly easier, since we don't
 * have to maintain or edit raw assembly code files.  Rather,
 * the VM is capable of generating the bulk of the
 * machine-specific code itself.
 *
 * The assembly code generator is supported only in
 * non-product builds.
 *
 * The typical development procedure is as follows:
 * (1) Build a debug version of the VM
 * (2) Run the VM with the "-generate"
 *     or "-generateoptimized" option to generate
 *     the assembly code interpreter.  The former
 *     option generates the debug version of the
 *     interpreter, while the latter option should
 *     be used for product builds.
 * (3) Rebuild the VM with the generated
 *     assembly code interpreter included.
 *
 * Note: If your target device cannot easily support
 * in-device generation of assembly code (e.g., because
 * of the lack of a file system), you can use the special
 * "loopgen" tool provided in the "/build/loopgen" directory
 * to generate the necessary assembly code on the development
 * workstation.
 */

#include "incls/_precompiled.incl"
#include "incls/_Generator.cpp.incl"

#define OOPMAP_GENERATOR(c) \
  { #c, c::iterate_oopmaps },

#if ENABLE_INTERPRETER_GENERATOR
void Generator::generate() {
  JvmPathChar asm_name[128];
  JvmPathChar arch[10];
  int i;
  const char *archstr = INTERPRETER_ARCH_NAME; // defined in auto-generated
                                               // jvmconfig.h
  for (i=0; archstr[i]; i++) {
    arch[i] = (JvmPathChar)archstr[i];
  }
  arch[i] = (JvmPathChar)0;

  JvmPathChar *extension;
  JvmPathChar extension_c[]   = {'.', 'c', 0};
  JvmPathChar extension_s[]   = {'.', 's', 0};
  JvmPathChar extension_asm[] = {'.', 'a', 's', 'm', 0};
  JvmPathChar interpreter[] = {
    'I','n','t','e','r','p','r','e','t','e','r','_', 0
  };

  if (GenerateInlineAsm) {
    extension = extension_c;
  } else {
    if (GenerateGNUCode) {
      extension = extension_s;
    } else {
      extension = extension_asm;
    }
  }

  // Our current interpreter loop design requires INTERP_LOG_SIZE-1 be
  // a power of 2.
  GUARANTEE_R(is_power_of_2(INTERP_LOG_SIZE-1), "sanity");

  asm_name[0] = 0;
  if (Arguments::generator_output_dir() != NULL) {
    JvmPathChar sep[] = {'/', 0};
    fn_strcat(asm_name, Arguments::generator_output_dir());
    fn_strcat(asm_name, sep);
  }
  fn_strcat(asm_name, interpreter);
  fn_strcat(asm_name, arch);
  fn_strcat(asm_name, extension);

  FileStream file(asm_name);
  DupStream ds(tty, &file);
  Stream *output;
  if (Verbose) {
    output = &ds;
  } else {
    output = &file;
  }

  if (!GenerateGPTableOnly) {
    tty->print("Generating %s assembly code: ... ",
               GenerateDebugAssembly ? "debug" : "optimized");
    JvmPathChar *p = asm_name;
    while (*p) {
      tty->print("%c", (char)*p++);
    }
    tty->cr();

    // Create a macro assembler.
    SourceMacros assembler(output);
    assembler.start();

    // Generate the interpreter.
    InterpreterGenerator interpreter_generator(output);
    interpreter_generator.generate();

    // Generate the stubs used by both the interpreter and the compiler.
    SharedStubs shared_stubs(output);
    shared_stubs.generate();

    // Generate the interpreter stubs.
    InterpreterStubs interpreter_stubs(output);
    interpreter_stubs.generate();

    // Generate the compiler stubs.
    CompilerStubs compiler_stubs(output);
    compiler_stubs.generate();

    // Generate native assembly functions.
    NativeGenerator native_generator(output);
    native_generator.generate();

#if ENABLE_FLOAT
    // Generate floating point support functions
    FloatSupport fp_generator(output);
    fp_generator.generate();
#endif

#if ENABLE_CPU_VARIANT
    CPUVariantSupport cpu_variant_support(output);
    cpu_variant_support.generate();
#endif

#if USE_COMPILER_GLUE_CODE
    // Special compiler glue code that can be efficiently linked to JIT code
    // (e.g. with a BL instruction on ARM).
    compiler_stubs.generate_glue_code();
#endif

    GPTableGenerator table_gen(output);
    table_gen.generate();

    // Stop the assembler.
    assembler.stop();
  } else {
    tty->print_cr("Generating GP table");

    GPTableGenerator table_gen(output);
    table_gen.start();
    table_gen.generate();
    table_gen.stop();
  }

  // Flush the output stream.
  output->flush();
}
#endif // ENABLE_INTERPRETER_GENERATOR

#if ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR
BasicType Generator::_prev_type;
size_t    Generator::_prev_offset;
size_t    Generator::_prev_index;
size_t    Generator::_offsets[256];
Stream*   Generator::_stream;

void Generator::save_offset(BasicType type, void *param, const char *name,
                            size_t offset, int flags) {
  GUARANTEE_R(offset > _prev_offset, 
              "fields must be iterated with increasing offsets");

  if (type == T_LONG || type == T_DOUBLE) {
    // We must order the XXXDesc fields properly, or add explicit
    // padding fields, to make sure 8-byte values are 8-byte aligned.
    // This ensures loopgen/romgen has the same C structure layout
    // as the target VM.
    //
    // IMPL_NOTE: generalize this to cover 1- and 2-byte fields as well

    int n = (int)_prev_offset + byte_size_for(_prev_type);
    int padding = n % 8;
    
    GUARANTEE_R(padding == 0,
                "8-byte values must be 8-byte aligned for portability");
    GUARANTEE_R((size_t)n == offset, 
                "8-byte values must be 8-byte aligned for portability");
    GUARANTEE_R((offset % 8) == 0, 
                "8-byte values must be 8-byte aligned for portability");
  }
  if (type == T_OBJECT && (flags & OOPMAP_VARIABLE_OBJ) == 0) {
    GUARANTEE(_prev_index <= 255, "Overflow");
    _offsets[_prev_index] = offset;
    _prev_index++;
  }
  _prev_type = type;
  _prev_offset = offset;
}

void Generator::generate_empty_oopmap(oopmaps_doer do_map, void*param) {
  // Do nothing
}

void Generator::generate_oopmap(Stream* stream, 
                                void do_class(oopmaps_doer do_map, void*param),
                                const char* name) {
  int i;

  _prev_index = 0;

  // The first slot, OopDesc::_klass, is not iterated explicitly
  _prev_type = T_OBJECT;
  _prev_offset = 0;
  
  // Callback to generator for each oop offset
  do_class(save_offset, NULL);

  int count = _prev_index;
  for (i = 0; i < count - 1; i++) {
    GUARANTEE(_offsets[i] < _offsets[i + 1], "Offsets must be increasing");
  }

  // Some compilers (e.g. gcc) want the "extern" declaration
  // before exporting oopmaps_XXX as a linkable symbol.
  stream->print_cr("extern const unsigned char oopmap_%s[];", name);
  stream->print("const unsigned char oopmap_%s[] = { ", name);
  for (i = 0; i < count; i++) { 
    int delta = _offsets[i] - ((i == 0) ? 0 : _offsets[i - 1]);
    delta = delta / BytesPerWord;
    while (delta >= OopMapEscape) {
      stream->print("%d, ", OopMapEscape);
      delta -= (OopMapEscape - 1);
    }
    stream->print("%d, ", delta);
  }
  stream->print("%d };\n", OopMapSentinel);
}


void Generator::generate_oopmaps() {
  JvmPathChar file_name[128];
  JvmPathChar oopmaps_cpp[] = {'O','o','p','M','a','p','s','.','c','p','p',0};

  static const struct {
    const char *name;
    void (*do_class)(oopmaps_doer do_map, void* param);
  } generators[] = {
    OOPMAP_CLASSES_DO(OOPMAP_GENERATOR)
  };

  int generator_count = sizeof(generators)/sizeof(generators[0]);

  file_name[0] = 0;
  if (Arguments::generator_output_dir() != NULL) {
    JvmPathChar sep[] = {'/', 0};
    fn_strcat(file_name, Arguments::generator_output_dir());
    fn_strcat(file_name, sep);
  }
  fn_strcat(file_name, oopmaps_cpp);

  tty->print_cr("Generating oopmaps");
  {
    FileStream file(file_name);
    DupStream ds(tty, &file);
    Stream *output;
    if (Verbose) {
       output = &ds;
    } else {
      output = &file;
    }
    
    const char * header = 
      "/*\n"
      " * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.\n"
      " * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER\n"
      " * \n"
      " * This program is free software; you can redistribute it and/or\n"
      " * modify it under the terms of the GNU General Public License version\n"
      " * 2 only, as published by the Free Software Foundation.\n"
      " * \n"
      " * This program is distributed in the hope that it will be useful, but\n"
      " * WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n"
      " * General Public License version 2 for more details (a copy is\n"
      " * included at /legal/license.txt).\n"
      " * \n"
      " * You should have received a copy of the GNU General Public License\n"
      " * version 2 along with this work; if not, write to the Free Software\n"
      " * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA\n"
      " * 02110-1301 USA\n"
      " * \n"
      " * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa\n"
      " * Clara, CA 95054 or visit www.sun.com if you need additional\n"
      " * information or have any questions.\n"
      " */\n"
      "\n"
      "/* This file is auto-generated. Do not edit */\n"
      "\n"
      "#include \"incls/_precompiled.incl\"\n"
      "\n"
      "extern \"C\" {\n";

    output->print("%s", header);

    generate_oopmap(output, generate_empty_oopmap, "Empty");

    for (int i = 0; i < generator_count; i++) { 
      generate_oopmap(output, generators[i].do_class, generators[i].name);
    }

    output->print("}\n\n");
    generate_oopmap_checks("loopgen", output);

    output->flush();
  }
}

void Generator::dump_entry(BasicType type, void* param, const char *name,
                           size_t offset, int flags){
  _stream->print_cr("\t(const char*)%4d, /*size=%d*/ \"%s\",",
                    offset, byte_size_for(type), name);
}

void Generator::generate_oopmap_checks(char *c_function_prefix, Stream* stream)
{
  char * prefix = c_function_prefix;
  int i;

  _stream = stream;
  static const struct {
    const char *name;
    void (*do_class)(oopmaps_doer do_map, void* param);
  } generators[] = {
    OOPMAP_CLASSES_DO(OOPMAP_GENERATOR)
  };
  int generator_count = sizeof(generators)/sizeof(generators[0]);

  stream->print_cr("#ifndef PRODUCT");
  stream->cr();

  stream->print_cr("#ifdef __cplusplus");
  stream->print_cr("extern \"C\" {");
  stream->print_cr("#endif");

  stream->print_cr("extern void jvm_check_oopmap(const char *generator_name, "
                   "const char* class_name, void *data);");

  for (i = 0; i < generator_count; i++) {
    const char *name = generators[i].name;
    stream->print_cr("static const char * %s_%s_oopmap_data[] = {", 
                     prefix,name);
    generators[i].do_class(dump_entry, NULL);
    stream->print_cr("\t(const char*)   0, (const char*)0");
    stream->print_cr("};");
  }

  stream->print_cr("void %s_check_oopmaps() {", prefix);
  for (i = 0; i < generator_count; i++) {
    const char *name = generators[i].name;
    stream->print_cr("  jvm_check_oopmap(\"%s\", \"%s\", &%s_%s_oopmap_data);",
                     prefix, name, prefix, name);
  }
  stream->print_cr("}");

  stream->print_cr("#ifdef __cplusplus");
  stream->print_cr("}");
  stream->print_cr("#endif");

  stream->print_cr("#endif /* PRODUCT*/");
}
#endif // ENABLE_INTERPRETER_GENERATOR || USE_SOURCE_IMAGE_GENERATOR

#ifndef PRODUCT
static void* _checking_oopmap_data;
static const char* _checking_oopmap_class;
static const char* _checking_oopmap_generator;

static void check_oopmap_entry(BasicType type, void* param, const char *name,
                               size_t offset, int flags) {
  char **ptr = (char**)_checking_oopmap_data;
  for (;;) {
    size_t stored_offset = (size_t)(*ptr++);
    char *stored_name = *ptr++;
    if (stored_name == NULL) {
      break;
    }
    if (jvm_strcmp(name, stored_name) == 0) {
      if (type == T_LONG || type == T_DOUBLE) {
        GUARANTEE_R((offset % 8) == 0, 
                    "8-byte values must be 8-byte aligned for portability");
      }
      GUARANTEE_R(stored_offset == offset, 
                  "offset information in generator and target VM must match");
      return;
    }
  }

  // IMPL_NOTE: check that we always use padding to align ints and longs on
  // 4-byte boundaries, and shorts on 2-byte boundaries.

  tty->print_cr("FATAL error:");
  tty->print_cr("Information about \"%s::%s\" field was not generated by "
                "an up-to-date %s",
                _checking_oopmap_class, name, _checking_oopmap_generator);
  tty->print_cr("FATAL error:");
  JVM_FATAL(empty_message);
  (void)flags; (void)param;
}

extern "C" void jvm_check_oopmap(const char *generator_name, const char* class_name,
                                 void* data) {
  _checking_oopmap_data = data;
  _checking_oopmap_generator = generator_name;
  _checking_oopmap_class = class_name;

  static const struct {
    const char *name;
    void (*do_class)(oopmaps_doer do_map, void* param);
  } generators[] = {
    OOPMAP_CLASSES_DO(OOPMAP_GENERATOR)
  };

  int generator_count = sizeof(generators)/sizeof(generators[0]);
  for (int i = 0; i < generator_count; i++) {
    if (jvm_strcmp(class_name, generators[i].name) == 0) {
      generators[i].do_class(check_oopmap_entry, NULL);
    }
  }
}
#endif
