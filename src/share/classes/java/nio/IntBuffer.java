/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

/** 
 * An int buffer.
 *
 * <p> This class is provided as part of the JSR 239 NIO Buffer
 * building block. It is a subset of the
 * <code>java.nio.IntBuffer</code> class in Java(TM) Standard Edition
 * version 1.4.2.  Differences are noted in <b><i>bold italic</i></b>.
 * The class documentation may make reference to classes that are not
 * present in the building block.
 *
 * <p><b><i> I/O channels, marking and resetting, and read-only buffers
 * are not supported.  Allocation of non-direct int buffers,
 * compaction, and duplication are not supported.  
 * The following methods are omitted:
 *
 * <ul>
 * <li><code>IntBuffer allocate(int capacity)</code></li>
 * <li><code>IntBuffer compact()</code></li>
 * <li><code>IntBuffer duplicate()</code></li>
 * <li><code>Buffer mark()</code></li>
 * <li><code>Buffer reset()</code></li>
 * <li><code>boolean isReadOnly()</code></li>
 * <li><code>IntBuffer asReadOnlyBuffer()</code></li>
 * </ul>
 * </i></b>
 *
 * <p> This class defines four categories of operations upon
 * int buffers:
 *
 * <ul>
 *
 *   <li><p> Absolute and relative {@link #get() </code><i>get</i><code>} and
 *   {@link #put(int) </code><i>put</i><code>} methods that read and write
 *   single ints; </p></li>
 *
 *   <li><p> Relative {@link #get(int[]) </code><i>bulk get</i><code>}
 *   methods that transfer contiguous sequences of ints from this buffer
 *   into an array;</li>
 *
 *   <li><p> Relative {@link #put(int[]) </code><i>bulk put</i><code>}
 *   methods that transfer contiguous sequences of ints from an
 *   int array or some other int
 *   buffer into this buffer; &#32;and </p></li>
 * 
 *   <li><p> Methods for compacting, duplicating, and {@link #slice
 *   slicing} an int buffer.  <b><i>JSR 239 does
 *   not support compacting and duplicating buffers.</i></b> </p></li>
 *
 * </ul>
 *
 * <p> Int buffers can be created either by <i>allocation</i>, which
 * allocates space for the buffer's content, by {@link #wrap(int[])
 * </code><i>wrapping</i><code>} an existing int array into a buffer,
 * or by creating a <a href="ByteBuffer.html#view"><i>view</i></a> of
 * an existing byte buffer. <b><i>JSR 239 supports allocation of
 * <code>ByteBuffer</code>s only.</i></b>
 *
 * <p> Like a byte buffer, an int buffer is either <a
 * href="ByteBuffer.html#direct"><i>direct</i> or <i>non-direct</i></a>.  A
 * int buffer created via the <tt>wrap</tt> methods of this class will
 * be non-direct.  An int buffer created as a view of a byte buffer will
 * be direct if, and only if, the byte buffer itself is direct.  Whether or not
 * an int buffer is direct may be determined by invoking the {@link
 * #isDirect isDirect} method.  </p>
 *
 * <p> Methods in this class that do not otherwise have a value to return are
 * specified to return the buffer upon which they are invoked.  This allows
 * method invocations to be chained.
 */
public abstract class IntBuffer extends Buffer implements Comparable {

    ByteBufferImpl parent;

    int[] array;
    int arrayOffset;

    boolean isDirect;

    boolean disposed = false;

    /**
     * Constructs a new <code>IntBuffer</code>.
     */
    IntBuffer() {}

    /**
     * Wraps an int array into a buffer.
     *
     * <p> The new buffer will be backed by the given int array;
     * that is, modifications to the buffer will cause the array to be modified
     * and vice versa.  The new buffer's capacity will be
     * <tt>array.length</tt>, its position will be <tt>offset</tt>, its limit
     * will be <tt>offset + length</tt>, and its mark will be undefined.  Its
     * {@link #array </code>backing array<code>} will be the given array, and
     * its {@link #arrayOffset </code>array offset<code>} will be zero.  </p>
     *
     * @param  array
     *         The array that will back the new buffer
     *
     * @param  offset
     *         The offset of the subarray to be used; must be non-negative and
     *         no larger than <tt>array.length</tt>.  The new buffer's position
     *         will be set to this value.
     *
     * @param  length
     *         The length of the subarray to be used;
     *         must be non-negative and no larger than
     *         <tt>array.length - offset</tt>.
     *         The new buffer's limit will be set to <tt>offset + length</tt>.
     *
     * @return  The new int buffer
     *
     * @throws  IndexOutOfBoundsException
     *          If the preconditions on the <tt>offset</tt> and <tt>length</tt>
     *          parameters do not hold
     */
    public static IntBuffer wrap(int[] array, int offset, int length) {
        if (offset < 0 || offset > array.length ||
            length < 0 || length > array.length - offset) {
            throw new IndexOutOfBoundsException();
        }
        
        IntBufferImpl ibi =
            new IntBufferImpl(null, array.length, array, 0, false);
        ibi.position(offset);
        ibi.limit(offset + length);
        return ibi;
    }

