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

// Oop map constants
enum {
  OopMapSentinel = 0x00,  // End of oop map
  OopMapEscape   = 0x80   // Escape value for ranges larger than 0x7f
};

/**
 * @class BasicOop
 *
 * @brief This class is the base class for all of the runtime handles
 * to <OopDesc>s allocated on the C/C++ stack.
 *
 * This class is a collection of the common accessor operations of the
 * <OopDesc> class hierarchy.
 *
 * NOTE: No virtuals are allowed.
 */
class BasicOop {
  OopDesc* _obj;
public:
  /// <OopDesc> access operations.
  ReturnOop obj() const { return _obj;}
  void set_obj(ReturnOop value) { _obj = value; }
  void set_obj(const BasicOop* value) { _obj = value->obj(); }
  void set_null() { _obj = NULL; }
  AZZERT_ONLY(static bool is_bootstrapping();)

public:
  /// Default constructor.
  BasicOop() : _obj(NULL) {}

  /// Copy constructors.
  BasicOop(ReturnOop ret) : _obj(ret) {
#if ENABLE_OOP_TAG
    if (ret != NULL && !rom_contains(ret) && oop_tag() == 0) {
      set_oop_tag(current_task_id() |
                  ((current_task_seq() << TASK_SEQ_SHIFT) & TASK_SEQ_MASK));
    }
#endif
  }
  BasicOop(BasicOop* obj) : _obj(obj->obj()) {
#if ENABLE_OOP_TAG
    if (obj->obj() != NULL && !rom_contains(obj->obj()) && oop_tag() == 0) {
      set_oop_tag(current_task_id() |
                  ((current_task_seq() << TASK_SEQ_SHIFT) & TASK_SEQ_MASK));
    }
#endif
  }

  /// Assignment operators.
  BasicOop& operator=(BasicOop& value) {
    if (this != &value) {
      set_obj(value.obj());
    }
    return *this;
  }
  BasicOop& operator=(ReturnOop value) { 
    set_obj(value);
    return *this;
  }
  BasicOop& operator=(const BasicOop* value) {
    if (this != value) {
      set_obj(value);
    }
    return *this;
  }

  /// <OopDesc> field access operations.
  ReturnOop obj_field(int offset) const { 
    return *obj()->obj_field_addr(offset);
  }
  void obj_field_put(int offset, const BasicOop* value) { 
    oop_write_barrier(obj()->obj_field_addr(offset), value->obj());
  }
  void obj_field_put(int offset, OopDesc* value) { 
    oop_write_barrier(obj()->obj_field_addr(offset), value);
  }
  void obj_field_clear(int offset) {
    *obj()->obj_field_addr(offset) = 0;
  }

  /// Type-casting operator.
  operator ReturnOop() const { return obj(); }

  /// Type-checking operations.
  bool is_instance() const;
  bool is_type_array() const;
  bool is_obj_array() const;
  bool is_array() const;
  bool is_java_oop() const;

  bool is_bool_array() const;
  bool is_char_array() const;
  bool is_int_array() const;
  bool is_short_array() const;
  bool is_byte_array() const;
  bool is_long_array() const;
  bool is_float_array() const;
  bool is_double_array() const;

  bool is_string() const;
  bool is_obj_near() const;
  bool is_java_near() const;
  bool is_throwable() const;
  bool is_compiled_method() const;
  bool is_symbol() const;
  bool is_method() const;
  bool is_constant_pool() const;
  bool is_obj_array_class() const;
  bool is_type_array_class() const;
  bool is_instance_class() const;
  bool is_java_class() const;
  bool is_class_info() const;
  bool is_stackmap_list() const;

  bool is_execution_stack() const;
  bool is_jvm_thread() const;
  bool is_thread() const;
  bool is_virtual_stack_frame() const;
  bool is_entry_activation() const;
  bool is_mixed_oop() const;
  bool is_task() const;
  bool is_boundary() const;
  bool is_task_mirror() const;
  bool is_file_decoder() const;
  bool is_inflater() const;
  bool is_jar_file_parser() const;

