/*
 *   
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

//-----------------------------------------------------------------------------
// PACKAGE DEFINITION
//-----------------------------------------------------------------------------
package sim.toolkit;


//import sat.AccessBuffer;

//-----------------------------------------------------------------------------
// IMPORTS
//-----------------------------------------------------------------------------
import javacard.framework.Util;
import javacard.framework.ISO7816;

/**
 *
 * The ViewHandler class offers basic services and contains basic 
 * methods to handle
 * a Simple TLV List, such as in a <b>Terminal Response</b> data field 
 * or in a BER-TLV
 * element (<b>Envelope</b> data field or <b>Proactive</b> command). 
 * The byte at offset 0 of a handler is the tag of the first Simple TLV. 
 *
 * @version 8.3.0
 *
 * @see ToolkitException
 */
public abstract class ViewHandler {
    /** Offset of BER_TLV_TAG. */
    protected static final byte BER_TLV_TAG_OFFSET = (byte)5;
    /** Offset of Lc in the APDU buffer. */
    protected static final byte OFFSET_LC = (byte)4;
    /** Offset of TPDUD. */
    protected static final byte TPUD_OFFSET = (byte)12;
    /** Tag of DEVICE_ID field. */
    private static final byte DEVICE_ID_TAG = (byte)0x82;
    /** Length of DEVICE_ID field. */
    private static final byte DEVICE_ID_LENGTH = (byte)0x02;
    /** Tag of ADDRESS field. */
    private static final byte ADDRESS_TAG = (byte)0x06;
    /** Tag of SMS_TPDU. */
    private static final byte SMS_TPDU_TAG = (byte)0x8B;
    
    /** Offset of first occurrence of TLV. */
    short currentTLVOffset;
    /** Offset of first occurrence of TLV. */
    short firstTLVOffset;
    
    /** Reference to SAT Accessor. */
    public static AccessSAT SATAccessor;
    
    /** Reference to TI which is now serviced. */
    public static ToolkitInterface currentTI;
    
    /**
     * Constructor.
     */
    ViewHandler() {}
	
    // ------------------------------- Constructor ---------------------------
    /**
     * Builds a new ViewHandler object.
     *
     * @param buffer a reference to the TLV buffer
     * @param offset the position in the TLV buffer
     * @param length the length of the TLV buffer
     *
     * @exception NullPointerException if <code>buffer</code> 
     * is <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if <code>offset</code> 
     * or <code>length</code> or both would cause access outside array bounds
     */
    ViewHandler(byte[] buffer, short offset, short length)
	throws 	NullPointerException,
		ArrayIndexOutOfBoundsException  {
    }

    // ------------------------------- Public methods -------------------------
    /**
     * Returns the length of the TLV list.
     *
     * @return length in bytes
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *          <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is 
     *          busy</ul>
     */
    public short getLength() throws ToolkitException {
        byte[] buffer = getAPDUBuffer();
        short length = (short)(buffer[(short)(currentTLVOffset + 1)] & 0xFF);
        return length;
    }

