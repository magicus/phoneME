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
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;

import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;

/** 
 * RtspConnectionBase is a portable base for RtspConnection platform-specific
 *  classes that represent a TCP/IP connection to an RTSP Server.
 */
public abstract class RtspConnectionBase extends Thread implements Runnable {
    /**
     * Flag inidicating whether the connection to
     * the RTSP Server is alive.
     */
    protected boolean connectionIsAlive = false;

    protected InputStream is = null;
    protected OutputStream os = null;
    protected RtspDS ds = null;

    /** Platform-specific implementations override this method
     * to create 'is' and 'os' objects
     */
    protected abstract void openStreams(RtspUrl url) throws IOException;

    protected void closeStreams() {
        if (null != is) {
            try {
                is.close();
            } catch (IOException e) {
                if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                    Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                        "IOException in RtspConnection.closeStreams(): " + e.getMessage());
                }
            }
            is = null;
        }

        if (null != os) {
            try {
                os.close();
            } catch (IOException e) {
                if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                    Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                        "IOException in RtspConnection.closeStreams(): " + e.getMessage());
                }
            }
            os = null;
        }
    }

    /** Creates a new RTSP connection.
     *
     * @param url RTSP URL object
     * @exception  IOException  Throws and IOException if a connection
     *                          to the RTSP server cannot be established.
     */
    public RtspConnectionBase(RtspDS ds) throws IOException {

        try {
            openStreams(ds.getUrl());
        } catch (IOException e) {
            throw e;
        }

        if (null == is) throw new IOException("InputStream creation failed");
        if (null == os) throw new IOException("OutputStream creation failed");

        connectionIsAlive = true;
        this.ds = ds;

        start();
    }


    /**
     * Sends a message to the RTSP server.
     *
     * @param  message  A byte array containing an RTSP message.
     * @return          Returns true, if the message was sent
     *                  successfully, otherwise false.
     */
    public boolean sendData(byte[] message) {
        try {
            // System.out.println("---------- sending RTSP message -------------------------");
            // System.out.println(new String(message));
            // System.out.println("---------------------------------------------------------");
            os.write(message);
            os.flush();
            return true;
        } catch (IOException e) {
            return false;
        }
    }


    /**
     * The main processing loop for incoming RTSP messages.
     */
    public void run() {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        byte[] rtp_packet = null;

        int ch0 = 0;
        int ch1 = 0;
        int ch2 = 0;
        int ch3 = 0;

        while (connectionIsAlive) {
            try {
                ch3 = ch2;
                ch2 = ch1;
                ch1 = ch0;
                ch0 = is.read();

                if (-1 != ch0) {
                    if (0 == baos.size() && '$' == ch0) {

                        // this is an interleaved RTP packet

                        int channel = is.read();
                        int len = is.read() * 256 + is.read();
                        rtp_packet = new byte[len];

                        int total_bytes_read = 0;
                        int bytes_read;

                        do {
                            bytes_read = is.read(rtp_packet, total_bytes_read, len - total_bytes_read);

                            // for (int i = 0; i < channel + 1; i++) System.out.print("   ");
                            // System.out.println("[" + channel + "]: read " + bytes_read + " bytes of " + len);

                            if (-1 != bytes_read) {
                                total_bytes_read += bytes_read;
                            }
                        } while (bytes_read != -1 && total_bytes_read < len);

                        if (len == total_bytes_read) {
                            ds.processRtpPacket(channel, rtp_packet);
                        } else {
                            connectionIsAlive = false;
                        }
                    } else {
                        baos.write(ch0);

                        if ('\r' == ch1 && '\n' == ch0 &&
                            '\r' == ch3 && '\n' == ch2) {

                            // RTSP message header is completely received

                            String header = new String(baos.toByteArray());

                            int content_length = getContentLength(header);

                            for (int i = 0; i < content_length; i++) {

                                ch0 = is.read();

                                if (-1 != ch0) {
                                    baos.write(ch0);
                                } else {
                                    connectionIsAlive = false;
                                    break;
                                }
                            }

                            // whole message is completely received

                            // System.out.println("---------- RTSP incoming message ------------------------");
                            // System.out.println(new String(baos.toByteArray()));
                            // System.out.println("---------------------------------------------------------");

                            ds.processIncomingMessage(baos.toByteArray());
                            baos.reset();
                        }
                    }
                } else {
                    connectionIsAlive = false;
                }

            } catch (Exception e) {
                connectionIsAlive = false;
            }
        }
    }

    /**
     * Gets the content length of an RTSP message.
     *
     * @param  msg_header  The RTSP message header.
     * @return             Returns the content length in bytes.
     */
    private static int getContentLength(String msg_header) {
        int length;

        int start = msg_header.indexOf("Content-length");

        if (start == -1) {
            // fix for QTSS:
            start = msg_header.indexOf("Content-Length");
        }

        if (start == -1) {
            length = 0;
        } else {
            start = msg_header.indexOf(':', start) + 2;

            int end = msg_header.indexOf('\r', start);

            String length_str = msg_header.substring(start, end);

            length = Integer.parseInt(length_str);
        }

        return length;
    }


    /**
     * Closes the RTSP connection.
     */
    public void close() {
        connectionIsAlive = false;
        closeStreams();
    }


    /**
     * Indicates whether the connection to the RTSP server
     * is alive.
     *
     * @return Returns true, if the connection is alive, false otherwise.
     */
    public boolean connectionIsAlive() {
        return connectionIsAlive;
    }
}
