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

struct HuffmanCodeTableHeader {
    unsigned short quickBits;   // quick bit size
    unsigned short maxCodeLen;  // Max number of bits in any code
};

// If this bit is set in a huffman entry, it means that this is not
// really an entry, but a pointer into the long codes table.
// The remaining 15 bits is the offset (in bytes) from the table header
// to first "long" entry representing this item.

#define HUFFINFO_LONG_MASK 0x8000 //  high bit set

#ifdef AZZERT
#define ziperr(msg) BREAKPOINT;
#else
#define ziperr(msg)
#endif

//=========================================================================
// JAR file reader defines and macros
//=========================================================================

#define LITXLEN_BASE 257

// Fixed by ZIP spec
#define DICTIONARY_SIZE         32768
// Can be modified, but recommended to be large enough to contain
// one compressed JAR block (usually about 16-18 Kb) or a single .class file
#define INFLATER_INPUT_BUFFER   32768
// To keep it safe we should provide output buffer at least 2*DICTIONARY_SIZE
#define INFLATER_OUTPUT_BUFFER  65536
// Controls the limit of output buffer usage when it is safe to continue
// decoding into the same buffer without dumping previously decoded data.
// Should be >= DICTIONARY_SIZE and < INFLATER_OUTPUT_BUFFER
#define INFLATER_OUTPUT_LIMIT   36864

// Any caller to inflate must ensure that it is safe to read at least
// this many bytes beyond compData + compLen
#define EXTRA_INPUT_BYTES           4
// It should be safe to output so many bytes at once
// because of maximum length code value
#define EXTRA_OUTPUT_BYTES        260

// Result status of decoding of one JAR block
#define INFLATE_ERROR     0
#define INFLATE_MORE      1
#define INFLATE_COMPLETE  2


/** The HuffmanCodeTable structure contains the dynamic huffman codes for
    the Code Length Codes or the Distance Codes. The structure is
    dynamically allocated. We just allocate enough space to contain all
    possible codes.  A normal sized huffman code table with a 9-bit quick bit
*/
typedef struct HuffmanCodeTable {
    struct HuffmanCodeTableHeader h;
    // There are 1 << quickBit entries.  512 is just an example.
    // For each entry:
    //     If the high bit is 0:
    //        Next 11 bits give the character
    //        Low   4 bits give the actual number of bits used
    //     If the high bit is 1:
    //        Next 15 bits give the offset (in bytes) from the header to
    //        the first long entry dealing with this long code.

    unsigned short entries[512];
} HuffmanCodeTable;


#define ARRAY_BASE(array) ((address)(array) + Array::base_offset())

#define ARRAY_LENGTH(array) (((ArrayDesc*)(array))->_length)

#define GET_HUFFMAN_ENTRY(table, quickBits, result)  {                      \
    unsigned int huff = table->entries[NEXTBITS(quickBits)];                \
    if (huff & HUFFINFO_LONG_MASK) {                                        \
        jint delta = (huff & ~HUFFINFO_LONG_MASK);                          \
        unsigned short *table2 = (unsigned short *)((char *)table + delta); \
        huff = table2[NEXTBITS(table->h.maxCodeLen) >> quickBits];          \
    }                                                                       \
    if (huff == 0) {                                                        \
      return false;                                                         \
    }                                                                       \
    DUMPBITS(huff & 0xF);                                                   \
    result = huff >> 4;                                                     \
  }                                                                         \

#define NEXTBYTE()    (unsigned int)(inFilePtr[inOffset++])

#define NEEDBITS(j) \
    while(inDataSize < (j)) {                \
      inData |= (NEXTBYTE()) << inDataSize;  \
      inDataSize += 8;                       \
    }                                        \
    GUARANTEE((inDataSize <= 32), "inflate: needbits: inDataSize > 32"); 

#define NEXTBITS(j)   (((juint)(inData & ((1 << (j)) - 1)))) 

#define DUMPBITS(j) \
    GUARANTEE(((j) <= inDataSize), "inflate: dumpbits: inDataSize too small");\
    inData >>= (j);                                             \
    inDataSize -= (j);                                          \

#define LOAD_IN \
    unsigned char* inFilePtr = ARRAY_BASE(in_buffer());             \
    const bool isIncremental = flags() & INCREMENTAL_INFLATE;       \
    const juint inLength = ARRAY_LENGTH(in_buffer()) -              \
                           (isIncremental ? EXTRA_INPUT_BYTES : 0); \
    juint inOffset       = in_offset();                             \
    juint inDataSize     = in_data_size();                          \
    juint inData         = in_data();

#define STORE_IN \
    set_in_offset(inOffset);      \
    set_in_data_size(inDataSize); \
    set_in_data(inData);

#define LOAD_OUT \
    unsigned char* outFilePtr = ARRAY_BASE(out_buffer());             \
    const juint outLength = ARRAY_LENGTH(out_buffer()) -              \
                            (isIncremental ? EXTRA_OUTPUT_BYTES : 0); \
    juint outOffset  = out_offset();

#define STORE_OUT \
    set_out_offset(outOffset);

