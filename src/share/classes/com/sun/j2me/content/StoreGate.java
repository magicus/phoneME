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
	int allocateBlockID();
	InvocationImpl getRequest(ApplicationID appID, int blockID);
	InvocationImpl getResponse(ApplicationID appID, int blockID);
	InvocationImpl getCleanup(ApplicationID appID);
	InvocationImpl getByTid(int tid, boolean next);
	void dispose(int tid);
	
	boolean waitForEvent(ApplicationID appID, boolean request, int blockID);
	void unblockWaitingThreads(int blockID);
}

interface StoreMessageProcessor extends MessageProcessor {
	int CODE_Size = 1;
	int CODE_Put = 2;
	int CODE_ResetListenNotifiedFlag = 3;
	int CODE_SetCleanupFlag = 4;
	int CODE_ResetFlags = 5;
	int CODE_Update = 6;
	int CODE_AllocateBlockID = 7;
	int CODE_GetRequest = 8;
	int CODE_GetResponse = 9;
	int CODE_GetCleanup = 10;
	int CODE_GetByTid = 11;
	int CODE_Dispose = 12;
	int CODE_WaitForEvent = 13;
	int CODE_UnblockWaitingThreads = 14;
}

class StoreRequestsConverter implements StoreGate {

	final private StoreMessageProcessor out;
	StoreRequestsConverter( StoreMessageProcessor out ){
		this.out = out;
	}
	
