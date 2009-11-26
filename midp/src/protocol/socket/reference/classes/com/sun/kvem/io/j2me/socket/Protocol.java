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

package com.sun.kvem.io.j2me.socket;

import com.sun.midp.security.SecurityToken;
import javax.microedition.io.Connection;

import java.io.IOException;

public class Protocol
    extends com.sun.midp.io.j2me.socket.Protocol {

    private int md = -1;
    private int port = -1;

    public Protocol() {
    }

    public Protocol(int port) {
        this.port = port;
    }

    public Connection openPrim(String name, int mode, boolean timeouts)
                 throws IOException {
        Connection con = super.openPrim(name, mode, timeouts);
        long groupid = System.currentTimeMillis();
        md = connect0(name, mode, groupid);
        connectionOpen = true;
        try{
            initSocketOption();
        } catch (IOException e){
            // ignore it
        }
        return con;
    }

    public void open(SecurityToken token) throws IOException {
        super.open(token);
        md = connect0("//:" + port, 3 /* READ_WRITE*/, -1);
    }

    public void disconnect()
                    throws IOException {
        super.disconnect();
        disconnect0(md);
    }

    protected int nonBufferedRead(byte[] b, int off, int len)
                           throws IOException {
        int n = super.nonBufferedRead(b, off, len);
        if (n > 0) {
            read0(md, b, off, n);
        }

        return n;
    }

    public int writeBytes(byte[] b, int off, int len)
                   throws IOException {

        int n = super.writeBytes(b, off, len);
        write0(md, b, off, len);

        return n;
    }

    public void setSocketOption(byte option, int value)
                         throws IOException {
        super.setSocketOption(option, value);
        setSocketOption0(md, option, value);
    }

    public void initSocketOption()
                          throws IOException {

        for (byte i = 0; i < 5; i++) {
            setSocketOption0(md, i, super.getSocketOption(i));
        }
    }

    protected native int connect0(String name, int mode, long groupid);

    protected native void disconnect0(int md);

    protected native void read0(int md, byte[] b, int off, int len);

    protected native void write0(int md, byte[] b, int off, int len);

    protected native void setSocketOption0(int md, byte option, int value);


    /**
     * Get the implementing serversocket class.
     *
     * @return     serversocket class
     */
    protected com.sun.midp.io.j2me.serversocket.Socket getServersocketClass() {
        return new com.sun.kvem.io.j2me.serversocket.Socket();
    }

}
