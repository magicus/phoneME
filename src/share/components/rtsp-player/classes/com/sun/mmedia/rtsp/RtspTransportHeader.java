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

package com.sun.mmedia.rtsp;

public class RtspTransportHeader {

    private int client_data_port;
    private int client_control_port;
    private int server_data_port;
    private int server_control_port;

    public RtspTransportHeader( String str ) {

        int start;
        int end;

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
            if (-1 == end) end = str.length();
            String control_str = str.substring(start, end);
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
            if (-1 == end) end = str.length();
            String control_str = str.substring(start, end);
            server_control_port = Integer.parseInt(control_str);
        }
    }

    public int getClientDataPort() {
        return client_data_port;
    }

    public int getClientControlPort() {
        return client_control_port;
    }

    public int getServerDataPort() {
        return server_data_port;
    }

    public int getServerControlPort() {
        return server_control_port;
    }
}

