/*
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

package java.nio;

import com.sun.jsr239.BufferManager;

/** 
 * A byte buffer.
 *
 * <p> This class is provided as part of the JSR 239 NIO Buffer
 * building block. It is a subset of the
 * <code>java.nio.ByteBuffer</code> class in Java(TM) Standard Edition
 * version 1.4.2.  Differences are noted in <b><i>bold italic</i></b>.
 * The class documentation may make reference to classes that are not
 * present in the building block.
 *
 * <p><b><i> I/O channels, marking and resetting, and read-only buffers
 * are not supported.  Allocation of non-direct byte buffers,
 * compaction, and duplication are not supported.  The
 * <code>char</code>, <code>long</code>, and <code>double</code>
 * datatypes are not supported. The following methods are omitted:
 *
 * <ul>
 * <li><code>ByteBuffer allocate(int capacity)</code></li>
 * <li><code>ByteBuffer compact()</code></li>
 * <li><code>ByteBuffer duplicate()</code></li>
 * <li><code>Buffer mark()</code></li>
 * <li><code>Buffer reset()</code></li>
 * <li><code>boolean isReadOnly()</code></li>
 * <li><code>ByteBuffer asReadOnlyBuffer()</code></li>
 * <li><code>CharBuffer asCharBuffer()</code></li>
 * <li><code>LongBuffer asLongBuffer()</code></li>
 * <li><code>DoubleBuffer asDoubleBuffer()</code></li>
 * <li><code>char getChar()</code></li>
 * <li><code>char getChar(int index)</code></li>
 * <li><code>long getLong()</code></li>
 * <li><code>long getLong(int index)</code></li>
 * <li><code>double getDouble()</code></li>
 * <li><code>double getDouble(int index)</code></li>
 * <li><code>ByteBuffer putChar(char value)</code></li>
 * <li><code>ByteBuffer putChar(int index, char value)</code></li>
 * <li><code>ByteBuffer putLong(int index, long value)</code></li>
 * <li><code>ByteBuffer putLong(long value)</code></li>
 * <li><code>ByteBuffer putDouble(double value)</code></li>
 * <li><code>ByteBuffer putDouble(int index, double value)</code></li>
 * </ul>
 * </i></b>
 *
 * <p> This class defines six categories of operations upon
 * byte buffers:
 *
 * <ul>
 *
 *   <li><p> Absolute and relative <A
 *   HREF="ByteBuffer.html#get()"><CODE></code><i>get</i><code></CODE></A>
 *   and <A
 *   HREF="ByteBuffer.html#put(byte)"><CODE></code><i>put</i><code></CODE></A>
 *   methods that read and write single bytes; </p></li>
 *
 *   <li><p> Relative <A
 *   HREF="ByteBuffer.html#get(byte[])"><CODE></code><i>bulk
 *   get</i><code></CODE></A> methods that transfer contiguous
 *   sequences of bytes from this buffer into an array; </p></li>
 *
 *   <li><p> Relative <A
 *   HREF="ByteBuffer.html#put(byte[])"><CODE></code><i>bulk
 *   put</i><code></CODE></A> methods that transfer contiguous
 *   sequences of bytes from a byte array or some other byte buffer
 *   into this buffer; </p></li>
 *
 *   <li><p> Absolute and relative <i>get</i> and <i>put</i> methods
 *   that read and write values of other primitive types, translating
 *   them to and from sequences of bytes in a particular byte order;
 *   <b><i>JSR 239 does not support certain multi-byte </i>get<i> and
 *   </i>put<i> methods.</i></b> </p></li>
 *
 *   <li><p> Methods for creating <i><a href="#views">view
 *   buffers</a></i>, which allow a byte buffer to be viewed as a
 *   buffer containing values of some other primitive type; and
 *   </p></li>
 *
 *   <li><p> Methods for compacting, duplicating, and <A
 *   HREF="ByteBuffer.html#slice()"><CODE></code>slicing<code></CODE></A>
 *   a byte buffer. <b><i>JSR 239 does not support compacting and
 *   duplicating buffers.</i></b> </p></li>
 *
 * </ul>
 *
 * <p> Byte buffers can be created either by <A
 * HREF="ByteBuffer.html#allocateDirect(int)"><CODE></code><i>allocation</i><code></CODE></A>,
 * which allocates space for the buffer's content, or by <A
 * HREF="ByteBuffer.html#wrap(byte[])"><CODE></code><i>wrapping</i><code></CODE></A>
 * an existing byte array into a buffer.
 *
 * <a name="direct">
 * <h4> Direct <i>vs.</i> non-direct buffers </h4>
 *
 * <p> A byte buffer is either <i>direct</i> or <i>non-direct</i>.
 * Given a direct byte buffer, the Java virtual machine will make a
 * best effort to perform native I/O operations directly upon it.
 * That is, it will attempt to avoid copying the buffer's content to
 * (or from) an intermediate buffer before (or after) each invocation
 * of one of the underlying operating system's native I/O operations.
 *
 * <p> A direct byte buffer may be created by invoking the <A
 * HREF="ByteBuffer.html#allocateDirect(int)"><CODE>allocateDirect</CODE></A>
 * factory method of this class.  The buffers returned by this method
 * typically have somewhat higher allocation and deallocation costs
 * than non-direct buffers.  The contents of direct buffers may reside
 * outside of the normal garbage-collected heap, and so their impact
 * upon the memory footprint of an application might not be obvious.
 * It is therefore recommended that direct buffers be allocated
 * primarily for large, long-lived buffers that are subject to the
 * underlying system's native I/O operations.  In general it is best
 * to allocate direct buffers only when they yield a measureable gain
 * in program performance. <b><i>Certain JSR 239 methods require the
 * use of direct buffers.</i></b>
 *
 * <b><i>JSR 239 does not support the techniques described in the
 * remainder of this paragraph.</i></b>
 * <p> A direct byte buffer may also be created by mapping a region of
 * a file directly into memory.  An implementation of the Java
 * platform may optionally support the creation of direct byte buffers
 * from native code via JNI.  If an instance of one of these kinds of
 * buffers refers to an inaccessible region of memory then an attempt
 * to access that region will not change the buffer's content and will
 * cause an unspecified exception to be thrown either at the time of
 * the access or at some later time.
 *
 * <p> Whether a byte buffer is direct or non-direct may be determined
 * by invoking its <A
 * HREF="ByteBuffer.html#isDirect()"><CODE>isDirect</CODE></A>
 * method.  This method is provided so that explicit buffer management
 * can be done in performance-critical code.
 *
 * <a name="bin">
 * <h4> Access to binary data </h4>
 *
 * <p> This class defines methods for reading and writing values of
 * all other primitive types, except <tt>boolean</tt>,
 * <b><i><code>char</code>, <code>long</code>, and
 * <code>double</code></i></b>.  Primitive values are translated to
 * (or from) sequences of bytes according to the buffer's current byte
 * order, which may be retrieved and modified via the
 * <CODE>order</CODE> methods.  Specific byte orders are represented
 * by instances of the <CODE>ByteOrder</CODE> class.  The initial
 * order of a byte buffer is always <CODE>BIG_ENDIAN</CODE>. <b><i>JSR
 * 239 does not support the <code>ByteOrder</code> class or the
 * <code>order</code> methods.  The inital order of a byte buffer is
 * the platform byte order.</i></b>
 *
 * <p> For access to heterogenous binary data, that is, sequences of
 * values of different types, this class defines a family of absolute
 * and relative <i>get</i> and <i>put</i> methods for each type.  For
 * 32-bit floating-point values, for example, this class defines:
 *
 * <blockquote><pre>
 *
 * float  <A HREF="ByteBuffer.html#getFloat()"><CODE>getFloat()</CODE></A>
 * float  <A HREF="ByteBuffer.html#getFloat(int)"><CODE>getFloat(int index)</CODE></A>
 *  void  <A HREF="ByteBuffer.html#putFloat(float)"><CODE>putFloat(float f)</CODE></A>
 *  void  <A HREF="ByteBuffer.html#putFloat(int, float)"><CODE>putFloat(int index, float f)</CODE></A></pre></blockquote>
 *
 * <p> Corresponding methods are defined for the types <tt>char</tt>,
 * <tt>short</tt>, <tt>int</tt>, <tt>long</tt>, and <tt>double</tt>.
 * <b><i>JSR 239 does not define the <code>char</code>,
 * <code>long</code>, or <code>double</code> methods.</b></i> The
 * index parameters of the absolute <i>get</i> and <i>put</i> methods
 * are in terms of bytes rather than of the type being read or
 * written.
 *
 * <a name="views">
 *
 * <p> For access to homogeneous binary data, that is, sequences of
 * values of the same type, this class defines methods that can create
 * <i>views</i> of a given byte buffer.  A <i>view buffer</i> is
 * simply another buffer whose content is backed by the byte buffer.
 * Changes to the byte buffer's content will be visible in the view
 * buffer, and vice versa; the two buffers' position, limit, and mark
 * values are independent.  The <A
 * HREF="ByteBuffer.html#asFloatBuffer()"><CODE>asFloatBuffer</CODE></A>
 * method, for example, creates an instance of the <A
 * HREF="FloatBuffer.html" title="class in
 * java.nio"><CODE>FloatBuffer</CODE></A> class that is backed by the
 * byte buffer upon which the method is invoked.  Corresponding
 * view-creation methods are defined for the types <tt>char</tt>,
 * <tt>short</tt>, <tt>int</tt>, <tt>long</tt>, and
 * <tt>double</tt>. <b><i>JSR 239 does not define views of type
 * <code>char</code>, <code>long</code>, or
 * <code>double</code>.</i></b>
 *
 * <p> View buffers have three important advantages over the families of
 * type-specific <i>get</i> and <i>put</i> methods described above:
 *
 * <ul>
 *
 *   <li><p> A view buffer is indexed not in terms of bytes but rather
 *   in terms of the type-specific size of its values; </p></li>
 *
 *   <li><p> A view buffer provides relative bulk <i>get</i> and
 *   <i>put</i> methods that can transfer contiguous sequences of
 *   values between a buffer and an array or some other buffer of the
 *   same type; and </p></li>
 *
 *   <li><p> A view buffer is potentially much more efficient because
 *   it will be direct if, and only if, its backing byte buffer is
 *   direct.  </p></li>
 *
 * </ul>
 *
 * <p> The byte order of a view buffer is fixed to be that of its byte
 * buffer at the time that the view is created.  </p>
 *
 * <h4> Invocation chaining </h4>
 *
 * <p> Methods in this class that do not otherwise have a value to
 * return are specified to return the buffer upon which they are
 * invoked.  This allows method invocations to be chained.
 *
 * <p> The sequence of statements
 *
 * <blockquote><pre>
 * bb.putInt(0xCAFEBABE);
 * bb.putShort(3);
 * bb.putShort(45);
 * </pre></blockquote>
 * can, for example, be replaced by the single statement
 *
 * <blockquote><pre>
 * bb.putInt(0xCAFEBABE).putShort(3).putShort(45);
 * </pre></blockquote>
 */
