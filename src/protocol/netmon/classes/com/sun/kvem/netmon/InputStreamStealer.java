/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/
package com.sun.kvem.netmon;

import java.io.IOException;
import java.io.InputStream;


/**
 * Wrapp an input stream and steal the data along the way. The data that
 * it steals is writen to the the StreamAgent given. The object first
 * ask for a new message descriptor and then all updateds to the agent
 * are on that descriptor.
 *
 *@author ah123546
 *@author Moshe Sayag
 *
 *@version 
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

        if (md >= 0) {
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

