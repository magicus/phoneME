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

#ifndef PRODUCT
#if ENABLE_COMPILER && ENABLE_VSF_MERGE_TEST

class VSFMergeTest : public AllStatic {
 public:
  static void verify_merge(VirtualStackFrame* src_frame, 
                           VirtualStackFrame* dst_frame JVM_TRAPS);
  static void run(OsFile_Handle config_file);

  static bool is_available_register(int index) {
    GUARANTEE(_initialized, "Sanity");
    GUARANTEE(0 <= index && index < Assembler::number_of_registers, "Sanity");
    return _is_available_register[index];
  }

  static unsigned int available_int_registers_count() {
    GUARANTEE(_initialized, "Sanity");
    return _available_int_registers_count;
  }

  static Assembler::Register available_int_register(unsigned int index) {
    GUARANTEE(_initialized && index < _available_int_registers_count, "Sanity");
    GUARANTEE(is_available_register(_available_int_registers[index]), "Sanity");
    return _available_int_registers[index];
  }

  static void initialize();

  enum {
    MAX_VSF_MERGE_CODE_SIZE = 1024,
    MAX_TEST_LOCATION_COUNT = 30
  };
 private:

  struct MergeTestCase {
    unsigned int location_count;

    BasicType types[MAX_TEST_LOCATION_COUNT];

    RawLocation::Status src_status[MAX_TEST_LOCATION_COUNT];
    RawLocation::Status dst_status[MAX_TEST_LOCATION_COUNT];

    Value::ValueLocation src_where[MAX_TEST_LOCATION_COUNT];
    Value::ValueLocation dst_where[MAX_TEST_LOCATION_COUNT];

    int src_value[MAX_TEST_LOCATION_COUNT];
    int dst_value[MAX_TEST_LOCATION_COUNT];
  };

  static void execute_test_cases(OsFile_Handle config_file JVM_TRAPS);
  static bool read_test_case(OsFile_Handle file, 
                             MergeTestCase * const test_case);
  static const char * next_word(const char * read_buffer);
  static void execute_test_case(const MergeTestCase * const test_case 
                                JVM_TRAPS);
  static void initialize_location(RawLocation * const loc,
                                  const BasicType type,
                                  const RawLocation::Status status,
                                  const Value::ValueLocation where,
                                  const int value);

  static bool is_valid_register(int value) {
    return value >= 0 && value < (int)available_int_registers_count();
  }

  static bool _initialized;
  static bool _is_available_register[Assembler::number_of_registers];
  static Assembler::Register 
      _available_int_registers[Assembler::number_of_registers];
  static unsigned int _available_int_registers_count;
};

class VSFMergeTester : public CodeGenerator {
 public:
  VSFMergeTester(VirtualStackFrame* src_frame, 
                 VirtualStackFrame* dst_frame,
                 CompilerState* state);
  void verify_merge();

  virtual void load_from_location(Value& result, jint index, Condition cond);
  virtual void store_to_location(Value& value, jint index);
  virtual void move(Assembler::Register dst, Assembler::Register src, 
                    Condition cond);
  virtual void move(const Value& dst, const Value& src, const Condition cond);
  void cleanup();

 private:
  enum {
    MAX_STACK_LOCATIONS = 1024,
    ILLEGAL_VALUE = 0x60000000,
    LOCATION_VALUE_BASE = 0x40000000,
    REGISTER_VALUE_BASE = 0x50000000
  };

  static int location_value(int index) {
    return index + LOCATION_VALUE_BASE;
  }

  static int min_location_value() {
    return location_value(0);
  }

  static int max_location_value() {
    return location_value(MAX_STACK_LOCATIONS - 1);
  }

  static int location_index(int value) {
    return value - LOCATION_VALUE_BASE;
  }

  static int original_register_value(int index) {
    return index + REGISTER_VALUE_BASE;
  }

  static int min_original_register_value() {
    return original_register_value(0);
  }

  static int max_original_register_value() {
    return original_register_value(Assembler::last_register);
  }

  // Modify immediate so that it doesn't hit any reserved values.
  static int validate(int value) {
    if (value >= min_location_value() && value <= max_location_value()) {
      return value + MAX_STACK_LOCATIONS;
    }
    if (value >= min_original_register_value() && 
        value <= max_original_register_value()) {
      return value + Assembler::number_of_registers;
    }
    if (value == ILLEGAL_VALUE) {
      return value + 1;
    }
    return value;
  }

  static bool is_available_register(int index) {
    return VSFMergeTest::is_available_register(index);
  }

  VirtualStackFrame* _src_frame;
  VirtualStackFrame* _dst_frame;
  CompilerState* _state;

  static int _location_values[MAX_STACK_LOCATIONS];
  static int _memory_values[MAX_STACK_LOCATIONS];
  static int _register_values[Assembler::number_of_registers];
  static int _register_hi_values[Assembler::number_of_registers];
};

#endif /* ENABLE_COMPILER */
#endif /* !PRODUCT */
