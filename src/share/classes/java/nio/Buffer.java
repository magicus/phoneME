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
 * <p> A container for data of a specific primitive type.
 * 
 * <p> This class is provided as part of the JSR 239 NIO Buffer
 * building block. It is a subset of the <code>java.nio.Buffer</code>
 * class in Java(TM) Standard Edition version 1.4.2.  Differences are
 * noted in <b><i>bold italic</i></b>.
 *
 * <p><b><i> I/O channels, marking and resetting, and read-only
 * buffers are not supported.  The <code>char</code>,
 * <code>long</code>, and <code>double</code> datatypes are not
 * supported. The following methods are omitted:
 *
 * <ul>
 * <li><code>Buffer mark()</code></li>
 * <li><code>Buffer reset()</code></li>
 * <li><code>boolean isReadOnly()</code></li>
 * </ul>
 * </i></b>
 *
 * To mimimize documentation differences from the full NIO package,
 * the omitted features continue to be mentioned in the
 * documentation. In each case, a note is added explaining that the
 * feature is not present.
 * 
 * <p> A buffer is a linear, finite sequence of elements of a
 * specific primitive type.  Aside from its content, the essential
 * properties of a buffer are its capacity, limit, and position: </p>
 * 
 * <blockquote>
 * 
 *   <p> A buffer's <i>capacity</i> is the number of elements it contains.  The
 *   capacity of a buffer is never negative and never changes.  </p>
 * 
 *   <p> A buffer's <i>limit</i> is the index of the first element that should
 *   not be read or written.  A buffer's limit is never negative and is never
 *   greater than its capacity.  </p>
 * 
 *   <p> A buffer's <i>position</i> is the index of the next element to be
 *   read or written.  A buffer's position is never negative and is never
 *   greater than its limit.  </p>
 * 
 * </blockquote>
 * 
 * <p> There is one subclass of this class for each non-boolean
 * primitive type.  <b><i>The <code>char</code>, <code>long</code>,
 * and <code>double</code> buffer subclasses are not supported in JSR
 * 239.</i></b>
 *  
 * <h4> Transferring data </h4>
 * 
 * <p> Each subclass of this class defines two categories of <i>get</i> and
 * <i>put</i> operations: </p>
 * 
 * <blockquote>
 * 
 *   <p> <i>Relative</i> operations read or write one or more elements
 *   starting at the current position and then increment the position
 *   by the number of elements transferred.  If the requested transfer
 *   exceeds the limit then a relative <i>get</i> operation throws a
 *   <A HREF="BufferUnderflowException.html"
 *   title="class in
 *   java.nio"><CODE>BufferUnderflowException</CODE></A>
 *   and a relative <i>put</i> operation throws a <A
 *   HREF="BufferOverflowException.html" title="class
 *   in java.nio"><CODE>BufferOverflowException</CODE></A>; in either
 *   case, no data is transferred.  </p>
 * 
 *   <p> <i>Absolute</i> operations take an explicit element index and
 *   do not affect the position.  Absolute <i>get</i> and <i>put</i>
 *   operations throw an <CODE>IndexOutOfBoundsException</CODE> if the
 *   index argument exceeds the limit.  </p>
 * 
 * </blockquote>
 * 
 * <p> Data may also, of course, be transferred in to or out of a
 * buffer by the I/O operations of an appropriate channel, which are
 * always relative to the current position. <b><i>Channels are not
 * supported in JSR 239.</i></b>.
 * 
 * <h4> Marking and resetting </h4>
 * 
 * <p> <b><i>Marking and resetting are not supported in JSR
 * 239.</i></b>
 * 
 * <p> A buffer's <i>mark</i> is the index to which its position will
 * be reset when the <CODE>reset</CODE>
 * method is invoked.  The mark is not always defined, but when it is
 * defined it is never negative and is never greater than the
 * position.  If the mark is defined then it is discarded when the
 * position or the limit is adjusted to a value smaller than the mark.
 * If the mark is not defined then invoking the <CODE>reset</CODE>
 * method causes an <CODE>InvalidMarkException</CODE> to
 * be thrown.
 * 
 * <h4> Invariants </h4>
 * 
 * <p> The following invariant holds for the mark, position, limit, and
 * capacity values:
 * 
 * <blockquote>
 *     <tt>0</tt> <tt>&lt;=</tt>
 *     <i>mark</i> <tt>&lt;=</tt>
 *     <i>position</i> <tt>&lt;=</tt>
 * 
 *     <i>limit</i> <tt>&lt;=</tt>
 *     <i>capacity</i>
 * </blockquote>
 * 
 * <p> A newly-created buffer always has a position of zero and a mark that is
 * undefined.  The initial limit may be zero, or it may be some other value
 * that depends upon the type of the buffer and the manner in which it is
 * constructed.  The initial content of a buffer is, in general,
 * undefined.
 * 
 * 
 * <h4> Clearing, flipping, and rewinding </h4>
 * 
 * <p> In addition to methods for accessing the position, limit, and capacity
 * values and for marking and resetting, this class also defines the following
 * operations upon buffers:
 * 
 * <ul>
 * 
 *   <li><p> <A
 *   HREF="Buffer.html#clear()"><CODE>clear()</CODE></A>
 *   makes a buffer ready for a new sequence of channel-read or
 *   relative <i>put</i> operations: It sets the limit to the capacity
 *   and the position to zero.  </p></li>
 * 
 *   <li><p> <A
 *   HREF="Buffer.html#flip()"><CODE>flip()</CODE></A>
 *   makes a buffer ready for a new sequence of channel-write or
 *   relative <i>get</i> operations: It sets the limit to the current
 *   position and then sets the position to zero.  </p></li>
 * 
 *   <li><p> <A
 *   HREF="Buffer.html#rewind()"><CODE>rewind()</CODE></A>
 *   makes a buffer ready for re-reading the data that it already
 *   contains: It leaves the limit unchanged and sets the position to
 *   zero.  </p></li>
 * 
 * </ul>
 * 
 * <h4> Read-only buffers </h4>
 *
 * <p><b><i>JSR 239 does not support read-only buffers</i></b>.
 * 
 * <p> Every buffer is readable, but not every buffer is writable.
 * The mutation methods of each buffer class are specified as
 * <i>optional operations</i> that will throw a
 * <CODE>ReadOnlyBufferException</CODE> when invoked upon a read-only
 * buffer.  A read-only buffer does not allow its content to be
 * changed, but its mark, position, and limit values are mutable.
 * Whether or not a buffer is read-only may be determined by invoking
 * its <CODE>isReadOnly</CODE> method.
 * 
 * <h4> Thread safety </h4>
 * 
 * <p> Buffers are not safe for use by multiple concurrent threads.
 * If a buffer is to be used by more than one thread then access to
 * the buffer should be controlled by appropriate synchronization.
 * 
 * <h4> Invocation chaining </h4>
 * 
 * <p> Methods in this class that do not otherwise have a value to
 * return are specified to return the buffer upon which they are
 * invoked.  This allows method invocations to be chained; for
 * example, the sequence of statements
 * 
 * <blockquote><pre>
 * b.flip();
 * b.position(23);
 * b.limit(42);
 * </pre></blockquote>
 * 
 * can be replaced by the single, more compact statement
 * 
 * <blockquote><pre>
 * b.flip().position(23).limit(42);
 * </pre></blockquote>
 */