public abstract class ByteBuffer extends Buffer implements Comparable {

    byte[] array;
    int arrayOffset;

    boolean isDirect;

    boolean disposed = false;

    /**
     * Buffers created by <code>allocateDirect</code> have an
     * accosiated buffer allocated in the native Heap. A user may
     * create a slice <code>Buffer</code> from a direct "parent"
     * <code>Buffer</code> (<code>Buffer.slice()</code>). The "slice" and
     * "parent" <code>Buffer</code> share the same native buffer. Native
     * buffer is released when the parent <code>Buffer</code> is collected.
     * <code>directParent</code> is a reference from slice object to the
     * parent. It guarantees that parent object is collected (and shared 
     * buffer is released) only after all its slice objects are collected.
     */
    Buffer directParent;

    /**
     * Constructs a new <code>ByteBuffer</code>.
     */
    ByteBuffer() {}

    /**
     * Allocates a new direct byte buffer.
     *
     * <p> The new buffer's position will be zero, its limit will be
     * its capacity, and its mark will be undefined. Whether or not it
     * has a backing array is unspecified. <b><i>For JSR 239, the mark
     * is undefined, and no backing array will be present.</i></b>.
     *
     * @param capacity The new buffer's capacity, in bytes.
     *
     * @return The new byte buffer.
     *
     * @throws IllegalArgumentException If the <code>capacity</code> is
     * a negative integer.
     */
    public static ByteBuffer allocateDirect(int capacity) {
        if (capacity < 0) {
            throw new IllegalArgumentException();
        }
	    int nativeAddress = ByteBufferImpl._allocNative(capacity);

        ByteBuffer buf = new ByteBufferImpl(capacity, null, nativeAddress,
            null /*directParent is null if direct buffer is a parent itself*/);

        // Record the address of this buffer along with a weak
        // reference; if the weak reference becomes null,
        // we will free the native heap memory.
        BufferManager.newBuffer(buf, nativeAddress);

        return buf;
    }

