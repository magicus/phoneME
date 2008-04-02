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

#if ENABLE_INTERPRETER_GENERATOR

enum j_operation {
    j_add, j_sub, j_mul, j_div, j_rem, j_and, j_or,
    j_xor, j_shl, j_shr, j_ushr
};

enum j_condition {
    j_equal, j_not_equal, j_less, j_less_equal, j_greater, j_greater_equal
};

enum template_options {
  no_options            = 0,
  does_dispatch         = 1,
  does_fp               = 2,
  align_code_base       = 4
};

class Template: public SourceMacros {
 public:
  void* operator new(size_t size) { return GlobalObj::operator new( size ); }

  Template(SourceMacros* assembler, Bytecodes::Code bc, int options) :
    SourceMacros(*assembler), _bc(bc), _options(options)
  { }

  // Accessors.
  bool dispatches() const {
    return _options & does_dispatch;
  }
  bool must_align() const {
    return EnableAlignmentInInterpreter && (_options & align_code_base);
  }
  bool does_floating_point() const {
    return _options & does_fp;
  }
  Bytecodes::Code bytecode() const {
    return _bc;
  }

  // Routines for checking indices, pointers, and types.
  void null_check (Register object, bool tos_cached = true);
  void index_check(Register array, Register index, Register tmp);
  void type_check (Register array, Register object, Register index);

  // Branch helper function.
  void branch(bool is_jsr, bool is_wide);

  // Load/store helper functions.
  void load_from_constantpool(bool is_wide, BasicType type,
                              int prefetch_step = 0);
  void fast_access_field(bool is_static, bool is_put, BasicType type, 
                         int param_bytes, bool extended=false, int offset=-1);

  // Helper function for converting j_conditions into negated assembler
  // conditions. 
  Condition j_not(j_condition cc);

  // Generate code from this template.
  virtual void generate() JVM_PURE_VIRTUAL;

 private:
  Bytecodes::Code  _bc;
  int              _options;
};

class TemplateTable: public AllStatic {
 public:
  // Template accessor functions.
  static Template* template_for     (Bytecodes::Code code)  {
    Bytecodes::check     (code); return _template_table[code];
  }
  static Template* template_for_wide(Bytecodes::Code code)  {
    Bytecodes::wide_check(code); return _template_table_wide[code];
  }
  
  static bool is_duplicate(Bytecodes::Code code) {
      return _duplicates[code] != code;
  }
    
  static Bytecodes::Code get_duplicate(Bytecodes::Code code) {
    GUARANTEE(is_duplicate(code), "Must be duplicate");
    return _duplicates[code];
  }

  static void initialize(SourceMacros* assembler);

  static void initialize_00(SourceMacros* assembler);
  static void initialize_20(SourceMacros* assembler);
  static void initialize_40(SourceMacros* assembler);
  static void initialize_60(SourceMacros* assembler);
  static void initialize_80(SourceMacros* assembler);
  static void initialize_A0(SourceMacros* assembler);
  static void initialize_C0(SourceMacros* assembler);
  static void initialize_fast(SourceMacros* assembler);

  static void initialize_duplicates();

 private:
  static Template*  _template_table     [Bytecodes::number_of_java_codes];
  static Template*  _template_table_wide[Bytecodes::number_of_java_codes];
  static Bytecodes::Code _duplicates    [Bytecodes::number_of_java_codes];

  // Methods for defining templates for bytecodes.
  static void def      (Bytecodes::Code code, Template* t);
  static void def_wide (Bytecodes::Code code, Template* t);
};

#define def_template_0(name)                                               \
  class name: public Template {                                            \
   public:                                                                 \
    name(SourceMacros* assembler, Bytecodes::Code bc, int options) :       \
      Template(assembler, bc, options) { }                                 \
    virtual void generate();                                               \
  };

#define def_template_1(name, argtype)                                      \
 class name: public Template {                                             \
   public:                                                                 \
    name(SourceMacros* assembler, Bytecodes::Code bc, int options,         \
         argtype arg):                                                     \
      Template(assembler, bc, options), _arg(arg) { }                      \
    virtual void generate() { generate(_arg); }                            \
    void generate(argtype arg1);                                           \
   private:                                                                \
    argtype _arg;                                                          \
  };

#define def_template_2(name, arg1type, arg2type)                           \
 class name: public Template {                                             \
   public:                                                                 \
    name(SourceMacros* assembler, Bytecodes::Code bc, int options,         \
         arg1type arg1, arg2type arg2):                                    \
      Template(assembler, bc, options), _arg1(arg1), _arg2(arg2) { }       \
    virtual void generate() { generate(_arg1, _arg2); }                    \
    void generate(arg1type arg1, arg2type arg2);                           \
   private:                                                                \
    arg1type _arg1;                                                        \
    arg2type _arg2;                                                        \
  };
  

