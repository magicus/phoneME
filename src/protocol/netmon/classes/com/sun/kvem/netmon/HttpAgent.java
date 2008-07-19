/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/
package com.sun.kvem.netmon;

/**
 * This class is responsible on communication with the network monitor j2se 
 * side. Spesifically it communicate with <code>HttpMsgReceicver</code> which
 * receives the data and produce http messages. Having implement the 
 * <code>StreamAgent</code> interface enables it to work with the stream data
 * "stealer".
 *
 *@author     ah123546
 *@created    December 5, 2001
 *@see     com.sun.kvem.netmon.InputStreamStealer
 *@see     com.sun.kvem.netmon.OutputStreamStealer
 *@see     com.sun.kvem.netmon.StreamConnectionStealer
 *@version
 */
public class HttpAgent implements StreamAgent {
    private static HttpAgent inst = new HttpAgent();



    /**
     *  The agent is a singletone.
     *
     *@return    Description of the Returned Value
     */
    public static HttpAgent instance() {
        return inst;
    }

    /**
     * Called when a new stream is opend. The stream will produce messages
     * at the receiver side.
     *
     *@return    Message descriptor that will identify the
     *           stream in future method calls.
     */
    public native int newStream0(String url, int direction, long groupid);
    public int newStream(String url, int direction, long groupid){
        return newStream0(url,direction, groupid);
    }


    /**
     * Update the message identified by the message descriptor with
     * the byte data.
     *
     *@param  md  Message descriptor.
     *@param  b   data
     *
     */
    public native void write(int md, int b);


    /**
     * Update the message identified by the message descriptor with
     * the buffer data.
     *
     *@param  md    Message descriptor
     *@param  buff
     *@param  off
     *@param  len
     */
    public native void writeBuff(int md, byte[] buff, int off, int len);

    /**
     * The stream that is identified by the message descriptor md has
     * closed, no ferthure communication will be made with that descriptor.
     *
     *@param  md  Message descriptor.
     */
    public native void close(int md);

}