    /**
     * Wraps a byte array into a buffer.
     *
     * <p> The new buffer will be backed by the the given byte array;
     * that is, modifications to the buffer will cause the array to be
     * modified and vice versa.  The new buffer's capacity will be
     * <tt>array.length</tt>, its position will be <tt>offset</tt>,
     * its limit will be <tt>offset + length</tt>, and its mark will
     * be undefined.  Its <A
     * HREF="ByteBuffer.html#array()"><CODE></code>backing
     * array<code></CODE></A> will be the given array, and its <A
     * HREF="ByteBuffer.html#arrayOffset()"><CODE></code>array
     * offset<code></CODE></A> will be zero.  </p>
     * 
     * @param array The array that will back the new buffer
     * @param offset The offset of the subarray to be used; must be
     * non-negative and no larger than <tt>array.length</tt>.  The new
     * buffer's position will be set to this value.
     * @param length The length of the subarray to be used; must be
     * non-negative and no larger than <tt>array.length - offset</tt>.
     * The new buffer's limit will be set to <tt>offset + length</tt>.
     *
     * @return The new byte buffer.
     *
     * @throws IndexOutOfBoundsException If the preconditions on the
     * <tt>offset</tt> and <tt>length</tt> parameters do not hold.
     */
    public static ByteBuffer wrap(byte[] array, int offset, int length) {
        if (offset < 0 || offset > array.length ||
            length < 0 || length > array.length - offset) {
	    throw new IndexOutOfBoundsException();
        }

        ByteBufferImpl bbi = new ByteBufferImpl(array.length, array, 0,
            null /*directParent is null for all nondirect buffers*/);
        bbi.position(offset);
        bbi.limit(offset + length);
        return bbi;
    }

    /**
     * Wraps a byte array into a buffer.
     *
     * <p> The new buffer will be backed by the the given byte array;
     * that is, modifications to the buffer will cause the array to be
     * modified and vice versa.  The new buffer's capacity and limit
     * will be <tt>array.length</tt>, its position will be zero, and
     * its mark will be undefined.  Its <A
     * HREF="ByteBuffer.html#array()"><CODE></code>backing
     * array<code></CODE></A> will be the given array, and its <A
     * HREF="ByteBuffer.html#arrayOffset()"><CODE></code>array
     * offset<code></CODE></A> will be zero.  </p>
     *
     * @param array The array that will back this buffer.
     *
     * @return The new byte buffer.
     */
    public static ByteBuffer wrap(byte[] array) {
	return wrap(array, 0, array.length);
    }

