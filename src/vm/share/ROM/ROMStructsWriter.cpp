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
#include "incls/_ROMStructsWriter.cpp.incl"

#if USE_SOURCE_IMAGE_GENERATOR

const char *ROMStructsWriter::reserved_words[] = {
    // ANSI C keywords
    "auto", "break", "case", "char", "const", "continue", "default",
    "do", "double", "else", "enum", "extern", "float", "for", "goto",
    "if", "int", "long", "register", "return", "short", "signed",
    "sizeof", "static", "struct", "switch", "typedef", "union",
    "unsigned", "void", "volatile", "while",

    // C++ keywords
    "bool", "catch", "class", "delete", "friend", "inline", "mutable",
    "new", "namespace", "operator", "private", "protected", "public",
    "template", "this", "throw", "try",

    // Misc stuff that would probably break
    "_asm", "asm", "offsetof", "__int64", "__uint64",
    "jlong", "jint", "jobject", "EOF", "NULL", "TRUE", "FALSE", "ERROR",

    // Termination
    NULL
};

void ROMStructsWriter::write(ROMWriter *rom_writer JVM_TRAPS) {
  FileStream stream;
  JvmPathChar filename[] = {'R','O','M','S','t','r','u','c','t','s','.','h',0};
  tty->print_cr("Writing ROMStructs.h ...");

  stream.open(filename);
  rom_writer->write_copyright(&stream, true);
  stream.cr();
  stream.print_cr("#ifndef _ROM_STRUCTS_H_");
  stream.print_cr("#define _ROM_STRUCTS_H_");
  stream.cr();

  stream.print_cr("#ifdef __cplusplus");
  stream.print_cr("extern \"C\" {");
  stream.print_cr("#endif");

  stream.print_cr("#define _ROM_STRUCTS_VERSION_ \"%s\"", JVM_BUILD_VERSION);
  stream.cr();

  stream.print_cr("/* Define JVM_LIMIT_OBJECT_FIELD_WRITES=0 if your C");
  stream.print_cr("   compiler does not allow the 'const' modifier");
  stream.print_cr("   in the ROM structs */");
  stream.print_cr("#ifndef JVM_LIMIT_OBJECT_FIELD_WRITES");
  stream.print_cr("#define JVM_LIMIT_OBJECT_FIELD_WRITES 0 /* IMPL_NOTE: TMP */");
  stream.print_cr("#endif");
  stream.print_cr("#if !JVM_LIMIT_OBJECT_FIELD_WRITES");
  stream.print_cr("#define JVM_FIELD_CONST");
  stream.print_cr("#else");
  stream.print_cr("#define JVM_FIELD_CONST const");
  stream.print_cr("#endif");
  stream.cr();

  // Print the primitive-array types
  write_primitive_array_rom_struct(&stream, "jboolean");
  write_primitive_array_rom_struct(&stream, "jchar");
  write_primitive_array_rom_struct(&stream, "jbyte");
  write_primitive_array_rom_struct(&stream, "jshort");
  write_primitive_array_rom_struct(&stream, "jint");
  write_primitive_array_rom_struct(&stream, "jlong");
  write_primitive_array_rom_struct(&stream, "jobject",
                           "struct Java_java_lang_Object * JVM_FIELD_CONST");
#if ENABLE_FLOAT
  write_primitive_array_rom_struct(&stream, "jfloat");
  write_primitive_array_rom_struct(&stream, "jdouble");
#endif
  stream.cr();

  if (GenerateROMStructsStatics) {
    stream.print_cr("void* kni_get_java_class_statics(jclass classHandle) ;");
  }
  // Print the class types
  for (SystemClassStream st; st.has_next();) {
    InstanceClass klass = st.next();
    if (GenerateROMStructsStatics) {
      write_rom_static_struct(&stream, &klass JVM_CHECK);
    }
    if (GenerateROMStructs) {
      write_rom_struct(&stream, &klass JVM_CHECK);
    }
  }

  stream.print_cr("#ifdef __cplusplus");
  stream.print_cr("}");
  stream.print_cr("#endif");

  stream.cr();
  stream.print_cr("#endif /*_ROM_STRUCTS_H_*/");
  stream.close();
}

