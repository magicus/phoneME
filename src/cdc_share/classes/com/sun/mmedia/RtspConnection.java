/*
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

package com.sun.mmedia.rtsp;

import com.sun.mmedia.rtsp.protocol.*;

import java.io.*;
import java.net.*;

/**
 * The RtspConnection object encapsulates a TCP/IP connection to an RTSP Server.
 *
 * @author     Marc Owerfeldt
 * @created    September 11, 2002
 */
public class RtspConnection extends Thread implements Runnable {

    private final boolean RTSP_DEBUG;
    
    private final int MAX_RTSP_MESSAGE_SIZE = 1024;

    private Socket socket;

    /**
     * A handle to the RTSP Manager object.
     */
    private RtspManager rtspManager;

    /**
     * Flag inidicating whether the connection to
     * the RTSP Server is alive.
     */
    private boolean connectionIsAlive;

    /**
     * The RTSP Server host address.
     */
    private String host;

    /**
     * The RTSP Server port (default is 554).
     */
    private int port;


    /**
     * Creates a new RTSP connection.
     *
     * @param  rtspManager      The RtspManager object.
     * @param  host             The hostname, i.e. 129.145.166.64
     * @param  port             The port (default port = 554)
     * @exception  IOException  Throws and IOException if a connection
     *                          to the RTSP server cannot be established.
     */
    public RtspConnection(RtspManager rtspManager,
                          String host, int port,
                          boolean rtsp_debug) throws IOException {
        this.rtspManager = rtspManager;
        this.host = host;
        this.port = port;
        
        RTSP_DEBUG = rtsp_debug;

        connectionIsAlive = true;

        try {
            socket = new Socket(host, port);
        } catch (IOException e) {
            // System.out.println("can't connect to " + host + ":" + port);
            throw new IOException("ConnectException");
        }
        
        if(RTSP_DEBUG) System.out.println( "Local address="+socket.getLocalAddress() );

        start();
    }


    /**
     * Sends a message to the RTSP server.
     *
     * @param  message  A byte array containing an RTSP message.
     * @return          Returns true, if the message was sent
     *                  successfully, otherwise false.
     */
    public boolean sendData(byte message[]) {
        boolean success = false;

        try {
            OutputStream out = socket.getOutputStream();
            out.write(message);
            out.flush();
            success = true;
        } catch (IOException e) {
            System.out.println("Error sending RTSP.");
        }
        
        return success;
    }


    /**
     * The main processing loop for incoming RTSP messages.
     */
    public void run() {
        while (connectionIsAlive) {
            try {
                InputStream in = socket.getInputStream();
                DataInputStream din = new DataInputStream(in);
                byte ch = din.readByte();
                ByteArrayOutputStream baos = new ByteArrayOutputStream();

                // read message header:
                baos.write(ch);
                while (!eomReached(baos.toByteArray())) {
                    baos.write(din.readByte());
                }

                // read message body:
                int length = getContentLength(new String(baos.toByteArray()));

                for (int i = 0; i < length; i++) {
                    baos.write(din.readByte());
                }
                    
                if(RTSP_DEBUG) System.out.println("--------- response ---------\n"
                                                  + baos
                                                  + "----------------------------\n");
                    
                Message msg = new Message(baos.toByteArray());
                rtspManager.rtspMessageIndication(msg);
            } catch (Exception e) {
                if(RTSP_DEBUG) System.out.println("-- exception in run():'"
                                                  + e 
                                                  + "', closing RTSP connection." );
                //if(RTSP_DEBUG) e.printStackTrace();
                connectionIsAlive = false;
            }
        }
    }


    /**
     * Tests whether the end of an RTSP message has been reached.
     *
     * @param  buffer  A byte array containing an RTSP message.
     * @return         Returns true, if this buffer contains the end
     *                 of an RTSP message, false otherwise.
     */
    private boolean eomReached(byte buffer[]) {
        boolean endReached = false;

        int size = buffer.length;

        if (size >= 4) {
            if (buffer[size - 4] == '\r' && buffer[size - 3] == '\n' &&
                buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
                endReached = true;
            }
        }

        return endReached;
    }


    /*
     *  private boolean eomReached(byte buffer[], int size) {
     *  boolean endReached = false;
     *  if (size >= 4) {
     *
     *  if (buffer[size - 4] == '\r' && buffer[size - 3] == '\n' &&
     *  buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
     *  endReached = true;
     *  }
     *
     *  if( buffer[size - 2] == '\r' && buffer[size - 1] == '\n') {
     *  endReached = true;
     *  }
     *  }
     *  return endReached;
     *  }
     */

    /**
     * Gets the content length of an RTSP message.
     *
     * @param  msg_header  The RTSP message header.
     * @return             Returns the content length in bytes.
     */
    private int getContentLength(String msg_header) {
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

        try {
            if (socket != null) {
                socket.close();
                socket = null;
            }
        } catch (IOException e) {
            // if(RTSP_DEBUG) System.out.println("Exception in RtspConnection.close.");
            // if(RTSP_DEBUG) e.printStackTrace();
        }
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