    /**
     * Creates a new byte buffer whose content is a shared
     * subsequence of this buffer's content.
     *
     * <p> The content of the new buffer will start at this buffer's
     * current position.  Changes to this buffer's content will be
     * visible in the new buffer, and vice versa; the two buffers'
     * position, limit, and mark values will be independent. <b><i>JSR
     * 239 does not support the mark.</i></b>
     *
     * <p> The new buffer's position will be zero, its capacity and
     * its limit will be the number of bytes remaining in this
     * buffer, and its mark will be undefined.  The new buffer will be
     * direct if, and only if, this buffer is direct, and it will be
     * read-only if, and only if, this buffer is read-only. <b><i>JSR
     * 239 does not support the mark or read-only buffers.</i></b>
     * </p>
     *
     * @return The new byte buffer.
     */
    public abstract ByteBuffer slice();

    /**
     * Relative <i>get</i> method.  Reads the byte at this
     * buffer's current position, and then increments the
     * position. </p>
     *
     * @return The byte at the buffer's current position.
     *
     * @throws BufferUnderflowException If the buffer's current
     * position is not smaller than its limit.
     */
    public abstract byte get();

    /**
     * Relative <i>put</i> method&nbsp;&nbsp;<i>(optional
     * operation)</i>.
     * 
     * <p> Writes the given byte into this buffer at the current
     * position, and then increments the position. </p>
     *
     * @param b The byte to be written.
     *
     * @return This buffer.
     *
     * @throws BufferOverflowException If this buffer's current
     * position is not smaller than its limit.
     *
     * @throws ReadOnlyBufferException If this buffer is
     * read-only. <b><i>JSR 239 does not support read-only buffer or
     * the <code>ReadOnlyBufferException</code> class.</i></b>
     */
    public abstract ByteBuffer put(byte b);

    /**
     * Absolute <i>get</i> method.  Reads the byte at the given
     * index. </p>
     *
     * @param  index The index from which the byte will be read.
     *
     * @return  The byte at the given index.
     *
     * @throws IndexOutOfBoundsException If <tt>index</tt> is negative
     * or not smaller than the buffer's limit.
     */
    public abstract byte get(int index);

    /**
     * Absolute <i>put</i> method&nbsp;&nbsp;<i>(optional operation)</i>.
     * 
     * <p> Writes the given byte into this buffer at the given
     * index. </p>
     *
     * @param index The index at which the byte will be written.
     *
     * @param b The byte value to be written.
     *
     * @return This buffer.
     *
     * @throws IndexOutOfBoundsException If <tt>index</tt> is negative
     * or not smaller than the buffer's limit.
     *
     * @throws ReadOnlyBufferException If this buffer is
     * read-only. <b><i>JSR 239 does not support read-only buffer or
     * the <code>ReadOnlyBufferException</code> class.</i></b>
     */
    public abstract ByteBuffer put(int index, byte b);
    
    /**
     * Relative bulk <i>get</i> method.
     *
     * <p> This method transfers bytes from this buffer into the
     * given destination array.  If there are fewer bytes
     * remaining in the buffer than are required to satisfy the
     * request, that is, if
     * <tt>length</tt>&nbsp;<tt>&gt;</tt>&nbsp;<tt>remaining()</tt>,
     * then no bytes are transferred and a {@link
     * BufferUnderflowException} is thrown.
     *
     * <p> Otherwise, this method copies <tt>length</tt> bytes
     * from this buffer into the given array, starting at the current
     * position of this buffer and at the given offset in the array.
     * The position of this buffer is then incremented by
     * <tt>length</tt>.
     *
     * <p> In other words, an invocation of this method of the form
     * <tt>src.get(dst,&nbsp;off,&nbsp;len)</tt> has exactly the same
     * effect as the loop
     *
     * <pre>
     *     for (int i = off; i < off + len; i++)
     *         dst[i] = src.get(); </pre>
     *
     * except that it first checks that there are sufficient
     * bytes in this buffer and it is potentially much more
     * efficient. </p>
     *
     * @param dst The array into which bytes are to be written.
     *
     * @param offset The offset within the array of the first
     * byte to be written; must be non-negative and no larger
     * than <tt>dst.length</tt>.
     *
     * @param length The maximum number of bytes to be written
     * to the given array; must be non-negative and no larger than
     * <tt>dst.length - offset</tt>.
     *
     * @return This buffer.
     *
     * @throws BufferUnderflowException If there are fewer than
     * <tt>length</tt> bytes remaining in this buffer.
     *
     * @throws IndexOutOfBoundsException If the preconditions on the
     * <tt>offset</tt> and <tt>length</tt> parameters do not hold.
     */
    public ByteBuffer get(byte[] dst, int offset, int length) {
        if (offset < 0 || offset > dst.length ||
            length < 0 || length > dst.length - offset) {
            throw new IndexOutOfBoundsException();
        }
        if (limit - position < length) {
            throw new BufferUnderflowException();
        }
	if (isDirect) {
	    ByteBufferImpl._getBytes(arrayOffset + position,
                                     dst, offset, length);
	} else {
	    System.arraycopy(array, arrayOffset + position,
			     dst, offset, length);
	}
	position += length;
	return this;
    }

