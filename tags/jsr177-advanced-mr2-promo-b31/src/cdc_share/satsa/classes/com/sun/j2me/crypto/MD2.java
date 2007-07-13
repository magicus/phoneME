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

package com.sun.j2me.crypto;

import java.security.MessageDigestSpi;
import java.security.DigestException;

/**
 * The MD2 class is used to compute an MD2 message digest over a given
 * buffer of bytes. It is an implementation of the RSA Data Security Inc
 * MD5 algorithim as described in internet RFC 1319.
 */
public final class MD2 extends MessageDigestSpi implements Cloneable {
    
    /** contains the computed message digest */
    private byte[] digest;
    
    private int state[]; /** State */
    private int checksum[]; /** Checksum */
    private int count; /** Number of bytes modulo 16 */
    private byte buffer[]; /** Input buffer  */
    
    private static final int MD2_LENGTH = 16;

    /** The magic S table  */
    private static final int S[] ={ 
        0x29, 0x2E, 0x43, 0xC9, 0xA2, 0xD8, 0x7C, 0x01,
        0x3D, 0x36, 0x54, 0xA1, 0xEC, 0xF0, 0x06, 0x13,
        0x62, 0xA7, 0x05, 0xF3, 0xC0, 0xC7, 0x73, 0x8C,
        0x98, 0x93, 0x2B, 0xD9, 0xBC, 0x4C, 0x82, 0xCA,
        0x1E, 0x9B, 0x57, 0x3C, 0xFD, 0xD4, 0xE0, 0x16,
        0x67, 0x42, 0x6F, 0x18, 0x8A, 0x17, 0xE5, 0x12,
        0xBE, 0x4E, 0xC4, 0xD6, 0xDA, 0x9E, 0xDE, 0x49,
        0xA0, 0xFB, 0xF5, 0x8E, 0xBB, 0x2F, 0xEE, 0x7A,
        0xA9, 0x68, 0x79, 0x91, 0x15, 0xB2, 0x07, 0x3F,
        0x94, 0xC2, 0x10, 0x89, 0x0B, 0x22, 0x5F, 0x21,
        0x80, 0x7F, 0x5D, 0x9A, 0x5A, 0x90, 0x32, 0x27,
        0x35, 0x3E, 0xCC, 0xE7, 0xBF, 0xF7, 0x97, 0x03,
        0xFF, 0x19, 0x30, 0xB3, 0x48, 0xA5, 0xB5, 0xD1,
        0xD7, 0x5E, 0x92, 0x2A, 0xAC, 0x56, 0xAA, 0xC6,
        0x4F, 0xB8, 0x38, 0xD2, 0x96, 0xA4, 0x7D, 0xB6,
        0x76, 0xFC, 0x6B, 0xE2, 0x9C, 0x74, 0x04, 0xF1,
        0x45, 0x9D, 0x70, 0x59, 0x64, 0x71, 0x87, 0x20,
        0x86, 0x5B, 0xCF, 0x65, 0xE6, 0x2D, 0xA8, 0x02,
        0x1B, 0x60, 0x25, 0xAD, 0xAE, 0xB0, 0xB9, 0xF6,
        0x1C, 0x46, 0x61, 0x69, 0x34, 0x40, 0x7E, 0x0F,
        0x55, 0x47, 0xA3, 0x23, 0xDD, 0x51, 0xAF, 0x3A,
        0xC3, 0x5C, 0xF9, 0xCE, 0xBA, 0xC5, 0xEA, 0x26,
        0x2C, 0x53, 0x0D, 0x6E, 0x85, 0x28, 0x84, 0x09,
        0xD3, 0xDF, 0xCD, 0xF4, 0x41, 0x81, 0x4D, 0x52,
        0x6A, 0xDC, 0x37, 0xC8, 0x6C, 0xC1, 0xAB, 0xFA,
        0x24, 0xE1, 0x7B, 0x08, 0x0C, 0xBD, 0xB1, 0x4A,
        0x78, 0x88, 0x95, 0x8B, 0xE3, 0x63, 0xE8, 0x6D,
        0xE9, 0xCB, 0xD5, 0xFE, 0x3B, 0x00, 0x1D, 0x39,
        0xF2, 0xEF, 0xB7, 0x0E, 0x66, 0x58, 0xD0, 0xE4,
        0xA6, 0x77, 0x72, 0xF8, 0xEB, 0x75, 0x4B, 0x0A,
        0x31, 0x44, 0x50, 0xB4, 0x8F, 0xED, 0x1F, 0x1A,
        0xDB, 0x99, 0x8D, 0x33, 0x9F, 0x11, 0x83, 0x14,
    };
    
    /** Create an MD2 digest object. */
    public MD2() {
        init();
    }

