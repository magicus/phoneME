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

package com.sun.kvem.netmon;

import java.io.IOException;
import java.io.OutputStream;


/**
 * Wrapp an OutputStream and steals the data along the way.
 * The data that it steals is sent the the StreamAgent given.
 * When created it request a new message descriptor from the 
 * agent. all updates are made on that descriptor.
 */
public class OutputStreamStealer
    extends OutputStream {

    private int md;
    private OutputStream dest;
    private StreamAgent netAgent;
    private String URL;
    private long groupid;
    private boolean newMsgFlag;
    private boolean closed;

    /**
     * Constructor for the OutputStreamStealer object
     *
     *@param url
     *@param dest
     *@param agent
     */
    public OutputStreamStealer(String url, OutputStream dest, 
                               StreamAgent agent, long groupid) {
        this.dest = dest;
        netAgent = agent;
        this.URL = url;
        this.groupid = groupid;
        md = netAgent.newStream(URL, HttpAgent.CLIENT2SERVER, groupid);
    }

    /**
     * Resets the stream stealer for a new usage of the connection
     * @param url New url
     * @param groupid New group id
     */
    public void reset(String url, long groupid) {
        this.URL = url;
        this.groupid = groupid;
        md = netAgent.newStream(URL, HttpAgent.CLIENT2SERVER, groupid);
    }

    /**
     * The name says it all.
     *
     *@param b
     *@param off
     *@param len
     *@exception IOException
     */
    public void write(byte[] b, int off, int len)
               throws IOException {
        dest.write(b, off, len);

        if (md >= 0) {

            if (newMsgFlag) {
                newMsgFlag = false;
                writeNewMsgWTKHeader();
            }

            netAgent.writeBuff(md, b, off, len);
        }
    }

    /**
     * The name says it all.
     *
     *@param b
     *@exception IOException
     */
    public void write(int b)
               throws IOException {

        dest.write(b);
        if (md >= 0) {

            if (newMsgFlag) {
                newMsgFlag = false;
                writeNewMsgWTKHeader();
            }

            netAgent.write(md, b);
        }       
    }
    
    /** Determines whether this sttream has been closed. */
    public boolean isClosed() {
        return closed;
    }

    /**
     * The name says it all.
     *
     *@exception IOException
     */
    public void close()
               throws IOException {
        // super.close();  - not necessary; the superclass is OutputStream
        dest.close();
        closed = true;

        disconnect();
    }
    
    /**
     * Report disconnected stream. Do not close the underlying stream, 
     * as it may be reused.     
     */         
    public void disconnect() {
        if (md >=0) {
            netAgent.close(md);
        }
    }

    /**
    * This method writes the wtk header which is not the HTTP header but
    * some data that is needed to be passed to the j2se side.
    * Thats for each message.
    */
    public void writeNewMsgWTKHeader()
                              throws IOException {

        long time = System.currentTimeMillis();
        String str = String.valueOf(time) + "\r\n";
        byte[] buff = str.getBytes();
        netAgent.writeBuff(md, buff, 0, buff.length);
    }

    /**
    * This method is called when a new message is about to start.
    * It is called even for a reused connection.
    */
    public void newMsg()
                throws IOException {
        newMsgFlag = true;
    }
}
