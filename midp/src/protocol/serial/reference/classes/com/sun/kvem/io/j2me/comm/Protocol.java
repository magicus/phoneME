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

package com.sun.kvem.io.j2me.comm;

import java.io.IOException;
import javax.microedition.io.Connection;

/**
 * The class subclass the comm protocol of the midp. By overriding
 * the connect method it wrapps the connection stream returnd with
 * a StreamConnectionStealer that "steals" the data along the way and
 * send it to the CommAgent and from there to the network monitor gui.
 *
 *@author Moshe Sayag
 *@version
 * @see com.sun.kem.netmon.StreamConnectionStealer
 */
public class Protocol
    extends com.sun.midp.io.j2me.comm.Protocol {

    private int md = -1;

    public Connection openPrim(String name, int mode, boolean timeouts)
            throws IOException {
        Connection con = super.openPrim(name, mode, timeouts);

        long groupid = System.currentTimeMillis();
        md = connect0(name, mode, groupid);
        return con;
    }

    protected void disconnect()
                       throws IOException {
        super.disconnect();
        disconnect0(md);
    }

    protected int readBytesNonBlocking(byte[] b, int off, int len)
                                throws IOException {

        int n = super.readBytesNonBlocking(b, off, len);
        read0(md, b, off, n);

        return n;
    }

    protected int nonBufferedRead(byte[] b, int off, int len)
                           throws IOException {

        int n = super.nonBufferedRead(b, off, len);
        read0(md, b, off, n);

        return n;
    }

    public int writeBytes(byte[] b, int off, int len)
                   throws IOException {

        int bytesWritten = super.writeBytes(b, off, len);
        write0(md, b, off, bytesWritten);

        return bytesWritten;
    }

    public int setBaudRate(int baudrate) {
        int oldBaudRate = super.setBaudRate(baudrate);
        int newBaudRate = getBaudRate();
        setBaudRate0(md, newBaudRate);
        return oldBaudRate;
    }

    private static native int connect0(String name, int mode, long groupid);

    private static native void disconnect0(int md);

    private static native void read0(int md, byte[] b, int off, int len);

    private static native void write0(int md, byte[] b, int off, int len);

    private static native void setBaudRate0(int md, int baudrate);
}
