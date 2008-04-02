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

/**
 * This class encapsulates JDWP command. Its superclass
 * is <code>Packet</code> so it's based on <code>ByteBuffer</code>.
 * The class allows to set and get JDWP command number and automatically
 * assign command ID. The typical use of <code>Command</code> class is
 * as follows:
 * <ul>
 * <li> Create a new command object with specified command number
 * <li> Fill command's data using standard <code>ByteBuffer</code>
 * and <code>Packet</code> methods (for example, <code>addInt()</code> or
 * <code>addReferenceTypeID()</code>)
 * <li> Execute command using <code>BackEndTest.checkReplyF()</code>,
 * <code>BackEndTest.sendCommand()</code> or
 * <code>BackEndTest.checkReply()</code>
 * <li> Work with received <code>Reply</code> object if needed
 * </ul>
 *
 * @see jdwp.Packet
 * @see jdwp.ByteBuffer
 * @see jdwp.BackEndTest#checkReplyF(jdwp.Command)
 * @see jdwp.BackEndTest#sendCommand(jdwp.Command)
 * @see jdwp.BackEndTest#checkReply(jdwp.Command)
 * @see jdwp.Reply
 *
 */
class Command extends Packet {

    /**
     * ID of next command. Each JDWP command must have
     * unique ID. The simplest way (that is used here) for generating
     * these IDs is a incremental counter. So this variable is incremented
     * after creating of each command and contains ID of next command.
     */
    private static int nextID = 1;

    /**
     * Creates a new <code>Command</code> object, assign an unique ID,
     * the specified command number and sets flags to <code>flNoFlags</code>.
     * Command number is two-byte integer where hi-order byte specifies the
     * JDWP command set and low-order byte specifies the command number in the
     * command set. For example, ArrayReference/GetValues JDWP command has a
     * number <code>0x0D02</code> where <code>0x0D = 14</code> is a number
     * of ArrayReference command set and <code>0x02</code> is a number of
     * GetValues command in this command set. For information about numbers
     * of specific commands and command sets see JDWP specification.
     *
     * @param command a command number for this command
     */
    public Command(int command) {
	super();
	setID(nextID++);
	setFlags(flNoFlags);
	setCommand(command);
    }

    /**
     * Gets number of the command assigned for this object. For information
     * about command numbers see description of the constructor.
     *
     * I suspect this method is not used currently by KJDB
     *
     * @return a command number
     */
    public int getCommand() {
        int id = 0;

	try {
            id = (int) getID(CommandOffset, 2);
	}
	catch (BoundException e) {};
	return id;
    }

    /**
     * Assign a command number to the object. For information
     * about command numbers see description of the constructor.
     * This method is used internally by constructor.
     *
     * @param command a command number to be assigned
     */
    public void setCommand(int command) {
	try {
            putID(CommandOffset, command, 2);
	}
	catch (BoundException e) {};
    }

    /**
     * Returns string representation of the object. This method is invoked
     * when reply packet of the command is not received (usually it's a
     * fatal error). It's useful for locating the problem.
     *
     * @return a string representation of the object
     */
    public String toString() {
        
    	int l = 0;
        try {
            l = getInt(LengthOffset);
        }
        catch (BoundException e) {};

	return "length     " + Tools.Hex(l, 8) + "\n" +
            "id         " + Tools.Hex(getID(), 8) + "\n" +
            "flags      " + Tools.Hex(getFlags(), 2) + "\n" +
            "command    " + Tools.Hex(getCommand(), 4) + "\n" +
            super.toString(PacketHeaderSize);
    }

    /**
     * Returns the ID of the command last created. I think that
     * this method is not used currently by KJDB.
     *
     * @return ID of last command
     */
    public static int getLastID() {
    	return (nextID - 1);
    }
}