public abstract class Buffer {

    int capacity;
    int limit;
    int position;

    Buffer() {}

    /**
     * Returns this buffer's capacity.
     *
     * @return The capacity of this buffer.
     */
    public final int capacity() {
	return capacity;
    }

    /**
     * Returns this buffer's position.
     *
     * @return The position of this buffer.
     */
    public final int position() {
	return position;
    }

    /**
     * Sets this buffer's position.  If the mark is defined and larger
     * than the new position then it is discarded. <b><i>JSR 239 does
     * not support marking and resetting.</i></b>
     *
     * @param newPosition The new position value; must be non-negative
     * and no larger than the current limit.
     *
     * @return This buffer.
     *
     * @throws IllegalArgumentException If the preconditions on
     * <code>newPosition</code> do not hold.
     */
    public final Buffer position(int newPosition) {
        if (newPosition < 0 || newPosition > limit) {
            throw new IllegalArgumentException();
        }
	this.position = newPosition;
	return this;
    }

    /**
     * Returns this buffer's limit.
     *
     * @return The limit of this buffer.
     */
    public final int limit() {
	return limit;
    }

    /**
     * Sets this buffer's limit. If the position is larger than the
     * new limit then it is set to the new limit. If the mark is
     * defined and larger than the new limit then it is
     * discarded. <b><i>JSR 239 does not support marking and
     * resetting.</i></b>
     *
     * @param newLimit the new limit value.
     *
     * @return this buffer.
     *
     * @throws IllegalArgumentException if <code>newLimit</code> is
     * negative or larger than this buffer's capacity.
     */
    public final Buffer limit(int newLimit) {
        if (newLimit < 0 || newLimit > capacity) {
            throw new IllegalArgumentException();
        }
        if (position > newLimit) {
            position = newLimit;
        }
	this.limit = newLimit;
	return this;
    }

