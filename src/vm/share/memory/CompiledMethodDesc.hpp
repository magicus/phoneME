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

#if ENABLE_TRAMPOLINE
class BranchItem{
private:
  address _inst_addr;  /*the mov pc... inst address*/
  address _caller_addr; /*caller's compiled method description*/
  address _callee_addr; /*callee's compiled method description*/
  jint _old_inst; /*original instrucition*/
public:
inline  address inst_addr()
  {
    return _inst_addr;
  }
inline  void set_inst_addr(address value)
  {
    _inst_addr = value;
  }
inline  address caller_addr(){
    return _caller_addr;
  }
inline  void set_caller_addr(address value){
    _caller_addr = value;
  }

inline  address callee_addr()
 {
    return _callee_addr;
 }
inline  void set_callee_addr(address value){
    _callee_addr = value;
  }
  jint old_inst(){
    return _old_inst;
  }

inline  void set_old_inst(jint value){
    _old_inst = value;
  }

  friend class BranchTable;
};
#define BRANCH_TABLE_LENGTH_LOG 7
#define BRANCH_TABLE_LENGTH (1<<BRANCH_TABLE_LENGTH_LOG)


class BranchTable{
private:
  static BranchItem* _b_items;
  static int _item_index;/*The index in which the new element can be put*/

public:
    
  static void init();
  

  static void append(address inst_addr, address caller_addr, 
    address callee_addr, jint old_inst);
 static void remove();
 static void revoke(address caller);
 static void remove_only(address cm);

 friend class Compiler;
};
  


#endif

class CompiledMethodDesc: public OopDesc {
public:
  size_t code_size() const {
    return _flags_and_size & SIZE_MASK;
  }

#if ENABLE_JVMPI_PROFILE 
  // Get the actual jitted code size
  size_t jvmpi_get_code_size() {
    return _jvmpi_code_size;
  }

  // Set the actual jitted code size
  void jvmpi_set_code_size(size_t code_size) {
    _jvmpi_code_size = code_size;
  }
#endif

  static size_t header_size() {
    return sizeof(CompiledMethodDesc);
  }
  static size_t entry_offset() {
    return header_size();
  }
 private:
  // Returns the size of a compiled method with the given bytes of code.
  static size_t allocation_size(int code_bytes) {
    GUARANTEE(code_bytes >= 0, "Sanity");
    return align_allocation_size(header_size() + code_bytes);
  }

 public:
#if ENABLE_REMOTE_TRACER
  static jint total_code;
  static jint total_items;
#endif

  // Returns the object size
  size_t object_size(void) const {
    return allocation_size(code_size());
  }

  // Machine-dependent, must be defined in CodeGenerator for CPU
  int  get_cache_index ( void ) const;
  void set_cache_index ( const int i );

  void variable_oops_do(void do_oop(OopDesc**));

  void oops_do( void do_oop(OopDesc**) ) {
    do_oop( (OopDesc**) this ); // _klass
    do_oop( (OopDesc**)&_method);
    variable_oops_do( do_oop );
  }

  void update_relative_offsets(int delta);

  // Accessor for FlatProfiler
  MethodDesc* method( void ) const { return _method; }

  inline address entry( void ) const {
    return ((address)this) + entry_offset();
  }

  // p points inside a compiled method within address-ordered table
  // table_size can be 0
  static unsigned find_index ( const CompiledMethodDesc* const table[],
                             const unsigned table_size, const void* const p ) {
    unsigned low = 0, high = table_size;
    for( unsigned middle; (middle = (low + high) >> 1) != low; ) {
      if( p < table[middle] ) {
        high = middle;
      } else {
        low = middle;
      }
    }
    return low;
  }

  // p points inside a compiled method within address-ordered table
  // table_size must be strictly positive
  static const CompiledMethodDesc* find ( const CompiledMethodDesc* const table[],
                             const unsigned table_size, const void* const p ) {
    const unsigned index = find_index( table, table_size, p );
    GUARANTEE( table_size != 0, "sanity" );
    const CompiledMethodDesc* method = table[index];
/*    GUARANTEE( unsigned(DISTANCE( method, p )) < method->object_size(),
      "Wrong inner pointer" );*/
    return method;
  }