void ROMStructsWriter::write_primitive_array_rom_struct(Stream *stream,
                                                        const char*type,
                                                        const char*field_type) {
  if (field_type == NULL) {
    field_type = type;
  }
  stream->print_cr("typedef struct {");
  stream->print_cr("    void * dummy;");
  stream->print_cr("    int length;");
  stream->print_cr("    %s elements[1];", field_type);
  stream->print_cr("} %s_array;", type);
}

void ROMStructsWriter::write_rom_struct(Stream *stream, 
                                        InstanceClass *klass JVM_TRAPS) {
  TypeArray byte_array = Natives::get_jni_class_name(klass JVM_CHECK);
  stream->print_cr("struct %s {", byte_array.base_address());

  write_rom_struct_fields(stream, klass, klass JVM_CHECK);

  stream->print_cr("};");
  stream->cr();
}

void ROMStructsWriter::write_rom_static_struct(Stream *stream, 
                                        InstanceClass *klass JVM_TRAPS) {
  TypeArray byte_array = Natives::get_jni_class_name(klass JVM_CHECK);
  stream->print_cr("struct %s_Class {", byte_array.base_address());

  stream->print_cr("\tvoid * __do_not_use__;");
  write_rom_struct_static_fields(stream, klass JVM_CHECK);

  stream->print_cr("};");
  stream->cr();
}

/*
 * Write the fields defined in klass and its super classes.
 * klass        = the class we're currently looking at
 * target_class = the class we're writing the rom_struct for.
 */
void ROMStructsWriter::write_rom_struct_fields(Stream *stream,
                                               InstanceClass *klass,
                                               InstanceClass *target_class
                                               JVM_TRAPS) {
  UsingFastOops level1;
  InstanceClass::Fast super = klass->super();
  Symbol::Fast name;

  if (super.not_null()) {
    // recursively write the fields of the super-classes
    write_rom_struct_fields(stream, &super, target_class JVM_CHECK);
  }

  stream->print("    /* ");
  klass->print_name_on(stream);
  stream->print_cr(" */");

  if (super.is_null()) {
    // This is java.lang.Object. Write the klass field
    stream->print_cr("\tvoid * __do_not_use__;");
  }

  TypeArray::Fast fields = klass->original_fields();
  int num_non_static_fields = fields().length()/5;
  int num_written = 0;
  int current_base = 0;
  int last_size = 0;
  
  for (int index = 0; index < fields().length(); index += 5) {
    OriginalField f(klass, index);
    if (f.is_static()) {
      num_non_static_fields --;
    }
  }

  while (num_written < num_non_static_fields) {
    int found = -1;
    int min = 0x7fffffff;
    for (int index = 0; index < fields().length(); index += 5) {
      int offset = fields().ushort_at(index + Field::OFFSET_OFFSET);
      int flags  = fields().ushort_at(index + Field::ACCESS_FLAGS_OFFSET);
      if (!(flags & JVM_ACC_STATIC)) {
        if (offset > current_base && offset < min) {
          found = index;
          min = offset;
        }
      }
    }

    OriginalField f(klass, found);
 
    GUARANTEE(found >= 0, "sanity");
    if (num_written == 0) {
      GUARANTEE((f.offset() % 4) == 0, "first field must be word-aligned");
    }

    if (!f.is_static()) {
      stream->print("    ");
      int offset = f.offset();
      if (offset < 10) {
        stream->print("/*  @%d */\t", offset);
      } else {
        stream->print("/* @%d */\t", offset);
      }

      last_size = write_rom_struct_type(stream, &f JVM_CHECK);
      stream->print(" ");

      name = f.name();
      int occurences = count_field_name_occurences(klass, target_class, &name);
      if (occurences > 1) {
        // This field name has been shadowed. Let's make it unique.
        stream->print("__dup%d__", occurences - 1);
      } else {
        if (is_reserved_word(&name)) {
          stream->print("__reserved__", occurences - 1);
          tty->print("Warning: reserved keyword \"");
          name().print_as_c_source_on(tty);
          tty->print_cr("\" used as field name.");
          tty->print("  => Changed to \"__reserved__");
          name().print_as_c_source_on(tty);
          tty->print_cr("\".");
        }
      }
      name().print_as_c_source_on(stream);
      stream->print_cr(";");
    }

    num_written ++;
    current_base = min;
  }

  // Explicitly add padding whenever we find a 'hole' in the object layout.
  // This is necessary if you have something like
  //     class Super {
  //         byte x;
  //     }
  //     class Child extends Super {
  //         byte y;
  //     }
  // ClassFileParses always start y at a 4-byte aligned address, so we need
  // to add 3 pad bytes.
  static int pad_serial = 0;
  int pad = 4 - ((current_base + last_size) % 4);
  if (pad != 4) {
    while (pad > 0) {
      current_base ++;
      pad --;
      pad_serial ++;
      stream->print("    /* @%d */\t", current_base);
      stream->print_cr("jbyte ___pad%d;", pad_serial);
    }
  }
}