    /**
     * Relative bulk <i>get</i> method.
     *
     * <p> This method transfers bytes from this buffer into the
     * given destination array.  An invocation of this method of the
     * form <tt>src.get(a)</tt> behaves in exactly the same way as the
     * invocation
     *
     * <pre>
     *     src.get(a, 0, a.length) </pre>
     *
     * @return This buffer.
     *
     * @throws BufferUnderflowException If there are fewer than
     * <tt>dst.length</tt> bytes remaining in this buffer.
     */
    public ByteBuffer get(byte[] dst) {
	return get(dst, 0, dst.length);
    }

    /**
     * Relative bulk <i>put</i> method&nbsp;&nbsp;<i>(optional
     * operation)</i>.
     *
     * <p> This method transfers the bytes remaining in the
     * given source buffer into this buffer.  If there are more
     * bytes remaining in the source buffer than in this buffer,
     * that is, if
     * <tt>src.remaining()</tt>&nbsp;<tt>&gt;</tt>&nbsp;<tt>remaining()</tt>,
     * then no bytes are transferred and a {@link
     * BufferOverflowException} is thrown.
     *
     * <p> Otherwise, this method copies
     * <i>n</i>&nbsp;=&nbsp;<tt>src.remaining()</tt> bytes from
     * the given buffer into this buffer, starting at each buffer's
     * current position.  The positions of both buffers are then
     * incremented by <i>n</i>.
     *
     * <p> In other words, an invocation of this method of the form
     * <tt>dst.put(src)</tt> has exactly the same effect as the loop
     *
     * <pre>
     *     while (src.hasRemaining())
     *         dst.put(src.get()); </pre>
     *
     * except that it first checks that there is sufficient space in
     * this buffer and it is potentially much more efficient. </p>
     *
     * @param src The source buffer from which bytes are to be
     * read; must not be this buffer.
     *
     * @return This buffer.
     *
     * @throws BufferOverflowException If there is insufficient space
     * in this buffer for the remaining bytes in the source
     * buffer.
     *
     * @throws IllegalArgumentException If the source buffer is this buffer.
     *
     * @throws ReadOnlyBufferException If this buffer is
     * read-only. <b><i>JSR 239 does not support read-only buffer or
     * the <code>ReadOnlyBufferException</code> class.</i></b>
     */
    public ByteBuffer put(ByteBuffer src) {
        if (src == this) {
            throw new IllegalArgumentException();
        }

	ByteBufferImpl srci = (ByteBufferImpl)src;

	int length = srci.limit - srci.position;
        if (length > this.limit - this.position) {
            throw new BufferOverflowException();
        }
        if (isDirect && srci.isDirect) {
            ByteBufferImpl._copyBytes(srci.arrayOffset + srci.position,
                                      this.arrayOffset + this.position,
                                      length);
        } else if (isDirect && !srci.isDirect) {
            ByteBufferImpl._putBytes(this.arrayOffset + this.position,
                                     srci.array,
                                     srci.arrayOffset + srci.position,
                                     length);
        } else if (!isDirect && srci.isDirect) {
            ByteBufferImpl._getBytes(srci.arrayOffset + srci.position,
                                     this.array,
                                     this.arrayOffset + this.position,
                                     length);
        } else if (!isDirect && !srci.isDirect) {
            System.arraycopy(srci.array, srci.arrayOffset + srci.position,
                             this.array, this.arrayOffset + this.position,
                             length);
        }
	
	srci.position += length;
	this.position += length;
	return this;
    }

    /**
     * Relative bulk <i>put</i> method&nbsp;&nbsp;<i>(optional
     * operation)</i>.
     *
     * <p> This method transfers bytes into this buffer from the
     * given source array.  If there are more bytes to be copied
     * from the array than remain in this buffer, that is, if
     * <tt>length</tt>&nbsp;<tt>&gt;</tt>&nbsp;<tt>remaining()</tt>,
     * then no bytes are transferred and a {@link
     * BufferOverflowException} is thrown.
     *
     * <p> Otherwise, this method copies <tt>length</tt> bytes
     * from the given array into this buffer, starting at the given
     * offset in the array and at the current position of this buffer.
     * The position of this buffer is then incremented by
     * <tt>length</tt>.
     *
     * <p> In other words, an invocation of this method of the form
     * <tt>dst.put(src,&nbsp;off,&nbsp;len)</tt> has exactly the same
     * effect as the loop
     *
     * <pre>
     *     for (int i = off; i < off + len; i++)
     *         dst.put(a[i]); </pre>
     *
     * except that it first checks that there is sufficient space in
     * this buffer and it is potentially much more efficient. </p>
     *
     * @param src The array from which bytes are to be read.
     *
     * @param offset The offset within the array of the first
     * byte to be read; must be non-negative and no larger than
     * <tt>array.length</tt>.
     *
     * @param length The number of bytes to be read from the
     * given array; must be non-negative and no larger than
     * <tt>array.length - offset</tt>.
     *
     * @return This buffer.
     *
     * @throws BufferOverflowException If there is insufficient space
     * in this buffer.
     *
     * @throws IndexOutOfBoundsException If the preconditions on the
     * <tt>offset</tt> and <tt>length</tt> parameters do not hold.
     *
     * @throws ReadOnlyBufferException If this buffer is
     * read-only. <b><i>JSR 239 does not support read-only buffer or
     * the <code>ReadOnlyBufferException</code> class.</i></b>
     */
    public ByteBuffer put(byte[] src, int offset, int length) {
        // need revisit -- overlapping backing store?
        if (offset < 0 || offset > src.length ||
            length < 0 || length > src.length - offset) {
            throw new IndexOutOfBoundsException();
        }
        if (length > limit - position) {
            throw new BufferOverflowException();
        }
	if (isDirect) {
	    ByteBufferImpl._putBytes(arrayOffset + position,
                                     src, offset, length);
	} else {
	    System.arraycopy(src, offset,
			     array, arrayOffset + position, length);
	}
	position += length;
	return this;
    }

