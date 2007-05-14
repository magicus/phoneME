/*
 * 
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved. 
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

package com.sun.pisces;

import java.io.InputStream;
import java.io.IOException;
import java.util.Hashtable;

class HuffmanTable {

    GZIPInputStream in;
    Hashtable codeTable;
    int minLen;

    public HuffmanTable(GZIPInputStream in, int[] lengths) {
        this.in = in;
        this.codeTable = buildHuffman(lengths);
        this.minLen = Integer.MAX_VALUE;
        for (int i = 0; i < lengths.length; i++) {
            if (lengths[i] < minLen) {
                minLen = lengths[i];
            }
        }
    }

    private static Integer getKey(int code, int len) {
        return new Integer((code << 8) | len);
    }

    private static Hashtable buildHuffman(int[] tlen) {
        int len = tlen.length;

        int[] bl_count = new int[33];
        int[] next_code = new int[33];
    
        int maxlen = -1;
        for (int i = 0; i < len; i++) {
            if (tlen[i] > maxlen) {
                maxlen = tlen[i];
            }
            ++bl_count[tlen[i]];
        }

        int code = 0;
        bl_count[0] = 0;
        for (int bits = 1; bits <= maxlen; bits++) {
            code = (code + bl_count[bits - 1]) << 1;
            next_code[bits] = code;
        }

        Hashtable codeTable = new Hashtable(len);
        for (int n = 0; n < len; n++) {
            int l = tlen[n];
            if (l != 0) {
                codeTable.put(getKey(next_code[l], l), new Integer(n));
                ++next_code[l];
            }
        }

        return codeTable;
    }

    private int getVal(int code, int len) {
        Object o = codeTable.get(getKey(code, len));
        if (o != null) {
            return ((Integer)o).intValue();
        }
        return -1;
    }

    public int readSymbol() throws IOException {
        int code = in.readHuffmanBits(minLen);
        int len = minLen;

        while (true) {
            int val = getVal(code, len);
            if (val != -1) {
                return val;
            }
            code = (code << 1) | in.readBit();
            ++len;
        }
    }
}

/**
 * A simple implementation of the gzip/deflate compression scheme.
 * The entire source stream is read and decoded at once; although this
 * requires allocating storage for the entire output, it avoids the
 * need for a pair of 32K buffers that would normally be used for
 * decompression purposes.  For the common use case of loading font
 * files, which will be decompressed and parsed into a
 * <code>PiscesFont</code> object immediately, there is no real
 * drawback to this eager approach.
 *
 * <p> For simplicitly, <code>mark</code> and <code>reset</code> are
 * not supported.
 *
 */
class GZIPInputStream extends InputStream {
    
    private static final int[] perm = { 16, 17, 18,
                                        0,  8, 7,  9, 6, 10, 5, 11,
                                        4, 12, 3, 13, 2, 14, 1, 15 };

    private static final int[] lengthsTable;
    /* {
         3, 4, 5, 6, 7, 8, 9, 10,
         11, 13, 15, 17,
         19, 23, 27, 31,
         35, 43, 51, 59,
         67, 83, 99, 115,
         131, 163, 195, 227,
         258
      } */

    private static final int[] lengthExtraBitsTable;
    /* {
         0, 0, 0, 0, 0, 0, 0, 0,
         1, 1, 1, 1,
         2, 2, 2, 2,
         3, 3, 3, 3,
         4, 4, 4, 4, 
         5, 5, 5, 5,
         0
       } */

    private static final int[] distancesTable;
    /* {
         1, 2, 3, 4,
         5, 7,
         9, 13,
         17, 25,
         33, 49,
         65, 97,
         129, 193,
         257, 385,
         513, 769,
         1025, 1537,
         2049, 3073,
         4097, 6145,
         8193, 12289,
         16385, 24577,
       } */

    private static final int[] distanceExtraBitsTable;
    /* {
         0, 0, 0, 0,
         1, 1,
         2, 2, 
         3, 3,
         4, 4,
         5, 5,
         6, 6,
         7, 7,
         8, 8,
         9, 9,
         10, 10,
         11, 11,
         12, 12,
         13, 13
       } */

