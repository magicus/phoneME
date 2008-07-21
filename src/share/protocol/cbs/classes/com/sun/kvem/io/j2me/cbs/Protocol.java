/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