    /**
     * Relative bulk <i>put</i> method&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> This method transfers the entire content of the given
     * source byte array into this buffer.  An invocation of
     * this method of the form <tt>dst.put(a)</tt> behaves in exactly
     * the same way as the invocation
     *
     * <pre>
     *     dst.put(a, 0, a.length) </pre>
     *
     * @return This buffer.
     *
     * @throws BufferOverflowException If there is insufficient space
     * in this buffer.
     *
     * @throws ReadOnlyBufferException If this buffer is
     * read-only. <b><i>JSR 239 does not support read-only buffer or
     * the <code>ReadOnlyBufferException</code> class.</i></b>
     */
    public final ByteBuffer put(byte[] src) {
	return put(src, 0, src.length);
    }

    /**
     * Tells whether or not this buffer is backed by an accessible
     * byte array.
     *
     * <p> If this method returns <tt>true</tt> then the {@link
     * #array() array} and {@link #arrayOffset() arrayOffset} methods
     * may safely be invoked.  </p>
     *
     * @return <tt>true</tt> if, and only if, this buffer is backed by
     * an array and is not read-only. <b><i>JSR 239 does not support
     * read-only buffers.</i></b>
     */
    public final boolean hasArray() {
        return !isDirect;
    }

    /**
     * Returns the byte array that backs this
     * buffer&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> Modifications to this buffer's content will cause the returned
     * array's content to be modified, and vice versa.
     *
     * <p> Invoke the {@link #hasArray hasArray} method before
     * invoking this method in order to ensure that this buffer has an
     * accessible backing array.  </p>
     *
     * @return The array that backs this buffer.
     *
     * @throws ReadOnlyBufferException If this buffer is
     * read-only. <b><i>JSR 239 does not support read-only buffer or
     * the <code>ReadOnlyBufferException</code> class.</i></b>
     *
     * @throws UnsupportedOperationException If this buffer is not
     * backed by an accessible array.
     */
    public final byte[] array() {
	if (isDirect) {
	    throw new UnsupportedOperationException();
	}
	return array;
    }

    /**
     * Returns the offset within this buffer's backing array of the
     * first element of the buffer&nbsp;&nbsp;<i>(optional
     * operation)</i>.
     *
     * <p> If this buffer is backed by an array then buffer position
     * <i>p</i> corresponds to array index
     * <i>p</i>&nbsp;+&nbsp;<tt>arrayOffset()</tt>.
     *
     * <p> Invoke the {@link #hasArray hasArray} method before
     * invoking this method in order to ensure that this buffer has an
     * accessible backing array.  </p>
     *
     * @return The offset within this buffer's array of the first
     * element of the buffer.
     *
     * @throws ReadOnlyBufferException If this buffer is
     * read-only. <b><i>JSR 239 does not support read-only buffer or
     * the <code>ReadOnlyBufferException</code> class.</i></b>
     *
     * @throws UnsupportedOperationException If this buffer is not
     * backed by an accessible array.
     */
    public final int arrayOffset() {
	if (isDirect) {
	    throw new UnsupportedOperationException();
	}
	return arrayOffset;
    }

    /**
     * Tells whether or not this byte buffer is direct. </p>
     *
     * @return  <tt>true</tt> if, and only if, this buffer is direct.
     */
    public abstract boolean isDirect();

    /**
     * Returns a string summarizing the state of this buffer.
     *
     * @return A summary string
     */
    public String toString() {
        return "java.nio.ByteBuffer[" +
            "pos=" + position() +
            "lim=" + limit() +
            "cap=" + capacity() +
            "]";
    }

    /**
     * Returns the current hash code of this buffer.
     *
     * <p> The hash code of a byte buffer depends only upon its remaining
     * elements; that is, upon the elements from <tt>position()</tt> up to, and
     * including, the element at <tt>limit()</tt>&nbsp;-&nbsp;<tt>1</tt>.
     *
     * <p> Because buffer hash codes are content-dependent, it is inadvisable
     * to use buffers as keys in hash maps or similar data structures unless it
     * is known that their contents will not change.  </p>
     *
     * @return  The current hash code of this buffer.
     */
    public int hashCode() {
	int h = 1;
	int p = position();
	for (int i = limit() - 1; i >= p; i--)
	    h = 31 * h + (int)get(i);
	return h;
    }