    /**
     * Clears this buffer. The position is set to zero, the limit is
     * set to the capacity, and the mark is discarded. <b><i>JSR 239 does
     * not support marking and resetting.</i></b>
     *
     * <p> Invoke this method before using a sequence of channel-read
     * or <i>put</i> operations to fill this buffer. For example:
     * 
     * <blockquote><pre>
     * buf.clear();     // Prepare buffer for reading
     * in.read(buf);    // Read data
     * </pre></blockquote>
     * 
     * <p> <b><i>JSR 239 does not support channels.</i></b>
     *
     * <p> This method does not actually erase the data in the buffer,
     * but it is named as if it did because it will most often be used
     * in situations in which that might as well be the case.
     * 
     * @return This buffer.
     */
    public final Buffer clear() {
        this.position = 0;
        this.limit = this.capacity;
        return this;
    }

    /**
     * Flips this buffer. The limit is set to the current position and
     * then the position is set to zero. If the mark is defined then
     * it is discarded. <b><i>JSR 239 does
     * not support marking and resetting.</i></b>
     *
     * <p> After a sequence of channel-read or <i>put</i> operations, invoke
     * this method to prepare for a sequence of channel-write or
     * relative <i>get</i> operations. For example:
     *
     * <blockquote><pre>
     * buf.put(magic);    // Prepend header
     * in.read(buf);      // Read data into rest of buffer
     * buf.flip();        // Flip buffer
     * out.write(buf);    // Write header + data to channel
     * </pre></blockquote>
     *
     * <p> This method is often used in conjunction with the compact
     * method when transferring data from one place to another.
     *
     * <p> <b><i>JSR 239 does not support channels.</i></b>
     *
     * @return This buffer.
     */
    public final Buffer flip() {
        this.limit = this.position;
        this.position = 0;
        return this;
    }

    /**
     * Rewinds this buffer. The position is set to zero and the mark
     * is discarded. <b><i>JSR 239 does
     * not support marking and resetting.</i></b>
     *
     * <p> Invoke this method before a sequence of channel-write or
     * <i>get</i> operations, assuming that the limit has already been set
     * appropriately. For example:
     *
     * <blockquote><pre>
     * out.write(buf);    // Write remaining data
     * buf.rewind();      // Rewind buffer
     * buf.get(array);    // Copy data into array
     * </pre></blockquote>
     *
     * <p> <b><i>JSR 239 does not support channels.</i></b>
     */
    public final Buffer rewind() {
        this.position = 0;
        return this;
    }

    /**
     * Returns the number of elements between the current position and
     * the limit.
     *
     * @return The number of elements remaining in this buffer.
     */
    public final int remaining() {
        return limit - position;
    }
    
    /**
     * Tells whether there are any elements between the current
     * position and the limit.
     *
     * @return <code>true</code> if, and only if, there is at least
     * one element remaining in this buffer.
     */
    public final boolean hasRemaining() {
        return position < limit;
    }

    // Removed methods: 
    //
    //     abstract boolean isReadOnly();
    //
    //     // Applications can maintain their own mark, use position(int) 
    //     Buffer mark();
    //     Buffer reset();
}
