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

/**
 * This class is responsible on communication with the network monitor j2se 
 * side. Spesifically it communicate with <code>HttpMsgReceicver</code> which
 * receives the data and produce http messages. Having implement the 
 * <code>StreamAgent</code> interface enables it to work with the stream data
 * "stealer".
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

