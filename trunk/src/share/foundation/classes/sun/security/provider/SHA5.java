/*
 * @(#)SHA5.java	1.6 06/10/10
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
 * This class implements the Secure Hash Algorithm SHA-512 developed by
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

public class SHA5 extends MessageDigestSpi implements Cloneable {

    private static final int LENGTH = 64;
    private static final long [] INITIAL_HASHES = {
	0x6a09e667f3bcc908L, 0xbb67ae8584caa73bL,
	0x3c6ef372fe94f82bL, 0xa54ff53a5f1d36f1L,
	0x510e527fade682d1L, 0x9b05688c2b3e6c1fL,
	0x1f83d9abfb41bd6bL, 0x5be0cd19137e2179L 
    };
    private static final int ITERATION = 80;
    // Constants for each round/iteration
    private static final long[] ROUND_CONSTS = {
	0x428A2F98D728AE22L, 0x7137449123EF65CDL, 0xB5C0FBCFEC4D3B2FL, 
	0xE9B5DBA58189DBBCL, 0x3956C25BF348B538L, 0x59F111F1B605D019L, 
	0x923F82A4AF194F9BL, 0xAB1C5ED5DA6D8118L, 0xD807AA98A3030242L, 
	0x12835B0145706FBEL, 0x243185BE4EE4B28CL, 0x550C7DC3D5FFB4E2L,
	0x72BE5D74F27B896FL, 0x80DEB1FE3B1696B1L, 0x9BDC06A725C71235L, 
	0xC19BF174CF692694L, 0xE49B69C19EF14AD2L, 0xEFBE4786384F25E3L, 
	0x0FC19DC68B8CD5B5L, 0x240CA1CC77AC9C65L, 0x2DE92C6F592B0275L, 
	0x4A7484AA6EA6E483L, 0x5CB0A9DCBD41FBD4L, 0x76F988DA831153B5L,
	0x983E5152EE66DFABL, 0xA831C66D2DB43210L, 0xB00327C898FB213FL, 
	0xBF597FC7BEEF0EE4L, 0xC6E00BF33DA88FC2L, 0xD5A79147930AA725L, 
	0x06CA6351E003826FL, 0x142929670A0E6E70L, 0x27B70A8546D22FFCL, 
	0x2E1B21385C26C926L, 0x4D2C6DFC5AC42AEDL, 0x53380D139D95B3DFL,
	0x650A73548BAF63DEL, 0x766A0ABB3C77B2A8L, 0x81C2C92E47EDAEE6L, 
	0x92722C851482353BL, 0xA2BFE8A14CF10364L, 0xA81A664BBC423001L, 
	0xC24B8B70D0F89791L, 0xC76C51A30654BE30L, 0xD192E819D6EF5218L, 
	0xD69906245565A910L, 0xF40E35855771202AL, 0x106AA07032BBD1B8L,
	0x19A4C116B8D2D0C8L, 0x1E376C085141AB53L, 0x2748774CDF8EEB99L, 
	0x34B0BCB5E19B48A8L, 0x391C0CB3C5C95A63L, 0x4ED8AA4AE3418ACBL, 
	0x5B9CCA4F7763E373L, 0x682E6FF3D6B2B8A3L, 0x748F82EE5DEFB2FCL, 
	0x78A5636F43172F60L, 0x84C87814A1F0AB72L, 0x8CC702081A6439ECL,
	0x90BEFFFA23631E28L, 0xA4506CEBDE82BDE9L, 0xBEF9A3F7B2C67915L, 
	0xC67178F2E372532BL, 0xCA273ECEEA26619CL, 0xD186B8C721C0C207L, 
	0xEADA7DD6CDE0EB1EL, 0xF57D4F7FEE6ED178L, 0x06F067AA72176FBAL, 
	0x0A637DC5A2C898A6L, 0x113F9804BEF90DAEL, 0x1B710B35131C471BL,
	0x28DB77F523047D84L, 0x32CAAB7B40C72493L, 0x3C9EBE0A15C9BEBCL, 
	0x431D67C49C100D4CL, 0x4CC5D4BECB3E42B6L, 0x597F299CFC657E2AL, 
	0x5FCB6FAB3AD6FAECL, 0x6C44198C4A475817L
    };

    private final long COUNT_MASK = 127; // block size - 1
    private long W[] = new long[ITERATION];
    private long count = 0;

    private long AA, BB, CC, DD, EE, FF, GG, HH;

    /**
     * logical function ch(x,y,z) as defined in spec:
     * @return (x and y) xor ((complement x) and z)
     * @param x long
     * @param y long
     * @param z long
     */
    private static long lf_ch(long x, long y, long z) {
	return (x & y) ^ ((~x) & z);
    }

    /**
     * logical function maj(x,y,z) as defined in spec:
     * @return (x and y) xor (x and z) xor (y and z)
     * @param x long
     * @param y long
     * @param z long
     */
    private static long lf_maj(long x, long y, long z) {
    	return (x & y) ^ (x & z) ^ (y & z);
    }

    /**
     * logical function R(x,s) - right shift
     * @return x right shift for s times
     * @param x long
     * @param s int
     */
    private static long lf_R(long x, int s) {
	return (x >>> s);
    }

    /**
     * logical function S(x,s) - right rotation
     * @return x circular right shift for s times
     * @param x long
     * @param s int
     */
    private static long lf_S(long x, int s) {
	return (x >>> s) | (x << (64 - s));
    }

    /**
     * logical function sigma0(x) - xor of results of right rotations
     * @return S(x,28) xor S(x,34) xor S(x,39)
     * @param x long
     */
    private static long lf_sigma0(long x) {
	return lf_S(x, 28) ^ lf_S(x, 34) ^ lf_S(x, 39);
    }

    /**
     * logical function sigma1(x) - xor of results of right rotations
     * @return S(x,14) xor S(x,18) xor S(x,41)
     * @param x long
     */
    private static long lf_sigma1(long x) {
	return lf_S(x, 14) ^ lf_S(x, 18) ^ lf_S(x, 41);
    }

    /**
     * logical function delta0(x) - xor of results of right shifts/rotations
     * @return long
     * @param x long
     */
    private static long lf_delta0(long x) {
	return lf_S(x, 1) ^ lf_S(x, 8) ^ lf_R(x, 7);
    }

    /**
     * logical function delta1(x) - xor of results of right shifts/rotations
     * @return long
     * @param x long
     */
    private static long lf_delta1(long x) {
	return lf_S(x, 19) ^ lf_S(x, 61) ^ lf_R(x, 6);
    }

    /**
     * Creates a SHA5 object with state (for cloning)
     */
    SHA5(SHA5 sha) {
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
    public SHA5() {
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
	int offset; //offset of this byte inside the word

	/* compute word index within the block and bit offset within the word.
	   block size is 128 bytes with word size is 8 bytes. offset is in 
	   terms of bits */
	word = (int) (count & COUNT_MASK) >>> 3;
	offset = (int) (~count & 7) << 3;

	// clear the byte inside W[word] and then 'or' it with b's byte value
	W[word] = (W[word] & ~(0xffL << offset)) | ((b & 0xffL) << offset);
	count++;

	/* If this is the last byte of a block, compute the partial hash */
	if ((count & COUNT_MASK) == 0) { 
	    computeBlock();
	}
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
	       (count & 7) != 0) {
	    engineUpdate(b[off]);
	    off++;
	    len--;
	}
    
	/* Assemble groups of 8 bytes to be inserted in long array */
	while (len >= 8) {

	    word = (int) (count & COUNT_MASK) >> 3;
	    W[word] = ((b[off] & 0xffL) << 56) |
		((b[off+1] & 0xffL) << 48) |
		((b[off+2] & 0xffL) << 40) |
		((b[off+3] & 0xffL) << 32) |
		((b[off+4] & 0xffL) << 24) |
		((b[off+5] & 0xffL) << 16) |
		((b[off+6] & 0xffL) << 8) |
		((b[off+7] & 0xffL) );
	    count += 8;
	    if ((count & COUNT_MASK) == 0) {
		computeBlock();
	    }
	    len -= 8;
	    off += 8;
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
    void init() {
	setInitialHash(INITIAL_HASHES);
	for (int i = 0; i < ITERATION; i++)
	    W[i] = 0;
	count = 0;
    }

    void setInitialHash(long[] values) {
	AA = values[0];
	BB = values[1];
	CC = values[2];
	DD = values[3];
	EE = values[4];
	FF = values[5];
	GG = values[6];
	HH = values[7];
    }

    /**
     * Resets the buffers and hash value to start a new hash.
     */
    protected void engineReset() {
	init();
    }
    
    /**
     * Computes the final hash and returns the final value as a 
     * byte array. The object is reset to be ready for further
     * use, as specified in java.security.MessageDigest javadoc
     * specification.  
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
	    throw new DigestException("output buffer too small " + 
		"to store the digest");
	}
	performDigest(hashvalue, offset, LENGTH);
	return LENGTH;
    }
    
    void performDigest(byte[] hashvalue, int offset, int resultLength) 
	throws DigestException {
	/* The input length in bits before padding occurs */
	long inputLength = count << 3;	
	
	update(0x80);

	/* Pad with zeros until overall length is a multiple of 896 */
	while ((int)(count & COUNT_MASK) != 112) {
	    update(0);
	}

	W[14] = 0;
	W[15] = inputLength;
	count += 16;
	computeBlock();

	// Copy out the result
	switch (resultLength) {
	case 64:
	    hashvalue[offset +63] = (byte)(HH >>> 0);
	    hashvalue[offset +62] = (byte)(HH >>> 8);
	    hashvalue[offset +61] = (byte)(HH >>> 16);
	    hashvalue[offset +60] = (byte)(HH >>> 24);
	    hashvalue[offset +59] = (byte)(HH >>> 32);
	    hashvalue[offset +58] = (byte)(HH >>> 40);
	    hashvalue[offset +57] = (byte)(HH >>> 48);
	    hashvalue[offset +56] = (byte)(HH >>> 56);
	    hashvalue[offset +55] = (byte)(GG >>> 0);
	    hashvalue[offset +54] = (byte)(GG >>> 8);
	    hashvalue[offset +53] = (byte)(GG >>> 16);
	    hashvalue[offset +52] = (byte)(GG >>> 24);
	    hashvalue[offset +51] = (byte)(GG >>> 32);
	    hashvalue[offset +50] = (byte)(GG >>> 40);
	    hashvalue[offset +49] = (byte)(GG >>> 48);
	    hashvalue[offset +48] = (byte)(GG >>> 56);
	case 48: 
	    hashvalue[offset +47] = (byte)(FF >>> 0);
	    hashvalue[offset +46] = (byte)(FF >>> 8);
	    hashvalue[offset +45] = (byte)(FF >>> 16);
	    hashvalue[offset +44] = (byte)(FF >>> 24);
	    hashvalue[offset +43] = (byte)(FF >>> 32);
	    hashvalue[offset +42] = (byte)(FF >>> 40);
	    hashvalue[offset +41] = (byte)(FF >>> 48);
	    hashvalue[offset +40] = (byte)(FF >>> 56);
	    hashvalue[offset +39] = (byte)(EE >>> 0);
	    hashvalue[offset +38] = (byte)(EE >>> 8);
	    hashvalue[offset +37] = (byte)(EE >>> 16);
	    hashvalue[offset +36] = (byte)(EE >>> 24);
	    hashvalue[offset +35] = (byte)(EE >>> 32);
	    hashvalue[offset +34] = (byte)(EE >>> 40);
	    hashvalue[offset +33] = (byte)(EE >>> 48);
	    hashvalue[offset +32] = (byte)(EE >>> 56);
	    hashvalue[offset +31] = (byte)(DD >>> 0);
	    hashvalue[offset +30] = (byte)(DD >>> 8);
	    hashvalue[offset +29] = (byte)(DD >>> 16);
	    hashvalue[offset +28] = (byte)(DD >>> 24);
	    hashvalue[offset +27] = (byte)(DD >>> 32);
	    hashvalue[offset +26] = (byte)(DD >>> 40);
	    hashvalue[offset +25] = (byte)(DD >>> 48);
	    hashvalue[offset +24] = (byte)(DD >>> 56);
	    hashvalue[offset +23] = (byte)(CC >>> 0);
	    hashvalue[offset +22] = (byte)(CC >>> 8);
	    hashvalue[offset +21] = (byte)(CC >>> 16);
	    hashvalue[offset +20] = (byte)(CC >>> 24);
	    hashvalue[offset +19] = (byte)(CC >>> 32);
	    hashvalue[offset +18] = (byte)(CC >>> 40);
	    hashvalue[offset +17] = (byte)(CC >>> 48);
	    hashvalue[offset +16] = (byte)(CC >>> 56);
	    hashvalue[offset +15] = (byte)(BB >>> 0);
	    hashvalue[offset +14] = (byte)(BB >>> 8);
	    hashvalue[offset +13] = (byte)(BB >>> 16);
	    hashvalue[offset +12] = (byte)(BB >>> 24);
	    hashvalue[offset +11] = (byte)(BB >>> 32);
	    hashvalue[offset +10] = (byte)(BB >>> 40);
	    hashvalue[offset + 9] = (byte)(BB >>> 48);
	    hashvalue[offset + 8] = (byte)(BB >>> 56);
	    hashvalue[offset + 7] = (byte)(AA >>> 0);
	    hashvalue[offset + 6] = (byte)(AA >>> 8);
	    hashvalue[offset + 5] = (byte)(AA >>> 16);
	    hashvalue[offset + 4] = (byte)(AA >>> 24);
	    hashvalue[offset + 3] = (byte)(AA >>> 32);
	    hashvalue[offset + 2] = (byte)(AA >>> 40);
	    hashvalue[offset + 1] = (byte)(AA >>> 48);
	    hashvalue[offset + 0] = (byte)(AA >>> 56);
	    break;
	default:
	    throw new DigestException("Unsupported Digest Length!");
	}
	engineReset();
    }

    /**
     * Compute the hash for the current block.
     *
     * This is in the same vein as Peter Gutmann's algorithm listed in
     * the back of Applied Cryptography, Compact implementation of
     * "old" NIST Secure Hash Algorithm.
     *
     */
    private void computeBlock() {
	long T1, T2, a, b, c, d, e, f, g, h;
	
	// The first 16 longs are from the byte stream, compute the rest of
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
	SHA5 that = null;
	that = new SHA5(this);
	return that;
    }
}

