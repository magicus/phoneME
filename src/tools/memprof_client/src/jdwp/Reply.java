/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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


package com.sun.cldchi.tools.memoryprofiler.jdwp;

import java.util.Vector;

/**
 * This class represents a JDWP reply packet. This class is widely
 * used in all parts of KJDB. A typical use of tyhis class is as follows:
 * <ul>
 * <li> Prepare JDWP command (see <code>jdwp.Command</code>) for details
 * <li> Execute the command using <code>BackEndTest.checkReplyF()</code>
 * method and receive the JDWP reply packet
 * <li> Use <code>resetDataParser()</code> method before starting reading
 * packet's data
 * <li> Read the packet's data using <code>get</code> <code>ByteBuffer</code>
 * and <code>Packet</code> methods
 * </ul> 
 * 
 * @see jdwp.Command
 * @see jdwp.BackEndTest#checkReplyF(jdwp.Command)
 * @see jdwp.Packet
 * @see jdwp.Packet#resetDataParser()
 * @see jdwp.ByteBuffer
 */

class Reply extends Packet implements VMReply {

    /**
     * Error code constant that indicates that no error occured.
     */
    public final static int errOk		 		= 0x0;
    
    /** 
     * Error code constant that indicates that the packet has wrong size.
     */
    public final static int errWrongPacketSize	= 0x400;
    
    /** 
     * Error code constant that indicates that JDWP reply packet is not
     * received as expected.
     */
    public final static int errNotAvailable		= 0x401;
    
    /** 
     * JDWP Event/Composite command number.
     */
    public final static int errEvent			= 0x4064;

    /**
     * Returns value of the "error code" field of JDWP reply packet.
     *
     * @return a error code of JDWP reply
     */
   	public int getErrorCode() {
            int err = 0;
            try {
            	err = (int) getID(ErrorCodeOffset, 2);
            }
            catch (BoundException e) {};
            return err;
    }

    /**
     * Sets value of the "error code" field of JDWP reply packet.
     *
     * @param err a error code of JDWP reply that should be set
     */

   	public void setErrorCode(long err) {
   		try {
   			putID(ErrorCodeOffset, err, 2);
   		}
   		catch (BoundException e) {};
    }

    /**
     * This method
     * prepares a JDWP reply packet with specified error code.
     *
     * @param ErrorCode a error code of the JDWP reply
     * @return a JDWP reply with the specified error code
     */

    static Reply Error(int ErrorCode) {

		Reply r = new Reply();
		r.setLength();
		r.setID(0);
		r.setFlags(flReply);
		r.setErrorCode(ErrorCode);
		return r;
    }

    /**
     * Returns true if this packet's error code equals zero.
     *
     * @return <code>true</code> if the JDWP reply has error code
     * <code>NONE</code> and <code>false</code> otherwise
     */
    public boolean ok() {
		return (getErrorCode() == errOk);
    }

    /**
     * Returns a string representation of the reply packet. This method
     * is used by <code>BackEndTest</code> to print all the
     * replies that were not requested by other parts of code. It's useful
     * for localizing problems.
     *
     * @return a string representation of the reply packet
     *
     * @see jdwp.BackEndTest#printReplies()
     */
    public String toString() {
        String s;
        if (getFlags() == 0)
            s = "command    ";
        else
            s = "error code ";
        return "length     " + Tools.Hex(length(), 8) + "\n" +
            "id         " + Tools.Hex(getID(), 8) + "\n" +
            "flags      " + Tools.Hex(getFlags(), 2) + "\n" +
            s + Tools.Hex(getErrorCode(), 4) + "\n" +
            super.toString(PacketHeaderSize);
    }
}
