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

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Hashtable;

interface MessageProcessor {
	public static final byte[] ZERO_BYTES = new byte[0];
	
	byte[] sendMessage( int msgCode, byte[] data ) throws IOException;
}

class Bytes extends DataOutputStream {
	public Bytes() { super( new ByteArrayOutputStream() ); }
	public byte[] toByteArray() throws IOException {
		flush();
		return ((ByteArrayOutputStream)out).toByteArray(); 
	}
	
	public void writeUTFN( String v ) throws IOException {
		writeBoolean(v != null);
		if( v != null ) writeUTF( v );
	}
}

class DataInputStreamExt extends DataInputStream {
	public DataInputStreamExt(InputStream in) {
		super(in);
	}
	
	public String readUTFN() throws IOException {
		if( !readBoolean() ) return null;
		return readUTF();
	}
}

class NativeMessageSender implements MessageProcessor {
	private final int queueId;
	public NativeMessageSender( int qId ){ 
		if( Logger.LOGGER != null )
			Logger.LOGGER.println("NativeMessageSender()");
		queueId = qId; 
	}

	public byte[] sendMessage(int msgCode, byte[] data) throws IOException {
		if( Logger.LOGGER != null ){
			Logger.LOGGER.println("NativeMessageSender.send( " + queueId + ", " + msgCode + " )");
			//new Exception("trace");
		}
		byte[] result = send(queueId, msgCode, data); 
		return result;
	}
	
	private static native byte[] send(int queueId, int msgCode, byte[] data) throws IOException;
}

class NativeMessageReceiver implements Runnable {
	static final private NativeMessageReceiver receiver = 
		new NativeMessageReceiver();
	
	final private Hashtable table = new Hashtable();
	
	public static void init() {
		receiver.addProcessor(StoreGate.channelID, 
				new StoreRequestsExecutor( InvocationStore.getInstance() ));
		receiver.addProcessor(RegistryGate.channelID, 
				new RegistryRequestExecutor( RegistryStore.getInstance() ));
		receiver.addProcessor(AMSGate.channelID, 
				new AMSRequestExecutor( AppProxy.getGateInstance() ));
		new Thread(receiver).start();
	}
	
	private NativeMessageReceiver(){}

	public void addProcessor( int qId, MessageProcessor p ){
		table.put(new Integer(qId), p);
	}

	public void run() {
		if( Logger.LOGGER != null )
			Logger.LOGGER.println("NativeMessageReceiver.run()");
		for(;;){
			if( Logger.LOGGER != null )
				Logger.LOGGER.println("NativeMessageReceiver.waitForRequest()");
			int queueId = waitForRequest();
			final MessageProcessor processor = 
				(MessageProcessor)table.get(new Integer(queueId));
			if( Logger.LOGGER != null )
				Logger.LOGGER.println("NativeMessageReceiver: request queue = " + queueId + ", " + processor);
			if( processor != null ){
				final int requestId = getRequestId();
				final int msgCode = getRequestMsgCode();
				final byte[] data = getRequestBytes();
				new Thread(){
					public void run(){
						try {
							byte[] response = processor.sendMessage( msgCode, data );
							postResponse( requestId, response );
						} catch (Exception e) {
							postResponse( requestId, null );
						}
					}
				}.start();
			}
			nextRequest();
		}
	}

	private static native int waitForRequest();
	private static native void nextRequest();
	private static native int getRequestId();
	private static native int getRequestMsgCode();
	private static native byte[] getRequestBytes();
	private static native void postResponse(int requestId, byte[] data);
}