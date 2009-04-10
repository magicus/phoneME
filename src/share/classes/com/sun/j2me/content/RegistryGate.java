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

interface RegistryGate {
    /**
     * Search flags for @link getHandler() method. 
     */
    static final int SEARCH_EXACT   = 0; /** Search by exact match with ID */
    static final int SEARCH_PREFIX  = 1; /** Search by prefix of given value */

    /** 
     * Content Handler fields indexes.
     * <BR>Used with functions: @link findHandler(), @link getValues() and 
     * @link getArrayField().
     * <BR> They should match according enums in jsr211_registry.h
     */
    static final int FIELD_ID         = 0;  /** Handler ID */
    static final int FIELD_TYPES      = 1;  /** Types supported by a handler */
    static final int FIELD_SUFFIXES   = 2;  /** Suffixes supported */
                                            /** by a handler */
    static final int FIELD_ACTIONS    = 3;  /** Actions supported */
                                            /** by a handler */
    static final int FIELD_LOCALES    = 4;  /** Locales supported */
                                            /** by a handler */
    static final int FIELD_ACTION_MAP = 5;  /** Handler action map */
    static final int FIELD_ACCESSES   = 6; /** Access list */
    static final int FIELD_COUNT      = 7; /** Total number of fields */

    ContentHandlerImpl.Data register(ApplicationID appID, ContentHandlerRegData handlerData);
	boolean unregister(String handlerID);
	void enumHandlers(String callerId, int fieldId, String value, 
							ContentHandlerImpl.Handle.Receiver output);
	ContentHandlerImpl[] findConflicted(String handlerID);
	ContentHandlerImpl.Data findHandler(String callerId, String handlerID, int searchMode);
	ContentHandlerImpl[] findHandler(String callerId, int fieldId, String value);
	ContentHandlerImpl[] forSuite(int suiteId);
	ContentHandlerImpl.Data getHandler(ApplicationID appID);
	String[] getValues(String callerId, int fieldId);
	ContentHandlerImpl.Data getHandlerData(String handlerID);
	String[] getHandlerValues(String handlerID, int fieldId);
}

interface RegistryMessageProcessor extends MessageProcessor {
	static final int CODE_Register = 1;
	static final int CODE_Unregister = 2;
	static final int CODE_FindHandlerByName = 3;
	static final int CODE_FindHandlerByField = 4;
	static final int CODE_ForSuite = 5;
	static final int CODE_GetAppHandler = 6;
	static final int CODE_GetValues = 7;
	static final int CODE_GetHandlerValues = 8;
	static final int CODE_GetHandlerData = 9;
}

class RegistryRequestsConverter implements RegistryGate {

	final private RegistryMessageProcessor out;
	RegistryRequestsConverter( RegistryMessageProcessor out ){
		this.out = out;
	}
	
	private ContentHandlerImpl.Data toHandlerData( DataInputStream in ) throws IOException {
		return new ContentHandlerImpl.Data( in );
	}

	private ContentHandlerImpl[] toHandlersArray( byte[] data ) throws IOException {
		DataInputStream in = new DataInputStream( new ByteArrayInputStream( data ) );
		ContentHandlerImpl[] result = new ContentHandlerImpl[in.readInt()];
		for( int i = 0; i < result.length; i++){
			result[ i ] = new ContentHandlerHandle( toHandlerData(in) ).get();
		}
		return result;
	}

	private String[] toStringArray(byte[] data) throws IOException {
		DataInputStream in = new DataInputStream( new ByteArrayInputStream( data ) );
		String[] result = new String[in.readInt()];
		for( int i = 0; i < result.length; i++){
			result[ i ] = in.readUTF();
		}
		return result;
	}

