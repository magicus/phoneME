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

package com.sun.satsa.gsmapplet;

import sim.toolkit.*;
import javacard.framework.*;

/**
 * This applets is suppose to simulate the working of the main GSM applet in
 * (U)SIM that controls access to all other applets. No other applet is 
 * selected directly but is invoked by this applet on basis of events.
 */
public class GSMApplet extends Applet implements ToolkitConstants {
    /** 
     * Instruction byte for an Envelope. Only Envelopes are supported at 
     * this time 
     */
    private static final byte ENVELOPE_INS = (byte)0xc2;
    
    /** Instruction byte value for GET RESPONSE APDU. */
    private static final byte GET_RESPONSE_INS = (byte)0xc0;
    
    /** 
     * A field to hold the instance of SATAccessor which is used by the
     * toolkit classes to set and get the APDU buffer.
     */
    AccessSATImpl SATAccessor;
    
    /** 
     * Constructor.
     */
    private GSMApplet() {
        register();
        SATAccessor = new AccessSATImpl();
        AID aid = JCSystem.getAID();
        ViewHandler.SATAccessor = (AccessSAT)
                    JCSystem.getAppletShareableInterfaceObject(aid, (byte)0);
    }

    /**
     * Applet's install method. Creates an instance of GSMApplet
     * @param bArray parameters array
     * @param bOffset offset in the parameters array
     * @param bLength length of parameters
     */
    public static void install(byte[] bArray, short bOffset, byte bLength) {
        new GSMApplet();
    }
    
    /**
     * This method is called by JCRE before it tries to uninstall this applet.
     */
    public void uninstall() {
        ViewHandler.SATAccessor = null;
    }
    
    /**
     * Returns shareable interface object to SIM Framework.
     *
     * @param clientAID - aid of SIM Framework
     * @param parameter
     * @return The current applet instance
     */
    public Shareable getShareableInterfaceObject(AID clientAID, 
                                                byte parameter) {
        return (Shareable)SATAccessor;
    }
    
    /**
     * Process method that processes all the APDUs received from the terminal.
     * @param apdu APDU that has been received from the terminal
     */
    public void process(APDU apdu) {
        byte[] buffer = apdu.getBuffer();

        // clear the channel information 
        buffer[ISO7816.OFFSET_CLA] = (byte)(buffer[ISO7816.OFFSET_CLA] 
                                            & (byte)0xFC);

        // check SELECT APDU command
        if ((buffer[ISO7816.OFFSET_CLA] == 0) &&
            (buffer[ISO7816.OFFSET_INS] == (byte)(0xA4)))
            return;

        switch (buffer[ISO7816.OFFSET_INS]) {
            case ENVELOPE_INS:
                short incomingData = apdu.setIncomingAndReceive();
                SATAccessor.resetBuffers();
                SATAccessor.setAPDUBuffer(buffer, (short)(incomingData + 5));
                
                for (byte i = 0; i < AccessSATImpl.MAX_LISTENERS; i++) {
                    ToolkitInterface ti = SATAccessor.tiList[i];
                    if (ti == null) {
                        continue;
                    }
                    ViewHandler.currentTI = ti;
                    ti.processToolkit(EVENT_UNFORMATTED_SMS_PP_ENV);
                    if (EnvelopeResponseHandler.status != 0) {
                        short dataLength = SATAccessor.getOutDataLength();
                        short st = (short)
                        ((EnvelopeResponseHandler.status << 8) | dataLength);
                        ISOException.throwIt(st);
                    } 
                }
                break;
            case GET_RESPONSE_INS:
                SATAccessor.setAPDUBuffer(buffer, (short)5);
                short dataLength = SATAccessor.getOutDataLength();
                apdu.setOutgoing();
                apdu.setOutgoingLength(dataLength);
                SATAccessor.setOutgoingAPDU();
                Util.arrayCopy(SATAccessor.getAPDUBuffer(), (short)0, 
                        buffer, (short)0, dataLength);
                apdu.sendBytes((short)0, dataLength);
                break;
            default:        
        ISOException.throwIt(ISO7816.SW_INS_NOT_SUPPORTED);
        }
    }
}