    /**
     * Copies the simple TLV list contained in the handler to the 
     * destination byte array.
     *
     * <p>
     * Notes:<ul>
     * <li><em>If </em><code>dstOffset</code><em> or 
     * </em><code>dstLength</code><em> parameter is negative 
     * an </em><code>ArrayIndexOutOfBoundsException</code>
     * <em> exception is thrown and no copy is performed.</em>
     * <li><em>If </em><code>dstOffset+dstLength</code><em> is greater 
     * than </em><code>dstBuffer.length</code><em>, the length
     * of the </em><code>dstBuffer</code><em> array an 
     * </em><code>ArrayIndexOutOfBoundsException</code><em> 
     * exception is thrown and no copy is performed.</em> 
     * </ul> 
     *
     * @param dstBuffer a reference to the destination buffer
     * @param dstOffset the position in the destination buffer
     * @param dstLength the data length to be copied
     *
     * @return <code>dstOffset+dstLength</code>
     *
     * @exception NullPointerException if <code>dstBuffer</code> 
     * is <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if copy would 
     * cause access of data outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>OUT_OF_TLV_BOUNDARIES</code> if <code>dstLength</code>
     *      is grater than the length of the simple TLV List.</ul>
     */
    public short copy(byte[] dstBuffer,
                        short dstOffset,
                        short dstLength) throws NullPointerException,
                                            ArrayIndexOutOfBoundsException,
                                            ToolkitException {
        byte[] buffer = getAPDUBuffer();
        short TLVLength = 
            (short)(buffer[(short)(currentTLVOffset + 1)] & 0xFF);
        if (TLVLength < dstLength) {
            // if length is greater than the TLV length itself, then 
            // throw exception
            ToolkitException.throwIt(ToolkitException.OUT_OF_TLV_BOUNDARIES);
        }
        // copy the current TLV list
        Util.arrayCopy(buffer, (short)(currentTLVOffset+2), dstBuffer, 
                        dstOffset, dstLength);
        return (short)(dstOffset + dstLength);
    }


    /**
     * Looks for the indicated occurrence of a TLV element from the beginning of
     * the TLV list (handler buffer). If the method is successful then the
     * corresponding TLV becomes current, else no TLV is selected.
     * This search method is Comprehension Required flag independent.
     *
     * @param tag the tag of the TLV element to search
     * @param occurrence the occurrence number of the TLV element (1 for the 
     * first, 2 for the second...)
     *
     * @return result of the method: <ul>
     *      <li><code>TLV_NOT_FOUND</code> if the required occurrence of the 
     *      TLV element does not exist
     *      <li><code>TLV_FOUND_CR_SET</code> if the required occurrence exists
     *      and Comprehension Required flag is set
     *      <li><code>TLV_FOUND_CR_NOT_SET</code> if the required occurrence 
     *      exists and Comprehension Required flag is not set</ul>
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>BAD_INPUT_PARAMETER</code> if an input parameter is not
     *      valid (e.g. occurrence = 0)</ul>
     */
    public byte findTLV(byte tag, byte occurrence) throws ToolkitException {
        byte count = 0; // count of occurances
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getTLVOffset(buffer, tag, Lc, occurrence);
        if (TLVOffset >= Lc)
            return ToolkitConstants.TLV_NOT_FOUND; // not found
        
        currentTLVOffset = TLVOffset;
        if ((byte)(buffer[TLVOffset] & ToolkitConstants.TAG_SET_CR) == 
                    ToolkitConstants.TAG_SET_CR)
            return ToolkitConstants.TLV_FOUND_CR_SET;
            
        return ToolkitConstants.TLV_FOUND_CR_NOT_SET;
    }

    /**
     * Gets the binary length of the value field for the last TLV element 
     * which has been found in the handler.
     *
     * @return length of the value field
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      TLV element</ul>
     */
    public short getValueLength() throws ToolkitException {
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getLastTLVOffset(buffer, Lc);
        if (TLVOffset >= Lc) {
            ToolkitException.throwIt(ToolkitException.UNAVAILABLE_ELEMENT);
        }
        return (short)(buffer[(short)
			     (TLVOffset + 1)] & 0xFF); // return the length
    }

    /**
     * Gets a byte from the last TLV element which has been found in the 
     * handler.
     *
     * @param valueOffset the offset of the byte to return in the TLV element
     *
     * @return element value (1 byte)
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable TLV 
     *      element
     *      <li><code>OUT_OF_TLV_BOUNDARIES</code> if 
     *      <code>valueOffset</code> is out of the current TLV </ul>
     */
    public byte getValueByte(short valueOffset) throws ToolkitException {
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getLastTLVOffset(buffer, Lc);
        if (TLVOffset >= Lc) {
            ToolkitException.throwIt(ToolkitException.UNAVAILABLE_ELEMENT);
        }
        short TLVLength = (short)(buffer[(short)(TLVOffset + 1)] & 0xFF);
        if (valueOffset > TLVLength) {
            ToolkitException.throwIt(ToolkitException.OUT_OF_TLV_BOUNDARIES);
        }
        // return the byte at offset
        return buffer[(short)(TLVOffset + 2 + valueOffset)]; 
    }

