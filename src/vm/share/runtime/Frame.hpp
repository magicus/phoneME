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

/** \class Frame
    Describes the stack layout of a JavaFrame or EntryFrame.
    See also the cpu specific files Frame_<cpu>.cpp.
*/

class JavaFrame;
class EntryFrame;

class Frame {

 private:
  static void fixup_all_frames();
  void fixup_frame();
  void push_frame() {
    _previous = _last_frame;
    _last_frame = this;
  }
  void pop_frame() {
    GUARANTEE(_last_frame == this, "Bad popping of frames");
    _last_frame = _previous;
  }


 public:
  //
  // Frames are accessed during many phases of execution.
  // To ensure we get consistent information a static
  // variable is introduced to check proper usage.
  // For instance, JavaFrame::oops_do may only be
  // invoked when _in_gc_state. This is
  // necessary since derived pointers exist during
  // normal execution.
  //
  static bool _in_gc_state;
  static bool in_gc_state()      { return _in_gc_state;  }
  static void set_gc_state()     { _in_gc_state = true;  }
  static void set_normal_state() { _in_gc_state = false; fixup_all_frames(); }

  // Note: the ((void*)this) makes ADS compiler happy.
  JavaFrame&  as_JavaFrame (void) { return *(JavaFrame *) ((void*)this); }
  EntryFrame& as_EntryFrame(void) { return *(EntryFrame*) ((void*)this); }

  const JavaFrame&  as_JavaFrame (void) const {
    return *(const JavaFrame *) ((void*)this);
  }
  const EntryFrame& as_EntryFrame(void) const {
    return *(const EntryFrame*) ((void*)this);
  }

  // Constructor for top frame
  void init(Thread* thread);
  Frame(Thread* thread) {
    // NB. init() is used here to fix CR 6297942.
    init(thread);
  }

  // Constructing a frame from a guessed fp
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void init(Thread* /*thread*/, address /*guessed_fp*/);
#else 
  void init(Thread* /*thread*/, address /*guessed_fp*/) {}
#endif
  Frame(Thread* thread, address guessed_fp) {
    // NB. init() is used here to fix CR 6297942.
    init(thread, guessed_fp);
  }

  // Make this frame be a copy of another frame
  Frame(const Frame& other) {
    this->operator=(other);
    push_frame();
  }
  ~Frame() { 
    pop_frame();
  }

  inline void set_values(Thread* thread, address stack_base,
                         address* pc_addr, address sp, address fp);

  static Frame* _last_frame;

  // Tells whether this is an entry frame.
  inline bool is_entry_frame( void ) const;

  // Tells whether this is a Java frame.
  bool is_java_frame( void ) const { return !is_entry_frame(); }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  bool is_valid_guessed_frame();
#endif

  // Accessor for the stack and frame pointers
  address sp( void ) const { return _sp; }
  address fp( void ) const { return _fp; }
  address pc( void ) const { return *(address*)(_pc_addr); }
  Thread * thread() const { return _thread; }

  void set_pc(address pc) { *(address*)(_pc_addr) = pc; }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void print_on(Stream*, int /*index*/, const char* /*title*/);
  void print_raw_frame_on(Stream*);
  static bool is_plausible_fp(address /*start*/, address /*top*/,
                              address /*bottom*/, address& /*fp*/,
                              address& /*sp*/, address& /*pc_addr*/);
  static bool is_within_stack_range(address /*p*/, address /*stack_top*/, 
                                    address /*stack_bottom*/);
#endif
#ifndef PRODUCT
  // Quick debug short-hands
  void p() {
    print_on(tty, 0, "single frame");
  }
  void pr() {
    print_raw_frame_on(tty);
  }
#endif
 private:
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void get_min_max(int& min, int& max, int value);
#endif

 public:
  Frame& operator=(const Frame& other) {
    _thread = other._thread;
    _stack_base = other._stack_base;
    _fp = other._fp;
    _sp = other._sp;
    _pc_addr = other._pc_addr;
    return *this;
  }

 private:
  Thread* _thread;
  address _stack_base;

  address _fp;
  address _sp;
  address _pc_addr;

  Frame* _previous;

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  bool   _is_valid_guessed_frame;
#endif

  friend class TemplateTable;
  friend class InterpreterGenerator;
  friend class JavaFrame;
  friend class EntryFrame;
  friend class Thread;
  friend class ExecutionStackDesc;
  friend class ObjectHeap;
};

/** \class StackValue
    Describes an item on the expression stack or local stack.
 */