  bool is_null() const  { return obj() == NULL; }
  bool not_null() const { return obj() != NULL; }

  /// Metada access operations.
  ReturnOop blueprint() const { return (OopDesc*) obj()->blueprint(); }
  static int klass_offset() { return FIELD_OFFSET(OopDesc, _klass); }
  void set_klass(BasicOop* value) { obj_field_put(klass_offset(), value); }

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
  void print_value_on(Stream* st);
  void iterate(OopVisitor* visitor);
  static void iterate_one_oopmap_entry(BasicType type, void *param, 
                                       const char *name, size_t offset, 
                                       int flags);

  bool check_valid_for_print(Stream* st);
  bool check_valid_for_print_cr(Stream* st);
#endif

#ifndef PRODUCT
  ReturnOop klass() const;
  void visit(OopVisitor* visitor);

  void p();
  void print_on(Stream* st);
  void print_rom_definition_on(Stream* st); // used by romizer
#else
  ReturnOop klass() const { return obj_field(klass_offset()); }
#endif

#if ENABLE_OOP_TAG
  enum {
    TASK_SEQ_SHIFT = 5,    // 5 bits of task id 0-31
    TASK_ID_MASK = 0x1F,
    TASK_SEQ_MASK = 0x000FFE00,  // 11 bits of seq no. 0-2048
  };

  static int tag_offset() { return FIELD_OFFSET(OopDesc, _oop_tag); }
  int oop_tag() {
    return int_field(tag_offset());
  }

  void set_oop_tag(int val) {
    int_field_put(tag_offset(), val);
  }

  bool rom_contains(OopDesc *);
  int current_task_id();
  int current_task_seq();
#endif


  /// Equality-testing operations.
  bool equals(const BasicOop* other) const { return obj() == other->obj(); }
  bool equals(ReturnOop other) const { return obj() == other; }

  /// Get size of object pointed to by handle.
  size_t object_size() const { return obj()->object_size(); }

  /// Dump the state of the object.
  void print() PRODUCT_RETURN;

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR
  int generate_fieldmap(TypeArray* field_map);
#endif /* PRODUCT || USE_PRODUCT_BINARY_IMAGE_GENERATOR */


public:
  // = Signed field access operations.

  /// Byte field access operations.
  jbyte byte_field(int offset) const { 
    return *obj()->byte_field_addr(offset);
  }
  void byte_field_put(int offset, jbyte value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->byte_field_addr(offset) = value;
  }

  /// Boolean field access operations.
  jboolean bool_field(int offset) const { 
    return *obj()->bool_field_addr(offset);
  }
  void bool_field_put(int offset, jboolean value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->bool_field_addr(offset) = value;
  }

  /// Char field access operations.
  jchar char_field(int offset) const { 
    return *obj()->char_field_addr(offset);
  }
  void char_field_put(int offset, jchar value) {
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->char_field_addr(offset) = value;
  }

  /// Integer field access operations.
  jint int_field(int offset) const { 
    return *obj()->int_field_addr(offset);
  }
  void int_field_put(int offset, jint value) {
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->int_field_addr(offset) = value;
  }

  /// Short field access operations.
  jshort short_field(int offset) const { 
    return *obj()->short_field_addr(offset);
  }
  void short_field_put(int offset, jshort value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->short_field_addr(offset) = value;
  }

  /// Long field access operations.
  jlong long_field(int offset) const {
#if !HOST_LITTLE_ENDIAN
    // those complications are for SPARC (maybe other RISCs)
    // SEGV if long is loaded/stored from/to not-8 byte aligned address
    // otherwise #else branch is just fine
    int* words = (int*)obj()->long_field_addr(offset);
    return jlong_from_msw_lsw(words[WORD_FOR_MSW_IN_LONG], 
                              words[WORD_FOR_LSW_IN_LONG]);
#else
    return *obj()->long_field_addr(offset);
#endif
  }
  void long_field_put(int offset, jlong value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
#if !HOST_LITTLE_ENDIAN
    int* words = (int*)obj()->long_field_addr(offset);
    words[WORD_FOR_MSW_IN_LONG] = msw(value);
    words[WORD_FOR_LSW_IN_LONG] = lsw(value);
#else
    *obj()->long_field_addr(offset) = value;
#endif
  }