    /**
     * Copies a part of the last TLV element which has been found, into a
     * destination buffer.
     *
     * <p>
     * Notes:<ul>
     * <li><em>If </em><code>dstOffset</code><em> or 
     * </em><code>dstLength</code><em> parameter is negative an 
     * </em><code>ArrayIndexOutOfBoundsException</code>
     * <em> exception is thrown and no copy is performed.</em>
     * <li><em>If </em><code>dstOffset+dstLength</code><em> is 
     * greater than </em><code>dstBuffer.length</code><em>, the length
     * of the </em><code>dstBuffer</code><em> array an 
     * </em><code>ArrayIndexOutOfBoundsException</code><em> exception 
     * is thrown and no copy is performed.</em> 
     * </ul> 
     *
     * @param valueOffset the offset of the first byte in the source 
     * TLV element
     * @param dstBuffer a reference to the destination buffer
     * @param dstOffset the position in the destination buffer
     * @param dstLength the data length to be copied
     *
     * @return <code>dstOffset+dstLength</code>
     *
     * @exception NullPointerException if <code>dstBuffer</code> is 
     * <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if copyValue would 
     * cause access of data outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      TLV element
     *      <li><code>OUT_OF_TLV_BOUNDARIES</code> if:
     *      <ul> 
     *          <li><code>valueOffset</code> parameter is negative or
     *          <li><code>valueOffset + dstLength</code> is greater than 
     *          the length of the current TLV
     *      </ul>
     *  </ul>
     */
    public short copyValue(short valueOffset,
			   byte[] dstBuffer,
			   short dstOffset,
			   short dstLength) 
	throws NullPointerException,
	       ArrayIndexOutOfBoundsException,
	       ToolkitException {
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getLastTLVOffset(buffer, Lc);
        if (TLVOffset >= Lc) {
            ToolkitException.throwIt(ToolkitException.UNAVAILABLE_ELEMENT);
        }
        short TLVLength = (short)(buffer[(short)(TLVOffset + 1)] & 0xFF);
        if (valueOffset > TLVLength || valueOffset < 0) {
            ToolkitException.throwIt(ToolkitException.OUT_OF_TLV_BOUNDARIES);
        }
        Util.arrayCopy(buffer, 
                        (short)(currentTLVOffset + 2 + valueOffset), 
                        dstBuffer, dstOffset, dstLength);
        return buffer[(short)(TLVOffset + 2 + valueOffset)]; 
    }

