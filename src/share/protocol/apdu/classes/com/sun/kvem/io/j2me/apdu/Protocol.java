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

package com.sun.kvem.io.j2me.apdu;

import javax.microedition.io.*;

import java.io.*;

public class Protocol extends com.sun.midp.io.j2me.apdu.Protocol {

    private int md = -1;

    public Protocol() {};

    public Connection openPrim(String name, int mode, boolean timeouts)
                                                        throws IOException {
        Connection c = super.openPrim(name, mode, timeouts);
        long groupid = System.currentTimeMillis();
        md = open0(name,  groupid);
        exchangeAPDU0(md, getConnectionResult());
        return c;
    }

    public byte[] exchangeAPDU(byte[] commandAPDU) throws
        IOException, InterruptedIOException {
            byte[] b = super.exchangeAPDU(commandAPDU);
            //If an exception has not been thrown:
            exchangeAPDU0(md, commandAPDU);
            exchangeAPDU0(md, b);
            return b;
    }

    private static native int open0(String buf, long groupid);

    private static native void exchangeAPDU0(int md, byte[] buf);
}