  /// Float field access operations.
  jfloat float_field(int offset) const { 
    return *obj()->float_field_addr(offset);
  }
  void float_field_put(int offset, jfloat value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->float_field_addr(offset) = value;
  }

  /// Double field access operations.
  jdouble double_field(int offset) const { 
#if !HOST_LITTLE_ENDIAN    
    int* words = (int*)obj()->double_field_addr(offset);
    return jdouble_from_msw_lsw(words[WORD_FOR_MSW_IN_DOUBLE], 
                                words[WORD_FOR_LSW_IN_DOUBLE]);  
#else
    return *obj()->double_field_addr(offset);
#endif
  }
  void double_field_put(int offset, jdouble value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
#if !HOST_LITTLE_ENDIAN
    jdouble_accessor tmp, *p;
    p = (jdouble_accessor*)obj()->double_field_addr(offset);
    tmp.double_value = value;
    p->words[0] = tmp.words[0];
    p->words[1] = tmp.words[1];
#else
    *obj()->double_field_addr(offset) = value;
#endif
  }

  // = Unsigned field access operations.

  /// Byte field access operations.
  jubyte ubyte_field(int offset) const { 
    return *obj()->ubyte_field_addr(offset);
  }
  void ubyte_field_put(int offset, jbyte value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->ubyte_field_addr(offset) = value;
  }

  /// Short field access operations.
  jushort ushort_field(int offset) const { 
    return *obj()->ushort_field_addr(offset);
  }
  void ushort_field_put(int offset, jushort value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->ushort_field_addr(offset) = value;
  }

  /// Int field access operations.
  juint uint_field(int offset) const { 
    return *obj()->uint_field_addr(offset);
  }
  void  uint_field_put(int offset, juint value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
    *obj()->uint_field_addr(offset) = value;
  }

  /// unsigned long field access operations.
  julong ulong_field(int offset) const { 
#if !HOST_LITTLE_ENDIAN
    jlong_accessor tmp, *p;
    p = (jlong_accessor*)obj()->long_field_addr(offset);
    tmp.words[0] = p->words[0];
    tmp.words[1] = p->words[1];
    return (julong)tmp.long_value;
#else
    return *obj()->ulong_field_addr(offset);
#endif
  }
  void ulong_field_put(int offset, julong value) { 
    GUARANTEE(!oop_check_barrier(obj()->obj_field_addr(offset)), "Write barrier bit set");
#if !HOST_LITTLE_ENDIAN
    jlong_accessor tmp, *p;
    p = (jlong_accessor*)obj()->long_field_addr(offset);
    tmp.long_value = (jlong)value;
    p->words[0] = tmp.words[0];
    p->words[1] = tmp.words[1];
#else
    *obj()->ulong_field_addr(offset) = value;
#endif
  }

  friend class Scheduler;
  friend class ObjectHeap;
  friend class CompiledMethodCache;
  friend class Thread; // Needs field_base()
  friend class BinaryAssemblerCommon;
  friend class FinalizerConsDesc;

protected:
  void* field_base(int offset) const { return (void*)&((char*)obj())[offset]; }

public:

#ifdef AZZERT
  static void enable_on_stack_check();
  static void disable_on_stack_check();
  static bool on_stack_check_disabled() {
    return _disable_on_stack_check;
  }
protected:
  static bool _disable_on_stack_check;
#else 
  static void enable_on_stack_check() {}
  static void disable_on_stack_check() {}
#endif
};

