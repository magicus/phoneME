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
 * The EnvelopeResponseHandler class contains basic methods to handle the 
 * <b>Envelope response
 * </b>data field. This class will be used by the Toolkit applet in order to
 * edit the response to current Envelope command. No constructor is available
 * for the Toolkit applet. The EnvelopeResponseHandler class is a <b>Temporary 
 * JCRE Entry Point Object</b>. The only way to get a EnvelopeResponseHandler 
 * reference is through the <code>getTheHandler()</code> static method.<p>
 *
 * @version 8.3.0
 * 
 * @see ViewHandler
 * @see EditHandler
 * @see EnvelopeHandler
 * @see ToolkitException
 */
public final class EnvelopeResponseHandler extends EditHandler {
    // private static EnvelopeResponseHandler theEnvelopeResponseHandler;
    boolean handlerAvailable;
    boolean postCalled;
    public static byte status;
    private short outDataSize;
    // ------------------------------- Constructors ---------------------------
    /**
     * Constructor
     */
    private EnvelopeResponseHandler() {
    }


    // ------------------------------- Public methods -------------------------
    /**
     * Returns the single system instance of the EnvelopeResponseHandler class. 
     * The applet shall get the reference of the handler at its triggering, 
     * the beginning of the processToolkit method.
     *
     * @return reference of the system instance
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy.</ul>
     */
    public static EnvelopeResponseHandler getTheHandler()
	throws ToolkitException {
        EnvelopeResponseHandler theEnvelopeResponseHandler = 
	    new EnvelopeResponseHandler();
        theEnvelopeResponseHandler.outDataSize = 0;
        theEnvelopeResponseHandler.handlerAvailable = false;
        theEnvelopeResponseHandler.postCalled = false;
        return theEnvelopeResponseHandler;
    }

    /**
     * Pepares the Envelope response. Should be used with Envelope SMS-PP Data
     * Download.
     *
     * @param statusType the status to be sent to the ME 
     * (SW1_RP_ACK or SW1_RP_ERROR)
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy </ul>
     */
    public void post(byte statusType) throws ToolkitException {
        if (postCalled) {
            ToolkitException.throwIt(ToolkitException.HANDLER_NOT_AVAILABLE);
        }
        postCalled = true;
        status = statusType;
    }

    /**
     * Prepare the Envelope response in a BER TLV structure. Should be used 
     * with Envelope Call Control by SIM or MO Short Message Control by SIM.
     * The tag value is to be used to set the Result for Call Control and 
     * MO Short Message Control by the SIM.
     *
     * @param statusType the status to be sent to the ME 
     * (SW1_RP_ACK or SW1_RP_ERROR)
     * @param tag the BER Tag to be used at the beginning of the SIMPLE_TLV 
     * list.
     *
     * @exception ToolkitException with the following reason codes: <ul>
     *      <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy</ul>
     */
    public void postAsBERTLV(byte statusType, byte tag) 
                                throws ToolkitException {
        if (postCalled) {
            ToolkitException
		.throwIt(ToolkitException.HANDLER_NOT_AVAILABLE);
        }
        postCalled = true;
        status = statusType;
    }
    
    /**
     * Returns the maximum size of the Simple TLV list managed by the handler.
     * @return size in bytes
     * @exception ToolkitException with the following reason codes: <ul>
     *    <li><code>HANDLER_NOT_AVAILABLE</code> if the handler is busy</ul>
     */
    public short getCapacity() throws ToolkitException {
        return (short)0;
    }
    
}
