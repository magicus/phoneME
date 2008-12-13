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

import java.io.IOException;
import java.util.Vector;

/**
 * This abstract class represents a generic transport for JDWP. In KJDB it's
 * used its subclass <code>SocketTransport</code>.
 *
 * @see jdwp.SocketTransport
 */

abstract class Transport {

    /**
     * A buffer that contains data received via JDWP. KJDB doesn't
     * use this field.
     *
     * @see jdwp.SocketTransportImpl
     */
    private static ByteBuffer buffer = new ByteBuffer();

    /**
     * A maximum allowed JDWP packet size.
     */
    private final static int MaxPacketSize		= 0x20000;
    
    /**
     * A delay between attempts to get expected JDWP reply.
     */
    private static final int delta = 2;
    
    /**
     * A timeout for handshake.
     */
    private static final int handshakeTimeout = 50000;

    /**
     * A list of received JDWP repliesa and still not requested
     * by other parts of the code.
     */
    public Vector Replies = new Vector();

    /**
     * Returns the number of bytes that can be received immediately.
     *
     * @return the number of bytes that can be received immediately
     */
    public abstract int available() throws IOException;

    /**
     * Receives the next byte of data. The value byte is
     * returned as an int in the range 0 to 255. If no byte is
     * available, the value -1 is returned.
     *
     * @return the next byte that is read via JDWP
     */
    public abstract int read() throws IOException;

    /**
     * Sends the specified byte via JDWP.
     *
     * @param b a byte to be sent
     */
    public abstract void write(int b) throws IOException;

    /**
     * Sends the specified bytes via JDWP.
     *
     * @param b an array of bytes
     * @param off an offset of the first byte to be sent
     * @param len a number of bytes to be sent
     */
    public void write(byte[] b, int off, int len) throws IOException {
        for (int i = 0; i < len; i++)
			write(b[off + i]);	
    }

    /**
     * Performs JDWP "handshake" procedure (undocumented ?). It sends
     * a <code>"JDWP-Handshake"</code> string and waits for receiving the
     * same string. After handshaking the JDWP connection is considered
     * estblished.
     */
    public void Handshake() throws IOException {

		String hs = "JDWP-Handshake";
		byte[] hsb = hs.getBytes();

		write(hsb, 0, hsb.length);


		int handshakeTime = handshakeTimeout;
		do{
			Tools.wait(500);
			handshakeTime -= 500;
		}while(handshakeTime > 0 && available() < hsb.length);

		if (available() < hsb.length)
			throw new IOException("Target failed to handshake.");

        for (int i = 0; i < hsb.length; i++) {
            byte b = (byte) (read() & 0xFF);
            if (b != hsb[i])
                throw new IOException("Target failed to handshake.");
        }
    }

    /**
     * Sends JDWP command packet.
     *
     * @param c a command to be sent
     */
    public void sendCommand(Command c) throws IOException {
        c.setLength();
		write(c.bytes, 0, c.length());
    }

    /**
     * Sends JDWP command packet with wrong length field. This method is
     * never used by KJDB but it's used by CLDC-DI and JavaCard BackEnd
     * test suite.
     *
     * @param c a command to be sent
     * @param WrongSize a size of the packet
     */
    public void sendBadCommandPacket(Command c, int WrongSize) throws IOException {

   	    try {
			c.putInt(Packet.LengthOffset, WrongSize);
        }
        catch (BoundException e) {};
		write(c.bytes, 0, c.length());
    }


    /**
     * Receives all available JDWP reply packets and places them into the 
     * vector <code>Replies</code>. This method is overwritten in KJDB and the
     * code below is not used.
     *
     * @see jdwp.SocketTransport
     * @see jdwp.SocketTransportImpl
     */
    public void receive() throws IOException {

        while (available() > 0)
			buffer.addByte(read());

		while (buffer.length() >= Packet.PacketHeaderSize) {

			int size = 0;

			buffer.resetParser();
			try {
				size = buffer.getInt();
			}
			catch (BoundException e) {}

	        if ((size < 0) || (size > MaxPacketSize))
    	    	throw new IOException("Wrong JDWP packet size: " + size + ".");

	        if (size > buffer.length())
				return;

			Reply r = new Reply();

			r.resetBuffer();
			r.addBytes(buffer.bytes, 0, size);

			buffer.deleteBytes(size);

			Replies.add(r);
		}
    }

    /**
     * Receives JDWP reply packet.
     * If there is no reply packet available, function returns packet
     * with error code <code>Reply.errNotAvailable</code>.
     *
     * @return a JDWP reply or <code>Reply.errNotAvailable</code> if not
     * available
     */
    public Reply receiveReply() throws IOException {

        receive();

		if (Replies.size() == 0)
			return Reply.Error(Reply.errNotAvailable);

		return (Reply) Replies.remove(0);
    }

    /**
     * Tries to receive a JDWP reply packet with specified ID. It looks
     * into a list of already received packets and tries to locate it.
     * If the desired packet is not found, <code>Reply.errNotAvailable</code>
     * is returned.
     *
     * @param ReplyID an ID of the desired JDWP reply packet
     * @return a JDWP reply packet with specified ID or 
     * <code>Reply.errNotAvailable</code> if not found
     */
    public Reply receiveReply(int ReplyID) throws IOException {
        
        receive();

        for (int i = 0; i < Replies.size(); i++)
       	if (((Reply) Replies.elementAt(i)).getFlags() == 0x80 &&
		((Reply) Replies.elementAt(i)).getID() == ReplyID)
			return (Reply) Replies.remove(i);

		return Reply.Error(Reply.errNotAvailable);
    }

    /**
     * Tries to receive a JDWP reply packet with specified ID. It looks
     * into a list of already received packets and tries to locate it.
     * If the desired packet is not found, is makes a pause and then
     * tries to receive the deired reply again.
     * In case of timeout <code>Reply.errNotAvailable</code>
     * is returned.
     *
     * @param ReplyID an ID of the desired JDWP reply packet
     * @param Delay a timeout for this operation
     * @return a JDWP reply packet with specified ID or 
     * <code>Reply.errNotAvailable</code> if not found
     */
    public Reply receiveReply(int ReplyID, int Delay) throws IOException {
		Reply r = receiveReply(ReplyID);

		while ((r.getErrorCode() == Reply.errNotAvailable) && (Delay > 0)) {
			Tools.wait(delta);
			r = receiveReply(ReplyID);
			Delay = Delay - delta;
		}
		return r;
    }

    /**
     * Tries to receive a JDWP event. It looks
     * into a list of already received packets and tries to locate it.
     * If the desired packet is not found, is makes a pause and then
     * tries to receive the deired reply again.
     * In case of timeout <code>Reply.errNotAvailable</code>
     * is returned.
     *
     * @param command a command number for Event/Composite JDWP command
     * (should be always <code>0x4064</code>)
     * @param Delay a timeout for this operation
     * @return a JDWP event or <code>Reply.errNotAvailable</code> if not found
     */
    public Reply receiveEvent(int command, int Delay) throws IOException {

    	Reply r;

        while (Delay > 0) {
	        receive();
	        for (int i = 0; i < Replies.size(); i++) {
	        	r = (Reply) Replies.elementAt(i);
	    	   	if (r.getFlags() == 0 && r.getErrorCode() == command)
					return (Reply) Replies.remove(i);
			}
			Tools.wait(delta);
			Delay = Delay - delta;
		}
		return Reply.Error(Reply.errNotAvailable);
    }
}
