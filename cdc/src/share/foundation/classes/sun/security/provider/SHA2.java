/*
 * @(#)SHA2.java	1.6 06/10/10
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

package sun.security.provider;

import java.security.*;
import java.math.BigInteger;

/**
 * This class implements the Secure Hash Algorithm SHA-256 developed by
 * the National Institute of Standards and Technology along with the
 * National Security Agency.
 *
 * <p>It implements java.security.MessageDigestSpi, and can be used
 * through Java Cryptography Architecture (JCA), as a pluggable 
 * MessageDigest implementation.
 * 
 * @version     1.6 10/10/06
 * @author	Valerie Peng
 */

public class SHA2 extends MessageDigestSpi implements Cloneable {

    private static final int LENGTH = 32;
    private static final int ITERATION = 64;
    private static final int COUNT_MASK = 63; // block size - 1
    // Constants for each round
    private static final int[] ROUND_CONSTS = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    private int W[] = new int[ITERATION];
    private long count = 0;
    private int AA, BB, CC, DD, EE, FF, GG, HH;

    /**
     * logical function ch(x,y,z) as defined in spec:
     * @return (x and y) xor ((complement x) and z)
     * @param x int
     * @param y int
     * @param z int
     */
    private static int lf_ch(int x, int y, int z) {
     return (x & y) ^ ((~x) & z);
    }

    /**
     * logical function maj(x,y,z) as defined in spec:
     * @return (x and y) xor (x and z) xor (y and z)
     * @param x int
     * @param y int
     * @param z int
     */
    private static int lf_maj(int x, int y, int z) {
     return (x & y) ^ (x & z) ^ (y & z);
    }

    /**
     * logical function R(x,s) - right shift
     * @return x right shift for s times
     * @param x int
     * @param s int
     */
    private static int lf_R( int x, int s ) {
	return (x >>> s);
    }

    /**
     * logical function S(x,s) - right rotation
     * @return x circular right shift for s times
     * @param x int
     * @param s int
     */
    private static int lf_S(int x, int s) {
	return (x >>> s) | (x << (32 - s));
    }

    /**
     * logical function sigma0(x) - xor of results of right rotations
     * @return S(x,2) xor S(x,13) xor S(x,22)
     * @param x int
     */
    private static int lf_sigma0(int x) {
	return lf_S(x, 2) ^ lf_S(x, 13) ^ lf_S(x, 22);
    }

    /**
     * logical function sigma1(x) - xor of results of right rotations
     * @return S(x,6) xor S(x,11) xor S(x,25)
     * @param x int
     */
    private static int lf_sigma1(int x) {
	return lf_S( x, 6 ) ^ lf_S( x, 11 ) ^ lf_S( x, 25 );
    }

    /**
     * logical function delta0(x) - xor of results of right shifts/rotations
     * @return int
     * @param x int
     */
    private static int lf_delta0(int x) {
	return lf_S(x, 7) ^ lf_S(x, 18) ^ lf_R(x, 3);
    }

    /**
     * logical function delta1(x) - xor of results of right shifts/rotations
     * @return int
     * @param x int
     */
    private static int lf_delta1(int x) {
	return lf_S(x, 17) ^ lf_S(x, 19) ^ lf_R(x, 10);
    }

    /**
     * Creates a SHA2 object.with state (for cloning) 
     */
    private SHA2(SHA2 sha) {
	this();
	System.arraycopy(sha.W, 0, this.W, 0, W.length);
	this.count = sha.count;
	this.AA = sha.AA;
	this.BB = sha.BB;
	this.CC = sha.CC;
	this.DD = sha.DD;
	this.EE = sha.EE;
	this.FF = sha.FF;
	this.GG = sha.GG;
	this.HH = sha.HH;
    }

    /**
     * Creates a new SHA object.
     */
    public SHA2() {
	init();
    }

    /**
     * @return the length of the digest in bytes
     */
    protected int engineGetDigestLength() {
	return (LENGTH);
    }

