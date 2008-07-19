/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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
 *
 *@author     ah123546
 *@created    December 5, 2001
 *@see     com.sun.kvem.netmon.InputStreamStealer
 *@see     com.sun.kvem.netmon.OutputStreamStealer
 *@see     com.sun.kvem.netmon.StreamConnectionStealer
 *@version 
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

