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
 
package com.sun.satsa.satapplet;

import javacard.framework.*;
import sim.toolkit.*;

/**
 * An example SAT applet to demonstrate how an SAT applet can be written.
 */
public class SATApplet extends SATBaseApplet {
                            
    /** Instance to toolkit registry which is used to register for events */
    private ToolkitRegistry reg;
    
    /**
     * Constructor.
     */
    private SATApplet() {
        // have to call register before getting the toolkit registry 
        // object
        register();
        // EVENT_UNFORMATTED_SMS_PP_ENV
        // register to the SIM Toolkit Framework
        reg = ToolkitRegistry.getEntry();
        // register to the EVENT_UNFORMATTED_SMS_PP_ENV
        reg.setEvent(EVENT_UNFORMATTED_SMS_PP_ENV);
    }

    /**
     * Applet's install method. Creates an instance of SATApplet.
     * @param bArray parameters array
     * @param bOffset offset in the parameters array
     * @param bLength length of parameters
     */
    public static void install(byte[] bArray, short bOffset, byte bLength) {
        new SATApplet();
    }

    /**
     * Process method called by GSMApplet when an evelope is received.
     * @param event Event which should be processed
     */
    public void processToolkit(byte event) {
        EnvelopeHandler          envHdlr = EnvelopeHandler.getTheHandler();
        EnvelopeResponseHandler  evpRspHdlr = 
                                    EnvelopeResponseHandler.getTheHandler();
        switch (event) {
            case EVENT_UNFORMATTED_SMS_PP_ENV:
                byte result = envHdlr.findTLV(TAG_SMS_TPDU, (byte)1);
                short length = envHdlr.getLength();
                byte[] rspBuffer = new byte[length];
                short rspLength = envHdlr.findAndCopyValue(TAG_SMS_TPDU, 
                                                    rspBuffer, (short)0);
                evpRspHdlr.appendArray(rspBuffer, (short)0, rspLength);
                evpRspHdlr.post(rspBuffer[(short)(length - 1)]);    
                break;
        }
    }
}
