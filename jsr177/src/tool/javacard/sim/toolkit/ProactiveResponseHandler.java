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

//-----------------------------------------------------------------------------
// IMPORTS
//-----------------------------------------------------------------------------

/**
 *
 * The ProactiveResponseHandler class contains basic methods to handle the 
 * <b>Terminal
 * Response</b> data field. This class will be used by the Toolkit applet to
 * get the response to the Proactive commands. No constructor is available for
 * the Toolkit applet. The ProactiveResponseHandler class is a <b>Temporary 
 * JCRE Entry Point Object</b>. The only way to get a ProactiveResponseHandler 
 * reference is through the <code>getTheHandler()</code> static method.<p>
 *
 * Example of use:<pre><code>
 *
 * private byte[] data;
 * data = new byte[32];                 // build a buffer
 *
 * ProactiveResponseHandler ProRespHdlr;            // get the system instance
 * ProRespHdlr = ProactiveResponseHandler.getTheHandler();
 *
 * // get General Result
 * byte result = ProRespHdlr.getGeneralResult();
 * 
 * // look for Device Identities
 * respHdlr.findTLV(TAG_DEVICE_IDENTITIES, 1);  
 * 
 * // read Device Identities
 * byte sourceDev = ProRespHdlr.getValueByte((short)0);
 * byte destinDev = ProRespHdlr.getValueByte((short)1);
 *
 * // look for Text String element
 * if (ProRespHdlr.findTLV(TAG_TEXT_STRING, (byte)1) == TLV_FOUND_CR_SET) {
 *     if ((short len = ProRespHdlr.getValueLength()) > 1) {
 *         // not empty string: to be copied
 *         ProRespHdlr.copyValue((short)1, data, (short)0, (short)(len - 1));
 *     }
 * }
 * </code></pre>
 *
 * @version 8.3.0
 * @see ViewHandler
 * @see ProactiveHandler
 * @see ToolkitException
 */
public final class ProactiveResponseHandler extends ViewHandler {

    // ------------------------------- Constructors ---------------------------
    /**
     * Constructor
     */
    private ProactiveResponseHandler() {
    }


    // ------------------------------- Public methods -------------------------
    /**
     * Returns the single system instance of the
     * ProactiveResponseHandler class.  The applet shall get the
     * reference of the handler at its triggering, the beginning of
     * the processToolkit method.
     *
     * @return reference of the system instance
     *
     * @exception ToolkitException with the following reason codes:
     *      <ul> <li><code>HANDLER_NOT_AVAILABLE</code> if the handler
     *      is busy.</ul>
     */
    public static ProactiveResponseHandler getTheHandler() 
                                            throws ToolkitException {
		    return null;
    }

    /**
     * Returns the general result byte of the Proactive command.
     * If the element is available it becomes the TLV selected.
     *
     * @return general result of the command (first byte of Result TLV in 
     * Terminal Response)
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      Result TLV element
     *      <li><code>OUT_OF_TLV_BOUNDARIES</code> if the general result 
     *      byte is missing in the Result Simple TLV</ul>
     */
    public byte getGeneralResult() throws ToolkitException {
        return 0;
    }

    /**
     * Returns the length of the additional information field from the first
     * Result TLV element of the current response data field.
     * If the element is available it becomes the TLV selected.
     *
     * @return additional information length
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable Result 
     *      TLV element</ul>
     */
    public short getAdditionalInformationLength() throws ToolkitException {
        return 0;
    }

    /**
     * Copies a part of the additional information field from the first Result
     * TLV element of the current response data field.
     * If the element is available it becomes the TLV selected.
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
     * </em><code>ArrayIndexOutOfBoundsException</code><em> exception is thrown
     * and no copy is performed.</em> 
     * </ul> 
     *
     * @param dstBuffer a reference to the destination buffer
     * @param dstOffset the position in the destination buffer
     * @param dstLength the data length to be copied
     *
     * @return <code>dstOffset+dstLength</code>
     *
     * @exception NullPointerException if <code>dstBuffer</code> is 
     * <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if copyAdditionalInformation 
     * would cause access of data outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      Result TLV element
     *	    <li><code>OUT_OF_TLV_BOUNDARIES</code> if <code>dstLength</code> 
     *      is greater than the value field of the available TLV</ul>
     */
    public short copyAdditionalInformation(byte[] dstBuffer, 
					   short dstOffset, 
					   short dstLength)
	throws NullPointerException,
	       ArrayIndexOutOfBoundsException,
	       ToolkitException {
        return 0;
    }
    
    /**
     * Returns the item identifier byte value from the first Item 
     * Identifier TLV element of the current response data field.
     * If the element is available it becomes the TLV selected.
     *
     * @return item identifier
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable 
     *      Item Identifier TLV element
     *	    <li><code>OUT_OF_TLV_BOUNDARIES</code> if the item identifier 
     *      byte is missing in the Item Identifier Simple TLV</ul>
     */
    public byte getItemIdentifier() throws ToolkitException {
        return 0;
    }

    /**
     * Returns the text string length value from the first Text String TLV
     * element of the current response data field. The Data Coding Scheme byte
     * is not taken into account.
     * If the element is available it becomes the TLV selected.
     *
     * @return text string length
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable Text 
     *      String TLV element</ul>
     */
    public short getTextStringLength() throws ToolkitException {
        return 0;
    }

    /**
     * Returns the data coding scheme byte from the first Text String TLV
     * element of the current response data field.
     * If the element is available it becomes the TLV selected.
     *
     * @return text string coding scheme
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable Text
     *      String TLV element
     *	    <li><code>OUT_OF_TLV_BOUNDARIES</code> if the Text String TLV is 
     *      present with a length 
     *	    of 0 (no DCS byte)</ul>
     */
    public byte getTextStringCodingScheme() throws ToolkitException {
        return 0;
    }

    /**
     * Copies the text string value from the first Text String TLV element 
     * of the
     * current response data field. The Data Coding Scheme byte is not copied.
     * If the element is available it becomes the TLV selected.
     *
     * @param dstBuffer a reference to the destination buffer
     * @param dstOffset the position in the destination buffer
     *
     * @return <code>dstOffset</code> + length of the copied value
     *
     * @exception NullPointerException if <code>dstBuffer</code> is 
     * <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if <code>dstOffset or 
     * dstOffset + (length of the TextString to be copied, 
     * without the Data Coding Scheme included)</code>, as specified for 
     * the returned value, would cause access outside array bounds.
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>UNAVAILABLE_ELEMENT</code> in case of unavailable Text 
     *      String TLV element</ul>
     */
    public short copyTextString(byte[] dstBuffer, short dstOffset) 
	throws 	NullPointerException,
		ArrayIndexOutOfBoundsException,
		ToolkitException {
        return 0;
    }
}