    /**
     * Wraps an int array into a buffer.
     *
     * <p> The new buffer will be backed by the given int array;
     * that is, modifications to the buffer will cause the array to be modified
     * and vice versa.  The new buffer's capacity and limit will be
     * <tt>array.length</tt>, its position will be zero, and its mark will be
     * undefined.  Its {@link #array </code>backing array<code>} will be the
     * given array, and its {@link #arrayOffset </code>array offset<code>} will
     * be zero.  </p> 
     *
     * @param  array
     *         The array that will back this buffer
     *
     * @return  The new int buffer
     */
    public static IntBuffer wrap(int[] array) {
	return wrap(array, 0, array.length);
    }
 
    /**
     * Creates a new int buffer whose content is a shared
     * subsequence of this buffer's content.
     *
     * <p> The content of the new buffer will start at this buffer's
     * current position.  Changes to this buffer's content will be
     * visible in the new buffer, and vice versa; the two buffers'
     * position, limit, and mark values will be independent. <b><i>JSR
     * 239 does not support the mark.</i></b>
     *
     * <p> The new buffer's position will be zero, its capacity and
     * its limit will be the number of ints remaining in this
     * buffer, and its mark will be undefined.  The new buffer will be
     * direct if, and only if, this buffer is direct, and it will be
     * read-only if, and only if, this buffer is read-only. <b><i>JSR
     * 239 does not support the mark or read-only buffers.</i></b>
     * </p>
     *
     * @return The new int buffer.
     */
    public abstract IntBuffer slice();

    /**
     * Relative <i>get</i> method.  Reads the int at this
     * buffer's current position, and then increments the
     * position. </p>
     *
     * @return The int at the buffer's current position.
     *
     * @throws BufferUnderflowException If the buffer's current
     * position is not smaller than its limit.
     */
    public abstract int get();

    /**
     * Relative <i>put</i> method&nbsp;&nbsp;<i>(optional
     * operation)</i>.
     * 
     * <p> Writes the given int into this buffer at the current
     * position, and then increments the position. </p>
     *
     * @param i The int to be written.
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
    public abstract IntBuffer put(int i);

    /**
     * Absolute <i>get</i> method.  Reads the int at the given
     * index. </p>
     *
     * @param  index The index from which the int will be read.
     *
     * @return  The int at the given index.
     *
     * @throws IndexOutOfBoundsException If <tt>index</tt> is negative
     * or not smaller than the buffer's limit.
     */
    public abstract int get(int index);

    /**
     * Absolute <i>put</i> method&nbsp;&nbsp;<i>(optional operation)</i>.
     * 
     * <p> Writes the given int into this buffer at the given
     * index. </p>
     *
     * @param index The index at which the int will be written.
     *
     * @param i The int value to be written.
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
    public abstract IntBuffer put(int index, int i);
    
    /**
     * Relative bulk <i>get</i> method.
     *
     * <p> This method transfers ints from this buffer into the
     * given destination array.  If there are fewer ints
     * remaining in the buffer than are required to satisfy the
     * request, that is, if
     * <tt>length</tt>&nbsp;<tt>&gt;</tt>&nbsp;<tt>remaining()</tt>,
     * then no ints are transferred and a {@link
     * BufferUnderflowException} is thrown.
     *
     * <p> Otherwise, this method copies <tt>length</tt> ints
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
     * ints in this buffer and it is potentially much more
     * efficient. </p>
     *
     * @param dst The array into which ints are to be written.
     *
     * @param offset The offset within the array of the first
     * int to be written; must be non-negative and no larger
     * than <tt>dst.length</tt>.
     *
     * @param length The maximum number of ints to be written
     * to the given array; must be non-negative and no larger than
     * <tt>dst.length - offset</tt>.
     *
     * @return This buffer.
     *
     * @throws BufferUnderflowException If there are fewer than
     * <tt>dst.length</tt> ints remaining in this buffer.
     *
     * @throws IndexOutOfBoundsException If the preconditions on the
     * <tt>offset</tt> and <tt>length</tt> parameters do not hold.
     */
    public IntBuffer get(int[] dst, int offset, int length) {
        if (offset < 0 || offset > dst.length ||
            length < 0 || length > dst.length - offset) {
            throw new IndexOutOfBoundsException();
        }
        if (limit - position < length) {
            throw new BufferUnderflowException();
        }

        int bytePtr = arrayOffset + (position << 2);
	if (isDirect) {
	    ByteBufferImpl._getInts(bytePtr, dst, offset, length);
	} else if (array != null) {
	    System.arraycopy(array, arrayOffset + position,
                             dst, offset, length);
	} else {
            for (int i = 0; i < length; i++) {
                dst[offset++] = parent.getInt(bytePtr);
                bytePtr += 4;
            }
        }
	position += length;
	return this;
    }