class StackValue {
private:
  address base() {
    return (address)this;
  }

  // machine-specific number of bytes between a stack tag value and its tag
  inline static int stack_tag_offset() {
    return StackValue__stack_tag_offset();
  }

public:
  static bool  is_int(jint tag)    { return tag == int_tag;    }
  static bool  is_long(jint tag)   { return tag == long_tag;   }
  static bool  is_long2(jint tag)  { return tag == long2_tag;  }
#if ENABLE_FLOAT
  static bool  is_float(jint tag)  { return tag == float_tag;  }
  static bool  is_double(jint tag) { return tag == double_tag; }
  static bool  is_double2(jint tag){ return tag == double2_tag; }
#endif
  static bool  is_obj(jint tag)    { return tag == obj_tag;    }
  static bool  is_ret(jint tag)    { return tag == ret_tag;    }
  static bool  is_big(jint tag) {
    return (tag & (long_tag | long2_tag | double_tag | double2_tag)) != 0;
  }
  static bool  is_uninitialized(jint tag) { return tag == uninitialized_tag; }

  bool      is_big()    { return is_big(tag());      }
  bool      is_obj()    { return is_obj(tag()); }

  jint*     int_addr()  { return (jint*) base(); }
  OopDesc** obj_addr()  { return (OopDesc**) base(); }

  jint      as_int()    { return *((jint*) base()); }
  jlong     as_long()   { 
    jint *addr = int_addr();
    return JavaStackDirection < 0
      ? jlong_from_low_high(addr[0], addr[WordsPerStackElement])
      : jlong_from_low_high(addr[-WordsPerStackElement], addr[0]);
  }

#if ENABLE_FLOAT
  jfloat    as_float()  { return *((jfloat*)base()); } 
  jdouble   as_double() { 
    jlong a = as_long();
    return *(jdouble *)&a;
  }
#endif

  ReturnOop as_obj()    { return ReturnOop(*((OopDesc**) base())); }
  jint      as_ret()    { return *((jint*) base()); }
  Oop*      as_oop()    { return (Oop*) (base()); }

  void set_long(jlong val) {
    jlong_accessor temp;
    jint *addr = int_addr();
    temp.long_value = val;
    if (JavaStackDirection < 0) { 
      addr[WordsPerStackElement]  = temp.words[1];
      addr[0]                     = temp.words[0];
    } else { 
      addr[0]                     = temp.words[1];
      addr[-WordsPerStackElement] = temp.words[0];        
    }
  }

  void set_double(jdouble val) {
    set_long(*(jlong*)&val);
  }

  // Printing
#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void      print_on(Stream*, jint /*tag*/);
#endif
  void      verify() PRODUCT_RETURN;

  jint tag() {
    return *((jint*) (base() + stack_tag_offset()));
  }
  void set_tag(jint value) {
    *((jint*) (base() + stack_tag_offset())) = value; 
  }
};

/** \class JavaFrame
    Describes the layout of a JavaFrame.
    The JavaFrame is created on entry to a method.  See the cpu specific
    Frame_<cpu>.cpp file for info on the layout of the JavaFrame.
 */
class JavaFrame : public Frame {
 public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  inline static int method_offset() {
    return JavaFrame__method_offset();
  }
  inline static int locals_pointer_offset() {
    return JavaFrame__locals_pointer_offset();
  }
  inline static int bcp_store_offset() {
    return JavaFrame__bcp_store_offset();
  }
  inline static int stack_bottom_pointer_offset() {
    return JavaFrame__stack_bottom_pointer_offset();
  }
  inline static int caller_fp_offset() {
    return JavaFrame__caller_fp_offset();
  }
  inline static int return_address_offset() {
    return JavaFrame__return_address_offset();
  }
  inline static int frame_desc_size() {
    return JavaFrame__frame_desc_size();
  }
  inline static int cpool_offset() {
    return JavaFrame__cpool_offset();
  }

  // The following is used only in compiled frames,
  // so we can reuse the slot used only during interpretation
  inline static int saved_pc_offset() {
    return JavaFrame__cpool_offset();
  }

  // The following two slots are used by C interpreter loop only
  inline static int stored_int_value1_offset() {
    return JavaFrame__stored_int_value1_offset();
  }
  inline static int stored_int_value2_offset() {
    return JavaFrame__stored_int_value2_offset();
  }

  inline static int end_of_locals_offset() {
    return JavaFrame__end_of_locals_offset();
  }
  inline static int empty_stack_offset() {
    return JavaFrame__empty_stack_offset();
  }

