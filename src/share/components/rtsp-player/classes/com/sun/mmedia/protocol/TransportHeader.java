/*
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
package com.sun.mmedia.rtsp.protocol;

/**
 * Parses the Transport Header in the RTSP response message.
 *
 * @author     Marc Owerfeldt
 * @created    June 7, 2003
 */
public class TransportHeader {
    /**
     * The transport protocol, i.e. RTP.
     */
    private String transportProtocol;

    /**
     * The profile, i.e. AVP
     */
    private String profile;

    /**
     * The lower transport layer, for example UDP.
     */
    private String lowerTransport;

    /**
     * The client data port.
     */
    private int client_data_port;

    /**
     * The client control port.
     */
    private int client_control_port;

    /**
     * The server data port.
     */
    private int server_data_port;

    /**
     * The server control port.
     */
    private int server_control_port;


    /**
     * Constructor for the TransportHeader object.
     *
     * @param  str  The transport header to be parsed.
     */
    public TransportHeader(String str) {
	// transport protocol:
        int end = str.indexOf('/');

        transportProtocol = str.substring(0, end);

	// profile:
	int start = end + 1;
	end = str.indexOf( ";", start);

	profile = str.substring( start, end);

	// lower layer transport:
	start = end + 1;
	end = str.indexOf( ";", start);

	lowerTransport = str.substring( start, end);

        // client port:
        start = str.indexOf("client_port");

        if (start > 0) {
            // client data port:
            start = str.indexOf("=", start) + 1;

            end = str.indexOf("-", start);

            String data_str = str.substring(start, end);

            client_data_port = Integer.parseInt(data_str);

            // client control port:
            start = end + 1;

            end = str.indexOf(";", start);

            String control_str;

            if (end > 0) {
                control_str = str.substring(start, end);
            } else {
                control_str = str.substring(start);
            }

            client_control_port = Integer.parseInt(control_str);
        }

        // server port:
        start = str.indexOf("server_port");

        if (start > 0) {
            // server data port:
            start = str.indexOf("=", start) + 1;

            end = str.indexOf("-", start);

            String data_str = str.substring(start, end);

            server_data_port = Integer.parseInt(data_str);

            // server control port:
            start = end + 1;

            end = str.indexOf(";", start);

            String control_str;

            if (end > 0) {
                control_str = str.substring(start, end);
            } else {
                control_str = str.substring(start);
            }

            server_control_port = Integer.parseInt(control_str);
        }
    }


    /**
     *  Gets the transport protocol
     *
     * @return    The transport protocol
     */
    public String getTransportProtocol() {
        return transportProtocol;
    }


    /**
     *  Gets the RTP profile
     *
     * @return    The profile
     */
    public String getProfile() {
        return profile;
    }


    /**
     *  Gets the lower layer transport protocol
     *
     * @return    The lower layer transport protocol
     */
    public String getLowerTransportProtocol() {
        return lowerTransport;
    }

    /**
     *  Gets the server data port
     *
     * @return    The server data port
     */
    public int getClientDataPort()
    {
        return client_data_port;
    }

    /**
     *  Gets the server data port
     *
     * @return    The server data port
     */
    public int getServerDataPort() {
        return server_data_port;
    }


    /**
     *  Gets the server control port
     *
     * @return    The server control port
     */
    public int getServerControlPort() {
        return server_control_port;
    }
}