    InputStream in;

    byte[] data = new byte[100];
    int count = 0;
    int idx = 0;

    int curByte;
    int curPos = 8;

    // It takes less space to initialize the literal/length and 
    // distance tables programatically than using explicit initialzers
    // due to the verbose way initializers are translated into bytecode.
    static {
        lengthExtraBitsTable = new int[29];
        lengthsTable = new int[29];
        distanceExtraBitsTable = new int[30];
        distancesTable = new int[30];

        int idx = 0;
        for (int i = 4; i < 29; i++) {
            lengthExtraBitsTable[i] = idx;
            if ((i % 4) == 3) {
                ++idx;
            }
        }

        int len = 3;
        for (int i = 0; i < lengthExtraBitsTable.length; i++) {
            lengthsTable[i] = len;
            len += 1 << lengthExtraBitsTable[i];
        }
        lengthsTable[28] = 258; // that's what the spec says...

        idx = 0;
        for (int i = 2; i < 30; i++) {
            distanceExtraBitsTable[i] = idx;
            if ((i % 2) == 1) {
                ++idx;
            }
        }

        int code = 1;
        for (int i = 0; i < distancesTable.length; i++) {
            distancesTable[i] = code;
            code += 1 << distanceExtraBitsTable[i];
        }
    }

    public GZIPInputStream(InputStream in) throws IOException {
        this.in = in;

        // Read GZIP header
        readGZIPHeader();

        // Read blocks until final block is reached
        while (!readBlock()) {
        }
    }

    private void readGZIPHeader() throws IOException {
        int id1 = in.read(); // 31
        int id2 = in.read(); // 139
        int cm = in.read();

        int flg = in.read();
        int ftext = flg & 0x1;
        int fhcrc = (flg >> 1) & 0x1;
        int fextra = (flg >> 2) & 0x1;
        int fname = (flg >> 3) & 0x1;
        int fcomment = (flg >> 4) & 0x1;

        int mtime0 = in.read();
        int mtime1 = in.read();
        int mtime2 = in.read();
        int mtime3 = in.read();
        long mtime = ((long)mtime3 << 24) | ((long)mtime2 << 16) |
            ((long)mtime1 << 8) | ((long)mtime0);

        int xfl = in.read();
        int os = in.read();

        // Skip optional header fields
        if (fextra == 0x1) {
            int xlen = (in.read() << 8) | in.read();
            for (int i = 0; i < xlen; i++) {
                in.read();
            }
        }

        if (fname == 0x1) {
            while (in.read() != 0) {
            }
        }
        if (fcomment == 0x1) {
            while (in.read() != 0) {
            }
        }
        if (fhcrc == 0x1) {
            int crc = (in.read() << 8) | in.read();
        }
    }

    int readBit() throws IOException {
        if (curPos == 8) {
            curByte = in.read();
            curPos = 0;
        }

        int bit = (curByte >> curPos++) & 0x1;
        return bit;
    }

    int readBits(int numBits) throws IOException {
        int val = 0;
        for (int i = 0; i < numBits; i++) {
            val |= readBit() << i;
        }
        return val;
    }

    int readHuffmanBits(int numBits)
        throws IOException {
        int val = 0;
        for (int i = 0; i < numBits; i++) {
            val <<= 1;
            val |= readBit();
        }
        return val;
    }

    private void emit(int b) {
        if (count >= data.length) {
            byte[] tmp = new byte[data.length + 512];
            System.arraycopy(data, 0, tmp, 0, data.length);
            data = tmp;
        }
        data[count++] = (byte)b;
    }