  inline static int arg_offset_from_sp(int index) {
    return JavaFrame__arg_offset_from_sp(index);
  }

#if ENABLE_EMBEDDED_CALLINFO
  inline static int callinfo_offset_from_return_address() {
    return JavaFrame__callinfo_offset_from_return_address();
  }
#endif // ENABLE_EMBEDDED_CALLINFO

  static int first_stack_lock_offset() {
    return JavaStackDirection < 0
        ? empty_stack_offset() - (sizeof(int) + StackLock::size())
        : empty_stack_offset() + sizeof(int);
  }

  static int pre_first_stack_lock_offset() {
    return JavaStackDirection < 0
        ? empty_stack_offset()
        : empty_stack_offset() - StackLock::size();
  }

 public:
  // Constructor
  JavaFrame(JavaFrame& other) : Frame(other) { }
  
  // Constructor for the top frame
  JavaFrame(Thread* thread)   : Frame(thread) { }

#if ENABLE_COMPILER
  ReturnOop compiled_method( void ) const {
    GUARANTEE( is_compiled_frame(), "Sanity check" );

#if ENABLE_APPENDED_CALLINFO
    CompiledMethodDesc * const cm = find_compiled_method(cooked_pc());
#if ENABLE_EMBEDDED_CALLINFO
    // Check that appended callinfo is consistent with embedded callinfo.
    GUARANTEE(cm == cooked_call_info()->compiled_method(), "Consistency check");
#endif // ENABLE_EMBEDDED_CALLINFO
    return cm;
#else
    return cooked_call_info()->compiled_method();
#endif // ENABLE_APPENDED_CALLINFO

  }

#if ENABLE_APPENDED_CALLINFO || ENABLE_CODE_PATCHING
  address cooked_pc( void ) const;

  static CompiledMethodDesc* find_compiled_method( const address pc );

  static bool in_compiled_code( const address pc );
#endif // ENABLE_APPENDED_CALLINFO || ENABLE_CODE_PATCHING

  // Fill in the frame descriptor for compiled methods.
  void fill_in_compiled_frame();

  // Deoptimize topmost frame.
  void deoptimize();
  // Deoptimize non-topmost frame, callee is the child frame method
  void deoptimize(const Method * callee);

  void deoptimize_and_continue(bool adjust_bci);

  // Replace with OSR entry, if possible
  void osr_replace_frame(jint bci);

  // Tells whether we are in compiled code
  bool is_compiled_frame( void ) const;

  bool is_heap_compiled_frame(void) const;

#else

  bool is_compiled_frame( void ) const {
    return false;
  }

#endif // ENABLE_COMPILER


  bool find_exception_frame(Thread *thread, JavaOop* exception JVM_TRAPS);

  // Returns calling frame
  void caller_is(Frame& result) const {
    const address caller_fp = *(address*) (fp() + caller_fp_offset());
    const address caller_sp = this->caller_sp();
    address* caller_pc_addr = (address*) (fp() + return_address_offset());
    result.set_values(_thread, _stack_base, caller_pc_addr, caller_sp, caller_fp);
  }

  // Accessor.
  ReturnOop method() const;

  // Get current byte code index.
  enum {
      compiled_frame_flag  = 0x80000000,
      exception_frame_flag = 0x40000000,
      overflow_frame_flag  = 0x20000000,
      actual_bci_mask      = 0x00FFFFFF
  };

  jint bci_with_flags() const;
  jint bci() const { return bci_with_flags() & actual_bci_mask; }

  // Accessors to locals.
  StackValue* local_at(int index, int length = 0) const;

  jint        local_length() const;

  // Accessors to expression stack.
  StackValue* expression_at(int index, int length = 0);
  jint        expression_length();
  
  // Accessors to stack locks.
  StackLock*  stack_lock_at(int index, int length = 0);
  ReturnOop   stack_lock_obj_at(int index) const;
  jint        stack_lock_length();

  // GC support
  void oops_do(void do_oop(OopDesc**));
  void gc_prologue(void do_oop(OopDesc**));
  void gc_epilogue(void);
  void relocate_internal_pointers(int delta, bool do_locks);
  static void relocate_starting_frame_pointers(Thread *, int) {}

#if !defined(PRODUCT)|| ENABLE_TTY_TRACE
  void print_on(Stream*, int /*index*/);
  void print_stack_address(Stream *, address /*addr*/);
#endif
  void verify() PRODUCT_RETURN;

#if 0
  JavaFrame& operator=(JavaFrame& other) {
    Frame::operator=(other);
    return *this;
  }
  JavaFrame& operator=(Frame& other) {
    Frame::operator=(other);
    return *this;
  }
#else
 public:
   JavaFrame& operator=(JavaFrame& other);
   JavaFrame& operator=(Frame& other);
#endif


