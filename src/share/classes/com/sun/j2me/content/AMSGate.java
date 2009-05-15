/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.j2me.content;

import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;

public interface AMSGate {
	static final int channelID = 3;
    static final AMSGate inst = Config.getAMSGate(); 
	
	int launchInvocationTarget(InvocationImpl invoc);
	void requestForeground(ApplicationID fromApp, ApplicationID toApp);
}

interface AMSMessageProcessor extends MessageProcessor {
	int CODE_LaunchInvocationTarget = 1;
	int CODE_RequestForeground = 2;
}

class AMSRequestsConverter implements AMSGate {

	final private MessageProcessor out;
	AMSRequestsConverter( MessageProcessor out ){
		this.out = out;
	}
	
	public int launchInvocationTarget(InvocationImpl invoc) {
		Bytes dataOut = new Bytes();
		try {
			invoc.serialize(dataOut);
			byte[] data = out.sendMessage(AMSMessageProcessor.CODE_LaunchInvocationTarget, dataOut.toByteArray());
			return new DataInputStream(new ByteArrayInputStream(data)).readInt();
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}
	
	public void requestForeground(ApplicationID fromApp, ApplicationID toApp) {
		Bytes dataOut = new Bytes();
		try {
			fromApp.serialize(dataOut);
			toApp.serialize(dataOut);
			out.sendMessage(AMSMessageProcessor.CODE_RequestForeground, 
								dataOut.toByteArray());
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}
}

class AMSRequestExecutor implements AMSMessageProcessor {
	
	private final AMSGate gate;
	AMSRequestExecutor( AMSGate gate ){
		this.gate = gate;
	}

	public byte[] sendMessage(int msgCode, byte[] data) throws IOException {
		DataInputStreamExt dataIn = new DataInputStreamExt( new ByteArrayInputStream( data ) );
		switch( msgCode ){
			case CODE_LaunchInvocationTarget:
				return launchInvocationTarget( dataIn );
			case CODE_RequestForeground:
				return requestForeground( dataIn );
			default:
				throw new RuntimeException( "illegal msg code " + msgCode );
		}
	}

	private byte[] launchInvocationTarget(DataInputStreamExt dataIn) throws IOException  {
		Bytes out = new Bytes();
		out.writeInt( gate.launchInvocationTarget(new InvocationImpl( dataIn )) );
		return out.toByteArray();
	}

	private byte[] requestForeground(DataInputStreamExt dataIn) throws IOException {
		ApplicationID fromApp = AppProxy.createAppID().read(dataIn);
		ApplicationID toApp = AppProxy.createAppID().read(dataIn);
		gate.requestForeground(fromApp, toApp);
		return MessageProcessor.ZERO_BYTES;
	}
}