    /**
     * Compares the last found TLV element with a buffer.
     *
     * <p>
     * Notes:<ul>
     * <li><em>If </em><code>compareOffset</code><em> or 
     * </em><code>compareLength</code><em> parameter is negative 
     * an </em><code>ArrayIndexOutOfBoundsException</code>
     * <em> exception is thrown and no compare is performed.</em>
     * <li><em>If </em><code>compareOffset+compareLength</code><em>is 
     * greater than </em><code>compareBuffer.length</code><em>, the length
     * of the </em><code>compareBuffer</code><em> array an 
     * </em><code>ArrayIndexOutOfBoundsException</code><em> exception 
     * is thrown and no compare is performed.</em> 
     * </ul> 
     *
     * @param valueOffset the offset of the first byte to compare in 
     * the TLV element
     * @param compareBuffer a reference to the comparison buffer
     * @param compareOffset the position in the comparison buffer
     * @param compareLength the length to be compared
     *
     * @return the result of the comparison as follows: <ul>
     *      <li><code>0</code> if identical
     *      <li><code>-1</code> if the first miscomparing byte in simple 
     *      TLV List is less than that in <code>compareBuffer</code>,
     *      <li><code>1</code> if the first miscomparing byte in simple 
     *      TLV List is greater than that in <code>compareBuffer</code>.</ul>
     *
     * @exception NullPointerException if <code>compareBuffer</code> is 
     * <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if compareValue would 
     * cause access of data outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      TLV element
     *      <li><code>OUT_OF_TLV_BOUNDARIES</code> if:
     *       <ul> 
     *          <li><code>valueOffset</code> parameter is negative or
     *          <li><code>valueOffset + compareLength</code> is greater 
     *          than the length of the current TLV
     *       </ul>
     *  </ul>
     */
    public byte compareValue(short valueOffset,
                            byte[] compareBuffer,
                            short compareOffset,
                            short compareLength) 
                            throws 	NullPointerException,
                            ArrayIndexOutOfBoundsException,
                            ToolkitException {
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getLastTLVOffset(buffer, Lc);
        if (TLVOffset >= Lc) {
            ToolkitException.throwIt(ToolkitException.UNAVAILABLE_ELEMENT);
        }
        short TLVLength = (short)(buffer[(short)(TLVOffset + 1)] & 0xFF);
        if (valueOffset > TLVLength || valueOffset < 0) {
            ToolkitException.throwIt(ToolkitException.OUT_OF_TLV_BOUNDARIES);
        }
        return Util.arrayCompare(buffer, 
                                (short)(currentTLVOffset + 2 + valueOffset), 
                                compareBuffer, compareOffset, compareLength);
    }

    /**
     * Looks for the first occurrence of a TLV element from the beginning 
     * of a TLV
     * list and copy its value into a destination buffer.
     * If no TLV element is found, the <code>UNAVAILABLE_ELEMENT</code> 
     * exception is thrown.
     * If the method is successful then the corresponding TLV becomes current,
     * else no TLV is selected.
     * This search method is Comprehension Required flag independent.
     *
     * <p>
     * Notes:<ul>
     * <li><em>If </em><code>dstOffset</code><em> parameter is negative or 
     * </em><code>dstOffset</code>
     * <em> is greater than </em><code>dstBuffer.length</code><em>, the 
     * length of the </em><code>dstBuffer</code>
     * <em> array an </em><code>ArrayIndexOutOfBoundsException</code><em> 
     * exception is thrown and no find is performed.</em> 
     * </ul> 
     *
     * @param tag the tag of the TLV element to search
     * @param dstBuffer a reference to the destination buffer
     * @param dstOffset the position in the destination buffer
     *
     * @return <code>dstOffset</code> + length of the copied value
     *
     * @exception NullPointerException if <code>dstBuffer</code> 
     * is <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if findAndCopyValue 
     * would cause access of data outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      TLV element</ul>
     */
    public short findAndCopyValue(byte tag,
                            byte[] dstBuffer,
                            short dstOffset) 
                            throws 	NullPointerException,
                            ArrayIndexOutOfBoundsException,
                            ToolkitException {
        // this method could potentialy use the other 
        // findAndCopyValue() method. The only problem is the length 
        // parameter required by that method
        
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getTLVOffset(buffer, tag, Lc, (short)1);
        
        if (TLVOffset >= Lc) {
            // TLV not found
            ToolkitException.throwIt(ToolkitException.UNAVAILABLE_ELEMENT);
        }
        currentTLVOffset = TLVOffset;
        short length = buffer[(short)(TLVOffset+1)];
        Util.arrayCopy(buffer, (short)(TLVOffset+2), dstBuffer, 
                        dstOffset, length);
        return (short)(dstOffset + length);
    }
    
    