 private:
  address stack_bottom_pointer();
  address locals_pointer() const;
  address caller_sp() const { return locals_pointer() - arg_offset_from_sp(-1); }
  address calculated_locals_pointer() const;
  OopDesc** stack_lock_obj_addr_at(int index) const;

  ReturnOop generate_stack_map(int& map_length);
  ReturnOop generate_compiled_method_stack_map(int& map_length);
  ReturnOop generate_tagged_method_stack_map(int& map_length);

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void print_raw_frame_on(Stream*);
#endif

 private:
  // Sets the stack bottom pointer
  void set_empty_stack_bottom_pointer();

  // Operations that are not GC safe

  // Returns the call info associated with the pc()
#if ENABLE_EMBEDDED_CALLINFO
  CallInfo* raw_call_info   ( void ) const;
  CallInfo* cooked_call_info( void ) const;
#endif // ENABLE_EMBEDDED_CALLINFO

  // Accessors to the byte code pointer
  address raw_bcp( void ) const {
    return *(address*)(fp() + bcp_store_offset());
  }
  void set_raw_bcp( const address value) {
    *(address*)(fp() + bcp_store_offset()) = value;
  }
  void set_raw_method(MethodDesc* value) {
    *(MethodDesc**) (fp() + method_offset()) = value;
  }
  MethodDesc* raw_method( void ) const {
   return *(MethodDesc**) (fp() + method_offset());
  }
  address cpool( void ) const { 
    return *(address*)(fp() + cpool_offset());
  }
  void set_cpool(address value) { 
    *(address*)(fp() + cpool_offset()) = value;
  }

#if ENABLE_COMPILER
  CompiledMethodDesc* raw_compiled_method( void ) const {
    return *(CompiledMethodDesc**) (fp() + method_offset());
  }

  void set_raw_compiled_method(CompiledMethodDesc* value) { 
    *(CompiledMethodDesc**) (fp() + method_offset()) = value;
  }
#else
  CompiledMethodDesc* raw_compiled_method( void ) const {
    return NULL;
  }
#endif

  void set_raw_pc(address value) {
    set_pc(value); /* *_pc_addr = value;*/
  }

  void get_min_max_offsets(int& min_offset, int& max_offset);
  void print_expression_stack_on(Stream* st, TypeArray* map, int map_length);
  void print_stack_locks_on(Stream *st);
  void print_locals_on(Stream* st, TypeArray* map);

  friend class LargeObject;
  friend class ObjectHeap;
  friend class StackFrameImpl;
  friend class StackTagIterator;
  friend class VMEvent;
  friend class VMEventModifier;
};

/** \class EntryFrame
    Describes the frame created by shared_entry.
    Shared_entry is used to start the execution of pending entries that are
    created in the VM.  See EntryActivation.
 */
class EntryFrame : public Frame {
 public:
  // we make those public to allow C interpreter to access offsets
  inline static int stored_last_sp_offset() {
    return EntryFrame__stored_last_sp_offset();
  }
  inline static int stored_last_fp_offset() {
    return EntryFrame__stored_last_fp_offset();
  }
  inline static int fake_return_address_offset() {
    return EntryFrame__fake_return_address_offset();
  }
  inline static int real_return_address_offset() {
    return EntryFrame__real_return_address_offset();
  }
  inline static int empty_stack_offset() {
    return EntryFrame__empty_stack_offset();
  }
  inline static int stored_int_value1_offset() {
    return EntryFrame__stored_int_value1_offset();
  }
  inline static int stored_int_value2_offset() {
    return EntryFrame__stored_int_value2_offset();
  }
  inline static int stored_obj_value_offset() {
    return EntryFrame__stored_obj_value_offset();
  }
  inline static int pending_exception_offset() {
    return EntryFrame__pending_exception_offset();
  }
  inline static int pending_activation_offset() {
    return EntryFrame__pending_activation_offset();
  }
  inline static int frame_desc_size() {
    return EntryFrame__frame_desc_size();
  }

 public:
  enum {
    FakeReturnAddress = 0xdeadbeef
  };
  EntryFrame(EntryFrame& other) : Frame(other) {}