    private MD2(MD2 md2) {
        this();
        this.count = (int)md2.count;
        this.buffer = (byte[])md2.buffer.clone();
        this.checksum = (int[])md2.checksum.clone();
        this.state = (int[])md2.state.clone();
        this.digest = (byte[])md2.digest.clone();
    }

    void transform(byte buf[], int offset) {
        int i, j, t;
        int x[] = new int[48];
        
        j = checksum[15];
        for (i = 0; i < 16; i++) {
            x[i] = state[i];
            x[i + 16] = t = (int)buf[i + offset] & 0xff;
            x[i + 32] = (t ^ state[i]);
            int k = t ^ j;
            j = checksum[i] ^= S[t ^ j];
        }
        
        t = 0;
        for (i = 0; i < 18; i++) {
            for (j = 0; j < 48; j++) {
                t = x[j] ^= S[t];                
            }
            t = (t + i) & 0xff;
        }

        for (i = 0; i < 16; i++) {
            state[i] = x[i];
        }        
    }
    
    /** 
     * Initialize the MD2 state information to the initial state for further use.
     */
    public void init() {
        count = 0;
        state = new int[16];
        buffer = new byte[16];
        checksum = new int[16];
        digest = new byte[16];
        
        for (int i = 0; i < 16; i++) {
            buffer[i] = 0;
            checksum[i] = 0;
            state[i] = 0;
            digest[i] = 0;
        }
    }

    protected void engineReset() {
        init();
    }

    /**
     * Return the digest length in bytes
     */
    protected int engineGetDigestLength() {
        return (MD2_LENGTH);
    }

    /**
     * Update a byte.
     * @param b	the byte
     */
    protected void engineUpdate(byte b) {
        int index;

        index = count & 0xf;
        count++;
        buffer[index] = b;
        if (index >= 15)
            transform(buffer, 0);        
    }
    
    /**
     * Adds the selected part of an array of bytes to the digest.
     * @param input input buffer of data to be hashed
     * @param offset offset within inBuf where input data begins
     * @param len length (in bytes) of data to be hashed
     */
    protected void engineUpdate(byte[] input, int offset, int len) {
        for (int i = offset; len > 0; ) {
            int	index = count & 0xf;
            
            if (index == 0 && len > 16) {
                count += 16;
                transform (input, i);
                len -= 16;
                i += 16;
            } else {
                count++;
                buffer[index] = input [i];
                if (index  >= 15)
                    transform (buffer, 0);
                i++;
                len--;
            }
        }
    }

    /**
     * Perform the final computations, any buffered bytes are added
     * to the digest, the count is added to the digest, and the resulting
     * digest is stored. After calling finish you will need to call
     * init() again to do another digest.
     */
    private void finish() {
        byte temp[];
        byte i;
        int index, padLen;

        index = count & 0xf;
        padLen = 16 - index;

        temp = new byte[16];

        for (i = 0; i < padLen; i++)
            temp[i] = i;
        engineUpdate(temp, 0, padLen);

        for (i = 0; i < 16; i++)
            temp[i] = (byte)(checksum[i] & 0xff);
        engineUpdate(temp, 0, 16);

        for (i = 0; i < 16; i++)
            digest[i] = (byte)(state[i] & 0xff);
    }

    
    protected byte[] engineDigest() {
        finish();

        byte[] result = new byte[MD2_LENGTH];
        System.arraycopy(digest, 0, result, 0, MD2_LENGTH);

        init();

        return result;
    }
    
    /**
     * Completes the hash computation by performing final operations
     * such as padding. The digest is reset after this call is made.
     *
     * @param buf output buffer for the computed digest
     * @param offset offset into the output buffer to begin storing the digest
     * @param len number of bytes within buf allotted for the digest
     * @return the number of bytes placed into <code>buf</code>
     * 
     * @exception DigestException if an error occurs.
     */
    public int engineDigest(byte[] buf, int offset, int len) throws DigestException {
        finish();

        if (len < MD2_LENGTH)
            throw new DigestException("partial digests not returned");
        if (buf.length - offset < MD2_LENGTH)
            throw new DigestException("insufficient space in the output " +
                        "buffer to store the digest");

        System.arraycopy(digest, 0, buf, offset, MD2_LENGTH);
        init();

        return MD2_LENGTH;
    }

    /** 
     * Clones the MessageDigest object.
     * @return a clone of this object
     */
    public Object clone() {
        MD2 that = null;
        try {
            that = (MD2)super.clone();
            that.state = (int[])this.state.clone();
            that.buffer = (byte[])this.buffer.clone();
            that.digest = (byte[])this.digest.clone();
            that.checksum = (int[])this.checksum.clone();
            that.count = this.count;
            return that;
        }
        catch (CloneNotSupportedException e) {
        }
        return that;
    }
}
