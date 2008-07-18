/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/

package com.sun.kvem.io.j2me.sms;

import javax.wireless.messaging.*;
import java.io.*;
import com.sun.midp.io.HttpUrl;
import com.sun.midp.io.j2me.sms.*;

public class Protocol extends com.sun.midp.io.j2me.sms.Protocol {
    private static long nextGroupID;
    private static DatagramImpl smsImpl;

    private static synchronized long getNextGroupID() {
        return nextGroupID++;
    }

    private long groupID;

    public Protocol() {
        super();
        groupID = getNextGroupID();
        if (smsImpl == null) {
            smsImpl = new DatagramImpl();
        }
    }

    long getGroupID() {
        return groupID;
    }

    boolean isOpen() { return open; }

    public String protocol() { return "sms"; }

    public void send(Message dmsg) throws IOException {

        byte[] buf = null;
        String type = null;

        super.send(dmsg);

        ensureOpen();
        if (dmsg instanceof TextMessage) {
            type = "text";
            buf = ((com.sun.midp.io.j2me.sms.TextObject)dmsg).getBytes();
        } else if (dmsg instanceof BinaryMessage) {
            buf = ((BinaryMessage)dmsg).getPayloadData();
            type = "binary";
        }

        /*
         * parse name into host and port
         */
        String addr = dmsg.getAddress();
        HttpUrl url = new HttpUrl(addr);

        long sendTime = smsImpl.send(type, dmsg.getAddress(), buf,
                                     (url.host == null ? Integer.toString(url.port) : null),
                                     getGroupID());

    }

    public synchronized Message receive()
        throws IOException {

        byte[] buf = null;
        String type = null;
        Message msg = super.receive();

        ensureOpen();
        if (msg instanceof TextMessage) {
            type = "text";
            buf = ((com.sun.midp.io.j2me.sms.TextObject)msg).getBytes();
        } else if (msg instanceof BinaryMessage) {
            buf = ((BinaryMessage)msg).getPayloadData();
            type = "binary";
        }

        long sendTime = smsImpl.receive(type, msg.getAddress(), buf, getAppID(),
                                        getGroupID(), "sms");

        return msg;
    }


    /**
     * Nadav, workaround June 12th, 2008
     * This is just for extracting the host and port
     * Think it is better to fix the real HttpUrl
     *
     * Similar to com.sun.midp.io.HttpUrl
     */
    class HttpUrl {
        public HttpUrl(String url) {



            int index1 = url.indexOf("://");
            if (index1 != -1) {
                int index2 = url.indexOf(':', index1+3);
                if (index2 != -1) {
                    port = Integer.parseInt(url.substring(index2+1));
	            host = url.substring(index1+3, index2);
                } else {
                    host = url.substring(index1+3);
                }
            }
        }
        public int port = -1;
        public String host;
    }
}
