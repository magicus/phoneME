/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.tck.wma;

import java.io.IOException;

/** 
 * A connection handler for generic message sending and receiving.
 */ 

public interface MessageConnection {
    // JAVADOC COMMENT ELIDED
    public static final String TEXT_MESSAGE = "text";

    // JAVADOC COMMENT ELIDED
    public static final String BINARY_MESSAGE = "binary";

    // JAVADOC COMMENT ELIDED
    public static final String MULTIPART_MESSAGE = "multipart";
    
    // JAVADOC COMMENT ELIDED
    public Message newMessage(String type);   
    
    // JAVADOC COMMENT ELIDED
    public Message newMessage(String type, String address);

    // JAVADOC COMMENT ELIDED
    public void send(Message msg) 
	throws java.io.IOException, java.io.InterruptedIOException;

    // JAVADOC COMMENT ELIDED
    public Message receive()
	throws java.io.IOException, java.io.InterruptedIOException;

    /** 
     * Close the connection.
     * When the connection has been closed access to all methods except this one
     * will cause an an IOException to be thrown. Closing an already closed
     * connection has no effect. Streams derived from the connection may be open
     * when method is called. Any open streams will cause the
     * connection to be held open until they themselves are closed.
     * @throws java.io.IOException - If an I/O error occurs
     */

    public void close() 
        throws IOException;
}