// Template      Name              Argument type
def_template_0 ( bc_nop                          )
def_template_0 ( bc_aconst_null                  )
def_template_1 ( bc_iconst       , jint          )
def_template_1 ( bc_lconst       , jlong         )
def_template_1 ( bc_fconst       , jfloat        )
def_template_1 ( bc_dconst       , jdouble       )
def_template_0 ( bc_bipush                       )
def_template_0 ( bc_sipush                       )
def_template_1 ( bc_goto         , bool          )
def_template_1 ( bc_call_vm_redo , const char*   )
def_template_1 ( bc_call_vm_dispatch, const char*)
def_template_2 ( bc_load         , BasicType, bool)
def_template_2 ( bc_load_n       , BasicType, int)
def_template_1 ( bc_array_load   , BasicType     )
def_template_2 ( bc_store        , BasicType, bool)
def_template_2 ( bc_store_n      , BasicType, int)
def_template_1 ( bc_array_store  , BasicType     )
def_template_0 ( bc_pop                          )
def_template_0 ( bc_pop2                         )
def_template_0 ( bc_dup                          )
def_template_0 ( bc_dup_x1                       )
def_template_0 ( bc_dup_x2                       )
def_template_0 ( bc_dup2                         )
def_template_0 ( bc_dup2_x1                      )
def_template_0 ( bc_dup2_x2                      )
def_template_0 ( bc_swap                         )
def_template_1 ( bc_iop2         , j_operation   )
def_template_1 ( bc_lop2         , j_operation   )
def_template_1 ( bc_iinc         , bool          )
def_template_1 ( bc_neg          , BasicType     )
def_template_0 ( bc_lcmp                         )
// floating point ops
def_template_1 ( bc_fcmp         , int           )
def_template_1 ( bc_dcmp         , int           )
def_template_0 ( bc_fadd                         )
def_template_0 ( bc_fsub                         )
def_template_0 ( bc_fmul                         )
def_template_0 ( bc_fdiv                         )
def_template_0 ( bc_frem                         )
def_template_0 ( bc_dadd                         )
def_template_0 ( bc_dsub                         )
def_template_0 ( bc_dmul                         )
def_template_0 ( bc_ddiv                         )
def_template_0 ( bc_drem                         )
def_template_0 ( bc_irem                         )
def_template_0 ( bc_idiv                         )
def_template_0 ( bc_lrem                         )
def_template_0 ( bc_ldiv                         )
def_template_0 ( bc_lmul                         )
def_template_1 ( bc_lshr         , bool          )
def_template_0 ( bc_lshl                         )
def_template_0 ( bc_i2l                          ) 
def_template_0 ( bc_i2f                          )  
def_template_0 ( bc_i2d                          )  
def_template_0 ( bc_l2i                          )  
def_template_0 ( bc_l2f                          )  
def_template_0 ( bc_l2d                          )  
def_template_0 ( bc_f2i                          )  
def_template_0 ( bc_f2l                          )  
def_template_0 ( bc_f2d                          )  
def_template_0 ( bc_d2i                          )  
def_template_0 ( bc_d2l                          )  
def_template_0 ( bc_d2f                          )  
def_template_0 ( bc_i2b                          )  
def_template_0 ( bc_i2c                          )  
def_template_0 ( bc_i2s                          ) 
def_template_1 ( bc_if_0cmp      , j_condition   ) 
def_template_1 ( bc_if_icmp      , j_condition   ) 
def_template_1 ( bc_if_acmp      , j_condition   ) 
def_template_1 ( bc_return       , BasicType     )
def_template_0 ( bc_tableswitch                  )
def_template_0 ( bc_lookupswitch                 )
def_template_1 ( bc_if_nullcmp   , j_condition   )
def_template_0 ( bc_new                          )
def_template_0 ( bc_newarray                     )
def_template_0 ( bc_anewarray                    )
def_template_0 ( bc_arraylength                  )
def_template_0 ( bc_athrow                       )
def_template_0 ( bc_checkcast                    )
def_template_0 ( bc_instanceof                   )
def_template_0 ( bc_monitorenter                 )
def_template_0 ( bc_monitorexit                  )
def_template_0 ( bc_unimplemented                )
def_template_0 ( bc_multianewarray               )
def_template_0 ( bc_wide                         )
#if ENABLE_JAVA_DEBUGGER
def_template_0 ( bc_breakpoint                   )
#endif

def_template_2 ( bc_fast_ldc             , BasicType, bool)
def_template_2 ( bc_fast_putstatic       , BasicType, bool)
def_template_2 ( bc_fast_getstatic       , BasicType, bool)
def_template_1 ( bc_fast_putfield        , BasicType     )
def_template_2 ( bc_fast_getfield        , BasicType, int)
def_template_1 ( bc_aload_0_fast_getfield_1,BasicType    )
def_template_0 ( bc_pop_and_npe_if_null                  )
def_template_1 ( bc_fast_invoke          , bool          )
def_template_0 ( bc_fast_invokeinterface                 )
def_template_0 ( bc_fast_invokenative                    )
def_template_0 ( bc_fast_checkcast                       )
def_template_0 ( bc_fast_instanceof                      )
def_template_0 ( bc_fast_new                             )
def_template_0 ( bc_fast_anewarray                       )
def_template_0 ( bc_fast_invokespecial                   )

def_template_1 ( bc_load_w       , BasicType     )
def_template_1 ( bc_store_w      , BasicType     )
#if !ENABLE_CPU_VARIANT
def_template_2 ( bc_aload_0_fast_getfield_n,BasicType, int)
def_template_0 ( bc_init_static_array                    )
#elif ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND

#endif //!ENABLE_CPU_VARIANT
#undef def_template_0
#undef def_template_1

#endif // ENABLE_INTERPRETER_GENERATOR
