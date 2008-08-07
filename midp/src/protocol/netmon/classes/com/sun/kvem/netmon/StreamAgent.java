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

package com.sun.kvem.netmon;

/**
 * A representation of a stream network monitor agent.  
 * A network agent is a unit that responsible on communication with a message
 * receiver in the emulator j2se side. Any network agent that implements this
 * interface can work with some stream classes that "steals" communication data
 * and using this interface, transfer that data to the emulator's network
 * monitor. Each of the implementors classes is responsible on a certain 
 * protocol.
 */
public interface StreamAgent {

	public static final int CLIENT2SERVER = 0;
	public static final int SERVER2CLIENT = 1;

	/**
	 * The method is called when a new message is about to be transfered.
	 * The meaning of a message in this scope is more a type of communication.
	 *
	 *@return    return a message descriptor that should be used when calling
	 * the other methods.
	 */
	public int newStream(String url, int direction, long groupid);


	/**
	 *  Writes one byte to the agent monitor. 
	 *
	 *@param  md  message descriptor.
	 *@param  b   byte.
	 */
	public void write(int md, int b);


	/**
	 *  Writes a buffer to the agent monitor.
	 *
	 *@param  md    message descriptor
	 *@param  buff  buffer
	 *@param  off   offset
	 *@param  len   length
	 */
	public void writeBuff(int md, byte[] buff, int off, int len);


	/**
	 * Close the message. Means that no more communication will be made using
	 * this message descriptor.
	 *
	 *@param  md  message descriptor.
	 */
	public void close(int md);
}