  // Returns calling frame
  void caller_is( Frame& result ) const {
    const address caller_fp = *(address*)(fp() + stored_last_fp_offset());
    const address caller_sp = *(address*)(fp() + stored_last_sp_offset());
    address* caller_pc_addr =  (address*)(caller_sp + JavaStackDirection * (int)sizeof(jint));
    result.set_values(_thread, _stack_base, caller_pc_addr,
                      caller_sp, caller_fp);
  }

  // Tells whether we are at the end of the stack
  bool is_first_frame() const {
    const address caller_fp = *(address*) (fp() + stored_last_fp_offset());
    return caller_fp == NULL;
  }

  // GC support
  void oops_do(void do_oop(OopDesc**));
  void relocate_internal_pointers(int delta);
  void gc_prologue(void do_oop(OopDesc**)) {
    oops_do(do_oop);
  }
  void gc_epilogue(void) {
  }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void print_on(Stream*, int /*index*/);
  void print_raw_frame_on(Stream*);
  void get_min_max_offsets(int& /*min_offset*/, int& /*max_offset*/);
#endif
    
  // I'm being lazy and doing these as lvalues rather than having
  // separate accessor and setter macros
  int& stored_int_value1( void ) const { 
    return *(int*)(fp() + stored_int_value1_offset());
  }
  int& stored_int_value2( void ) const { 
    return *(int*)(fp() + stored_int_value2_offset());
  }
  ReturnOop& stored_obj_value( void ) const { 
    return *(OopDesc**)(fp() + stored_obj_value_offset());
  }
  ReturnOop& pending_exception( void ) const { 
    return *(OopDesc**)(fp() + pending_exception_offset());
  }
  ReturnOop& pending_activation( void ) const { 
    return *(OopDesc**)(fp() + pending_activation_offset());
  }

  friend class SharedStubs;
  friend class Frame;
};

/** \class RunTimeFrame
    Describes the frame on entry to interpreter support routines.
    Used to access data on the stack when InterpreterRuntime is called.
*/

class RuntimeFrame: public Frame {
 private:
  address   base(jint index) const {
    return sp() + JavaFrame::arg_offset_from_sp(index);
  }

  address   address_for    (const jint index) const { 
    return base(index);                     
  }
  address   lo_address_for (jint index) const {
    return JavaStackDirection < 0 ? base(index) : base(index + 1);
  }
  address   hi_address_for (jint index) const {
    return JavaStackDirection < 0 ? base(index+1) : base(index);
  }

 public:
  // constructor for the runtime frame - always the top one
  RuntimeFrame(Thread* thread) : Frame(thread) { /* no lazy filling */ }

  // read-only access to the expression stack
  jint int_at ( const jint index) const {
    return *((jint*) address_for(index));
  }
  jlong     long_at   (jint index) const {
    return jlong_from_low_high(*((jint*) lo_address_for(index)),
                               *((jint*) hi_address_for(index)));
  }
#if ENABLE_FLOAT
  jfloat    float_at  (const jint index) const {
    return *((jfloat*) address_for(index));
  }
  jdouble   double_at (const jint index) const { 
    jlong value = long_at(index);
    return *(double *)&value;
  }
      
#endif
  ReturnOop obj_at    (const jint index) const {
    return ReturnOop(*((OopDesc**) (address_for(index))));
  }
  Oop*      oop_at    (const jint index) const {
    return (Oop*) address_for(index);
  }  
};

inline bool Frame::is_entry_frame( void ) const {
  // EntryFrames always have FakeReturnAddress in what would normally be
  // the returnAddress if it were a JavaFrame
  GUARANTEE(EntryFrame::fake_return_address_offset() == 
            JavaFrame::return_address_offset(), "Test Sanity");

  const address ret_addr = *(address*) (fp() + JavaFrame::return_address_offset());
  return ret_addr == (address)EntryFrame::FakeReturnAddress;
}

inline void Frame::set_values(Thread* thread, address stack_base,
                              address* pc_addr, address sp, address fp) {
  _thread = thread;
  _stack_base = stack_base;
  _fp = fp;
  _sp = sp;

#if ENABLE_COMPRESSED_VSF
  if (*pc_addr >= (address)compiler_callvm_stubs_start &&
      *pc_addr <  (address)compiler_callvm_stubs_end) {
    _pc_addr = fp + JavaFrame::saved_pc_offset();
  } else
#endif
    _pc_addr = (address)pc_addr;

#ifdef AZZERT
  if (JavaStackDirection < 0) {
    GUARANTEE(fp >= sp, "Invalid frame");
  } else { 
    GUARANTEE(fp <= sp, "Invalid frame");
  }
#endif
}