    /**
     * Update a byte.
     *
     * @param b	the byte
     */
    protected void engineUpdate(byte b) {
	update((int)b);
    }

    private void update(int b)  {
	int word;  // index inside this block, i.e. from 0 to 15.
	int offset; // offset of this byte inside the word

	/* compute word index within the block and bit offset within the word.
	   block size is 64 bytes with word size is 4 bytes. offset is in 
	   terms of bits */
	word = ((int)count & COUNT_MASK) >>> 2;
	offset = (~(int)count & 3) << 3;

	// clear the byte inside W[word] and then 'or' it with b's byte value
	W[word] = (W[word] & ~(0xff << offset)) | ((b & 0xff) << offset);

	/* If this is the last byte of a block, compute the partial hash */
	if (((int)count & COUNT_MASK) == COUNT_MASK) {
	    computeBlock();
	}
	count++;
    }
    	
    /**
     * Update a buffer.
     *
     * @param b	the data to be updated.
     * @param off the start offset in the data
     * @param len the number of bytes to be updated.
     */
    protected void engineUpdate(byte b[], int off, int len) {
	int word;
	int offset;

	if ((off < 0) || (len < 0) || (off + len > b.length))
	    throw new ArrayIndexOutOfBoundsException();

	// Use single writes until integer aligned
	while ((len > 0) &&
	       ((int)count & 3) != 0) {
	    engineUpdate(b[off]);
	    off++;
	    len--;
	}
    
	/* Assemble groups of 4 bytes to be inserted in integer array */
	while (len >= 4) {

	    word = ((int)count & COUNT_MASK) >> 2;

	    W[word] = ((b[off] & 0xff) << 24) |
		((b[off+1] & 0xff) << 16) |
		((b[off+2] & 0xff) << 8) |
		((b[off+3] & 0xff) );
	    
	    count += 4;
	    if (((int)count & COUNT_MASK) == 0) {
		computeBlock();
	    }
	    len -= 4;
	    off += 4;
	}
	
	/* Use single writes for last few bytes */
	while (len > 0) {
	    engineUpdate(b[off++]);
	    len--;
	}
    }
    
    /**
     * Resets the buffers and hash value to start a new hash.
     */
    private void init() {
	AA = 0x6a09e667;
	BB = 0xbb67ae85;
	CC = 0x3c6ef372;
	DD = 0xa54ff53a;
	EE = 0x510e527f;
	FF = 0x9b05688c;
	GG = 0x1f83d9ab;
	HH = 0x5be0cd19;
	for (int i = 0; i < ITERATION; i++)
	    W[i] = 0;
	count = 0;
    }

    /**
     * Resets the buffers and hash value to start a new hash.
     */
    protected void engineReset() {
	init();
    }
    
    /**
     * Computes the final hash and returns the final value as a
     * byte[32] array. The object is reset to be ready for further
     * use, as specified in the JavaSecurity MessageDigest
     * specification.  
     * @return the digest
     */
    protected byte[] engineDigest() {
	byte hashvalue[] = new byte[LENGTH];

	try {
	    int outLen = engineDigest(hashvalue, 0, hashvalue.length);
	} catch (DigestException e) {
	    throw new InternalError("");
	}
	return hashvalue;
    }