	public int size() {
		try {
			byte[] data = out.sendMessage(StoreMessageProcessor.CODE_Size, 
								MessageProcessor.ZERO_BYTES);
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
	
	public int allocateBlockID(){
		try {
			byte[] data = out.sendMessage(StoreMessageProcessor.CODE_AllocateBlockID, 
											MessageProcessor.ZERO_BYTES);
			return new DataInputStream(new ByteArrayInputStream(data)).readInt();
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public InvocationImpl getRequest(ApplicationID appID, int blockID) {
		Bytes dataOut = new Bytes();
		try {
			appID.serialize(dataOut);
			dataOut.writeInt(blockID);
			byte[] data = out.sendMessage(StoreMessageProcessor.CODE_GetRequest, 
											dataOut.toByteArray());
			return new InvocationImpl( new DataInputStream(new ByteArrayInputStream(data)) );
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public InvocationImpl getResponse(ApplicationID appID, int blockID) {
		Bytes dataOut = new Bytes();
		try {
			appID.serialize(dataOut);
			dataOut.writeInt(blockID);
			byte[] data = out.sendMessage(StoreMessageProcessor.CODE_GetResponse, 
											dataOut.toByteArray());
			return new InvocationImpl( new DataInputStream(new ByteArrayInputStream(data)) );
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
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

	public boolean waitForEvent(ApplicationID appID, boolean request, int blockID) {
		Bytes dataOut = new Bytes();
		try {
			appID.serialize(dataOut);
			dataOut.writeBoolean(request);
			dataOut.writeInt(blockID);
			byte[] data = out.sendMessage(StoreMessageProcessor.CODE_WaitForEvent, 
											dataOut.toByteArray());
			return new DataInputStream(new ByteArrayInputStream(data)).readBoolean();
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}
	
	public void unblockWaitingThreads(int blockID) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeInt(blockID);
			out.sendMessage(StoreMessageProcessor.CODE_UnblockWaitingThreads, 
								dataOut.toByteArray());
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}
}

class StoreRequestsExecutor implements StoreMessageProcessor {
	private final StoreGate gate;
	
	public StoreRequestsExecutor( StoreGate gate ){
		this.gate = gate;
	}

	public byte[] sendMessage(int msgCode, byte[] data) throws IOException {
		DataInputStream dataIn = new DataInputStream( new ByteArrayInputStream( data ) );
		switch( msgCode ){
			case CODE_Size: return size(dataIn); 
			case CODE_Put: return put(dataIn);
			case CODE_ResetListenNotifiedFlag: return resetListenNotifiedFlag(dataIn);
			case CODE_SetCleanupFlag: return setCleanupFlag(dataIn);
			case CODE_ResetFlags: return resetFlags(dataIn);
			case CODE_Update: return update(dataIn);
			case CODE_AllocateBlockID: return allocateBlockID(dataIn);
			case CODE_GetRequest: return getRequest(dataIn);
			case CODE_GetResponse: return getResponse(dataIn);
			case CODE_GetCleanup: return getCleanup(dataIn);
			case CODE_GetByTid: return getByTid(dataIn);
			case CODE_Dispose: return dispose(dataIn);
			case CODE_WaitForEvent: return waitForEvent(dataIn);
			case CODE_UnblockWaitingThreads: return unblockWaitingThreads(dataIn);
			default:
				throw new RuntimeException( "illegal msg code " + msgCode );
		}
	}

	private byte[] size(DataInputStream dataIn) throws IOException {
		Bytes out = new Bytes();
		out.writeInt( gate.size() );
		return out.toByteArray();
	}

	private byte[] put(DataInputStream dataIn) throws IOException {
		gate.put(new InvocationImpl( dataIn ));
		return MessageProcessor.ZERO_BYTES;
	}

	private byte[] resetListenNotifiedFlag(DataInputStream dataIn) throws IOException {
		ApplicationID appID = AppProxy.createAppID().read(dataIn);
		boolean request = dataIn.readBoolean();
		gate.resetListenNotifiedFlag(appID, request);
		return MessageProcessor.ZERO_BYTES;
	}

	private byte[] setCleanupFlag(DataInputStream dataIn) throws IOException {
		ApplicationID appID = AppProxy.createAppID().read(dataIn);
		boolean cleanup = dataIn.readBoolean();
		gate.setCleanupFlag(appID, cleanup);
		return MessageProcessor.ZERO_BYTES;
	}

	private byte[] resetFlags(DataInputStream dataIn) throws IOException {
		gate.resetFlags(dataIn.readInt());
		return MessageProcessor.ZERO_BYTES;
	}

	private byte[] update(DataInputStream dataIn) throws IOException {
		gate.update(new InvocationImpl(dataIn));
		return MessageProcessor.ZERO_BYTES;
	}

	private byte[] allocateBlockID(DataInputStream dataIn) throws IOException {
		Bytes out = new Bytes();
		out.writeInt( gate.allocateBlockID() );
		return out.toByteArray();
	}

	private byte[] getRequest(DataInputStream dataIn) throws IOException {
		ApplicationID appID = AppProxy.createAppID().read(dataIn);
		int blockID = dataIn.readInt();
		Bytes out = new Bytes();
		gate.getRequest(appID, blockID).serialize(out);
		return out.toByteArray();
	}

	private byte[] getResponse(DataInputStream dataIn) throws IOException {
		ApplicationID appID = AppProxy.createAppID().read(dataIn);
		int blockID = dataIn.readInt();
		Bytes out = new Bytes();
		gate.getResponse(appID, blockID).serialize(out);
		return out.toByteArray();
	}

	private byte[] getCleanup(DataInputStream dataIn) throws IOException {
		ApplicationID appID = AppProxy.createAppID().read(dataIn);
		Bytes out = new Bytes();
		gate.getCleanup(appID).serialize(out);
		return out.toByteArray();
	}

	private byte[] getByTid(DataInputStream dataIn) throws IOException {
		int tid = dataIn.readInt();
		boolean next = dataIn.readBoolean();
		Bytes out = new Bytes();
		gate.getByTid(tid, next).serialize(out);
		return out.toByteArray();
	}

	private byte[] dispose(DataInputStream dataIn) throws IOException {
		int tid = dataIn.readInt();
		gate.dispose(tid);
		return MessageProcessor.ZERO_BYTES;
	}

	private byte[] waitForEvent(DataInputStream dataIn) throws IOException {
		ApplicationID appID = AppProxy.createAppID().read(dataIn);
		boolean request = dataIn.readBoolean();
		int blockID = dataIn.readInt();
		Bytes out = new Bytes();
		out.writeBoolean( gate.waitForEvent(appID, request, blockID) );
		return out.toByteArray();
	}

	private byte[] unblockWaitingThreads(DataInputStream dataIn) throws IOException {
		int blockID = dataIn.readInt();
		gate.unblockWaitingThreads(blockID);
		return MessageProcessor.ZERO_BYTES;
	}
	
}