    /**
     * Relative bulk <i>get</i> method.
     *
     * <p> This method transfers ints from this buffer into the
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
     * <tt>length</tt> ints remaining in this buffer.
     */
    public IntBuffer get(int[] dst) {
	return get(dst, 0, dst.length);
    }

    /**
     * Relative bulk <i>put</i> method&nbsp;&nbsp;<i>(optional
     * operation)</i>.
     *
     * <p> This method transfers the ints remaining in the
     * given source buffer into this buffer.  If there are more
     * ints remaining in the source buffer than in this buffer,
     * that is, if
     * <tt>src.remaining()</tt>&nbsp;<tt>&gt;</tt>&nbsp;<tt>remaining()</tt>,
     * then no ints are transferred and a {@link
     * BufferOverflowException} is thrown.
     *
     * <p> Otherwise, this method copies
     * <i>n</i>&nbsp;=&nbsp;<tt>src.remaining()</tt> ints from
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
     * @param src The source buffer from which ints are to be
     * read; must not be this buffer.
     *
     * @return This buffer.
     *
     * @throws BufferOverflowException If there is insufficient space
     * in this buffer for the remaining ints in the source
     * buffer.
     *
     * @throws IllegalArgumentException If the source buffer is this buffer.
     *
     * @throws ReadOnlyBufferException If this buffer is
     * read-only. <b><i>JSR 239 does not support read-only buffer or
     * the <code>ReadOnlyBufferException</code> class.</i></b>
     */
    public IntBuffer put(IntBuffer src) {
        if (src == this) {
            throw new IllegalArgumentException();
        }
	
	IntBufferImpl srci = (IntBufferImpl)src;

	int length = srci.limit - srci.position;
        if (length > this.limit - this.position) {
            throw new BufferOverflowException();
        }
	if (isDirect && srci.isDirect) {
	    ByteBufferImpl._copyBytes(srci.arrayOffset +
                                      (srci.position << 2),
                                      this.arrayOffset +
                                      (this.position << 2),
                                      (length << 2));
	} else if (isDirect && !srci.isDirect) {
            if (srci.array != null) {
                ByteBufferImpl._putInts(this.arrayOffset +
                                        (this.position << 2),
                                        srci.array,
                                        srci.arrayOffset + srci.position,
                                        length);
            } else {
                byte[] srcArray = srci.parent.array;
                int srciArrayOffset = srci.parent.arrayOffset +
                    srci.arrayOffset +
                    (srci.position << 2);
                
                ByteBufferImpl._putBytes(this.arrayOffset +
                                         (this.position << 2),
                                         srcArray,
                                         srciArrayOffset,
                                         4*length);
            }
	} else if (!isDirect && srci.isDirect) {
            if (array != null) {
                ByteBufferImpl._getInts(srci.arrayOffset +
                                        (srci.position << 2),
                                        this.array,
                                        this.arrayOffset + this.position,
                                        length);
            } else {
                byte[] dstArray = parent.array;
                int dstArrayOffset = parent.arrayOffset +
                    arrayOffset +
                    (position << 2);
                
                ByteBufferImpl._getBytes(srci.arrayOffset +
                                         (srci.position << 2),
                                         dstArray,
                                         dstArrayOffset,
                                         4*length);
            }
	} else if (!isDirect && !srci.isDirect) {
            if (array != null && srci.array != null) {
                System.arraycopy(srci.array,
                                 srci.arrayOffset + srci.position,
                                 this.array,
                                 this.arrayOffset + this.position,
                                 length);
            } else {
                for (int i = 0; i < length; i++) {
                    put(i, srci.get(i));
                }
            }
	}
	
	srci.position += length;
	this.position += length;
	return this;
    }