/*
 * Write the fields defined in klass and its super classes.
 * klass        = the class we're currently looking at
 * target_class = the class we're writing the rom_struct for.
 */
void ROMStructsWriter::write_rom_struct_static_fields(Stream *stream,
                                               InstanceClass *klass
                                               JVM_TRAPS) {
  UsingFastOops level1;
  //InstanceClass::Fast super = klass->super();
  Symbol::Fast name;

  TypeArray::Fast fields = klass->original_fields();
  int num_static_fields = 0;
  int num_written = 0;
  int current_base = 0;
  int last_size = 0;
  
  for (int index = 0; index < fields().length(); index += 5) {
    OriginalField f(klass, index);
    if (f.is_static()) {
      num_static_fields ++;
    }
  }

  int pad_serial = 0;
  while (num_written < num_static_fields) {
    int found = -1;
    int min = 0x7fffffff;
    for (int index = 0; index < fields().length(); index += 5) {
      int offset = fields().ushort_at(index + Field::OFFSET_OFFSET);
      int flags  = fields().ushort_at(index + Field::ACCESS_FLAGS_OFFSET);
      if ((flags & JVM_ACC_STATIC)) {
        if (offset > current_base && offset < min) {
          found = index;
          min = offset;
        }
      }
    }

    OriginalField f(klass, found);
 
    GUARANTEE(found >= 0, "sanity");
    if (num_written == 0) {
      GUARANTEE((f.offset() % 4) == 0, "first field must be word-aligned");
      for (int j = 1; j < f.offset() / 4; j++) {
        stream->print_cr("    /* @%d */\t void * __do_not_use__%d;", j*4, j);
      }
    }

    GUARANTEE(f.is_static(), "sanity");    
    stream->print("    ");    
    stream->print("/* @%d */\t", f.offset());

    last_size = write_rom_struct_type(stream, &f JVM_CHECK);
    stream->print(" ");

    name = f.name();
    if (is_reserved_word(&name)) {
      stream->print("__reserved__");
      tty->print("Warning: reserved keyword \"");
      name().print_as_c_source_on(tty);
      tty->print_cr("\" used as field name.");
      tty->print("  => Changed to \"__reserved__");
      name().print_as_c_source_on(tty);
      tty->print_cr("\".");
    }
    name().print_as_c_source_on(stream);
    stream->print_cr(";");

    num_written ++;
    current_base = min;

    
    int pad = 4 - ((current_base + last_size) % 4);
    if (pad != 4) {
      while (pad > 0) {
        current_base ++;
        pad --;
        pad_serial ++;
        stream->print("    /* @%d */\t", current_base);
        stream->print_cr("jbyte ___pad%d;", pad_serial);
      }
    }
  }
}


