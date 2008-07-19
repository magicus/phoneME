/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/
package com.sun.kvem.netmon;

import com.sun.kvem.io.j2me.ssl.Protocol;

/**
 * This class is responsible on communication with the network monitor j2se 
 * side. Spesifically it communicate with <code>SSLMsgReceicver</code> which
 * receives the data and produce SSL messages. Having implement the 
 * <code>StreamAgent</code> interface enables it to work with the stream data
 * "stealer".
 *
 *@see     com.sun.kvem.netmon.InputStreamStealer
 *@see     com.sun.kvem.netmon.OutputStreamStealer
 *@see     com.sun.kvem.netmon.StreamConnectionStealer
 */
public class SSLInputAgent implements StreamAgent {
    private Protocol protocol;
    public SSLInputAgent(Protocol protocol) {
        this.protocol = protocol;
    }

    /**
     * Called when a new stream is opend. The stream will produce messages
     * at the receiver side.
     *
     *@return    Message descriptor that will identify the
     *           stream in future method calls.
     */
    public int newStream(String url, int direction, long groupid) {
        return 1;
    }

    /**
     * Update the message identified by the message descriptor with
     * the byte data.
     *
     *@param  md  Message descriptor.
     *@param  b   data
     *
     */
    public void write(int md, int b) {
        writeBuff(md, new byte[] {(byte) b}, 0, 1);
    }    

    /**
     * Update the message identified by the message descriptor with
     * the buffer data.
     *
     *@param  md    Message descriptor
     *@param  buff  
     *@param  off   
     *@param  len   
     */
    public void writeBuff(int md, byte[] buff, int off, int len) {
        protocol.read(buff, off, len);
    }

    /**
     * The stream that is identified by the message descriptor md has 
     * closed, no ferthure communication will be made with that descriptor.
     *
     *@param  md  Message descriptor.
     */
    public void close(int md) {
    }
}