    /**
     * Tells whether or not this buffer is equal to another object.
     *
     * <p> Two byte buffers are equal if, and only if,
     *
     * <p><ol>
     *
     *   <li><p> They have the same element type,  </p></li>
     *
     *   <li><p> They have the same number of remaining elements, and
     *   </p></li>
     *
     *   <li><p> The two sequences of remaining elements, considered
     *   independently of their starting positions, are pointwise equal.
     *   </p></li>
     *
     * </ol>
     *
     * <p> A byte buffer is not equal to any other type of object.  </p>
     *
     * @param  ob  The object to which this buffer is to be compared.
     *
     * @return  <tt>true</tt> if, and only if, this buffer is equal to the
     *           given object.
     */
    public boolean equals(Object ob) {
	if (!(ob instanceof ByteBuffer))
	    return false;
	ByteBuffer that = (ByteBuffer)ob;
	if (this.remaining() != that.remaining())
	    return false;
	int p = this.position();
	for (int i = this.limit() - 1, j = that.limit() - 1; i >= p; i--, j--) {
	    byte v1 = this.get(i);
	    byte v2 = that.get(j);
	    if (v1 != v2) {
		if ((v1 != v1) && (v2 != v2))	// For float and double
		    continue;
		return false;
	    }
	}
	return true;
    }

    /**
     * Compares this buffer to another.
     *
     * <p> Two byte buffers are compared by comparing their sequences of
     * remaining elements lexicographically, without regard to the starting
     * position of each sequence within its corresponding buffer.
     *
     * <p> A byte buffer is not comparable to any other type of object.
     *
     * @return  A negative integer, zero, or a positive integer as this buffer
     *		is less than, equal to, or greater than the given buffer.
     * @throws  ClassCastException If the argument is not a byte buffer.
     */
    public int compareTo(Object ob) {
	ByteBuffer that = (ByteBuffer)ob;
	int n = this.position() + Math.min(this.remaining(), that.remaining());
	for (int i = this.position(), j = that.position(); i < n; i++, j++) {
	    byte v1 = this.get(i);
	    byte v2 = that.get(j);
	    if (v1 == v2)
		continue;
	    if ((v1 != v1) && (v2 != v2)) 	// For float and double
		continue;
	    if (v1 < v2)
		return -1;
	    return +1;
	}
	return this.remaining() - that.remaining();
    }

    /**
     * Relative <i>get</i> method for reading a short value.
     *
     * <p> Reads the next two bytes at this buffer's current position,
     * composing them into a short value according to the current byte order,
     * and then increments the position by two.  </p>
     *
     * @return  The short value at the buffer's current position
     *
     * @throws  BufferUnderflowException
     *          If there are fewer than two bytes
     *          remaining in this buffer
     */
    public abstract short getShort();

    /**
     * Relative <i>put</i> method for writing a short
     * value&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> Writes two bytes containing the given short value, in the
     * current byte order, into this buffer at the current position, and then
     * increments the position by two.  </p>
     *
     * @param  value
     *         The short value to be written
     *
     * @return  This buffer
     *
     * @throws  BufferOverflowException
     *          If there are fewer than two bytes
     *          remaining in this buffer
     *
     * @throws  ReadOnlyBufferException
     *          If this buffer is read-only
     */
    public abstract ByteBuffer putShort(short value);

    /**
     * Absolute <i>get</i> method for reading a short value.
     *
     * <p> Reads two bytes at the given index, composing them into a
     * short value according to the current byte order.  </p>
     *
     * @param  index
     *         The index from which the bytes will be read
     *
     * @return  The short value at the given index
     *
     * @throws  IndexOutOfBoundsException
     *          If <tt>index</tt> is negative
     *          or not smaller than the buffer's limit,
     *          minus one
     */
    public abstract short getShort(int index);

    /**
     * Absolute <i>put</i> method for writing a short
     * value&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> Writes two bytes containing the given short value, in the
     * current byte order, into this buffer at the given index.  </p>
     *
     * @param  index
     *         The index at which the bytes will be written
     *
     * @param  value
     *         The short value to be written
     *
     * @return  This buffer
     *
     * @throws  IndexOutOfBoundsException
     *          If <tt>index</tt> is negative
     *          or not smaller than the buffer's limit,
     *          minus one
     *
     * @throws  ReadOnlyBufferException
     *          If this buffer is read-only
     */
    public abstract ByteBuffer putShort(int index, short value);

    /**
     * Creates a view of this byte buffer as a short buffer.
     *
     * <p> The content of the new buffer will start at this buffer's
     * current position. Changes to this buffer's content will be
     * visible in the new buffer, and vice versa; the two buffers'
     * position, limit, and mark values will be independent. <b><i>JSR
     * 239 does not support the mark.</i></b>
     *
     * <p> The new buffer's position will be zero, its capacity and
     * its limit will be the number of bytes remaining in this buffer
     * divided by two, and its mark will be undefined. The new buffer
     * will be direct if, and only if, this buffer is direct, and it
     * will be read-only if, and only if, this buffer is read-only.
     * <b><i>JSR 239 does not support the mark or read-only
     * buffers.</i></b>
     *
     * @return A new short buffer.
     */
    public abstract ShortBuffer asShortBuffer();
 
    /**
     * Relative <i>get</i> method for reading an int value.
     *
     * <p> Reads the next four bytes at this buffer's current position,
     * composing them into an int value according to the current byte order,
     * and then increments the position by four.  </p>
     *
     * @return  The int value at the buffer's current position
     *
     * @throws  BufferUnderflowException
     *          If there are fewer than four bytes
     *          remaining in this buffer
     */
    public abstract int getInt();