    /**
     * Looks for the indicated occurrence of a TLV element from the 
     * beginning of a TLV
     * list and copy its value into a destination buffer.
     * If no TLV element is found, the <code>UNAVAILABLE_ELEMENT</code> 
     * exception is thrown.
     * If the method is successful then the corresponding TLV becomes current,
     * else no TLV is selected.
     * This search method is Comprehension Required flag independent.
     *
     * <p>
     * Notes:<ul>
     * <li><em>If </em><code>dstOffset</code><em> or 
     * </em><code>dstLength</code><em> parameter is negative 
     * an </em><code>ArrayIndexOutOfBoundsException</code>
     * <em> exception is thrown and no copy is performed.</em>
     * <li><em>If </em><code>dstOffset+dstLength</code><em>is greater 
     * than </em><code>dstBuffer.length</code><em>, the length
     * of the </em><code>dstBuffer</code><em> array an 
     * </em><code>ArrayIndexOutOfBoundsException</code><em> exception 
     * is thrown and no copy is performed.</em> 
     * </ul> 
     *
     * @param tag the tag of the TLV element to search
     * @param occurrence the occurrence number of the TLV element 
     * (1 for the first, 2 for the second...)
     * @param valueOffset the offset of the first byte in the source 
     * TLV element
     * @param dstBuffer a reference to the destination buffer
     * @param dstOffset the position in the destination buffer
     * @param dstLength the data length to be copied
     *
     * @return <code>dstOffset + dstLength</code>
     *
     * @exception NullPointerException if <code>dstBuffer</code> is 
     * <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if findAndCopyValue 
     * would cause access of data outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      TLV element
     *      <li><code>OUT_OF_TLV_BOUNDARIES</code> if:
     *        <ul> 
     *              <li><code>valueOffset</code> parameter is negative or
     *              <li><code>valueOffset + dstLength</code> is greater 
     *              than the length of the current TLV
     *        </ul> 
     *      <li><code>BAD_INPUT_PARAMETER</code> if an input parameter 
     *      is not valid (e.g. occurrence = 0)</ul>
     */
    public short findAndCopyValue(byte tag,
                                byte occurrence,
                                short valueOffset,
                                byte[] dstBuffer,
                                short dstOffset,
                                short dstLength) 
                                throws 	NullPointerException,
                                ArrayIndexOutOfBoundsException,
                                ToolkitException {
                                                			  
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getTLVOffset(buffer, tag, Lc, occurrence);
        
        if (TLVOffset >= Lc) {
            // TLV not found
            ToolkitException.throwIt(ToolkitException.UNAVAILABLE_ELEMENT);
        }
        // this is the current TLV
        currentTLVOffset = TLVOffset;
        short length = buffer[(short)(TLVOffset+1)];
        if ((valueOffset < 0) || (short)(valueOffset + dstLength) > length) {
            ToolkitException.throwIt(ToolkitException.OUT_OF_TLV_BOUNDARIES);
        }
        Util.arrayCopy(buffer, (short)(TLVOffset+2+valueOffset), 
                        dstBuffer, dstOffset, dstLength);
        return (short)(dstOffset + length);

    }

