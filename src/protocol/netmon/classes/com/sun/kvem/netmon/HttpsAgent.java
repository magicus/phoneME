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
 */
public class HttpsAgent implements StreamAgent {
    private static HttpsAgent inst = new HttpsAgent();



    /**
     *  Description of the Method
     *
     *@return    Description of the Returned Value
     */
    public static HttpsAgent instance() {
        return inst;
    }


    /**
     *  Description of the Method
     *
     *@return    Description of the Returned Value
     */
    public native int newStream(String url, int direction, long groupid);


    /**
     *  Description of the Method
     *
     *@param  md  Description of Parameter
     *@param  b   Description of Parameter
     */
    public native void write(int md, int b);


    /**
     *  Description of the Method
     *
     *@param  md    Description of Parameter
     *@param  buff  Description of Parameter
     *@param  off   Description of Parameter
     *@param  len   Description of Parameter
     */
    public native void writeBuff(int md, byte[] buff, int off, int len);


    /**
     *  Description of the Method
     *
     *@param  md  Description of Parameter
     */
    public native void close(int md);

}