	public ContentHandlerImpl.Data register(ApplicationID appID,
										ContentHandlerRegData handlerRegData) {
		Bytes dataOut = new Bytes();
		try {
			appID.serialize(dataOut);
			handlerRegData.serialize(dataOut);
			byte[] data = 
				out.sendMessage(RegistryMessageProcessor.CODE_Register, 
									dataOut.toByteArray());
			if( data.length == 0 ) 
				return null;
			return toHandlerData(
					new DataInputStream(new ByteArrayInputStream(data)));
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public boolean unregister(String handlerID) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeUTF(handlerID);
			byte[] data = 
				out.sendMessage(RegistryMessageProcessor.CODE_Unregister, 
									dataOut.toByteArray());
			return new DataInputStream(new ByteArrayInputStream(data)).readBoolean();
		} catch (IOException e) {
			throw new RuntimeException(e.getMessage());
		}
	}

	public void enumHandlers(String callerId, int fieldId, String value,
								ContentHandlerImpl.Handle.Receiver output) {
		ContentHandlerImpl[] result = findHandler(callerId, fieldId, value);
		for( int i = 0; i < result.length; i++)
			output.push(result[i].handle);
	}

	public ContentHandlerImpl[] findConflicted(String testID) {
		return findHandler(null, FIELD_ID, testID);
	}

	public ContentHandlerImpl.Data findHandler(String callerId, String handlerID, int searchMode) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeUTF(callerId);
			dataOut.writeUTF(handlerID);
			dataOut.writeInt(searchMode);
			byte[] data = out.sendMessage(RegistryMessageProcessor.CODE_FindHandlerByName, 
									dataOut.toByteArray());
			if( data.length == 0 ) 
				return null;
			return toHandlerData( new DataInputStream( new ByteArrayInputStream( data ) ) );
		} catch (IOException e) {
			throw new RuntimeException( e.getMessage() );
		}
	}

	public ContentHandlerImpl[] findHandler(String callerId, int fieldId, String value) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeUTF(callerId);
			dataOut.writeInt(fieldId);
			dataOut.writeUTF(value);
			return toHandlersArray(out.sendMessage(RegistryMessageProcessor.CODE_FindHandlerByField, 
										dataOut.toByteArray()));
		} catch (IOException e) {
			throw new RuntimeException( e.getMessage() );
		}
	}

	public ContentHandlerImpl[] forSuite(int suiteId) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeInt(suiteId);
			return toHandlersArray(out.sendMessage(RegistryMessageProcessor.CODE_ForSuite, 
										dataOut.toByteArray()));
		} catch (IOException e) {
			throw new RuntimeException( e.getMessage() );
		}
	}

	public ContentHandlerImpl.Data getHandler(ApplicationID appID) {
		Bytes dataOut = new Bytes();
		try {
			appID.serialize( dataOut );
			byte[] data = out.sendMessage(RegistryMessageProcessor.CODE_GetAppHandler, 
								dataOut.toByteArray());
			if( data.length == 0 ) 
				return null;
			return toHandlerData( new DataInputStream( new ByteArrayInputStream( data ) ) );
		} catch (IOException e) {
			throw new RuntimeException( e.getMessage() );
		}
	}

	public String[] getValues(String callerId, int fieldId) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeUTF(callerId);
			dataOut.writeInt(fieldId);
			byte[] data = out.sendMessage(RegistryMessageProcessor.CODE_GetValues, 
								dataOut.toByteArray());
			return toStringArray( data );
		} catch (IOException e) {
			throw new RuntimeException( e.getMessage() );
		}
	}

	public String[] getHandlerValues(String handlerID, int fieldId) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeUTF(handlerID);
			dataOut.writeInt(fieldId);
			byte[] data = out.sendMessage(RegistryMessageProcessor.CODE_GetHandlerValues, 
								dataOut.toByteArray());
			return toStringArray( data );
		} catch (IOException e) {
			throw new RuntimeException( e.getMessage() );
		}
	}

	public ContentHandlerImpl.Data getHandlerData(String handlerID) {
		Bytes dataOut = new Bytes();
		try {
			dataOut.writeUTF(handlerID);
			byte[] data = out.sendMessage(RegistryMessageProcessor.CODE_GetHandlerData, 
								dataOut.toByteArray());
			return new ContentHandlerImpl.Data( 
							new DataInputStream( new ByteArrayInputStream( data ) ) );
		} catch (IOException e) {
			throw new RuntimeException( e.getMessage() );
		}
	}
}