  // GC support.
 private:
  void _set_size(size_t size) { 
    _flags_and_size = (_flags_and_size & ~SIZE_MASK) | size;
  }
  // Initializes the object after allocation
  void initialize(OopDesc* klass, size_t size) {
    OopDesc::initialize(klass);
    _set_size(size);
  }

  // Support for shrinking compiled methods
  void reduce_size(size_t new_size) {
    GUARANTEE(new_size <= code_size(), "sanity check");
    _set_size(new_size);

#if ENABLE_REMOTE_TRACER
    if (RemoteTracePort > 0) {
      total_code += object_size();
      RemoteTracer::set_compiled_code_size(total_code);
      total_items ++;
      RemoteTracer::set_compiled_method_count(total_items);
    }
#endif
  }

  // Support for shrinking/expanding compiled methods
  void set_size(size_t new_size) {
    _set_size(new_size);

#if ENABLE_REMOTE_TRACER
    if (RemoteTracePort > 0) {
      total_code += object_size();
      RemoteTracer::set_compiled_code_size(total_code);
      total_items ++;
      RemoteTracer::set_compiled_method_count(total_items);
    }
#endif
  }

  // High 9 bits are machine-dependent flags. Low 23 bits are size
  size_t      _flags_and_size;
  MethodDesc* _method;

#if ENABLE_JVMPI_PROFILE 
  // The actual code size, not include the compiled method header and
  // the relocation. Just the instruction code size.
  size_t _jvmpi_code_size;
#endif

public:
  // GUARANTEEs that this CompiledMethodDesc is not in any active frame,
  // so the GC doesn't have to relocate any embedded oops.
  void guarantee_unused() PRODUCT_RETURN;

  friend class CompiledMethod;
  friend class Universe;
  friend class ObjectHeap;


  // The bitmasks for accessing the different parts of _flags_and_size
  //
  // cache_index  has_embedded_oop
  //    |          |
  //    v          v
  // [31.......23][22][21][20...................0]
  //                    ^       ^
  //                    |       |
  //  has_branch_relocation     size-in-bytes

  enum {
    NUMBER_OF_SIZE_BITS = 21,  // The lowe 21 bits in flags_and_size is
                               // used for size (in bytes).

    NUMBER_OF_EXTRA_BITS = 2,  // Two bits for HAS_BRANCH_RELOCATION_MASK and
                               // HAS_EMBEDDED_OOP_MASK.

    NUMBER_OF_NON_INDEX_BITS = NUMBER_OF_SIZE_BITS + NUMBER_OF_EXTRA_BITS,
                               // The number of bits that are not used for
                               // the index. We can get the index by
                               //     jiunt(flags_and_size) >> 
                               //     NUMBER_OF_NON_INDEX_BITS

    SIZE_MASK = (1 << NUMBER_OF_SIZE_BITS) - 1,
                               // The bits in flags_and_size that are occupied
                               // by the size (in bytes)

    HAS_BRANCH_RELOCATION_MASK = 1 << (NUMBER_OF_SIZE_BITS + 0),
                               // Does this compiled method have branches
                               // that need to be relocated when the compiled
                               // methods move.

    HAS_EMBEDDED_OOP_MASK      = 1 << (NUMBER_OF_SIZE_BITS + 1),
                               // Does this compiled method have embedded oop
                               // that need to be relocated when objects in 
                               // the heap move.

    INDEX_MASK = 0xffffffff << NUMBER_OF_NON_INDEX_BITS,
                               // The bits in flags_and_size that are occupied
                               // by the cache index

    NUMBER_OF_EXECUTION_SENSOR_BYTES = 6
                               // The number of bytes of instructions
                               // to update execution sensor (thumb compile
                               // only).

  };
};
