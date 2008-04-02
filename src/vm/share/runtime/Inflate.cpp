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
#include "incls/_Inflate.cpp.incl"

#ifdef INFLATE_DEBUG_FILE
#include <sys/stat.h>
#endif

HANDLE_CHECK(Inflater, is_inflater())

ReturnOop Inflater::allocate(OsFile_Handle handle, int pos, int size,
                             int comp_len, juint expected_crc,
                             int flags JVM_TRAPS)
{
  UsingFastOops fast_oops;
  Inflater::Fast inflater = Universe::new_mixed_oop(
                              MixedOopDesc::Type_Inflater,
                              InflaterDesc::allocation_size(),
                              InflaterDesc::pointer_count()
                              JVM_CHECK_0);

  inflater().set_file_handle(handle);
  inflater().set_file_pos(pos);
  inflater().set_file_size(size);
  inflater().set_bytes_remain(size);
  inflater().set_flags(flags);
  inflater().set_expected_crc(expected_crc);
  inflater().set_block_type(BTYPE_UNKNOWN);

  int in_size = comp_len + EXTRA_INPUT_BYTES;
  if (flags & INCREMENTAL_INFLATE) {
    in_size = min(in_size, INFLATER_INPUT_BUFFER + EXTRA_INPUT_BYTES);
    size = min(size, INFLATER_OUTPUT_BUFFER) + EXTRA_OUTPUT_BYTES;
  }

  if (comp_len >= 0) {
    Buffer::Raw in_buf = Universe::new_byte_array_raw(in_size JVM_CHECK_0);
    inflater().set_in_buffer(&in_buf);
    if (handle != NULL) {
      // Initially fill the buffer
      inflater().get_bytes_raw(in_buf().base_address(), in_size);
    }
  }

  if (size >= 0) {
    Buffer::Raw out_buf = Universe::new_byte_array_raw(size JVM_CHECK_0);
    inflater().set_out_buffer(&out_buf);
  }

  return inflater;
}

int Inflater::get_bytes(ArrayPointer* dest, int count JVM_TRAPS) {
  int result = 0;
  for (;;) {
    juint upper_bound = out_offset();
    juint lower_bound = out_dumped();
    address src_address = ARRAY_BASE(out_buffer()) + lower_bound;
    address dest_address = dest->base_address() + result;
    int avail = upper_bound - lower_bound;

    if (count <= avail) {
      jvm_memcpy(dest_address, src_address, count);
      set_out_dumped(lower_bound + count);
      return result + count;
    }

    jvm_memcpy(dest_address, src_address, avail);
    result += avail;
    count -= avail;

    if (upper_bound > INFLATER_OUTPUT_LIMIT) {
      Buffer::Raw new_out_buffer =
        Universe::new_byte_array_raw(ARRAY_LENGTH(out_buffer()) JVM_CHECK_0);
      address source = ARRAY_BASE(out_buffer()) + upper_bound - DICTIONARY_SIZE;
      jvm_memcpy(new_out_buffer().base_address(), source, DICTIONARY_SIZE);
      set_out_buffer(&new_out_buffer);
      set_out_offset(DICTIONARY_SIZE);
      set_out_dumped(DICTIONARY_SIZE);
    } else {
      set_out_dumped(upper_bound);
    }

    int status = do_inflate(JVM_SINGLE_ARG_CHECK_0);
    if (status == INFLATE_ERROR) {
      return -1;
    } else if (status == INFLATE_COMPLETE) {
      return result;
    }
    GUARANTEE(status == INFLATE_MORE, "Sanity");
  }
}

ReturnOop Inflater::read_completely(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  JarFileParser::Fast jfp = get_jar_parser_if_needed(JVM_SINGLE_ARG_CHECK_0);
  (void)jfp;

  int status;
  do {
    status = do_inflate(JVM_SINGLE_ARG_CHECK_0);
  } while (status == INFLATE_MORE);

  const int size = file_size();
  if (status == INFLATE_ERROR || (int) out_offset() != size) {
    return NULL;
  }

  if (file_handle() != NULL) {
    juint actual_crc = crc32(ARRAY_BASE(out_buffer()), size);
    if (actual_crc != expected_crc()) {
      return NULL;
    }
  }

  return out_buffer();
}

