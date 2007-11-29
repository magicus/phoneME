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

import sim.toolkit.AccessSAT;
import sim.toolkit.ToolkitInterface;
import sim.toolkit.ToolkitException;
import javacard.framework.AID;
import javacard.framework.APDU;
import javacard.framework.Util;
import javacard.framework.JCSystem;

/**
 * A class that implements the AccessSAT interface which
 * allows the toolkit classes to access the APDUBuffer
 * and do other functions on objects in the GSMApplet's context.
 */
public class AccessSATImpl implements AccessSAT {
    /** Maximum length of in and out buffers */
    static final short MAX_BUFFER_LENGTH = (short)64;    
    
    /** Maximum number of listeners allowed */
    static final byte MAX_LISTENERS = (byte)16;
    
    /** 
     * Size used by the GSM applet to send data when 
     * responding to GET RESPONSE
     */
    short outDataSize;
    
    /** Buffer that holds the data received from the terminal. */
    public byte[] inBuffer;
    
    /** Buffer that holds the data to be sent to the terminal. */
    public byte[] outBuffer;
    
    /** Buffer that holds the APDU data. */
    private byte[] apduBuffer;
    /** Length of APDU buffer. */
    private short apduBufferLength = 0;
    
    /** A list of listeners for EVENT_SMS_PP_DATA_DOWNLOAD. */
    public ToolkitInterface[] tiList;
    
    /**
     * Constructor
     */
    AccessSATImpl() {
        inBuffer = JCSystem.makeTransientByteArray(MAX_BUFFER_LENGTH, 
                                              JCSystem.CLEAR_ON_DESELECT);
        outBuffer = JCSystem.makeTransientByteArray(MAX_BUFFER_LENGTH, 
                                              JCSystem.CLEAR_ON_DESELECT);
        apduBuffer = JCSystem.makeTransientByteArray(MAX_BUFFER_LENGTH, 
                                              JCSystem.CLEAR_ON_DESELECT);
        tiList = new ToolkitInterface[MAX_LISTENERS];
    }
    
    /**
     * Resets the buffers and fields when a new envelope is received
     */
    void resetBuffers() {
        Util.arrayFillNonAtomic(inBuffer, (short)0, 
                                MAX_BUFFER_LENGTH, (byte)0);
        Util.arrayFillNonAtomic(outBuffer, (short)0, 
                                MAX_BUFFER_LENGTH, (byte)0);
        outDataSize = (short)0;         
        apduBufferLength = (short)0;
                       
    }
    
    /**
     * Returns the APDUBuffer.
     * @return apdu buffer
     */
    public byte[] getAPDUBuffer() {
        return apduBuffer;
    }
    
    /**
     * Sets the APDUBuffer.
     * @param buffer apdu buffer
     * @param length length of apdu buffer
     */
    public void setAPDUBuffer(byte[] buffer, short length) {
        Util.arrayCopyNonAtomic(buffer, (short)0, apduBuffer, (short)0, length);
        Util.arrayCopyNonAtomic(buffer, (short)0, inBuffer, (short)0, length);
        apduBufferLength = length;
    }
    
    /**
     * Gets one byte from the APDUBuffer.
     * @param index Index of requested byte in the buffer
     * @return requested byte
     */
    public byte getAPDUBufferByte(short index) {
        if (index >= MAX_BUFFER_LENGTH || index >= apduBufferLength) {
            return (byte)0;
        } else {
            return apduBuffer[index];
        }
    }
    
    /**
     * Sets one byte from the APDUBuffer.
     * @param index Index of byte in the buffer
     * @param value The value to be set
     */
    public void setAPDUBufferByte(short index, byte value) {
        if (index < MAX_BUFFER_LENGTH) {
            if (index > apduBufferLength) {
                apduBufferLength = index;
            }
            apduBuffer[index] = value;
        }
    }
    
    /**
     * Gets the length of the APDUBuffer.
     * @return requested length
     */
    public short getAPDUBufferLength() {
        return apduBufferLength;
    }
    
    /**
     * Gets the maximum length of the APDUBuffer.
     * @return requested length
     */
    public short getAPDUBufferMax() {
        return MAX_BUFFER_LENGTH;
    }
    
    /**
     * Sets the data in the out buffer.
     * @param length length of data
     */
    public void setOutBufferData(short length) {
        byte[] buffer = apduBuffer;
        outDataSize = Util.arrayCopy(buffer, (short)0, 
                                                outBuffer, 
                                                outDataSize,
                                                length);
        // restore the bytes from the original command APDU in
        // the APDU buffer because the data recieved in the 
        // envelope is suppose to be available while
        // in processToolkit method
        Util.arrayCopy(inBuffer, (short)0, buffer, 
                        (short)0, (short)length);
    }

    /**
     * Returns the length of Data that has been set in the out buffer.
     * @return length of data
     */
    public short getOutDataLength() {
        return outDataSize;
    }

    /**
     * This method is called by GSMApplet to set the data in the APDU buffer
     * so that it can be sent to the terminal in response to a 
     * GET RESPONSE APDU.
     */
    public void setOutgoingAPDU() {
        // this will be called when there is data to be sent
        byte[] buffer = apduBuffer;
        Util.arrayCopy(outBuffer, (short)0, buffer, (short)0, outDataSize);
    }
    
    /**
     * Sets the event listener applet.
     * @param aid applet AID
     */
    public void setEventListener(AID aid) {
        ToolkitInterface ti = (ToolkitInterface)
                    JCSystem.getAppletShareableInterfaceObject(aid, (byte)0);
        if (ti == null) {
            ToolkitException.throwIt(ToolkitException.BAD_INPUT_PARAMETER);
        }
        // if listener hasn't already registered, register it for this event
        if (findListener(ti) == (byte)-1) {
            for (short i = 0; i < MAX_LISTENERS; i++) {
                if (tiList[i] == null) {
                    tiList[i] = ti;
                    break;
                }
            }
        }
    }
    
    /**
     * Removes the event listener from the list of listeners.
     * @param aid applet AID
     */
    public void clearEventListener(AID aid) {
        ToolkitInterface ti = (ToolkitInterface)
                    JCSystem.getAppletShareableInterfaceObject(aid, (byte)0);
        if (ti == null) {
            return;
        }
        byte index = findListener(ti);
        if (index != (byte)-1) {
            // remove listener
            tiList[index] = null;
        }
    }
    
    
    /**
     * Returns true if the applet corresponding to the AID passed to this 
     * method is found in the list of listeners.
     * @param aid applet AID
     * @return true if the applet is a listener and false otherwise
     */
    public boolean isEventListenerSet(AID aid) {
        ToolkitInterface ti = (ToolkitInterface)
                    JCSystem.getAppletShareableInterfaceObject(aid, (byte)0);
        if (ti == null) {
            return false;
        }
        byte index = findListener(ti);
        if (index != (byte)-1) {
            return true;
        }
        return false;
    }
    
    /**
     * Finds a listener in the list of listener 
     * @param ti toolkit interface
     * @return index in the listener table
     */
    private byte findListener(ToolkitInterface ti) {
        for (byte i = 0; i < MAX_LISTENERS; i++) {
            if (tiList[i] != null) {
                if (tiList[i].equals(ti)) 
		    return i;
            }            
        }
        return (byte)-1;
    }
}
