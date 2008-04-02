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

import java.io.IOException;

/**
 * Encapsulates an RTSP URL of the form rtsp://host:port/file.
 *
 * @author     Marc Owerfeldt
 * @created    June 7, 2003
 */
public class RtspUrl {
    /**
     * The RTSP URL.
     */
    private String url;


    /**
     * Constructor for the RtspUrl object
     *
     * @param  url              The RTSP URL to be parsed.
     * @exception  IOException  Thows an IOException if the URL cannot 
     *                          be parsed as a valid RTP URL.
     */
    public RtspUrl(String url) throws IOException {
        this.url = url;

        if (url.length() < 7) {
            throw new IOException("Malformed URL");
        }

        if (!url.startsWith("rtsp://")) {
            throw new IOException("Malformed URL");
        }
    }


    /**
     * Gets the file attribute of the RtspUrl object.
     *
     * @return    The file value
     */
    public String getFile() {
        String str = url.substring(7);

        int start = str.indexOf('/');

        String file = "";

        if (start != -1) {
            file = str.substring(start + 1);
        }

        return file;
    }


    /**
     * Gets the host attribute of the RtspUrl object.
     *
     * @return    The host value
     */
    public String getHost() {
        String host = null;

        String str = url.substring(7);

        int end = str.indexOf(':');

        if (end == -1) {
            end = str.indexOf('/');

            if (end == -1) {
                host = str;
            } else {
                host = str.substring(0, end);
            }
        } else {
            host = str.substring(0, end);
        }

        return host;
    }


    /**
     * Gets the port attribute of the RtspUrl object
     *
     * @return    The port value
     */
    public int getPort() {
        int port = 554;
        // default port for RTSP

        String str = url.substring(7);

        int start = str.indexOf(':');

        if (start != -1) {
            int end = str.indexOf('/');

            port = Integer.parseInt(str.substring(start + 1, end));
        }

        return port;
    }
}

