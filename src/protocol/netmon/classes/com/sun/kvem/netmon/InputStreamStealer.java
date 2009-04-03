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
import java.io.InputStream;


/**
 * Wrapp an input stream and steal the data along the way. The data that
 * it steals is writen to the the StreamAgent given. The object first
 * ask for a new message descriptor and then all updateds to the agent
 * are on that descriptor.
 */
public class InputStreamStealer
    extends InputStream {
    private int md;
    private InputStream src;
    private StreamAgent netAgent;
    private String URL;
    private long groupid;
    private boolean newMsgFlag = false;

    /**
     * Constructor for the InputStreamStealer object
     *
     *@param url the opend url.
     *@param src the input stream to steal from
     *@param agent the agent to update.
     */
    public InputStreamStealer(String url, InputStream src, 
                              StreamAgent agent, long groupid) {
        this.src = src;
        netAgent = agent;
        this.URL = url;
        md = netAgent.newStream(URL, HttpAgent.SERVER2CLIENT, groupid);
        this.groupid = groupid;
    }

    /**
     * Resets the stream stealer for a new usage of the connection
     * @param url New url
     * @param groupid New group id
     */
    public void reset(String url, long groupid) {
        this.URL = url;
        this.groupid = groupid;
        md = netAgent.newStream(URL, HttpAgent.SERVER2CLIENT, groupid);
    }

    /**
     * The name says it all.
     *
     *@param b
     *@param off
     *@param len
     *@return
     *@exception IOException
     */
    public int read(byte[] b, int off, int len)
             throws IOException {
        int ret = src.read(b, off, len);

        if (md >= 0) {
            if (ret < 0) {
                // the input stream has ended so telling the monitor that
                // no more communication will be made on this md.
                netAgent.close(md);
            } else {
                if (newMsgFlag) {
                    newMsgFlag = false;
                    writeNewMsgWTKHeader();
                }

                netAgent.writeBuff(md, b, off, ret);
            }
        }

        return ret;
    }

    /**
     * The name says it all.
     *
     *@return
     *@exception IOException
     */
    public int read()
             throws IOException {
        int ch = src.read();

        if (md >= 0) {
            if (ch < 0) {
                // the input stream has ended so telling the monitor that
                // no more communication will be made on this md.
                netAgent.close(md);
            } else {
                if (newMsgFlag) {
                    newMsgFlag = false;
                    writeNewMsgWTKHeader();
                }

                netAgent.write(md, ch);
            }
        }

        return ch;
    }

    /**
     * The name says it all.
     *
     *@exception IOException
     */
    public void close()
               throws IOException {
        super.close();
        src.close();

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
    public void writeNewMsgWTKHeader() throws IOException {
        long time = System.currentTimeMillis();
        String str = String.valueOf(time) + "\r\n";
        byte[] buff = str.getBytes();
        netAgent.writeBuff(md, buff, 0, buff.length);
    }

    /**
    * This method is called when a new message is about to start.
    * It is called even for a reused connection.
    */
    public void newMsg() throws IOException {
        newMsgFlag = true;
    }

    public int available() throws IOException {
        return src.available();
    }
}