class Inflater : public FileDecoder {
public:
  HANDLE_DEFINITION_CHECK(Inflater, FileDecoder);

private:
  static int in_buffer_offset() {
    return FIELD_OFFSET(InflaterDesc, _in_buffer);
  }
  static int out_buffer_offset() {
    return FIELD_OFFSET(InflaterDesc, _out_buffer);
  }
  static int length_buffer_offset() {
    return FIELD_OFFSET(InflaterDesc, _length_buffer);
  }
  static int distance_buffer_offset() {
    return FIELD_OFFSET(InflaterDesc, _distance_buffer);
  }
  static int out_offset_offset() {
    return FIELD_OFFSET(InflaterDesc, _out_offset);
  }
  static int out_dumped_offset() {
    return FIELD_OFFSET(InflaterDesc, _out_dumped);
  }
  static int in_offset_offset() {
    return FIELD_OFFSET(InflaterDesc, _in_offset);
  }
  static int in_data_size_offset() {
    return FIELD_OFFSET(InflaterDesc, _in_data_size);
  }
  static int in_data_offset() {
    return FIELD_OFFSET(InflaterDesc, _in_data);
  }
  static int expected_crc_offset() {
    return FIELD_OFFSET(InflaterDesc, _expected_crc);
  }
  static int block_type_offset() {
    return FIELD_OFFSET(InflaterDesc, _block_type);
  }

#if ENABLE_FAST_CRC32
  const static juint _fast_crc32_table[];
#endif

  static juint crc32(const unsigned char *data, const juint length);

public:
  ReturnOop in_buffer() {
    return obj_field(in_buffer_offset());
  }
  void set_in_buffer(Oop* value) {
    obj_field_put(in_buffer_offset(), value);
  }
  void clear_in_buffer() {
    obj_field_put(in_buffer_offset(), (OopDesc*) NULL);
  }
  ReturnOop out_buffer() {
    return obj_field(out_buffer_offset());
  }
  void set_out_buffer(Oop* value) {
    obj_field_put(out_buffer_offset(), value);
  }
  ReturnOop length_buffer() {
    return obj_field(length_buffer_offset());
  }
  void set_length_buffer(Oop* value) {
    obj_field_put(length_buffer_offset(), value);
  }
  ReturnOop distance_buffer() {
    return obj_field(distance_buffer_offset());
  }
  void set_distance_buffer(Oop* value) {
    obj_field_put(distance_buffer_offset(), value);
  }

  juint out_offset() {
    return uint_field(out_offset_offset());
  }
  void set_out_offset(juint value) {
    uint_field_put(out_offset_offset(), value);
  }
  juint out_dumped() {
    return uint_field(out_dumped_offset());
  }
  void set_out_dumped(juint value) {
    uint_field_put(out_dumped_offset(), value);
  }
  juint in_offset() {
    return uint_field(in_offset_offset());
  }
  void set_in_offset(juint value) {
    uint_field_put(in_offset_offset(), value);
  }
  juint in_data_size() {
    return uint_field(in_data_size_offset());
  }
  void set_in_data_size(juint value) {
    uint_field_put(in_data_size_offset(), value);
  }
  juint in_data() {
    return uint_field(in_data_offset());
  }
  void set_in_data(juint value) {
    uint_field_put(in_data_offset(), value);
  }
  juint expected_crc() {
    return uint_field(expected_crc_offset());
  }
  void set_expected_crc(juint value) {
    uint_field_put(expected_crc_offset(), value);
  }
  juint block_type() {
    return uint_field(block_type_offset());
  }
  void set_block_type(juint value) {
    uint_field_put(block_type_offset(), value);
  }

  ReturnOop read_completely(JVM_SINGLE_ARG_TRAPS);
  int get_bytes(ArrayPointer* destination, int count JVM_TRAPS);

  static ReturnOop allocate(OsFile_Handle handle, int pos, int size,
                            int comp_len, juint expected_crc,
                            int flags JVM_TRAPS);

private:
  void refill_input(int processed JVM_TRAPS);
  int do_inflate(JVM_SINGLE_ARG_TRAPS);
  int inflate_stored(JVM_SINGLE_ARG_TRAPS);
  int inflate_huffman(bool fixedHuffman JVM_TRAPS);
  int decode_dynamic_huffman_tables(JVM_SINGLE_ARG_TRAPS);
  ReturnOop make_code_table( unsigned char *codelen,
                             unsigned numElems,
                             unsigned maxQuickBits JVM_TRAPS);

  static const unsigned char ll_extra_bits[];
  static const unsigned short ll_length_base[];
  static const unsigned char dist_extra_bits[];
  static const unsigned int dist_base[];
  static const unsigned char reverse5[];

  enum { 
    MAX_ZIP_EXTRA_DISTANCE_BITS = 13,
    MAX_ZIP_DISTANCE_CODE = 29,
    MAX_ZIP_EXTRA_LENGTH_BITS = 5,

    MAX_QUICK_CXD   = 6,
    MAX_QUICK_LXL   = 9,
    MAX_BITS        = 15   // Maximum number of code bits in Huffman Code Table
  };

  enum { 
    BTYPE_NO_COMPRESSION  = 0x00,
    BTYPE_FIXED_HUFFMAN   = 0x01,
    BTYPE_DYNA_HUFFMAN    = 0x02,
    BTYPE_INVALID         = 0x03,
    BTYPE_UNKNOWN         = 0x7F
  };

  unsigned int reverse_5bits(unsigned int code) { 
    return reverse5[code];
  }

  unsigned int reverse_9bits(unsigned int code) {
    return ((reverse5[((code) & 0x1F)] << 4) | reverse5[(code) >> 4]);
  }

  unsigned int reverse_15bits(unsigned int code) {
    return (reverse5[code & 0x1F] << 10)
        | (reverse5[((code) >> 5) & 0x1F] << 5)
        | (reverse5[code >> 10]) ;
  }

};
