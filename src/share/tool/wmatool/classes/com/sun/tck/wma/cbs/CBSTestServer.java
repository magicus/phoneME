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

package com.sun.tck.wma.cbs;

import com.sun.wma.api.server.CBSServer;
import com.sun.tck.wma.Message;
import com.sun.tck.wma.MessageConnection;
import com.sun.tck.wma.MessageTransportConstants;
import com.sun.tck.wma.BinaryMessage;
import com.sun.tck.wma.TextMessage;
import java.io.IOException;

/**
 * Implements CBSserver interface. This class will be used
 * by the JDTS test harness to test CBS receive functionality.
 */

public class CBSTestServer implements CBSServer {

    private CBSMessageConnection cbsmess;
    private MessageConnection conn = null;

    private char UCS_CHAR = 0x00a4; // this character does not have equivalent

    /** The URL scheme for the CBS protocol. */
    private final String CBS_SCHEME = "cbs:";

    /**
     * Construct a new CBS test server.
     */
    public CBSTestServer() {}

    /**
     * Send a CBS message.
     *
     * @param type  "gsm7", "ucs2", or "binary"
     * @param segNum Number of message segemnts
     * @param address CBS address for the message
     *
     */
    public void send (String type, int segNum, String address) {

        String urlFragment = address.substring(CBS_SCHEME.length());

        try {
            conn = cbsmess.openPrim(urlFragment);
        } catch (IOException ioe) {
            System.err.println("Exception thrown by CBS Test server: " + ioe);
        }

        int num = 0;

        if (type.equals("gsm7")) {
            TextMessage tmsg = (TextMessage)cbsmess.newMessage(MessageConnection.TEXT_MESSAGE, address);

            // construct the message
            switch (segNum){
                case 1:
                    num = 24;
                    break;
                case 2:
                    num = 26;
                    break;
                case 3:
                    num = 50;
		    break;
                case 4:
                    num = 74;
		    break;
                default:
                    num = 0;
            }

            String long_msg = "";;
            for (int i = 0; i < num; i++) {
                long_msg += "Hello "; // the string contains 6 chars
            }

            tmsg.setPayloadText(long_msg);
         
            try {
                conn.send(tmsg);
            } catch (Exception e) {
                System.err.println("Exception thrown by CBS Test server: " + e);
            }

        } else if (type.equals("ucs2")) {
            TextMessage tmsg = (TextMessage)cbsmess.newMessage(MessageConnection.TEXT_MESSAGE, address);

            // construct the message
            switch (segNum){
                case 1:
                    num = 60;
                    break;
                case 2:
                    num = 68;
		    break;
                case 3:
		    num = 128;
		    break;
                case 4:
                    num = 190;
		    break;
                default:
		    num = 0;
            }

            char[] ucs_chars = new char[num];
		
            for (int i = 0; i < num; i++) {
                ucs_chars[i] = UCS_CHAR; // char UCS_CHAR  = 0x00a4;
            }
            String long_msg = new String(ucs_chars);

            tmsg.setPayloadText(long_msg);

            try {
                conn.send(tmsg);
            } catch (Exception e) {
                System.err.println("Exception thrown by CBS Test server: " + e);
            }

        } else if (type.equals("binary")) {
            BinaryMessage bmsg = (BinaryMessage)cbsmess.newMessage(MessageConnection.BINARY_MESSAGE, address);

            // construct the message
            switch (segNum){
                case 1:
                    num = 130;
                    break;
                case 2:
                    num = 136;
                    break;
                case 3:
                    num = 260;
		    break;
                case 4:
                    num = 390;
		    break;
                default:
		    num = 0;
            }

            byte[] byte_msg = new byte[num];
            for (int i = 0; i < num; i++) {
                byte_msg[i] = (byte) i; // create a byte[]
            }

            bmsg.setPayloadData(byte_msg);

            try {
                conn.send(bmsg);
            } catch (Exception e) {
                System.err.println("Exception thrown by CBS Test server: " + e);
            }
        }

    }

    /**
     * Initialize CBS server.
     *
     */
    public void init () {
        cbsmess = new CBSMessageConnection();
    }

    /**
     * CBS server terminates any active operations and frees up
     * resources..
     *
     */
    public void die () {
        if (conn != null) {
            try {
                conn.close();
            } catch (Exception e) {
                System.err.println("Exception thrown by CBS Test server: " + e);
            }
        }
    } 

}
