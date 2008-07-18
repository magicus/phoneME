
/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/

package com.sun.kvem.io.j2me.cbs;

import javax.wireless.messaging.*;
import java.io.*;
import com.sun.midp.io.HttpUrl;
import com.sun.midp.io.j2me.sms.*;
import com.sun.kvem.io.j2me.sms.DatagramImpl;

public class Protocol extends com.sun.midp.io.j2me.cbs.Protocol {
    private static long nextGroupID;
    private static DatagramImpl cbsImpl;
    
    private static synchronized long getNextGroupID() {
        return nextGroupID++;
    }
    
    private long groupID;
    
    public Protocol() {
        super();
        groupID = getNextGroupID();
        if (cbsImpl == null) {
            cbsImpl = new DatagramImpl();
        }
    }
    
    long getGroupID() {
        return groupID;
    }
    
    boolean isOpen() { return open; }

    public String protocol() { return "cbs"; }

    public synchronized Message receive()
        throws IOException {

        byte[] buf = null;
        String type = null;
        Message msg = super.receive();

        ensureOpen();
        if (msg instanceof TextMessage) {
            type = "text";
            buf = ((com.sun.midp.io.j2me.cbs.TextObject)msg).getBytes();
        } else if (msg instanceof BinaryMessage) {
            buf = ((BinaryMessage)msg).getPayloadData();
            type = "binary";
        }

        long sendTime = cbsImpl.receive(type, msg.getAddress(), buf, getAppID(), getGroupID(), "cbs");

        return msg;
    }

}