void Inflater::refill_input(int processed JVM_TRAPS) {
  if (file_handle() == NULL) {
    // We are known to read romized resource
    return;
  }

  UsingFastOops fast_oops;
  Buffer::Fast in_buf = in_buffer();
  GUARANTEE(in_buf.not_null(), "Sanity");

  int length = in_buf().length();
  int remainder = length - processed;
  GUARANTEE(remainder <= processed, "Sanity");

  Buffer::Raw new_in_buffer = Universe::new_byte_array_raw(length JVM_CHECK);
  address base = new_in_buffer().base_address();
  jvm_memcpy(base, in_buf().base_address() + processed, remainder);
  get_bytes_raw(base + remainder, processed);

  // Note: we do not set here in_offset = 0,
  // because this field is usually cached in inOffset local variable
  set_in_buffer(&new_in_buffer);
}

int Inflater::do_inflate(JVM_SINGLE_ARG_TRAPS) {
  juint btype = block_type();

  for (;;) { // no more than 2 iterations
    switch (btype) {
    case BTYPE_UNKNOWN: {
      if (flags() & LAST_BLOCK) {
        return INFLATE_COMPLETE;
      }
      
      LOAD_IN;
      if (inOffset >= inLength) { // check input overflow
        refill_input(inOffset JVM_CHECK_0);
        inFilePtr = ARRAY_BASE(in_buffer());
        inOffset = 0;
      }
      NEEDBITS(3);
      btype = NEXTBITS(3);
      DUMPBITS(3);
      if (btype & 1) {
        add_flags(LAST_BLOCK);
      }
      btype >>= 1;
      STORE_IN;
      
      set_block_type(btype);
      if (btype == BTYPE_DYNA_HUFFMAN) {
        int status = decode_dynamic_huffman_tables(JVM_SINGLE_ARG_CHECK_0);
        if (status == INFLATE_ERROR) {
          return INFLATE_ERROR;
        }
      }
      continue;
    }
      
    case BTYPE_INVALID:
      ziperr(KVM_MSG_JAR_INVALID_BTYPE);
      return INFLATE_ERROR;
      
    case BTYPE_NO_COMPRESSION:
      return inflate_stored(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      
    case BTYPE_FIXED_HUFFMAN:
    case BTYPE_DYNA_HUFFMAN:
      return inflate_huffman((btype == BTYPE_FIXED_HUFFMAN)
                             JVM_NO_CHECK_AT_BOTTOM);

    default:
      SHOULD_NOT_REACH_HERE();
    }
  }
}

int Inflater::inflate_stored(JVM_SINGLE_ARG_TRAPS) {
  LOAD_IN;
  LOAD_OUT;

  DUMPBITS(inDataSize & 7);   // move to byte boundary
  NEEDBITS(32);
  int length = NEXTBITS(16);
  DUMPBITS(16);
  int nlength = NEXTBITS(16);
  DUMPBITS(16);
  (void)inLength;

  GUARANTEE(inDataSize == 0, "inDataSize != 0");
  GUARANTEE(outOffset == out_dumped(), "output buffer should be fresh");
  GUARANTEE(length <= (int) outLength, "INFLATER_OUTPUT_BUFFER too small");
  AZZERT_ONLY_VAR(outLength);

  if (length + nlength != 0xFFFF) {
    ziperr(KVM_MSG_JAR_BAD_LENGTH_FIELD);
    return INFLATE_ERROR;
  }

  if (outOffset + length > outLength) {
    // Do not have enough room in existing output buffer
    Buffer::Raw new_out_buffer =
      Universe::new_byte_array_raw(ARRAY_LENGTH(out_buffer()) JVM_CHECK_0);
    inFilePtr = ARRAY_BASE(in_buffer()); // adjust after possible GC
    outFilePtr = new_out_buffer().base_address();

    int preserve_bytes = DICTIONARY_SIZE - length;
    if (preserve_bytes > 0) {
      address source = ARRAY_BASE(out_buffer()) + outOffset - preserve_bytes;
      jvm_memcpy(outFilePtr, source, preserve_bytes);
      outOffset = preserve_bytes;
    } else {
      outOffset = 0;
    }
    set_out_buffer(&new_out_buffer);
    set_out_dumped(outOffset);
  }

  outFilePtr += outOffset;
  outOffset += length;
  
  // Ready-to-use bytes in in_buffer
  int avail = ARRAY_LENGTH(in_buffer()) - inOffset;
  int deficit = length - avail;
  if (deficit > 0) {
    // We still need 'defict' bytes to be read directly from the file
    if (get_bytes_raw(outFilePtr + avail, deficit) != deficit) {
      return INFLATE_ERROR;
    }
    length = avail;
  }
  jvm_memcpy(outFilePtr, inFilePtr + inOffset, length);
  inOffset += length;

  STORE_IN;
  STORE_OUT;
  set_block_type(BTYPE_UNKNOWN);
  return INFLATE_MORE;
}

int Inflater::inflate_huffman(bool fixedHuffman JVM_TRAPS) {
  unsigned int code, litxlen;
  unsigned int quickDataSize = 0, quickDistanceSize = 0;
  HuffmanCodeTable *lcodes = NULL, *dcodes = NULL;

  if (!fixedHuffman) {
    lcodes = (HuffmanCodeTable*) ARRAY_BASE(length_buffer());
    dcodes = (HuffmanCodeTable*) ARRAY_BASE(distance_buffer());
    quickDataSize = lcodes->h.quickBits;
    quickDistanceSize = dcodes->h.quickBits;
  }

  LOAD_IN;
  LOAD_OUT;

  bool buffer_full = false;
  do {
    if (inOffset >= inLength) { // check input overflow
      refill_input(inOffset JVM_CHECK_0);
      inOffset = 0;
      break;
    }
    NEEDBITS(MAX_BITS + MAX_ZIP_EXTRA_LENGTH_BITS);
    
    if (fixedHuffman) {
      //   literal (hex)
      // 0x100 - 0x117   7   0.0000.00   -  0.0101.11
      //     0 -    8f   8   0.0110.000  -  1.0111.111
      //   118 -   11f   8   1.1000.000  -  1.1000.111
      //    90 -    ff   9   1.1001.0000 -  1.1111.1111
            
      // Get 9 bits, and reverse them.
      code = NEXTBITS(9);
      code = reverse_9bits(code);
      
      if (code <  0x060) {
        // A 7-bit code 
        DUMPBITS(7);
        litxlen = 0x100 + (code >> 2);
      } else if (code < 0x190) {
        DUMPBITS(8);
        litxlen = (code >> 1) + ((code < 0x180) ? (0x000 - 0x030)
                                 : (0x118 - 0x0c0));
      } else {
        DUMPBITS(9);
        litxlen = 0x90 + code - 0x190;
      }
    } else {
      GET_HUFFMAN_ENTRY(lcodes, quickDataSize, litxlen);
    }

    if (litxlen <= 255) {
      if (outOffset >= outLength) {
        if (isIncremental) {
          buffer_full = true;
        } else {
          ziperr(KVM_MSG_JAR_OUTPUT_OVERFLOW);
          return INFLATE_ERROR;
        }
      }
      outFilePtr[outOffset++] = (unsigned char)litxlen;
    } else if (litxlen == 256) {               // end of block
      set_block_type(BTYPE_UNKNOWN);
      break;
    } else if (litxlen > 285) {
      ziperr(KVM_MSG_JAR_INVALID_LITERAL_OR_LENGTH);
      return INFLATE_ERROR;
    } else {
      unsigned int n = litxlen - LITXLEN_BASE;
      unsigned int length = ll_length_base[n];
      unsigned int moreBits = ll_extra_bits[n];
      unsigned int d0, distance;

      // The NEEDBITS(..) above took care of this
      length += (juint) NEXTBITS(moreBits);
      DUMPBITS(moreBits);

      NEEDBITS(MAX_BITS);
      if (fixedHuffman) {
        d0 = (juint) NEXTBITS(5);
        d0 = reverse_5bits(d0);
        DUMPBITS(5);
      } else {
        GET_HUFFMAN_ENTRY(dcodes, quickDistanceSize, d0);
      }

      if (d0 > MAX_ZIP_DISTANCE_CODE) {
        ziperr(KVM_MSG_JAR_BAD_DISTANCE_CODE);
        return INFLATE_ERROR;
      }

      NEEDBITS(MAX_ZIP_EXTRA_DISTANCE_BITS);
      distance = dist_base[d0];
      moreBits = dist_extra_bits[d0];
      distance += (juint) NEXTBITS(moreBits);
      DUMPBITS(moreBits);

      if (outOffset < distance) {
        ziperr(KVM_MSG_JAR_COPY_UNDERFLOW);
        return INFLATE_ERROR;
      }
      if (outOffset + length > outLength) {
        if (isIncremental) {
          buffer_full = true;
        } else {
          ziperr(KVM_MSG_JAR_OUTPUT_OVERFLOW);
          return INFLATE_ERROR;
        }
      }
      juint prev = outOffset - distance;
      // src and destination may overlap, and we are to copy
      // in left-to-right order.
      while (length > distance) {
        outFilePtr[outOffset++] = outFilePtr[prev++];
        length--;
      }
      // With length <= distance, a memcpy is safe
      jvm_memcpy(&outFilePtr[outOffset], &outFilePtr[prev], length);
      outOffset += length;
    }
  } while (!buffer_full);
  
  STORE_IN;
  STORE_OUT;
  return INFLATE_MORE;
}

// Read in and decode the huffman tables in the compressed file

int Inflater::decode_dynamic_huffman_tables(JVM_SINGLE_ARG_TRAPS) {
  HuffmanCodeTable *ccodes;
  int hlit, hdist, hclen;
  int i;
  unsigned int quickBits;
  const int codelen_buf_size = 286 + 32;
  DECLARE_STATIC_BUFFER(unsigned char, codelen, codelen_buf_size);
  unsigned char *codePtr, *endCodePtr;

  // 5 Bits: hlit,  # of Literal/Length codes  (257 - 286) 
  // 5 Bits: hdist, # of Distance codes        (1 - 32) 
  // 4 Bits: hclen, # of Code Length codes     (4 - 19) 
    
  LOAD_IN;

  NEEDBITS(14);
  hlit = 257 + (jint) NEXTBITS(5);
  DUMPBITS(5);
  hdist = 1 + (jint) NEXTBITS(5);
  DUMPBITS(5);
  hclen = 4 + (jint) NEXTBITS(4);
  DUMPBITS(4);

  /*
   * The order in which the code lengths of the Code Length Alphabet is
   * given. See section 3.2.7 of RFC 1951.
   */
  static const char ccode_idx[] = {
      16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
  };

  //  hclen x 3 bits: code lengths for the code length
  //  alphabet given just above, in the order: 16, 17, 18,
  //  0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
  //
  //  These code lengths are interpreted as 3-bit integers
  //  (0-7); as above, a code length of 0 means the
  //  corresponding symbol (literal/length or distance code
  //  length) is not used.

  jvm_memset(codelen, 0x0, 19);
  for (i=0; i<hclen; i++) {
    NEEDBITS(3);
    if (inOffset >= inLength) { // check input overflow
      refill_input(inOffset JVM_CHECK_0);
      inFilePtr = ARRAY_BASE(in_buffer());
      inOffset = 0;
    }
    codelen[(int)ccode_idx[i]] = (unsigned char)(NEXTBITS(3));
    DUMPBITS(3);
  }

  UsingFastOops fast_oops;
  Buffer::Fast ccodesBuf =
    make_code_table(codelen, 19, MAX_QUICK_CXD JVM_CHECK_0);
  if (ccodesBuf.is_null()) {
    return INFLATE_ERROR;
  }
  inFilePtr = ARRAY_BASE(in_buffer()); // adjust after possible GC
  
  // DANGER:  ccodes is a heap object.   It can become
  // unusable anytime we allocate from the heap.
  ccodes = (HuffmanCodeTable *)ccodesBuf().base_address();
  quickBits = ccodes->h.quickBits;

  //
  // hlit code lengths for the literal/length alphabet,
  // encoded using the code length Huffman code.
  //
  // hdist code lengths for the distance alphabet,
  // encoded using the code length Huffman code.
  //
  // The code length repeat codes can cross from the literal/length
  // to the code length alphabets.  In other words, all code lengths form
  // a single sequence of hlit + hdist
    

  jvm_memset(codelen, 0x0, codelen_buf_size);
  for (codePtr = codelen, endCodePtr = codePtr + hlit + hdist;
       codePtr < endCodePtr; ) {

    unsigned int val;
    NEEDBITS(MAX_BITS + 7); // 7 is max repeat bits below
    GET_HUFFMAN_ENTRY(ccodes, quickBits, val);
    
    if (inOffset >= inLength) { // check input overflow
      refill_input(inOffset JVM_CHECK_0);
      inFilePtr = ARRAY_BASE(in_buffer());
      inOffset = 0;
      // adjust after possible GC
      ccodes = (HuffmanCodeTable *)ccodesBuf().base_address();
      quickBits = ccodes->h.quickBits;
    }

    //
    //  0 - 15: Represent code lengths of 0 - 15
    //      16: Copy the previous code length 3 - 6 times.
    //          3 + (2 bits of length)
    //      17: Repeat a code length of 0 for 3 - 10 times.
    //          3 + (3 bits of length)
    //      18: Repeat a code length of 0 for 11 - 138 times
    //          11 + (7 bits of length)
            
    if (val <= 15) {
      *codePtr++ = (unsigned char)val;
    } else if (val <= 18) {
      unsigned repeat  = (val == 18) ? 11 : 3;
      unsigned bits    = (val == 18) ? 7 : (val - 14);

      repeat += (unsigned) NEXTBITS(bits); // The NEEDBITS is above
      DUMPBITS(bits);

      if (codePtr + repeat > endCodePtr) {
        ziperr(KVM_MSG_JAR_BAD_REPEAT_CODE);
      }

      if (val == 16) {
        if (codePtr == codelen) {
          ziperr(KVM_MSG_JAR_BAD_REPEAT_CODE);
          return INFLATE_ERROR;
        }
        jvm_memset(codePtr, codePtr[-1], repeat);
      } else {
        // The values have already been set to zero, above, so
        // we don't have to do anything
      }
      codePtr += repeat;
    } else {
      ziperr(KVM_MSG_JAR_BAD_CODELENGTH_CODE);
      return INFLATE_ERROR;
    }
  }

  STORE_IN;

  Buffer::Raw lcodes =
    make_code_table(codelen, hlit, MAX_QUICK_LXL JVM_CHECK_0);
  if (lcodes.is_null()) {
    return INFLATE_ERROR;
  }
  set_length_buffer(&lcodes);

  Buffer::Raw dcodes =
    make_code_table(codelen + hlit, hdist, MAX_QUICK_CXD JVM_CHECK_0);
  if (dcodes.is_null()) {
    return INFLATE_ERROR;
  }
  set_distance_buffer(&dcodes);

  return INFLATE_MORE;
}

ReturnOop Inflater::make_code_table(unsigned char *codelen, // Code lengths
                                    // Number of elements of the alphabet
                                    unsigned numElems,  
                                    unsigned maxQuickBits JVM_TRAPS)
  // If the length of a code is longer than <maxQuickBits> number of bits, the
  // code is stored in the sequential lookup table instead of the quick 
  // lookup array.
{
  DECLARE_STATIC_BUFFER2(checker1, unsigned int, bitLengthCount, MAX_BITS + 1);
  DECLARE_STATIC_BUFFER2(checker2, unsigned int, codes,          MAX_BITS + 1);
  unsigned bits, minCodeLen = 0, maxCodeLen = 0;
  const unsigned char *endCodeLen = codelen + numElems;
  unsigned int code, quickMask;
  unsigned char *p;

  HuffmanCodeTable *table;
  unsigned int mainTableLength, longTableLength, numLongTables;
  unsigned int tableSize;
  unsigned int j;

  unsigned short *nextLongTable;

  // Count the number of codes for each code length
  jvm_memset(bitLengthCount, 0, (MAX_BITS + 1) * sizeof(bitLengthCount[0]));
  for (p = codelen; p < endCodeLen; p++) {
    bitLengthCount[*p]++;
  }

  if (bitLengthCount[0] == numElems) {
    // Empty code table is allowed!
  }

  // If the lookup table size heuristic is enabled, we set the size of the
  // first-level lookup table to the value that minimized the total size of
  // lookup tables for this Huffman tree. 
  if (EnableLookupTableSizeHeuristic) {
    // Find the minimum and maximum.  It's faster to do it in a separate
    // loop that goes 1..MAX_BITS, than in the above loop that looks at
    // every code element
    code = minCodeLen = maxCodeLen = 0;
    for (bits = MAX_BITS; bits >= 1; bits--) {
      if (bitLengthCount[bits] != 0) {
        maxCodeLen = bits;
        break;
      }
    }

    unsigned int optQuickBits = 0;
    unsigned int optTableSize = 0;

    for (bits = 1; bits <= maxCodeLen; bits++) {
      codes[bits] = code;
      if (bitLengthCount[bits] != 0) {
        if (minCodeLen == 0) { 
          minCodeLen = bits;
        }
        code += bitLengthCount[bits] << (MAX_BITS - bits);
      }

      tableSize = (1 << MAX_BITS) - code;
      tableSize >>= MAX_BITS - bits;
      tableSize <<= maxCodeLen - bits;
      tableSize += (1 << bits);

      if (optQuickBits == 0 || tableSize < optTableSize) {
        optTableSize = tableSize;
        optQuickBits = bits;
      }
    }

    maxQuickBits = optQuickBits;
  } else {
    // Find the minimum and maximum.  It's faster to do it in a separate
    // loop that goes 1..MAX_BITS, than in the above loop that looks at
    // every code element
    code = minCodeLen = maxCodeLen = 0;
    for (bits = 1; bits <= MAX_BITS; bits++) {
      codes[bits] = code;
      if (bitLengthCount[bits] != 0) {
        if (minCodeLen == 0) { 
          minCodeLen = bits;
        }
        maxCodeLen = bits;
        code += bitLengthCount[bits] << (MAX_BITS - bits);
      }
    }
  }

  // Calculate the size of the code table and allocate it.
  if (maxCodeLen <= maxQuickBits) {
    // We don't need any subtables.  We may even be able to get
    // away with a table smaller than maxCodeLen
        
    maxQuickBits = maxCodeLen;
    mainTableLength = (1 << maxCodeLen);
    numLongTables = longTableLength = 0;
  } else {
    mainTableLength = (1 << maxQuickBits);
    numLongTables = (1 << MAX_BITS) - codes[maxQuickBits + 1];
    numLongTables = numLongTables >> (MAX_BITS - maxQuickBits);
    longTableLength = 1 << (maxCodeLen - maxQuickBits);
  }

  GUARANTEE(((int)mainTableLength == 1 << maxQuickBits),
            "Main Table length error");
  tableSize = sizeof(HuffmanCodeTableHeader)
    + (mainTableLength + numLongTables * longTableLength)
    * sizeof(table->entries[0]);
  Buffer::Raw tableBuf = Universe::new_byte_array(tableSize JVM_CHECK_0);
  table = (HuffmanCodeTable *)tableBuf().base_address();

  nextLongTable = &table->entries[mainTableLength];

  // new_byte_array clears object
  //  jvm_memset(tableBuf.base_address(), 0, tableSize);

  table->h.maxCodeLen = (unsigned short)maxCodeLen;
  table->h.quickBits  = (unsigned short)maxQuickBits;

  quickMask = (1 << maxQuickBits) - 1;

  for (p = codelen; p < endCodeLen; p++) {
    unsigned short huff;
    bits = *p;
    if (bits != 0) {

      // Get the next code of the current length
      code = codes[bits];
      codes[bits] += 1 << (MAX_BITS - bits);
      code = reverse_15bits(code);
      huff = (unsigned short) (((p - codelen) << 4) + bits);

      if (bits <= maxQuickBits) {
        unsigned stride = 1 << bits;
        for (j = code; j < mainTableLength; j += stride) {
          table->entries[j] = huff;
        }
      } else {
        unsigned short *thisLongTable;
        unsigned stride = 1 << (bits - maxQuickBits);
        unsigned int prefixCode = code & quickMask;
        unsigned int suffixCode = code >> maxQuickBits;

        if (table->entries[prefixCode] == 0) {
          // This in the first long code with the indicated prefix.
          // Create a pointer to the subtable
          jint delta = (char *)nextLongTable - (char *)table;
          table->entries[prefixCode] = 
            (unsigned short)(HUFFINFO_LONG_MASK | delta);
          thisLongTable = nextLongTable;
          nextLongTable += longTableLength;
        } else {
          jint delta = table->entries[prefixCode] & ~HUFFINFO_LONG_MASK;
          thisLongTable = (unsigned short *)((char *)table + delta);
        }

        for (j = suffixCode; j < longTableLength; j += stride) {
          thisLongTable[j] = huff;
        }
      }
    }
  }

  GUARANTEE(nextLongTable == 
            &table->entries[mainTableLength + numLongTables * longTableLength],
            "nextLongTable incorrect");

  return tableBuf;
}

const unsigned char Inflater::ll_extra_bits[] = {
    /* 257 */   0,   /* 3       */
    /* 258 */   0,   /* 4       */
    /* 259 */   0,   /* 5       */
    /* 260 */   0,   /* 6       */
    /* 261 */   0,   /* 7       */
    /* 262 */   0,   /* 8       */
    /* 263 */   0,   /* 9       */
    /* 264 */   0,   /* 10      */
    /* 265 */   1,   /* 11,12   */
    /* 266 */   1,   /* 13,14   */
    /* 267 */   1,   /* 15,16   */
    /* 268 */   1,   /* 17,18   */
    /* 269 */   2,   /* 19-22   */
    /* 270 */   2,   /* 23-26   */
    /* 271 */   2,   /* 27-30   */
    /* 272 */   2,   /* 31-34   */
    /* 273 */   3,   /* 35-42   */
    /* 274 */   3,   /* 43-50   */
    /* 275 */   3,   /* 51-58   */
    /* 276 */   3,   /* 59-66   */
    /* 277 */   4,   /* 67-82   */
    /* 278 */   4,   /* 83-98   */
    /* 279 */   4,   /* 99-114  */
    /* 280 */   4,   /* 115-130 */
    /* 281 */   5,   /* 131-162 */
    /* 282 */   5,   /* 163-194 */
    /* 283 */   5,   /* 195-226 */
    /* 284 */   5,   /* 227-257 */
    /* 285 */   0,   /* 258     */
};

#define MAX_ZIP_EXTRA_LENGTH_BITS 5

const unsigned short Inflater::ll_length_base[] = {
    /* 257   0 */   3,
    /* 258   0 */   4,
    /* 259   0 */   5,
    /* 260   0 */   6,
    /* 261   0 */   7,
    /* 262   0 */   8,
    /* 263   0 */   9,
    /* 264   0 */   10,
    /* 265   1 */   11,  /* - 12 */ 
    /* 266   1 */   13,  /* - 14 */ 
    /* 267   1 */   15,  /* - 16 */ 
    /* 268   1 */   17,  /* - 18 */ 
    /* 269   2 */   19,  /* - 22 */ 
    /* 270   2 */   23,  /* - 26 */ 
    /* 271   2 */   27,  /* - 30 */ 
    /* 272   2 */   31,  /* - 34 */ 
    /* 273   3 */   35,  /* - 42 */ 
    /* 274   3 */   43,  /* - 50 */ 
    /* 275   3 */   51,  /* - 58 */ 
    /* 276   3 */   59,  /* - 66 */ 
    /* 277   4 */   67,  /* - 82 */ 
    /* 278   4 */   83,  /* - 98 */ 
    /* 279   4 */   99,  /* - 114 */ 
    /* 280   4 */   115, /* - 130 */ 
    /* 281   5 */   131, /* - 162 */ 
    /* 282   5 */   163, /* - 194 */ 
    /* 283   5 */   195, /* - 226 */ 
    /* 284   5 */   227, /* - 257 */ 
    /* 285   0 */   258
};

const unsigned char Inflater::dist_extra_bits[] = {
    /*  0 */   0,    /* 1            */
    /*  1 */   0,    /* 2            */
    /*  2 */   0,    /* 3            */
    /*  3 */   0,    /* 4            */
    /*  4 */   1,    /* 5,6          */
    /*  5 */   1,    /* 7,8          */
    /*  6 */   2,    /* 9-12         */
    /*  7 */   2,    /* 13-16        */
    /*  8 */   3,    /* 17-24        */
    /*  9 */   3,    /* 25-32        */

    /* 10 */   4,    /* 33-48        */
    /* 11 */   4,    /* 49-64        */
    /* 12 */   5,    /* 65-96        */
    /* 13 */   5,    /* 97-128       */
    /* 14 */   6,    /* 129-192      */
    /* 15 */   6,    /* 193-256      */
    /* 16 */   7,    /* 257-384      */
    /* 17 */   7,    /* 385-512      */
    /* 18 */   8,    /* 513-768      */
    /* 19 */   8,    /* 769-1024     */

    /* 20 */   9,    /* 1025-1536    */
    /* 21 */   9,    /* 1537-2048    */
    /* 22 */   10,   /* 2049-3072    */
    /* 23 */   10,   /* 3073-4096    */
    /* 24 */   11,   /* 4097-6144    */
    /* 25 */   11,   /* 6145-8192    */
    /* 26 */   12,   /* 8193-12288   */
    /* 27 */   12,   /* 12289-16384  */
    /* 28 */   13,   /* 16385-24576  */
    /* 29 */   13,   /* 24577-32768  */
};

#define MAX_ZIP_EXTRA_DISTANCE_BITS 13
#define MAX_ZIP_DISTANCE_CODE 29

const unsigned int Inflater::dist_base[] = {
    /*  0   0  */   1,
    /*  1   0  */   2,
    /*  2   0  */   3,
    /*  3   0  */   4,
    /*  4   1  */   5,      /* -6     */
    /*  5   1  */   7,      /* -8     */
    /*  6   2  */   9,      /* -12    */
    /*  7   2  */   13,     /* -16    */
    /*  8   3  */   17,     /* -24    */
    /*  9   3  */   25,     /* -32    */
    /* 10   4  */   33,     /* -48    */
    /* 11   4  */   49,     /* -64    */
    /* 12   5  */   65,     /* -96    */
    /* 13   5  */   97,     /* -128   */
    /* 14   6  */   129,    /* -192   */
    /* 15   6  */   193,    /* -256   */
    /* 16   7  */   257,    /* -384   */
    /* 17   7  */   385,    /* -512   */
    /* 18   8  */   513,    /* -768   */
    /* 19   8  */   769,    /* -1024  */
    /* 20   9  */   1025,   /* -1536  */
    /* 21   9  */   1537,   /* -2048  */
    /* 22   10 */   2049,   /* -3072  */
    /* 23   10 */   3073,   /* -4096  */
    /* 24   11 */   4097,   /* -6144  */
    /* 25   11 */   6145,   /* -8192  */
    /* 26   12 */   8193,   /* -12288 */
    /* 27   12 */   12289,  /* -16384 */
    /* 28   13 */   16385,  /* -24576 */
    /* 29   13 */   24577,  /* -32768 */
};

/* A table for reversing 5 bits in a binary number */

const unsigned char Inflater::reverse5[] = {
    0x0, 0x10, 0x8, 0x18, 0x4, 0x14, 0xc, 0x1c,
    0x2, 0x12, 0xa, 0x1a, 0x6, 0x16, 0xe, 0x1e,
    0x1, 0x11, 0x9, 0x19, 0x5, 0x15, 0xd, 0x1d,
    0x3, 0x13, 0xb, 0x1b, 0x7, 0x17, 0xf, 0x1f
};

#if ENABLE_FAST_CRC32
const juint Inflater::_fast_crc32_table[] = {
  0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 
  0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
  0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
  0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
  0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
  0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
  0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
  0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
  0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
  0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
  0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
  0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
  0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
  0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
  0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
  0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
  0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
  0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
  0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
  0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
  0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
  0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
  0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
  0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
  0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
  0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
  0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
  0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
  0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
  0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
  0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
  0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
  0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
  0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
  0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
  0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
  0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
  0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
  0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
  0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
  0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
  0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
  0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
  0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
  0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
  0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
  0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
  0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
  0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
  0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
  0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
  0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
  0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
  0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
  0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
  0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
  0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
  0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
  0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
  0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
  0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
  0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
  0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
  0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};
#endif
