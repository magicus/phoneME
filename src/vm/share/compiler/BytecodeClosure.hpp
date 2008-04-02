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

class BytecodeClosure : public StackObj {
 public:
  BytecodeClosure() {
    // The existence of this constructor helps GCC 2.9 generate
    // smaller code
  }
  ~BytecodeClosure() {
    // The existence of this destructor helps GCC 2.9 generate
    // smaller code
  }

  // called before/after each bytecode
  virtual void bytecode_prolog(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void bytecode_epilog(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}

  // Loads & stores
  virtual void push_int   (jint    /*value*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void push_long  (jlong   /*value*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void push_float (jfloat  /*value*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void push_double(jdouble /*value*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void push_obj   (Oop*    /*value*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}

  virtual void load_local(BasicType /*kind*/, int /*index*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
  }
  virtual void store_local(BasicType /*kind*/, int /*index*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
  }

  virtual void increment_local_int(int /*index*/, jint /*offset*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
  }

  // Array operations
  virtual void array_length(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}

  virtual void load_array(BasicType /*kind*/ JVM_TRAPS)  {JVM_IGNORE_TRAPS;}
  virtual void store_array(BasicType /*kind*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}

  // Stack operations
  virtual void nop(JVM_SINGLE_ARG_TRAPS)     {JVM_IGNORE_TRAPS;}
  virtual void pop(JVM_SINGLE_ARG_TRAPS)     {JVM_IGNORE_TRAPS;}
  virtual void pop2(JVM_SINGLE_ARG_TRAPS)    {JVM_IGNORE_TRAPS;}
  virtual void dup(JVM_SINGLE_ARG_TRAPS)     {JVM_IGNORE_TRAPS;}
  virtual void dup2(JVM_SINGLE_ARG_TRAPS)    {JVM_IGNORE_TRAPS;}

  virtual void dup_x1(JVM_SINGLE_ARG_TRAPS)  {JVM_IGNORE_TRAPS;}
  virtual void dup2_x1(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void dup_x2(JVM_SINGLE_ARG_TRAPS)  {JVM_IGNORE_TRAPS;}
  virtual void dup2_x2(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void swap(JVM_SINGLE_ARG_TRAPS)    {JVM_IGNORE_TRAPS;}

  // Unary arithmetic operations
  virtual void neg(BasicType /*kind*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}

  // Binary operations. These must be in fixed order as checked by
  // BytecodeClosure::verify() because other parts of the system depend
  // on the order.
  enum binary_op {
      bin_add,
      bin_sub,
      bin_mul,
      bin_div,
      bin_rem,
      bin_shl,
      bin_shr,
      bin_ushr,
      bin_and,
      bin_or,
      bin_xor,
      bin_min, // used for inlining Math.min
      bin_max, // used for inlining Math.max
      bin_rsb,
      number_of_binary_ops
  };

  enum unary_op {
      una_neg,
      una_abs  // Used for inlining Math.abs
  };

  virtual void binary(BasicType /*kind*/, binary_op /*op*/ JVM_TRAPS) 
                                                        {JVM_IGNORE_TRAPS;}

  // Conversion operations
  virtual void convert(BasicType /*from*/, BasicType /*to*/ JVM_TRAPS)
                                                        {JVM_IGNORE_TRAPS;}

  // Control transfer operations
  enum cond_op {
      null, nonnull, 
      eq, ne, lt, ge, gt, le, /* The order of these 6 must match the order
                               * of the corresponding bytecode */
#if ENABLE_CONDITIONAL_BRANCH_OPTIMIZATIONS
      negative, positive,
      number_of_ext_cond_ops,
      number_of_cond_ops = negative
#else
      number_of_ext_cond_ops,
      number_of_cond_ops = number_of_ext_cond_ops
#endif
  };

  static const cond_op reverse_condition_table[number_of_cond_ops];
  static const cond_op negate_condition_table[number_of_ext_cond_ops];

  static cond_op reverse(cond_op op) {
    GUARANTEE(unsigned(op) < number_of_cond_ops, "Legal condition");
    return reverse_condition_table[op];
  }
  static cond_op negate(cond_op op)  {
    GUARANTEE(unsigned(op) < number_of_ext_cond_ops, "Legal condition");
    return negate_condition_table[op];
  }

  virtual void branch_if     (cond_op /*cond*/, int /*dest*/ JVM_TRAPS) 
                                                         {JVM_IGNORE_TRAPS;}
  virtual void branch_if_icmp(cond_op /*cond*/, int /*dest*/ JVM_TRAPS)
                                                         {JVM_IGNORE_TRAPS;}
  virtual void branch_if_acmp(cond_op /*cond*/, int /*dest*/ JVM_TRAPS) 
                                                         {JVM_IGNORE_TRAPS;}

  virtual void compare(BasicType /*kind*/, cond_op /*cond*/ JVM_TRAPS)
                                                         {JVM_IGNORE_TRAPS;}

  virtual void branch(int /*dest*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void return_op(BasicType /*kind*/ JVM_TRAPS)   {JVM_IGNORE_TRAPS;}

  virtual void table_switch(jint /*table_index*/, jint /*default_dest*/,
                            jint /*low*/, jint /*high*/ JVM_TRAPS)
                                                         {JVM_IGNORE_TRAPS;}
  virtual void lookup_switch(jint /*table_index*/, jint /*default_dest*/,
                             jint /*num_of_pairs*/ JVM_TRAPS)
                                                         {JVM_IGNORE_TRAPS;}

  // get/set for dynamic class loading
  virtual void get_field(int /*index*/ JVM_TRAPS)        {JVM_IGNORE_TRAPS;}
  virtual void put_field(int /*index*/ JVM_TRAPS)        {JVM_IGNORE_TRAPS;}
    
  // For dynamic initialization and loading
  virtual void get_static(int /*index*/ JVM_TRAPS)       {JVM_IGNORE_TRAPS;}
  virtual void put_static(int /*index*/ JVM_TRAPS)       {JVM_IGNORE_TRAPS;}

  // Method invocation
  virtual void invoke_interface(int /*index*/, int /*num_of_args*/ JVM_TRAPS) 
                                                         {JVM_IGNORE_TRAPS;}

  // Method invocations for dynamic loading of classes
  virtual void invoke_special(int /*index*/ JVM_TRAPS)   {JVM_IGNORE_TRAPS;}
  virtual void invoke_static(int /*index*/ JVM_TRAPS)    {JVM_IGNORE_TRAPS;}
  virtual void invoke_virtual(int /*index*/ JVM_TRAPS)   {JVM_IGNORE_TRAPS;}
  virtual void invoke_native(BasicType /*return_kind*/, address /*entry*/
                             JVM_TRAPS)                  {JVM_IGNORE_TRAPS;}

  // Exception handling
  virtual void throw_exception(JVM_SINGLE_ARG_TRAPS)     {JVM_IGNORE_TRAPS;}

  // Object and array allocation
  virtual void new_object(int /*index*/ JVM_TRAPS)       {JVM_IGNORE_TRAPS;}
  virtual void new_basic_array(int /*type*/ JVM_TRAPS)   {JVM_IGNORE_TRAPS;}
  virtual void new_object_array(int /*index*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void new_multi_array(int /*index*/, int /*num_of_dims*/ JVM_TRAPS) 
                                                         {JVM_IGNORE_TRAPS;}

  virtual void check_cast(int /*index*/ JVM_TRAPS)       {JVM_IGNORE_TRAPS;}
  virtual void instance_of(int /*index*/ JVM_TRAPS)      {JVM_IGNORE_TRAPS;}

  // Monitors
  virtual void monitor_enter(JVM_SINGLE_ARG_TRAPS)       {JVM_IGNORE_TRAPS;}
  virtual void monitor_exit(JVM_SINGLE_ARG_TRAPS)        {JVM_IGNORE_TRAPS;}

  // Accessor to locate method and bci
  Method*       method  (void) const { return &((Method&)_method);    }
  ConstantPool* cp      (void) const { return &((ConstantPool&)_cp);  }
  int           bci     (void) const { return _bci;                   }

  // Extra methods for handling fast bytecodes that are hard to revert.
  virtual void fast_invoke_virtual(int /*index*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void fast_invoke_virtual_final(int /*index*/ JVM_TRAPS) 
                              {JVM_IGNORE_TRAPS;}
  virtual void fast_invoke_special(int /*index*/ JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void fast_get_field(BasicType /*field_type*/, int /*field_offset*/
                              JVM_TRAPS) {JVM_IGNORE_TRAPS;}
  virtual void fast_put_field(BasicType /*field_type*/, int /*field_offset*/
                              JVM_TRAPS) {JVM_IGNORE_TRAPS;}

  virtual void aload_0_fast_get_field_1(BasicType /*field_type*/ JVM_TRAPS);  
  
  virtual void pop_and_npe_if_null(JVM_SINGLE_ARG_TRAPS);
#if !ENABLE_CPU_VARIANT
  virtual void aload_0_fast_get_field_n(int /*bytecode*/ JVM_TRAPS);
  virtual void init_static_array(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}
#endif 
  virtual void uncommon_trap(JVM_SINGLE_ARG_TRAPS) {JVM_IGNORE_TRAPS;}

  virtual void illegal_code(JVM_SINGLE_ARG_TRAPS); 

  // Invoked if handling of a bytecode raised an exception.
  // Invoked after a bytecode has been handled, 
  // but before bytecode_epilog.
  virtual void handle_exception(JVM_SINGLE_ARG_TRAPS) {
    JVM_IGNORE_TRAPS;
    GUARANTEE(CURRENT_HAS_PENDING_EXCEPTION, "No pending exception");
  }

  // Setter for method and bytecode index.
  void set_bytecode(const int bci) {
    _bci = bci;
  }

  static Bytecodes::Code bytecode_at( const Method* method, const jint bci ) {
    return method->bytecode_at( bci );
  }
  Bytecodes::Code bytecode_at( const jint bci ) const {
    return bytecode_at( method(), bci );
  }
  Bytecodes::Code current_bytecode( void ) const {
    return bytecode_at( bci() );
  }

  static int bytecode_length_for( const Method* method, const jint bci ) {
    return method->bytecode_length_for( bci );
  }
  int bytecode_length_for( const jint bci ) const {
    return bytecode_length_for( method(), bci );
  }
  int current_bytecode_length( void ) const {
    return bytecode_length_for( bci() );
  }

  static int next_bci( const Method* method, const jint bci ) {
    return method->next_bci( bci );
  }
  int next_bci( const jint bci ) const {
    return next_bci( method(), bci );
  }

  static int method_size( const Method* method ) {
    return method->code_size();
  }
  int method_size( void ) const {
    return method_size( method() );
  }

  int branch_destination( const int bci ) const {
    return method()->branch_destination( bci );
  }

  void initialize(Method* method);

  static void verify() {
#ifdef AZZERT
    // This particular order is required by CodeGenerator_arm.cpp, among other
    // things, to reduce C++ code footprint.
    GUARANTEE(bin_add == 0, "sanity");
    GUARANTEE(bin_sub == 1, "sanity");
    GUARANTEE(bin_mul == 2, "sanity");
    GUARANTEE(bin_div == 3, "sanity");
    GUARANTEE(bin_rem == 4, "sanity");
    GUARANTEE(bin_shl == 5, "sanity");
    GUARANTEE(bin_shr == 6, "sanity");
    GUARANTEE(bin_ushr== 7, "sanity");
    GUARANTEE(bin_and == 8, "sanity");
    GUARANTEE(bin_or  == 9, "sanity");
    GUARANTEE(bin_xor ==10, "sanity");
    GUARANTEE(bin_min ==11, "sanity");
    GUARANTEE(bin_max ==12, "sanity");
    GUARANTEE(bin_rsb ==13, "sanity");

    // This order is needed by Method::iterate_bytecodes() to simplify the 
    // dispatch of bytecode handlers.
    GUARANTEE(null    == 0, "sanity");
    GUARANTEE(nonnull == 1, "sanity");
    GUARANTEE(eq      == 2, "sanity");
    GUARANTEE(ne      == 3, "sanity");
    GUARANTEE(lt      == 4, "sanity");
    GUARANTEE(ge      == 5, "sanity");
    GUARANTEE(gt      == 6, "sanity");
    GUARANTEE(le      == 7, "sanity");
#endif
  }

 private:
  FastOopInStackObj   _must_precede_fast_oop;
  Method::Fast        _method;
  ConstantPool::Fast  _cp;
  int                 _bci;
};
