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

package java.util;

/**
 * An instance of this class is used to generate a stream of
 * pseudorandom numbers. The class uses a 48-bit seed, which is
 * modified using a linear congruential formula. (See Donald Knuth,
 * <i>The Art of Computer Programming, Volume 2</i>, Section 3.2.1.)
 * <p>
 * If two instances of <code>Random</code> are created with the same
 * seed, and the same sequence of method calls is made for each, they
 * will generate and return identical sequences of numbers. In order to
 * guarantee this property, particular algorithms are specified for the
 * class <tt>Random</tt>. Java implementations must use all the algorithms
 * shown here for the class <tt>Random</tt>, for the sake of absolute
 * portability of Java code. However, subclasses of class <tt>Random</tt>
 * are permitted to use other algorithms, so long as they adhere to the
 * general contracts for all the methods.
 * <p>
 * The algorithms implemented by class <tt>Random</tt> use a
 * <tt>protected</tt> utility method that on each invocation can supply
 * up to 32 pseudorandomly generated bits.
 * <p>
 *
 * @version 1.33, 12/04/99 (CLDC 1.0, Spring 2000)
 * @since   JDK1.0
 */
public
class Random {

    /**
     * The internal state associated with this pseudorandom number generator.
     * (The specs for the methods in this class describe the ongoing
     * computation of this value.)
     */
    private long seed;

    private final static long multiplier = 0x5DEECE66DL;
    private final static long addend = 0xBL;
    private final static long mask = (1L << 48) - 1;

    /**
     * Creates a new random number generator. Its seed is initialized to
     * a value based on the current time:
     * <blockquote><pre>
     * public Random() { this(System.currentTimeMillis()); }</pre></blockquote>
     *
     * @see     java.lang.System#currentTimeMillis()
     */
    public Random() { this(System.currentTimeMillis()); }

    /**
     * Creates a new random number generator using a single
     * <code>long</code> seed:
     * <blockquote><pre>
     * public Random(long seed) { setSeed(seed); }</pre></blockquote>
     * Used by method <tt>next</tt> to hold
     * the state of the pseudorandom number generator.
     *
     * @param   seed   the initial seed.
     * @see     java.util.Random#setSeed(long)
     */
    public Random(long seed) {
        setSeed(seed);
    }

    /**
     * Sets the seed of this random number generator using a single
     * <code>long</code> seed. The general contract of <tt>setSeed</tt>
     * is that it alters the state of this random number generator
     * object so as to be in exactly the same state as if it had just
     * been created with the argument <tt>seed</tt> as a seed. The method
     * <tt>setSeed</tt> is implemented by class Random as follows:
     * <blockquote><pre>
     * synchronized public void setSeed(long seed) {
     *       this.seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1);
     * }</pre></blockquote>
     * The implementation of <tt>setSeed</tt> by class <tt>Random</tt>
     * happens to use only 48 bits of the given seed. In general, however,
     * an overriding method may use all 64 bits of the long argument
     * as a seed value.
     *
     * @param   seed   the initial seed.
     */
    synchronized public void setSeed(long seed) {
        this.seed = (seed ^ multiplier) & mask;
    }

    /**
     * Generates the next pseudorandom number. Subclass should
     * override this, as this is used by all other methods.<p>
     * The general contract of <tt>next</tt> is that it returns an
     * <tt>int</tt> value and if the argument bits is between <tt>1</tt>
     * and <tt>32</tt> (inclusive), then that many low-order bits of the
     * returned value will be (approximately) independently chosen bit
     * values, each of which is (approximately) equally likely to be
     * <tt>0</tt> or <tt>1</tt>. The method <tt>next</tt> is implemented
     * by class <tt>Random</tt> as follows:
     * <blockquote><pre>
     * synchronized protected int next(int bits) {
     *       seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
     *       return (int)(seed >>> (48 - bits));
     * }</pre></blockquote>
     * This is a linear congruential pseudorandom number generator, as
     * defined by D. H. Lehmer and described by Donald E. Knuth in <i>The
     * Art of Computer Programming,</i> Volume 2: <i>Seminumerical
     * Algorithms</i>, section 3.2.1.
     *
     * @param   bits random bits
     * @return  the next pseudorandom value from this random number generator's sequence.
     * @since   JDK1.1
     */
    synchronized protected int next(int bits) {
        long nextseed = (seed * multiplier + addend) & mask;
        seed = nextseed;
        return (int)(nextseed >>> (48 - bits));
    }

    private static final int BITS_PER_BYTE = 8;
    private static final int BYTES_PER_INT = 4;

    /**
     * Returns the next pseudorandom, uniformly distributed <code>int</code>
     * value from this random number generator's sequence. The general
     * contract of <tt>nextInt</tt> is that one <tt>int</tt> value is
     * pseudorandomly generated and returned. All 2<font size="-1"><sup>32
     * </sup></font> possible <tt>int</tt> values are produced with
     * (approximately) equal probability. The method <tt>nextInt</tt> is
     * implemented by class <tt>Random</tt> as follows:
     * <blockquote><pre>
     * public int nextInt() {  return next(32); }</pre></blockquote>
     *
     * @return  the next pseudorandom, uniformly distributed <code>int</code>
     *          value from this random number generator's sequence.
     */
    public int nextInt() {  return next(32); }

    /**
     * Returns the next pseudorandom, uniformly distributed <code>long</code>
     * value from this random number generator's sequence. The general
     * contract of <tt>nextLong</tt> is that one long value is pseudorandomly
     * generated and returned. All 2<font size="-1"><sup>64</sup></font>
     * possible <tt>long</tt> values are produced with (approximately) equal
     * probability. The method <tt>nextLong</tt> is implemented by class
     * <tt>Random</tt> as follows:
     * <blockquote><pre>
     * public long nextLong() {
     *       return ((long)next(32) << 32) + next(32);
     * }</pre></blockquote>
     *
     * @return  the next pseudorandom, uniformly distributed <code>long</code>
     *          value from this random number generator's sequence.
     */
    public long nextLong() {
        // it's okay that the bottom word remains signed.
        return ((long)(next(32)) << 32) + next(32);
    }

}