    /**
     * Relative bulk <i>put</i> method&nbsp;&nbsp;<i>(optional
     * operation)</i>.
     *
     * <p> This method transfers ints into this buffer from the
     * given source array.  If there are more ints to be copied
     * from the array than remain in this buffer, that is, if
     * <tt>length</tt>&nbsp;<tt>&gt;</tt>&nbsp;<tt>remaining()</tt>,
     * then no ints are transferred and a {@link
     * BufferOverflowException} is thrown.
     *
     * <p> Otherwise, this method copies <tt>length</tt> ints
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
     * @param src The array from which ints are to be read.
     *
     * @param offset The offset within the array of the first
     * int to be read; must be non-negative and no larger than
     * <tt>array.length</tt>.
     *
     * @param length The number of ints to be read from the
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
    public IntBuffer put(int[] src, int offset, int length) {
        if (offset < 0 || offset > src.length ||
            length < 0 || length > src.length - offset) {
            throw new IndexOutOfBoundsException();
        }
        if (length > limit - position) {
            throw new BufferOverflowException();
        }

        int bytePtr = arrayOffset + (position << 2);
	if (isDirect) {
	    ByteBufferImpl._putInts(bytePtr, src, offset, length);
	} else if (array != null) {
	    System.arraycopy(src, offset,
			     array, arrayOffset + position, length);
	} else {
            for (int i = 0; i < length; i++) {
                parent.putInt(bytePtr, src[offset++]);
                bytePtr += 4;
            }
        }
	position += length;
	return this;
    }

    /**
     * Relative bulk <i>put</i> method&nbsp;&nbsp;<i>(optional operation)</i>.
     *
     * <p> This method transfers the entire content of the given
     * source int array into this buffer.  An invocation of
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
    public final IntBuffer put(int[] src) {
	return put(src, 0, src.length);
    }

    /**
     * Tells whether or not this buffer is backed by an accessible
     * int array.
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
	return array != null;
    }

    /**
     * Returns the int array that backs this
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
    public final int[] array() {
	if (array == null) {
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
	if (array == null) {
	    throw new UnsupportedOperationException();
	}
	return arrayOffset;
    }

    /**
     * Tells whether or not this int buffer is direct. </p>
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
        return "java.nio.IntBuffer[" +
            "pos=" + position() +
            "lim=" + limit() +
            "cap=" + capacity() +
            "]";
    }

    /**
     * Returns the current hash code of this buffer.
     *
     * <p> The hash code of an int buffer depends only upon its remaining
     * elements; that is, upon the elements from <tt>position()</tt> up to, and
     * including, the element at <tt>limit()</tt>&nbsp;-&nbsp;<tt>1</tt>.
     *
     * <p> Because buffer hash codes are content-dependent, it is inadvisable
     * to use buffers as keys in hash maps or similar data structures unless it
     * is known that their contents will not change.  </p>
     *
     * @return  The current hash code of this buffer
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
     * <p> Two int buffers are equal if, and only if,
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
     * <p> An int buffer is not equal to any other type of object.  </p>
     *
     * @param  ob  The object to which this buffer is to be compared.
     *
     * @return  <tt>true</tt> if, and only if, this buffer is equal to the
     *           given object.
     */
    public boolean equals(Object ob) {
	if (!(ob instanceof IntBuffer))
	    return false;
	IntBuffer that = (IntBuffer)ob;
	if (this.remaining() != that.remaining())
	    return false;
	int p = this.position();
	for (int i = this.limit() - 1, j = that.limit() - 1; i >= p; i--, j--) {
	    int v1 = this.get(i);
	    int v2 = that.get(j);
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
     * <p> Two int buffers are compared by comparing their sequences of
     * remaining elements lexicographically, without regard to the starting
     * position of each sequence within its corresponding buffer.
     *
     * <p> An int buffer is not comparable to any other type of object.
     *
     * @return  A negative integer, zero, or a positive integer as this buffer
     *		is less than, equal to, or greater than the given buffer.
     * @throws  ClassCastException If the argument is not an int buffer.
     */
    public int compareTo(Object ob) {
        IntBuffer that = (IntBuffer)ob;
	int n = this.position() + Math.min(this.remaining(), that.remaining());
	for (int i = this.position(), j = that.position(); i < n; i++, j++) {
	    int v1 = this.get(i);
	    int v2 = that.get(j);
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

}