    /**
     * Looks for the first occurrence of a TLV element 
     * from beginning of a TLV
     * list and compare its value with a buffer.
     * If no TLV element is found, the 
     * <code>UNAVAILABLE_ELEMENT</code> exception is thrown.
     * If the method is successful then the corresponding TLV 
     * becomes current, else no TLV is selected.
     * This search method is Comprehension Required flag independent.
     *
     * <p>
     * Notes:<ul>
     * <li><em>If </em><code>compareOffset</code><em> parameter is 
     * negative or </em><code>compareOffset</code>
     * <em> is greater than </em><code>compareBuffer.length</code><em>, 
     * the length of the </em><code>compareBuffer</code>
     * <em> array an </em><code>ArrayIndexOutOfBoundsException</code><em> 
     * exception is thrown and no find is performed.</em> 
     * </ul> 
     *
     * @param tag the tag of the TLV element to search
     * @param compareBuffer a reference to the comparison buffer
     * @param compareOffset the position in the comparison buffer
     *
     * @return the result of the comparison as follows: <ul>
     *      <li><code>0</code> if identical
     *      <li><code>-1</code> if the first miscomparing byte in simple 
     *      TLV is less than that in <code>compareBuffer</code>,
     *      <li><code>1</code> if the first miscomparing byte in simple 
     *      TLV is greater than that in <code>compareBuffer</code>.</ul>
     *
     * @exception NullPointerException if <code>compareBuffer</code> is 
     * <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if findAndCompareValue 
     * would cause access of data outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      TLV element</ul>
     */
    public byte findAndCompareValue(byte tag,
                                    byte[] compareBuffer,
                                    short compareOffset) 
                                    throws	NullPointerException,
                                    ArrayIndexOutOfBoundsException,
                                    ToolkitException {
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getTLVOffset(buffer, tag, Lc, (byte)1);
        
        if (TLVOffset >= Lc) {
            // TLV not found
            ToolkitException.throwIt(ToolkitException.UNAVAILABLE_ELEMENT);
        }
        // this is the current TLV
        currentTLVOffset = TLVOffset;
        short length = buffer[(short)(TLVOffset+1)];
        return Util.arrayCompare(buffer, (short)(TLVOffset+2), compareBuffer,
                                compareOffset, length);
    }

    /**
     * Looks for the indicated occurrence of a TLV element from the 
     * beginning of a TLV list and compare its value with a buffer.
     * If no TLV element is found, the <code>UNAVAILABLE_ELEMENT</code> 
     * exception is thrown.
     * If the method is successful then the corresponding TLV becomes 
     * current, else no TLV is selected.
     * This search method is Comprehension Required flag independent.
     *
     * <p>
     * Notes:<ul>
     * <li><em>If </em><code>compareOffset</code><em> or 
     * </em><code>compareLength</code><em> parameter is 
     * negative an </em><code>ArrayIndexOutOfBoundsException</code>
     * <em> exception is thrown and no find and compare is performed.</em>
     * <li><em>If </em><code>compareOffset+compareLength</code><em> is 
     * greater than </em><code>compareBuffer.length</code><em>, the length
     * of the </em><code>compareBuffer</code><em> array an 
     * </em><code>ArrayIndexOutOfBoundsException</code><em> exception 
     * is thrown and no find and compare is performed.</em> 
     * </ul> 
     *
     * @param tag the tag of the TLV element to search
     * @param occurrence the occurrence number of the TLV element 
     * (1 for the first, 2 for the second...)
     * @param valueOffset the offset of the first byte in the source 
     * TLV element
     * @param compareBuffer a reference to the comparison buffer
     * @param compareOffset the position in the comparison buffer
     * @param compareLength the length to be compared
     *
     * @return the result of the comparison as follows: <ul>
     *      <li><code>0</code> if identical
     *      <li><code>-1</code> if the first miscomparing byte in 
     *      simple TLV is less than that in <code>compareBuffer</code>,
     *      <li><code>1</code> if the first miscomparing byte in simple 
     *      TLV is greater than that in <code>compareBuffer</code>.</ul>
     *
     * @exception NullPointerException if <code>compareBuffer</code> 
     * is <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if findAndCompareValue 
     * would cause access of data outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      TLV element
     *      <li><code>OUT_OF_TLV_BOUNDARIES</code> if:
     *        <ul> 
     *              <li><code>valueOffset</code> parameter is negative or
     *              <li><code>valueOffset + compareLength</code> is greater 
     *              than the length of the current TLV
     *        </ul> 
     *      <li><code>BAD_INPUT_PARAMETER</code> if an input parameter is 
     *       not valid (e.g. occurrence = 0)</ul>
     */
    public byte findAndCompareValue(byte tag,
                                byte occurrence,
                                short valueOffset,
                                byte[] compareBuffer,
                                short compareOffset,
                                short compareLength) 
                                throws NullPointerException,
                                ArrayIndexOutOfBoundsException,
                                ToolkitException {
        byte[] buffer = getAPDUBuffer();
        short Lc = (short)(buffer[OFFSET_LC] & 0xFF);
        short TLVOffset = getTLVOffset(buffer, tag, Lc, occurrence);
        
        if (TLVOffset >= Lc) {
            // TLV not found
            ToolkitException.throwIt(ToolkitException.UNAVAILABLE_ELEMENT);
        }
        // this is the current TLV
        currentTLVOffset = TLVOffset;
        short length = buffer[(short)(TLVOffset+1)];
        if ((valueOffset < 0) || (short)
	    (valueOffset + compareLength) > length) {
            ToolkitException.throwIt(ToolkitException.OUT_OF_TLV_BOUNDARIES);
        }
        return Util.arrayCompare(buffer, (short)(TLVOffset+2+valueOffset), 
                                compareBuffer, compareOffset, compareLength);
    }

    
    /**
     * Helper method for <code>findAndCompareValue</code>.
     * @param buffer APDU buffer
     * @param tag What tag we are finding
     * @param Lc Length of command data
     * @param occurrence The occurrence number of the TLV element
     * @return Offset of the tag
     */
    protected short getTLVOffset(byte[] buffer, byte tag, 
                                short Lc, short occurrence) {
        byte count = 0; // count of occurances
        short offset = firstTLVOffset;
        tag = (byte)(tag & 0x7F);
        while (offset < Lc) {
            if ((byte)(buffer[offset] & 0x7F) == tag) {
                count++;
                if (count != occurrence) {
                    continue;
                }
                return offset;
            } else {
                // advance to next TLV
                offset++;
                short length = buffer[offset];
                offset = (short)(offset + length + 1);
            }
        }
        return offset; // not found
    }
    
