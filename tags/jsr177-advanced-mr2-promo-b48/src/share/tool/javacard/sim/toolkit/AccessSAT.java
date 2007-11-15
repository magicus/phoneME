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

package sim.toolkit;

import javacard.framework.*;

/**
 * The interface of SAT Accessor.
 */
public interface AccessSAT extends Shareable {
    /**
     * Gets the length of the APDUBuffer.
     * @return requested length
     */
    public short getAPDUBufferLength();
    /**
     * Gets the maximum length of the APDUBuffer.
     * @return requested length
     */
    public short getAPDUBufferMax();
    /**
     * Gets one byte from the APDUBuffer.
     * @param index Index of requested byte in the buffer
     * @return requested byte
     */
    public byte getAPDUBufferByte(short index);
    /**
     * Sets one byte from the APDUBuffer.
     * @param index Index of byte in the buffer
     * @param value The value to be set
     */
    public void setAPDUBufferByte(short index, byte value);
    /**
     * Sets the data in the out buffer.
     * @param length length of data
     */
    public void setOutBufferData(short length);
    /**
     * Returns the length of Data that has been set in the out buffer.
     * @return length of data
     */
    public short getOutDataLength();
    /**
     * Sets the event listener applet.
     * @param aid applet AID
     */
    public void setEventListener(AID aid);
    /**
     * Removes the event listener from the list of listeners.
     * @param aid applet AID
     */
    public void clearEventListener(AID aid);
    /**
     * Returns true if the applet corresponding to the AID passed to this 
     * method is found in the list of listeners.
     * @param aid applet AID
     * @return true if the applet is a listener and false otherwise
     */
    public boolean isEventListenerSet(AID aid);
}