    /**
     * Relative <i>put</i> method for writing an int
     * value&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> Writes four bytes containing the given int value, in the
     * current byte order, into this buffer at the current position, and then
     * increments the position by four.  </p>
     *
     * @param  value
     *         The int value to be written
     *
     * @return  This buffer
     *
     * @throws  BufferOverflowException
     *          If there are fewer than four bytes
     *          remaining in this buffer
     *
     * @throws  ReadOnlyBufferException
     *          If this buffer is read-only
     */
    public abstract ByteBuffer putInt(int value);

    /**
     * Absolute <i>get</i> method for reading an int value.
     *
     * <p> Reads four bytes at the given index, composing them into a
     * int value according to the current byte order.  </p>
     *
     * @param  index
     *         The index from which the bytes will be read
     *
     * @return  The int value at the given index
     *
     * @throws  IndexOutOfBoundsException
     *          If <tt>index</tt> is negative
     *          or not smaller than the buffer's limit,
     *          minus three
     */
    public abstract int getInt(int index);

    /**
     * Absolute <i>put</i> method for writing an int
     * value&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> Writes four bytes containing the given int value, in the
     * current byte order, into this buffer at the given index.  </p>
     *
     * @param  index
     *         The index at which the bytes will be written
     *
     * @param  value
     *         The int value to be written
     *
     * @return  This buffer
     *
     * @throws  IndexOutOfBoundsException
     *          If <tt>index</tt> is negative
     *          or not smaller than the buffer's limit,
     *          minus three
     *
     * @throws  ReadOnlyBufferException
     *          If this buffer is read-only
     */
    public abstract ByteBuffer putInt(int index, int value);

    /**
     * Creates a view of this byte buffer as an int buffer.
     *
     * <p> The content of the new buffer will start at this buffer's
     * current position. Changes to this buffer's content will be
     * visible in the new buffer, and vice versa; the two buffers'
     * position, limit, and mark values will be independent. <b><i>JSR
     * 239 does not support the mark.</i></b>
     *
     * <p> The new buffer's position will be zero, its capacity and
     * its limit will be the number of bytes remaining in this buffer
     * divided by four, and its mark will be undefined. The new buffer
     * will be direct if, and only if, this buffer is direct, and it
     * will be read-only if, and only if, this buffer is read-only.
     * <b><i>JSR 239 does not support the mark or read-only
     * buffers.</i></b>
     *
     * @return A new int buffer.
     */
    public abstract IntBuffer asIntBuffer();

    /**
     * Relative <i>get</i> method for reading a float value.
     *
     * <p> Reads the next four bytes at this buffer's current position,
     * composing them into a float value according to the current byte order,
     * and then increments the position by four.  </p>
     *
     * @return  The float value at the buffer's current position
     *
     * @throws  BufferUnderflowException
     *          If there are fewer than four bytes
     *          remaining in this buffer
     */
    public abstract float getFloat();

    /**
     * Relative <i>put</i> method for writing a float
     * value&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> Writes four bytes containing the given float value, in the
     * current byte order, into this buffer at the current position, and then
     * increments the position by four.  </p>
     *
     * @param  value
     *         The float value to be written
     *
     * @return  This buffer
     *
     * @throws  BufferOverflowException
     *          If there are fewer than four bytes
     *          remaining in this buffer
     *
     * @throws  ReadOnlyBufferException
     *          If this buffer is read-only
     */
    public abstract ByteBuffer putFloat(float value);

    /**
     * Absolute <i>get</i> method for reading a float value.
     *
     * <p> Reads four bytes at the given index, composing them into a
     * float value according to the current byte order.  </p>
     *
     * @param  index
     *         The index from which the bytes will be read
     *
     * @return  The float value at the given index
     *
     * @throws  IndexOutOfBoundsException
     *          If <tt>index</tt> is negative
     *          or not smaller than the buffer's limit,
     *          minus three
     */
    public abstract float getFloat(int index);

    /**
     * Absolute <i>put</i> method for writing a float
     * value&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> Writes four bytes containing the given float value, in the
     * current byte order, into this buffer at the given index.  </p>
     *
     * @param  index
     *         The index at which the bytes will be written
     *
     * @param  value
     *         The float value to be written
     *
     * @return  This buffer
     *
     * @throws  IndexOutOfBoundsException
     *          If <tt>index</tt> is negative
     *          or not smaller than the buffer's limit,
     *          minus three
     *
     * @throws  ReadOnlyBufferException
     *          If this buffer is read-only
     */
    public abstract ByteBuffer putFloat(int index, float value);

    /**
     * Creates a view of this byte buffer as a float buffer.
     *
     * <p> The content of the new buffer will start at this buffer's
     * current position. Changes to this buffer's content will be
     * visible in the new buffer, and vice versa; the two buffers'
     * position, limit, and mark values will be independent. <b><i>JSR
     * 239 does not support the mark.</i></b>
     *
     * <p> The new buffer's position will be zero, its capacity and
     * its limit will be the number of bytes remaining in this buffer
     * divided by four, and its mark will be undefined. The new buffer
     * will be direct if, and only if, this buffer is direct, and it
     * will be read-only if, and only if, this buffer is read-only.
     * <b><i>JSR 239 does not support the mark or read-only
     * buffers.</i></b>
     *
     * @return A new float buffer.
     */
    public abstract FloatBuffer asFloatBuffer();
}