/**
 * @class LinkedBasicOop
 *
 * @brief This class is the base class for all of the linked handles
 * to <OopDesc>s allocated on the C/C++ stack.
 *
 * This class implements a simple gc-safe handle mechanism whereby the
 * last handle on the C/C++ stack is pointed to by the global variable
 * <_last_handle>.  This way, the garbage collector is able to update all
 * such linked handles by simply following the chain with head <_last_handle>.
 *
 * When a <LinkedBasicOop> is constructed on the C/C++ stack, its constructor
 * sets its <_previous> member and links it to <_last_handle>.  However, when
 * the <LinkedBasicOop> is destroyed by leaving its current scope, its destructor
 * does NOT set <_last_handle> to the <Oop> pointed to by its <_previous> member.
 *
 * NOTE: No virtuals are allowed.
 */
class LinkedBasicOop: public BasicOop { 
protected:
  Oop* _previous;

  /// Gets <_previous> linked handle.
  Oop* previous() const {
    GUARANTEE(!is_persistent(), "Must be thread local!");
    return _previous;
  }

  /// Sets <_previous> linked handle.
  void set_previous(Oop* value) { 
    GUARANTEE(!is_persistent(), "Must be thread local!"); 
    _previous = value; 
  }

  /// Handle verification operations.
  AZZERT_ONLY(void pre_push_handle_verification();)
  AZZERT_ONLY(void pre_pop_handle_verification();)
  AZZERT_ONLY(void handle_uniqueness_verification();)

  /// Explicitly link this handle and set <_last_handle>.
  void push_handle() {  
    AZZERT_ONLY(pre_push_handle_verification();)
    set_previous(_last_handle);
    _last_handle = (Oop*)((void*)this);
  }

  /// Explicitly unlink this handle and set <_last_handle>.
  void pop_handle() {
    AZZERT_ONLY(pre_pop_handle_verification();)
     _last_handle = previous();
  }

public:
  /// Default constructor.
  LinkedBasicOop() : BasicOop() { push_handle(); }

  /// Copy constructors.
  LinkedBasicOop(ReturnOop ret) : BasicOop(ret) { push_handle(); }
  LinkedBasicOop(BasicOop *oop) : BasicOop(oop) { push_handle(); }

  /// Assignment operators.
  LinkedBasicOop& operator=(const BasicOop* value) {
    if (this != value) {
      set_obj(value);
    }
    return *this;
  }
  LinkedBasicOop& operator=(ReturnOop value) { 
    set_obj(value);
    return *this;
  }
  LinkedBasicOop& operator=(LinkedBasicOop& value) {
    if (this != &value) {
      set_obj(value.obj());
    }
    return *this;
  }

#ifndef PRODUCT
  bool is_persistent() const;
  static void verify_basic_properties();
#endif /* PRODUCT */

  /// Friend classes
  friend class CompiledMethodCache;
  friend class LargeObject;
  friend class ObjectHeap;
  friend class Scheduler;
  friend class UsingFastOops;
  friend class FileDecoder;
};

#ifdef AZZERT
#define HANDLE_CHECK(this_class, check)                         \
  void this_class::type_check() {                               \
  GUARANTEE(is_null() || Universe:: is_bootstrapping()          \
  || ObjectHeap::is_gc_active() || check, "dynamic type check");\
}
#else
#define HANDLE_CHECK(this_class, check)
#endif /* AZZERT */

#define HANDLE_DEFINITION(this_class, super_class) \
  void type_check() {}                      \
  __HANDLE_DEFINITION__(this_class, super_class)

#define HANDLE_DEFINITION_CHECK(this_class, super_class)     \
  AZZERT_ONLY(void type_check();)                            \
  __HANDLE_DEFINITION__(this_class, super_class)

/* 
 * HANDLE_DEFINITION_WITH_DISPOSING_FAST additionally defines a nested class
 * DisposingFast (a subclass of Fast). 
 * DisposingFast is an Oop that automatically disposes of the referenced java
 * object with a supplied free() routine.
 * DisposingFast is convenient if there is a need to allocate java objects 
 * without Java heap.
 */
