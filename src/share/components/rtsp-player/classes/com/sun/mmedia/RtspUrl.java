/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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