class RegistryRequestExecutor implements RegistryMessageProcessor {
	
	private final RegistryGate gate;

	RegistryRequestExecutor( RegistryGate gate ){
		this.gate = gate;
	}

	public byte[] sendMessage(int msgCode, byte[] data) throws IOException {
		DataInputStream dataIn = new DataInputStream( new ByteArrayInputStream( data ) );
		switch( msgCode ){
			case CODE_Register: return register(dataIn);
			case CODE_Unregister: return unregister(dataIn);
			case CODE_FindHandlerByName: return findHandlerByName(dataIn);
			case CODE_FindHandlerByField: return findHandlerByField(dataIn);
			case CODE_ForSuite: return forSuite(dataIn);
			case CODE_GetAppHandler: return getAppHandler(dataIn);
			case CODE_GetValues: return getValues(dataIn);
			case CODE_GetHandlerValues: return getHandlerValues(dataIn);
			case CODE_GetHandlerData: return getHandlerData(dataIn);
			default:
				throw new RuntimeException( "illegal msg code " + msgCode );
		}
	}

	private byte[] toBytes(ContentHandlerImpl.Data data) throws IOException {
		if( data == null )
			return ZERO_BYTES;
		Bytes out = new Bytes();
		data.serialize( out );
		return out.toByteArray();
	}
	
	private byte[] toBytes(ContentHandlerImpl[] res) throws IOException {
		Bytes out = new Bytes();
		out.writeInt(res.length);
		for( int i = 0; i < res.length; i++)
			res[i].getHandlerData().serialize(out);
		return out.toByteArray();
	}
	
	private byte[] toBytes(String[] values) throws IOException {
		Bytes out = new Bytes();
		out.writeInt(values.length);
		for( int i = 0; i < values.length; i++)
			out.writeUTF( values[i] );
		return out.toByteArray();
	}

	private byte[] register(DataInputStream dataIn) throws IOException {
		ApplicationID appID = AppProxy.createAppID().read(dataIn);
		ContentHandlerImpl.Data data = 
			gate.register( appID, new ContentHandlerRegData(dataIn) );
		return toBytes(data);
	}

	private byte[] unregister(DataInputStream dataIn) throws IOException {
		String handlerID = dataIn.readUTF();
		Bytes out = new Bytes();
		out.writeBoolean( gate.unregister(handlerID) );
		return out.toByteArray();
	}

	private byte[] findHandlerByName(DataInputStream dataIn) throws IOException {
		String callerId = dataIn.readUTF();
		String handlerID = dataIn.readUTF();
		int searchMode = dataIn.readInt();
		ContentHandlerImpl.Data data = 
			gate.findHandler(callerId, handlerID, searchMode);
		return toBytes(data);
	}

	private byte[] findHandlerByField(DataInputStream dataIn) throws IOException {
		String callerId = dataIn.readUTF();
		int fieldId = dataIn.readInt();
		String value = dataIn.readUTF();
		return toBytes( gate.findHandler(callerId, fieldId, value) );
	}

	private byte[] forSuite(DataInputStream dataIn) throws IOException {
		int suiteId = dataIn.readInt();
		return toBytes( gate.forSuite(suiteId) );
	}

	private byte[] getAppHandler(DataInputStream dataIn) throws IOException {
		ApplicationID appID = AppProxy.createAppID().read(dataIn);
		ContentHandlerImpl.Data data = gate.getHandler(appID);
		return toBytes( data );
	}

	private byte[] getValues(DataInputStream dataIn) throws IOException {
		String callerId = dataIn.readUTF();
		int fieldId = dataIn.readInt();
		return toBytes( gate.getValues(callerId, fieldId) );
	}

	private byte[] getHandlerValues(DataInputStream dataIn) throws IOException {
		String handlerID = dataIn.readUTF();
		int fieldId = dataIn.readInt();
		return toBytes( gate.getHandlerValues(handlerID, fieldId) );
	}

	private byte[] getHandlerData(DataInputStream dataIn) throws IOException {
		String handlerID = dataIn.readUTF();
		return toBytes( gate.getHandlerData(handlerID) );
	}
}