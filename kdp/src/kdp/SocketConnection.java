/*
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
 * 
 * 
 */

package kdp;

import kdp.classparser.*;
import java.net.*;
import java.io.*;
import java.util.*;

class SocketConnection implements Runnable {
    Socket socket;
    DataOutputStream out;
    DataInputStream in;
    ProxyListener proxy;

    SocketConnection(ProxyListener proxy, Socket socket)
        throws IOException {

        this.proxy = proxy;
        this.socket = socket;
        socket.setTcpNoDelay(true);
        in = new DataInputStream(new BufferedInputStream(socket.getInputStream()));
        out = new DataOutputStream(new BufferedOutputStream(socket.getOutputStream()));
    }

    public void close() {
        try {
            out.flush();
            out.close();
            in.close();
            socket.close();
        } catch (Exception e) {
            ;
        }
    }

    public byte receiveByte() throws IOException {
            int b = in.read();
            return (byte)b;
    }

    public void sendByte(byte b) throws IOException {
            out.write(b);
            out.flush();
        }

    public void run() {
        //        Thread.currentThread().setPriority(Thread.MAX_PRIORITY);
        try {
            while (true) {
                Packet p = receivePacket();
                if ((p.flags & Packet.Reply) == 0 || p.id >= 0) {
                    proxy.newPacket(p);
                } else {
                    // A reply to a packet we generated and sent to VM
                    proxy.replyReceived(p);
                }
            }
        } catch (Exception e) {
            Log.LOGN(4, "Socket exception in " + proxy + e + " ...exiting");
            //                  e.printStackTrace();
            proxy.setStop();
        }
    }

    public Packet receivePacket() throws IOException {

        Packet p = new Packet();
    
        // length
        int length = in.readInt();

        // id
        p.id = in.readInt();
    
        p.flags = in.readByte();
        if ((p.flags & Packet.Reply) == 0) {
            p.cmdSet = in.readByte();
            p.cmd = in.readByte();
        } else {
            p.errorCode = in.readShort();
        }
    
        length -= 11; // subtract the header
    
        if (length < 0) {
            // This shouldn't be happening!
            throw new IOException("packet length < 0");
        }
        p.data = new byte[length];
    
        int n = 0;
        while (n < p.data.length) {
            int count = in.read(p.data, n, p.data.length - n);
            if (count < 0) {
                throw new EOFException();
            }
            n += count;
        }
    
        return p;
    }

    public void send(Packet p) throws IOException {

        int length = p.data.length + 11;
    
        // Length
        out.writeInt(length);

        // id
        out.writeInt(p.id);
    
        out.write(p.flags);
    
        if ((p.flags & Packet.Reply) == 0) {
            out.write(p.cmdSet);
            out.write(p.cmd);
        } else {
            out.writeShort(p.errorCode);
        }
        out.write(p.data);
    
        out.flush();
    }
}

