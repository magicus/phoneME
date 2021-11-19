/*
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

package kdp;
import java.util.*;
import java.io.*;

abstract class ProxyListener extends Thread {

    int verbose=0;
    static int method_index_base = 0;
    static boolean VM_Version_3 = false;
    static boolean proxyMode = false;
    static int suspendCount = 0;

    // if bits 15-16 == 10 on the return option bits from the VM then the method
    // index is base one, not base zero.  We use '10' so that we protect from 
    // implementations that just blindly send all 0's or all 1's or 0xFFFF
    final int METHOD_BASE_SHIFT = 15;
    final int METHOD_BASE_BITS = 3 << METHOD_BASE_SHIFT;
    final int METHOD_BASE_ONE_FLAG = 2;

    // Support for line/var tables in VM
    //             dispose command in VM
    //             classesbysig command in VM
    final int KVM_VERSION_3_SHIFT = 17;
    final int KVM_VERSION_3_BITS = 3 << KVM_VERSION_3_SHIFT;
    final int KVM_VERSION_3_FLAG = 2;

    final int MAJOR_VERSION = 1;
    //    final int MINOR_VERSION = 2; // handle method id starting at '1'
    //    final int MINOR_VERSION = 3; // support for MVM debugging
    final int MINOR_VERSION = 4;  // Can work with VM that has line numbers


    Map waitingQueue = new HashMap(8, 0.75f);
    protected List packetQueue;

    
    public ProxyListener() {
        packetQueue = Collections.synchronizedList(new LinkedList());
    }

    public void newPacket(Packet p) {

        if (p == null) {
            synchronized(packetQueue) {
                packetQueue.notify();
            }
            return;
        }
        synchronized(packetQueue) {
            packetQueue.add(p);
            packetQueue.notify();
        }
    }

    public Packet waitForPacket() {

        synchronized(packetQueue) {
            while (packetQueue.size() == 0) {
                try {
                    packetQueue.wait();
                } catch (InterruptedException e) {
                    Log.LOGN(3, this + " waitForPacket Interrupted");
                    throw new ProxyConnectionException();
                }
            }
        }
        return ((Packet)packetQueue.remove(0));
    }

    void replyReceived(Packet p) {
        Packet p2;
        if (p == null) {
            synchronized(waitingQueue) {
                Iterator iter = waitingQueue.values().iterator();
                while (iter.hasNext()) {
                    p2 = (Packet)iter.next();
                    synchronized(p2) {
                        p2.notify();
                    }
                }
            }
            return;
        }
            
        String idString = String.valueOf(p.id);
        synchronized(waitingQueue) {
            p2 = (Packet)waitingQueue.get(idString);
            if (p2 != null)
                waitingQueue.remove(idString);
        }
        if (p2 == null) {
            System.err.println("Received reply with no sender!");
            return;
        }
        p2.errorCode = p.errorCode;
        p2.data = p.data;
        p2.replied = true;
        synchronized(p2) {
            p2.notify();
        }

    }
    
    public void waitForReply(Packet p) {

        synchronized(p) {
            while (!p.replied) {
                try { p.wait();
                } catch (InterruptedException e) {
                    System.out.println(this + " waitForReply Interrupted");
                    throw new ProxyConnectionException();
                }
            }
            if (!p.replied)
                throw new RuntimeException();
        }
    }

    public void verbose( int lvl ) {
        verbose = lvl;
    }
    abstract public void send(Packet p) throws ProxyConnectionException;


    abstract public void setStop();

    protected void vp( int vlevel, String str ) {
        if ( verbose >= vlevel )
            System.out.print( str );
    }

    protected void vpe( int vlevel, String str ) {
        if ( verbose == vlevel )
            vp( vlevel, str );
    }

    class MethodID {
        // Encapsulate method ID which can be an int for older VMs or
        // a long for newer VMs
        long _method_id_long;
        int  _method_id_int;

        public MethodID() {}

        public MethodID(int class_id, int method_id) {
            if (VM_Version_3) {
                _method_id_long = ((long)class_id << 32) + method_id;
            } else {
                _method_id_int = method_id;
            }
        }


        protected int readMethodPart(PacketStream ps) throws PacketStreamException {
            if (VM_Version_3) {
                _method_id_long = ps.readLong();
                return methodPart(_method_id_long);
            } else {
                _method_id_int = ps.readInt();
                return _method_id_int;
            }
        }

        protected int methodPart(long mid) {
            return (int)(mid & 0xFFFFFFFF);
        }

        protected void setMethodPart(int id) {
            _method_id_int = id;
            if (VM_Version_3) {
                _method_id_long = (_method_id_long & 0xFFFFFFFF00000000L) | id;
            }
        }

        protected void writeMethodID(PacketStream ps) throws PacketStreamException {
            if (VM_Version_3) {
                ps.writeLong(_method_id_long);
            } else {
                ps.writeInt(_method_id_int);
            }
        }
        public String toString() {
            if (VM_Version_3) {
                return Long.toHexString(_method_id_long);
            } else {
                return Integer.toHexString(_method_id_int);
            }
            
        }
    }
} // ProxyListener
