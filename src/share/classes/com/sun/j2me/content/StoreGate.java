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

interface StoreGate {
	int size();
	void put(InvocationImpl invoc);
	void resetListenNotifiedFlag(ApplicationID appID, boolean request);
	void setCleanupFlag(ApplicationID appID, boolean cleanup);
	void resetFlags(int tid);
	void update(InvocationImpl invoc);
	InvocationImpl getRequest(ApplicationID appID, boolean shouldBlock, 
						Counter cancelCounter);
	InvocationImpl getResponse(ApplicationID appID, boolean shouldBlock, 
						Counter cancelCounter);
	InvocationImpl getCleanup(ApplicationID appID);
	InvocationImpl getByTid(int tid, boolean next);
	void dispose(int tid);
	
	boolean waitForEvent(ApplicationID appID, boolean request, 
						Counter cancelCounter);
	void unblockWaitingThreads();
}

interface StoreMessageProcessor extends MessageProcessor {

	int CODE_Size = 0;
	int CODE_Put = 0;
	int CODE_ResetListenNotifiedFlag = 0;
	int CODE_SetCleanupFlag = 0;
	int CODE_ResetFlags = 0;
	int CODE_Update = 0;
	int CODE_GetCleanup = 0;
	int CODE_GetByTid = 0;
	int CODE_Dispose = 0;
	int CODE_UnblockWaitingThreads = 0;
	
}

class StoreRequestsConverter implements StoreGate {

	private static final byte[] ZERO_BYTES = new byte[0];
	final private StoreMessageProcessor out;
	StoreRequestsConverter( StoreMessageProcessor out ){
		this.out = out;
	}
	
	public int size() {
		try {
			byte[] data = out.sendMessage(StoreMessageProcessor.CODE_Size, ZERO_BYTES);
			return new DataInputStream(new ByteArrayInputStream(data)).readInt();
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public void put(InvocationImpl invoc) {
		Bytes dataOut = new Bytes();
		try {
			invoc.serialize(dataOut);
			out.sendMessage(StoreMessageProcessor.CODE_Put, dataOut.toByteArray());
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public void resetListenNotifiedFlag(ApplicationID appID, boolean request) {
		Bytes dataOut = new Bytes();
		try {
			appID.serialize(dataOut);
			dataOut.writeBoolean(request);
			out.sendMessage(StoreMessageProcessor.CODE_ResetListenNotifiedFlag, 
								dataOut.toByteArray());
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public void setCleanupFlag(ApplicationID appID, boolean cleanup) {
		Bytes dataOut = new Bytes();
		try {
			appID.serialize(dataOut);
			dataOut.writeBoolean(cleanup);
			out.sendMessage(StoreMessageProcessor.CODE_SetCleanupFlag, 
								dataOut.toByteArray());
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public void resetFlags(int tid) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeInt(tid);
			out.sendMessage(StoreMessageProcessor.CODE_ResetFlags, 
								dataOut.toByteArray());
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public void update(InvocationImpl invoc) {
		Bytes dataOut = new Bytes();
		try {
			invoc.serialize(dataOut);
			out.sendMessage(StoreMessageProcessor.CODE_Update, dataOut.toByteArray());
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public InvocationImpl getRequest(ApplicationID appID, boolean shouldBlock,
										Counter cancelCounter) {
		// TODO Auto-generated method stub
		return null;
	}

	public InvocationImpl getResponse(ApplicationID appID, boolean shouldBlock,
										Counter cancelCounter) {
		// TODO Auto-generated method stub
		return null;
	}

	public InvocationImpl getCleanup(ApplicationID appID) {
		Bytes dataOut = new Bytes();
		try {
			appID.serialize(dataOut);
			byte[] data = out.sendMessage(StoreMessageProcessor.CODE_GetCleanup, 
											dataOut.toByteArray());
			return new InvocationImpl( new DataInputStream(new ByteArrayInputStream(data)) );
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public InvocationImpl getByTid(int tid, boolean next) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeInt(tid);
			dataOut.writeBoolean(next);
			byte[] data = out.sendMessage(StoreMessageProcessor.CODE_GetByTid, 
											dataOut.toByteArray());
			return new InvocationImpl( new DataInputStream(new ByteArrayInputStream(data)) );
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public void dispose(int tid) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeInt(tid);
			out.sendMessage(StoreMessageProcessor.CODE_Dispose, dataOut.toByteArray());
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public boolean waitForEvent(ApplicationID appID, boolean request,
									Counter cancelCounter) {
		// TODO Auto-generated method stub
		return false;
	}
	
	public void unblockWaitingThreads() {
		try {
			out.sendMessage(StoreMessageProcessor.CODE_UnblockWaitingThreads, ZERO_BYTES);
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}
}

class StoreRequestsExecutor implements StoreMessageProcessor {

	public byte[] sendMessage(int msgCode, byte[] data) throws IOException {
		// TODO Auto-generated method stub
		return null;
	}
	
}