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

import java.io.*;
import java.net.*;

import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;

/**
 * The RtspConnection object encapsulates a TCP/IP connection to an RTSP Server.
 */
public class RtspConnection extends RtspConnectionBase {
    private Socket socket;

    protected void openStreams(RtspUrl url) throws IOException {
        socket = new Socket(url.getHost(), url.getPort());
        is = socket.getInputStream();
        os = socket.getOutputStream();
    }

    protected void closeStreams() {

        super.closeStreams();

        if (null != socket) {
            try {
                socket.close();
            } catch (IOException e) {
                if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                    Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                        "IOException in RtspConnection.closeStreams(): " + e.getMessage());
                }
            }
            socket = null;
        }
    }

    public RtspConnection(RtspDS ds) throws IOException {
        super(ds);
    }
}