#define HANDLE_DEFINITION_WITH_DISPOSING_FAST(this_class, super_class) \
  HANDLE_DEFINITION(this_class, super_class);    \
  class DisposingFast : public Fast {            \
    static void check_use() {                    \
    }                                            \
    void check_and_init() {                      \
      AZZERT_ONLY(((this_class*)this)->type_check();) \
    }                                            \
  public:                                        \
    DisposingFast(JvmFreeProc free_proc) : Fast() {   \
      _free_proc = free_proc;                    \
    }                                            \
    DisposingFast& operator=(ReturnOop ret) {    \
      BasicOop::operator = (ret);                \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    DisposingFast& operator=(DisposingFast& value) {  \
      BasicOop::operator = (value);              \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    DisposingFast& operator=(const BasicOop* oop) {    \
      BasicOop::operator = (oop);                \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    ReturnOop obj() const {                      \
      AZZERT_ONLY(check_use();)                  \
      return BasicOop::obj();                    \
    }                                            \
    this_class& operator() () {                  \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    const this_class& operator() () const {      \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    this_class& operator() (int /*i*/) {         \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    const this_class& operator() (int /*i*/) const { \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    this_class* operator& () {                   \
      AZZERT_ONLY(check_use();)                  \
      return (this_class*)this;                  \
    }                                            \
    const this_class* operator& () const {       \
      AZZERT_ONLY(check_use();)                  \
      return (this_class*)this;                  \
    }                                            \
    void dispose();                              \
    ~DisposingFast() {                           \
      dispose();                                 \
    }                                            \
  private:                                       \
    JvmFreeProc _free_proc;                      \
  }

#define HANDLE_IMPLEMENTATION_WITH_DISPOSING_FAST(this_class) \
  void this_class::DisposingFast::dispose() {        \
    AZZERT_ONLY(handle_uniqueness_verification();)  \
    if (_free_proc != NULL) {                       \
      OopDesc* oop_desc = obj();                    \
      if (oop_desc != NULL) {                       \
        set_null();                                 \
        _free_proc(oop_desc);                       \
      }                                             \
    }                                               \
  }

#define __HANDLE_DEFINITION__(this_class, super_class) \
  /* Default constructor. */                     \
  this_class() : super_class() {}                \
  /* Copy constructors. */                       \
  this_class(ReturnOop ret) : super_class(ret) { \
    AZZERT_ONLY(type_check();)                   \
  }                                              \
  this_class(BasicOop* obj) : super_class(obj) { \
    AZZERT_ONLY(type_check();)                   \
  }                                              \
  /* Assignment operators. */                    \
  this_class& operator=(ReturnOop ret) {         \
    BasicOop::operator=(ret);                    \
    AZZERT_ONLY(type_check();)                   \
    return *this;                                \
  }                                              \
  this_class& operator=(const BasicOop* obj) {   \
    BasicOop::operator=(obj);                    \
    AZZERT_ONLY(type_check();)                   \
    return *this;                                \
  }                                              \
  this_class& operator=(this_class& obj) {       \
    BasicOop::operator=(obj);                    \
    AZZERT_ONLY(type_check();)                   \
    return *this;                                \
  }                                              \
  class Raw : public BasicOop {                  \
  private:                                       \
    AZZERT_ONLY(OopDesc** _current_allocation_top;) \
    AZZERT_ONLY(jint _current_check_count;)      \
    AZZERT_ONLY(Raw* _previous;)                      \
    void push_handle() {                              \
      AZZERT_ONLY(_previous = (Raw*)last_raw_handle;) \
      AZZERT_ONLY(last_raw_handle = (Oop*)this;)      \
    }                                                 \
    void pop_handle() {                               \
      AZZERT_ONLY(last_raw_handle = (Oop*)_previous;) \
    }                                                 \
    void check_and_init() {                      \
      AZZERT_ONLY(((this_class*)this)->type_check();)                \
      AZZERT_ONLY(_current_allocation_top = _inline_allocation_top;) \
      AZZERT_ONLY(_current_check_count = global_check_count;)        \
    }                                            \
    void check_use() const {                     \
      GUARANTEE(is_bootstrapping() ||            \
                 _current_allocation_top == _inline_allocation_top, \
        "Illegal use of " #this_class "::Raw after allocation"); \
      GUARANTEE(is_bootstrapping() || is_null() ||             \
                 AllocationDisabler__disabling_count != 0 ||   \
                 _current_check_count == global_check_count || \
                 _current_check_count+1 == global_check_count, \
        "Illegal use of " #this_class "::Raw after check");    \
    }                                            \
  public:                                        \
    Raw()              : BasicOop()    { push_handle(); check_and_init(); } \
    Raw(ReturnOop ret) : BasicOop(ret) { push_handle(); check_and_init(); } \
    Raw(BasicOop *oop) : BasicOop(oop) { push_handle(); check_and_init(); } \
    Raw& operator=(ReturnOop ret) {              \
      BasicOop::operator=(ret);                  \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    Raw& operator=(Raw& value) {                 \
      BasicOop::operator=(value);                \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    Raw& operator=(const BasicOop* oop) {        \
      BasicOop::operator=(oop);                  \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    ReturnOop obj() const {                      \
      AZZERT_ONLY(check_use();)                  \
      return BasicOop::obj();                    \
    }                                            \
    this_class& operator() ()     {              \
      /* Shorthand for .as_oop() */              \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    const this_class& operator() () const {      \
      /* Shorthand for .as_oop() */              \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    this_class& as_oop() {                       \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    const this_class& as_oop() const {           \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    this_class* operator& () {                   \
      AZZERT_ONLY(check_use();)                  \
      return (this_class*)this;                  \
    }                                            \
    const this_class* operator& () const {       \
      AZZERT_ONLY(check_use();)                  \
      return (this_class*)this;                  \
    }                                            \
    AZZERT_ONLY(~Raw() {pop_handle();})          \
    AZZERT_ONLY(friend class ObjectHeap;)        \
  };                                             \
  class Fast : public LinkedBasicOop {           \
    static void check_use() {                    \
    }                                            \
    void check_and_init() {                      \
      AZZERT_ONLY(((this_class*)this)->type_check();) \
    }                                            \
  public:                                        \
    Fast()              : LinkedBasicOop()    {} \
    Fast(ReturnOop ret) : LinkedBasicOop(ret) {} \
    Fast(BasicOop *oop) : LinkedBasicOop(oop) {} \
    Fast& operator=(ReturnOop ret) {             \
      BasicOop::operator = (ret);                \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    Fast& operator=(Fast& value) {               \
      BasicOop::operator = (value);              \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    Fast& operator=(const BasicOop* oop) {       \
      BasicOop::operator = (oop);                \
      check_and_init();                          \
      return *this;                              \
    }                                            \
    ReturnOop obj() const {                      \
      AZZERT_ONLY(check_use();)                  \
      return BasicOop::obj();                    \
    }                                            \
    this_class& operator() () {                  \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    const this_class& operator() () const {      \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    this_class& operator() (int) {               \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    const this_class& operator() (int) const {   \
      AZZERT_ONLY(check_use();)                  \
      return *(this_class*)this;                 \
    }                                            \
    this_class* operator& () {                   \
      AZZERT_ONLY(check_use();)                  \
      return (this_class*)this;                  \
    }                                            \
    const this_class* operator& () const {       \
      AZZERT_ONLY(check_use();)                  \
      return (this_class*)this;                  \
    }                                            \
  }

/**
 * @class UsingFastOops
 *
 * @brief This class explicitly manipulates the <_last_handle>
 * global variable.
 *
 * When an instance of this class is constructed on the C/C++
 * stack, it sets its <_previous> member to <_last_handle>; when
 * the instance is destroyed by leaving its current scope, it
 * re-sets <_last_handle> to the value in its <_previous> member.
 *
 * Hence, <_last_handle> will contain the value it had prior to
 * any other handles created past and within the current scope
 * of this <UsingFastOops> instance.
 */
class UsingFastOops {
private:
  AZZERT_ONLY(void pre_fast_oops_verification (void) const;)
  AZZERT_ONLY(void post_fast_oops_verification(void) const;)
  Oop* _previous;

public:
  /// Default constructor.
  UsingFastOops() { 
    AZZERT_ONLY(pre_fast_oops_verification();)
     _previous = _last_handle; 
  }
  /// Default destructor.
  ~UsingFastOops() { 
    AZZERT_ONLY(post_fast_oops_verification();)
    _last_handle = _previous; 
  }
};

/**
 * @class FastOopInStackObj
 *
 * @brief This class explicitly manipulates the <_last_handle>
 * global variable.
 *
 */
class FastOopInStackObj : public UsingFastOops {
public:
  FastOopInStackObj() {}
  ~FastOopInStackObj() {}
};

/**
 * @class Oop
 *
 * @brief This class is the base class for all of the linked handles
 * to <OopDesc>s allocated on the C/C++ stack.
 *
 * This class implements a simple gc-safe handle mechanism whereby the
 * last handle on the C/C++ stack is pointed to by the global variable
 * <_last_handle>.  This way, the garbage collector is able to update all
 * such linked handles by simply following the chain with head <_last_handle>.
 *
 * When an <Oop> is constructed on the C/C++ stack, its constructor sets
 * its <_previous> member and links it to <_last_handle>.  Similarly, when
 * the <Oop> is destroyed by leaving its current scope, its destructor sets
 * <_last_handle> to the <Oop> pointed to by its <_previous> member.
 */
class Oop: public LinkedBasicOop {
public:
  /// Constructors & assignment operators.
  HANDLE_DEFINITION(Oop, LinkedBasicOop);

  /// Default destructor.
  ~Oop() { pop_handle(); }

};

#define ForAllHandles( var )\
  for( Oop* var = ::_last_handle; var; var = var->previous() )

/**
 * DEFINE_ACCESSOR_OBJ and DEFINE_ACCESSOR_NUM
 *
 * These two macros are for defining the access functions in subclasses
 * of Oop. For example, if you have the class:
 *
 * class FooDesc: public OopDesc {
 *   MethodDesc * _dick;
 *   int          _rick;
 *   short        _vick;   
 * };
 *
 * The Oop subclass can be declared as:
 *
 * class Foo: public Oop {
 *   DEFINE_ACCESSOR_OBJ(Foo, Method,  dick)
 *   DEFINE_ACCESSOR_NUM(Foo, int,     rick)
 *   DEFINE_ACCESSOR_NUM(Foo, short,   vick)
 * };
 */

#ifdef UNDER_ADS
#define EMPTY_PRIVATE 
#else
#define EMPTY_PRIVATE private:
#endif

#define DEFINE_ACCESSOR_OBJ(oop_class, obj_type, name) \
 private: \
  static int name ## _offset()  { \
    return FIELD_OFFSET(oop_class ## Desc, _ ## name); \
  } \
 public: \
  ReturnOop name() const { \
    return obj_field(name ## _offset()); \
  } \
  void set_ ## name(obj_type* value) \
    { obj_field_put(name ## _offset(), (Oop*) value); \
  } \
 EMPTY_PRIVATE 

#define DEFINE_ACCESSOR_NUM(oop_class, num_type, name) \
 private: \
  static int name ## _offset() { \
    return FIELD_OFFSET(oop_class ## Desc, _ ## name); } \
 public: \
  num_type name() const { \
    return num_type ## _field(name ## _offset()); \
  } \
  void set_ ## name(num_type value) { \
    num_type ## _field_put(name ## _offset(), value); \
  } \
 EMPTY_PRIVATE