    /**
     * Helper method for <code>getValue...</code> and 
     * <code>copyValue...</code>.
     * @param buffer APDU buffer
     * @param Lc Length of command data
     * @return Offset of the tag
     */
    protected short getLastTLVOffset(byte[] buffer, short Lc) {
        short offset = firstTLVOffset;
        short lastTLVOffset;
        do {
            lastTLVOffset = offset;
            // advance to next TLV
            offset++;
            short length = buffer[offset];
            offset = (short)(offset + length + 1);
        } while (offset < Lc);
        return lastTLVOffset;
    }

    /**
     * Copies the APDUBuffer content into provided buffer.
     * @param buffer The buffer
     * @return Provided buffer filled with APDUBuffer content
     */
    public static byte[] getAPDUBuffer(byte[] buffer) {
        short len = SATAccessor.getAPDUBufferLength();
        
        for (short i = 0; i < len; i++) {
            buffer[i] = SATAccessor.getAPDUBufferByte(i);
        }
        return buffer;
    }
    
    /**
     * Copies the APDUBuffer content into current buffer.
     * @return apdu buffer
     */
    public static byte[] getAPDUBuffer() {
        return getAPDUBuffer(currentTI.getAPDUBuffer());
    }
    
    /**
     * Copies content of provided buffer into the APDUBuffer.
     * @param buffer The buffer
     * @param length Length of the buffer
     */
    public static void setAPDUBuffer(byte[] buffer, short length) {
        
        for (short i = 0; i < length; i++) {
            SATAccessor.setAPDUBufferByte(i, buffer[i]);
        }
    }
    
    /**
     * Copies content of current buffer into the APDUBuffer.
     * @param length Length of the current buffer
     */
    public static void setAPDUBuffer(short length) {
        setAPDUBuffer(currentTI.getAPDUBuffer(), length);
    }
    
    /**
     * Sets the data in the out buffer.
     * @param length Length of data
     */
    public static void setOutBufferData(short length) {
        setAPDUBuffer(length);
        SATAccessor.setOutBufferData(length);
    }
}