bool ROMStructsWriter::is_reserved_word(Symbol *field_name) {
  for (const char **p = reserved_words; (*p != NULL); p++) {
    const char *kw = *p;
    int len = jvm_strlen(kw);
    if (field_name->length() != len) {
      continue;
    }
    if (jvm_strncmp(kw, field_name->base_address(), len) == 0) {
      return true;
    }
  }
  return (ROMWriter::_singleton)->_optimizer.reserved_words()->contains(field_name);
}

int ROMStructsWriter::write_rom_struct_type(Stream *stream, Field *field
                                            JVM_TRAPS)
{
  const char * type = NULL;
  int size = 4;

  switch (field->type()) {
  case T_BOOLEAN: type = "jboolean"; size = 1; break;
  case T_CHAR:    type = "jchar";    size = 2; break;
  case T_FLOAT :  type = "jfloat";   size = 4; break;
  case T_DOUBLE:  type = "jdouble";  size = 8; break;
  case T_BYTE:    type = "jbyte";    size = 1; break;
  case T_SHORT:   type = "jshort";   size = 2; break;
  case T_INT:     type = "jint";     size = 4; break;
  case T_LONG:    type = "jlong";    size = 8; break;
  }

  if (field->type() == T_ARRAY) {
    size = 4;
    FieldType::Raw ft = field->signature();
    ArrayClass::Raw ac = ft().object_type();

    if (ac().is_type_array_class()) {
      TypeArrayClass::Raw tac = ac.obj();
      BasicType element_type = (BasicType)tac().type();
      switch (element_type) {
      case T_BOOLEAN: type = "jboolean_array * JVM_FIELD_CONST"; break;
      case T_CHAR:    type = "jchar_array * JVM_FIELD_CONST";    break;
      case T_FLOAT :  type = "jfloat_array * JVM_FIELD_CONST";   break;
      case T_DOUBLE:  type = "jdouble_array * JVM_FIELD_CONST";  break;
      case T_BYTE:    type = "jbyte_array * JVM_FIELD_CONST";    break;
      case T_SHORT:   type = "jshort_array * JVM_FIELD_CONST";   break;
      case T_INT:     type = "jint_array * JVM_FIELD_CONST";     break;
      case T_LONG:    type = "jlong_array * JVM_FIELD_CONST";    break;
      default: SHOULD_NOT_REACH_HERE();
      }
    } else {
      type = "jobject_array * JVM_FIELD_CONST"; 
    }
  }
  

  if (type != NULL) {
    stream->print("%s", type);
  } else {
    // This is an object type
    FieldType::Raw field_type = field->signature();
    InstanceClass::Raw obj_type = field_type().object_type();
    TypeArray::Raw byte_array = Natives::get_jni_class_name(&obj_type JVM_CHECK_0);
    stream->print("struct %s * JVM_FIELD_CONST", byte_array().base_address()); //struct
  }

  return size;
}

/*
 * Count the number of times that the given field name has occurred
 * from <target_class> through <klass>. <target_class> must be a
 * subclass of <klass>
 *
 * E.g., if we have the following class hierarchy:
 * class A {int f;}
 * class B extends A {}
 * class C extends B {int f}
 * class D extends C {int f}
 *
 * count_field_name_occurences(A, D, "f") = 3
 *    -- it's occurred in D, C, A
 */
int ROMStructsWriter::count_field_name_occurences(InstanceClass *klass,
                                                  InstanceClass *target_class,
                                                  Symbol *field_name) {
  InstanceClass::Raw ic = target_class;
  int occurences = 0;

  for (;;) {
    TypeArray::Raw fields = ic().original_fields();

    for (int index = 0; index < fields().length(); index += 5) {
      OriginalField f(&ic, index);
      if (!f.is_static()) {
        Symbol::Raw name = f.name();
        if (field_name->equals(&name)) {
          occurences ++;
        }
      }
    }

    if (ic.equals(klass)) {
      return occurences;
    }
    ic = ic().super();
  }
}

#endif // USE_SOURCE_IMAGE_GENERATOR