    private boolean readBlock() throws IOException {
        int bfinal = readBits(1);
        int btype = readBits(2);

        if (btype == 0) {
            // Uncompressed data
            readBits(5); // skip extra bits
            int len = (in.read() << 8) | in.read();
            in.read();
            in.read();
            for (int i = 0; i < len; i++) {
                emit(in.read());
            }
        } else if (btype == 1 || btype == 2) {
            HuffmanTable lltable = null;
            HuffmanTable dtable = null;

            if (btype == 2) {
                // Dynamic Huffman codes

                int hlit = readBits(5) + 257;
                int hdist = readBits(5) + 1;
                int hclen = readBits(4) + 4;
                
                int[] hlengths = new int[19];
                for (int i = 0; i < hclen; i++) {
                    int len = readBits(3);
                    hlengths[perm[i]] = len;
                }
                HuffmanTable htable = new HuffmanTable(this, hlengths);

                int[] lengths = new int[hlit + hdist];
                int idx = 0;

                do {
                    int sym = htable.readSymbol();
                    if (sym <= 15) {
                        lengths[idx++] = sym;
                    } else if (sym == 16) {
                        int repeat = readBits(2) + 3;
                        int prev = lengths[idx - 1];
                        for (int i = 0; i < repeat; i++) {
                            lengths[idx++] = prev;
                        }
                    } else {
                        int bits = (sym == 17) ? 3 : 7;
                        int repeat = readBits(bits);
                        repeat += (sym == 17) ? 3 : 11;
                        for (int i = 0; i < repeat; i++) {
                            lengths[idx++] = 0;
                        }
                    }
                } while (idx < hlit + hdist);

                int[] hlitlengths = new int[hlit];
                System.arraycopy(lengths, 0, hlitlengths, 0, hlit);
                lltable = new HuffmanTable(this, hlitlengths);

                int[] hdistlengths = new int[hdist];
                System.arraycopy(lengths, hlit, hdistlengths, 0, hdist);
                dtable = new HuffmanTable(this, hdistlengths);
            }

            // Decompress actual data
            while (true) {
                int llcode = -1;
                if (btype == 1) {
                    int code = readHuffmanBits(7);
                    
                    if (code <= 23) {
                        llcode = 256 + code;
                    } else {
                        // 8 bit codes
                        code <<= 1;
                        code |= readBit();
                        
                        if (code < 192) {
                            llcode = code - 48;
                        } else if (code < 200) {
                            llcode = 280 + code - 192;
                        } else {
                            // 9 bit codes
                            code <<= 1;
                            code |= readBit();
                            llcode = 144 + code - 400;
                        }
                    }
                } else if (btype == 2) {
                    llcode = lltable.readSymbol();
                }
                
                if (llcode < 256) {
                    emit(llcode);
                } else if (llcode == 256) {
                    break;
                } else if (llcode <= 285) {
                    int length = lengthsTable[llcode - 257];
                    int extraLengthBits = lengthExtraBitsTable[llcode - 257];
                    if (extraLengthBits > 0) {
                        int extra = readBits(extraLengthBits);
                        length += extra;
                    }

                    int distanceCode = -1;
                    if (btype == 1) {
                        distanceCode = readHuffmanBits(5);
                    } else if (btype == 2) {
                        distanceCode = dtable.readSymbol();
                    }
                    
                    int distance = distancesTable[distanceCode];
                    int extraDistBits = distanceExtraBitsTable[distanceCode];
                    if (extraDistBits > 0) {
                        int extra = readBits(extraDistBits);
                        distance += extra;
                    }

                    for (int i = 0; i < length; i++) {
                        emit(data[count - distance]);
                    }
                } else {
                    // error
                }
            }
        } else {
            // error
        }

        return (bfinal == 0x1);
    }

    public int available() throws IOException {
        return count - idx;
    }

    public int read() throws IOException {
        if (idx == count) {
            return -1;
        } else {
            return data[idx++] & 0xff;
        }
    }

    public int read(byte[] buf, int off, int len) throws IOException {
        for (int i = off; i < off + len; i++) {
            if (idx == count) {
                return i - off;
            }
            buf[i] = data[idx++];
        }
        return len;
    }

    public long skip(long n) throws IOException {
        int saveIdx = idx;
        idx = Math.min((int)(idx + n), count);
        return idx - saveIdx;
    }

    public void close() throws IOException {
        // do nothing
    }

    public boolean markSupported() {
        return false;
    }

    public synchronized void mark(int readlimit) {
        // do nothing
    }

    public synchronized void reset() throws IOException {
        throw new IOException("mark/reset not supported");
    }
}
