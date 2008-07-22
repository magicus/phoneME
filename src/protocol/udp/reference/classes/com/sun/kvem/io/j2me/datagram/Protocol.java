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

package com.sun.kvem.io.j2me.datagram;

import javax.microedition.io.Connection;
import javax.microedition.io.Datagram;
import java.io.IOException;

public class Protocol extends com.sun.midp.io.j2me.datagram.Protocol {

    private int md = -1;

    public Connection openPrim(String name, int mode, boolean timeouts)
                                                        throws IOException {
	Connection con = super.openPrim(name, mode, timeouts);
        long groupid = System.currentTimeMillis();
        md = open0(name, mode, groupid);
        return con;
    }

    public void send(Datagram dgram) throws IOException {
        super.send(dgram);
        send0(md, dgram.getData(), dgram.getOffset(), dgram.getLength());
    }

    public synchronized void receive(Datagram dgram)
        throws IOException {
            receive0(md);
            super.receive(dgram);
            received0(md, dgram.getData(), dgram.getOffset(), dgram.getLength());
    }

    public void close() throws IOException {
        super.close();
        close0(md);
    }

    private static native int open0(String name, int mode, long groupid);
    private static native void close0(int md);
    private static native void send0(int md, byte b[], int off, int len);
    private static native void receive0(int md);
    private static native void received0(int md, byte b[], int off, int len);
}

