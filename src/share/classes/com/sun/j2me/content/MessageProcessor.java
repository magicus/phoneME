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

