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

package com.sun.kvem.io.j2me.ssl;

import com.sun.kvem.netmon.SSLInputAgent;
import com.sun.kvem.netmon.SSLOutputAgent;
import com.sun.kvem.netmon.StreamConnectionStealer;

import javax.microedition.io.Connection;
import javax.microedition.io.StreamConnection;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class Protocol extends com.sun.midp.io.j2me.ssl.Protocol {
    private int md = -1;
    private StreamConnectionStealer stealer;

    public Connection openPrim(String name, int mode, boolean timeouts) throws IOException {
        StreamConnection con = (StreamConnection) super.openPrim(name, mode, timeouts);
        SSLInputAgent inAgent = new SSLInputAgent(this);
        SSLOutputAgent outAgent = new SSLOutputAgent(this);
        stealer = new StreamConnectionStealer(name, con, inAgent, outAgent);
        InputStream in = super.openInputStream();
        OutputStream out = super.openOutputStream();
        stealer.setStreams(in, out);
        long groupid = System.currentTimeMillis();
        md = connect0(name, mode, groupid);
//        connectionOpen = true;
        initSocketOption();
        return this;
    }

    public void close() throws IOException {
        super.close();
        stealer.close();
        disconnect0(md);
    }

    public InputStream openInputStream() throws IOException {
        return stealer.openInputStream();
    }

    public DataInputStream openDataInputStream() throws IOException {
        return stealer.openDataInputStream();
    }

    public OutputStream openOutputStream() throws IOException {
        return stealer.openOutputStream();
    }

    public DataOutputStream openDataOutputStream() throws IOException {
        return stealer.openDataOutputStream(); 
    }

    public void read(byte b[], int off, int len) {
        read0(md, b, off, len);
    }
    public void write(byte b[], int off, int len) {
        write0(md, b, off, len);
    }

    public void setSocketOption(byte option,  int value)
            throws IllegalArgumentException, IOException {
        super.setSocketOption(option, value);
        setSocketOption0(md, option, value);
    }

    public void initSocketOption()
                          throws IOException {

        for (byte i = 0; i < 5; i++) {     
            setSocketOption0(md, i, super.getSocketOption(i));
        }
    }

    private static native int connect0(String name, int mode, long groupid);
    private static native void disconnect0(int md);
    private static native void read0(int md, byte b[], int off, int len);    
    private static native void write0(int md, byte b[], int off, int len);
    private static native void setSocketOption0(int md, byte option,  int value);    
}