    /**
     * Computes the final hash and places the final value in the
     * specified array. The object is reset to be ready for further
     * use, as specified in java.security.MessageDigest javadoc
     * specification.
     * @param hashvalue buffer to hold the digest
     * @param offset offset for storing the digest
     * @param len length of the buffer
     * @return length of the digest in bytes
     */
    protected int engineDigest(byte[] hashvalue, int offset, int len)
	throws DigestException {

	if (len < LENGTH) {
	    throw new DigestException("partial digests not returned");
	}
	if (hashvalue.length - offset < LENGTH) {
	    throw new DigestException("insufficient space in the output " +
				      "buffer to store the digest");
	}
	/* The input length in bits before padding occurs */
	long inputLength = count << 3;	
	
	update(0x80);

	/* Pad with zeros until overall length is a multiple of 448 */
	while ((int)(count & COUNT_MASK) != 56) {
	    update(0);
	}

	W[14] = (int)(inputLength >>> 32);
	W[15] = (int)(inputLength & 0xffffffff);

	count += 8;
	computeBlock();

	// Copy out the result
	hashvalue[offset + 0] = (byte)(AA >>> 24);
	hashvalue[offset + 1] = (byte)(AA >>> 16);
	hashvalue[offset + 2] = (byte)(AA >>> 8);
	hashvalue[offset + 3] = (byte)(AA >>> 0);

	hashvalue[offset + 4] = (byte)(BB >>> 24);
	hashvalue[offset + 5] = (byte)(BB >>> 16);
	hashvalue[offset + 6] = (byte)(BB >>> 8);
	hashvalue[offset + 7] = (byte)(BB >>> 0);

	hashvalue[offset + 8] = (byte)(CC >>> 24);
	hashvalue[offset + 9] = (byte)(CC >>> 16);
	hashvalue[offset + 10] = (byte)(CC >>> 8);
	hashvalue[offset + 11] = (byte)(CC >>> 0);

	hashvalue[offset + 12] = (byte)(DD >>> 24);
	hashvalue[offset + 13] = (byte)(DD >>> 16);
	hashvalue[offset + 14] = (byte)(DD >>> 8);
	hashvalue[offset + 15] = (byte)(DD >>> 0);

	hashvalue[offset + 16] = (byte)(EE >>> 24);
	hashvalue[offset + 17] = (byte)(EE >>> 16);
	hashvalue[offset + 18] = (byte)(EE >>> 8);
	hashvalue[offset + 19] = (byte)(EE >>> 0);

	hashvalue[offset + 20] = (byte)(FF >>> 24);
	hashvalue[offset + 21] = (byte)(FF >>> 16);
	hashvalue[offset + 22] = (byte)(FF >>> 8);
	hashvalue[offset + 23] = (byte)(FF >>> 0);

	hashvalue[offset + 24] = (byte)(GG >>> 24);
	hashvalue[offset + 25] = (byte)(GG >>> 16);
	hashvalue[offset + 26] = (byte)(GG >>> 8);
	hashvalue[offset + 27] = (byte)(GG >>> 0);

	hashvalue[offset + 28] = (byte)(HH >>> 24);
	hashvalue[offset + 29] = (byte)(HH >>> 16);
	hashvalue[offset + 30] = (byte)(HH >>> 8);
	hashvalue[offset + 31] = (byte)(HH >>> 0);

	engineReset();		// remove the evidence
	
	return LENGTH;
    }
    
    /**
     * Compute the hash for the current block and stores the results
     * in internal variable AA, BB, CC, DD, EE, FF, GG, HH
     */
    private void computeBlock() {
	int T1, T2, a, b, c, d, e, f, g, h;
	
	// The first 16 ints are from the byte stream, compute the rest of
	// the W[]'s
	for (int t = 16; t < ITERATION; t++) {
	    W[t] = lf_delta1(W[t-2]) + W[t-7] + lf_delta0(W[t-15]) 
		   + W[t-16];
	}
	
	a = AA;
	b = BB;
	c = CC;
	d = DD;
	e = EE;
	f = FF;
	g = GG;
	h = HH;

	for (int i = 0; i < ITERATION; i++) {
	    T1 = h + lf_sigma1(e) + lf_ch(e,f,g) + ROUND_CONSTS[i] + W[i];
	    T2 = lf_sigma0(a) + lf_maj(a,b,c);
	    h = g;
	    g = f;
	    f = e;
	    e = d + T1;
	    d = c;
	    c = b;
	    b = a;
	    a = T1 + T2;
	}
	AA += a;
	BB += b;
	CC += c;
	DD += d;
	EE += e;
	FF += f;
	GG += g;
	HH += h;
    }

    /*
     * Clones this object.
     */
    public Object clone() {
	SHA2 that = null;
	that = (SHA2) new SHA2(this);
	return that;
    }
}

