/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/
package com.sun.kvem.netmon;

import java.io.IOException;
import java.io.OutputStream;


/**
 * Wrapp an OutputStream and steals the data along the way.
 * The data that it steals is sent the the StreamAgent given.
 * When created it request a new message descriptor from the 
 * agent. all updates are made on that descriptor.
 *
 *@author ah123546
 *@author Moshe Sayag
 *
 *@version 
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

        if (md >= 0) {
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
