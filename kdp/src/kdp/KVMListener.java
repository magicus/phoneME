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

import kdp.classparser.*;
import kdp.classparser.attributes.*;
import java.io.*;
import java.net.*;

class KVMListener extends ProxyListener implements VMConstants {

    SocketConnection connKvm;
    ProxyListener debuggerListener=null;
    ClassManager manager;
    boolean Ready = false;
    Socket remoteSocket = null;
    boolean stopKVMListener = false;
    Thread myThread;
    int localPort = 0;
    
    public KVMListener() {
        super();
        proxyMode = Options.getProxyMode();
    }

    public void set( ProxyListener debuggerListener, ClassManager manager ) {
        this.debuggerListener = debuggerListener;
        this.manager = manager;
    }
    /*
     * Sends packet to our output socket
     */
    public synchronized void send(Packet p) throws ProxyConnectionException{
        while (!Ready) {
            synchronized(this) {
                try {
                    this.wait();
                } catch (InterruptedException e) {
                }
            }
        }
        if (p == null) {
            return;
        }
        String id = String.valueOf(p.id);
        if ((p.flags & Packet.Reply) == 0 && p.id < 0) {
            synchronized(waitingQueue) {
                waitingQueue.put(id, p);
            }
        }
        try {
            connKvm.send(p);
        } catch (IOException e) {
            throw new ProxyConnectionException();
        }
    }

    public void setStop() {
        Log.LOGN(3, "KVMlistener: setStop called");
        stopKVMListener = true;
        myThread.interrupt();
        synchronized(packetQueue) {
            packetQueue.notify();
        }
        synchronized(waitingQueue) {
            waitingQueue.notify();
        }
        try {
            if (remoteSocket != null) {
                remoteSocket.close();
            }
        } catch (IOException e) {}
    }


    public void run() {

        boolean handled;
        PacketStream ps;
        byte typeTag;
        int classID;
        String className;
        int classStatus;
        ClassFile cf;

        this.myThread = Thread.currentThread();
        try {
            /* Attempt to reconnect to KVM by polling every 2000ms until 
             * connection is established.
             */
            while (remoteSocket == null) {
                try { 
                    Log.LOGN(3, "KVMListener: attempting connection");
                    remoteSocket = new Socket(Options.getRemoteHost(), 
                                              Options.getRemotePort());
                    Log.LOGN(3, "KVMListener: got connection " + remoteSocket);
                } catch (ConnectException e) {
                    System.err.println("KVMListener: Exception " + e + " KVM not ready");
                    try {
                        Thread.sleep(2000);
                    } catch (InterruptedException ie) {}
                }
            }
            localPort = remoteSocket.getLocalPort();
            connKvm = new SocketConnection(this, remoteSocket);
        } catch (IOException e) {
            System.out.println("KVMListener: " + e.getMessage());
        } catch (SecurityException e) {
            System.out.println("KVMListener: " + e.getMessage());
        }

        if (!proxyMode) {
            byte [] handshake = new String("JDWP-Handshake").getBytes();
            try {
                // debugger -> vm
                for ( int i=0; i < handshake.length; i++ )
                    connKvm.sendByte(handshake[i]);

                // debugger <- vm
                for ( int i=0; i < handshake.length; i++ )
                    connKvm.receiveByte();
            } catch (IOException e) {
            }
        }

        synchronized(this) {
            Ready = true;
            this.notify();
        }
        new Thread(connKvm).start();
        
        if (proxyMode) {
            
            ps = new PacketStream(this, KVM_CMDSET, KVM_HANDSHAKE_CMD);
            ps.writeString("KVM Reference Debugger Agent");
            ps.writeByte((byte)MAJOR_VERSION);    // Major version
            ps.writeByte((byte)MINOR_VERSION);    // minor version
            try {
                ps.send();
                ps.waitForReply();
            } catch (Exception e) {
                System.out.println("Exception during handshake: " + e +
                                   " exiting...");
                Runtime.getRuntime().exit(1);
            }
            String s = ps.readString();
            int option_bits = ps.readInt();
            Log.LOGN(1, "KVM Handshake return string: " + s);
            Log.LOGN(1, "KVM Handshake return options: " + Log.intToHex(option_bits));

            if (((option_bits & METHOD_BASE_BITS) >> METHOD_BASE_SHIFT) == METHOD_BASE_ONE_FLAG) {
                Log.LOGN(1, "Method index base being set to 1");
                method_index_base = 1;
            }
            if (((option_bits & KVM_VERSION_3_BITS) >> KVM_VERSION_3_SHIFT) == KVM_VERSION_3_FLAG) {
                Log.LOGN(1, "VM has line number table");
                Log.LOGN(1, "VM supports DISPOSE command");
                Log.LOGN(1, "VM supports ClassesBySig command");
                VM_Version_3 = true;
            }
            /*
            Log.LOGN(3, "KVMListener: Sending AllClasses command");
            ps = new PacketStream(this, VIRTUALMACHINE_CMDSET,
                                  ALL_CLASSES_CMD);
            try {
                ps.send();
                ps.waitForReply();
                int numClasses = ps.readInt();
                Log.LOGN(2, numClasses + " classes");
                for (int i = 0; i < numClasses; i++) {
                    typeTag = ps.readByte();
                    classID = ps.readInt();
                    className = ps.readString();
                    if (typeTag != TYPE_TAG_ARRAY) {
                        // strip off leading 'L' and trailing ';'
                        className = new String(className.substring(1,
                                                                   className.length() - 1));
                    }
                    classStatus = ps.readInt();
                    if ((cf =
                         (ClassFile)ClassManager.classMap.get(
                               new Integer(classID))) == null) {
                        Log.LOGN(3, "allclasses: new class: " +
                                 className + " " + Log.intToHex(classID));
                        cf = manager.findClass(classID, className,
                                               typeTag, classStatus);
                        if (cf == null) {
                            Log.LOGN(3, "allclasses: couldn't find class "
                                     + className);
                        }
                    }
                }
            } catch (Exception e) {
                System.out.println("Couldn't get list of classes from KVM");
            }
            */
        }
        DebuggerListener.OptionsReady();  // options ready for use
        try {
            while (!stopKVMListener) {
                PacketStream in;
     
                handled = false;
                Packet p = waitForPacket();
                if (p == null){
                    Log.LOGN(3, "KVMListener: quitting");
                    debuggerListener.setStop();
                    break;        // must be time to quit
                }

                Log.LOG(3, "KVMListener: start:" + localPort + ": ");
                Log.LOGN(3, p);

                if (proxyMode && (p.flags & Packet.Reply) == 0) {
                    switch (p.cmdSet) {
                    case EVENT_CMDSET:
                        switch (p.cmd) {
                        case COMPOSITE_CMD:
                            in = new PacketStream(this, p);
                            byte suspendPolicy = in.readByte();
                            if (suspendPolicy > 0) {
                                suspendCount++;
                            }
                            int numEvents = in.readInt();
                            // we KNOW that KVM only sends one event
                            byte eventKind = in.readByte();
                            if (eventKind != JDWP_EventKind_CLASS_PREPARE) {
                                break;
                            }
                            int requestID = in.readInt();
                            int threadID = in.readInt();
                            typeTag = in.readByte();
                            classID = in.readInt();
                            className = in.readString();
                            classStatus = in.readInt();
                            if (typeTag != TYPE_TAG_ARRAY) {
                                // strip off leading 'L' and trailing ';'
                                className =
                                    new String(className.substring(1,
                                               className.length() - 1));
                            }
                            Log.LOGN(3, "ClassPrepare:  " + className +
                                     ", ID = " + Log.intToHex(classID));
                            // see if we have a class parser reference
                            if ((cf = (ClassFile)manager.classMap.get(new Integer(classID))) == null) {
                                cf = manager.findClass(classID, className,
                                                       typeTag, classStatus);
                                if (cf == null) {
                                    Log.LOGN(3, "ClassPrepare: Could not load classfile " + className);
                                }
                            } else {
                                Log.LOGN(3, "ClassPrepare: got classfile " +
                                         cf.getClassName());
                                cf.setClassStatus(classStatus);
                            }
                            break;
                        }
                        break;
                    case KVM_CMDSET:
                        switch (p.cmd) {
                        case KVM_STEPPING_EVENT_CMD: /* need stepping info */
                            in = new PacketStream(this, p);
                            handleSteppingInfo(in);
                            handled = true;
                            break;
                        }
                        break;
                    }
                }
                if ( !handled ) {
                    Log.LOG(6, "KVMListener:" + localPort + ": ");
                    Log.LOGN(6, p);
                    debuggerListener.send(p);
                }
            }
        } catch (ProxyConnectionException e) {
            Log.LOGN(3, "KVMListener: caught ProxyConnectionException");
            debuggerListener.setStop();
            return;
        }
    }

    private void handleSteppingInfo(PacketStream in)
        throws ProxyConnectionException {

        ClassFile cf;
        MethodInfo mi;
        int index, index2, currentIndex;
        long offs, offs2, offs3;

        int cep = in.readInt();    // opaque buffer pointer we just pass back
        int cid = in.readInt();
        MethodID mid = new MethodID();
        int methodIndex = mid.readMethodPart(in) - method_index_base;
        long offset = in.readLong();

        Log.LOGN(3, "handleSteppingInfo: cep = " + Log.intToHex(cep) +
                 " cid = " + Log.intToHex(cid) + " mid = " + mid.toString());
        PacketStream ps = new PacketStream(this, KVM_CMDSET,
                                           KVM_GET_STEPPINGINFO_CMD);

        ps.writeInt(cep);
        cf = (ClassFile)manager.classMap.get(new Integer(cid));
        if (cf == null || (mi = cf.getMethodInfoByIndex(methodIndex)) == null) {
            ps.writeLong(0);
            ps.writeLong(0);
            ps.send();
            ps.waitForReply();
            return;
        }

        LineNumberTableAttribute table = null;
        CodeAttribute ca = mi.getCodeAttribute();
        if (ca != null) {
            table = ca.getLineNumberTable();
        }
/*
  int line = table.getLineThatContainsOpcode( offset );
  System.out.println( "handle..." + table.getLineThatContainsOpcode( 271 ) );
*/

        //line = table.getNextExecutableLineIndex( line );
        if (ca == null || table == null) {
            offs = offs2 = offs3 = -1;
            index = index2 = currentIndex = -1;
        } else {
            // where are we now.
            currentIndex = table.getCurrentLineCodeIndex(offset);
            // what is the index of the next line we should execute
            index = table.getNextExecutableLineCodeIndex( offset );
            // does the current offset have a duplicate line?
            index2 = table.getDupCurrentExecutableLineCodeIndex(offset);
            // offset of current line
            offs = table.getStartPCFromIndex(index);
            // offset of possible duplicate of current line
            offs2 = table.getStartPCFromIndex(index2);
            // offset of next line after duplicate
            offs3 = table.getOffsetofDupNextLine(index2);
        }
        Log.LOGN(3, "handleSteppingInfo  current offset = " + offset );
        Log.LOGN(3, "handleSteppingInfo  target offset = " + offs );
        Log.LOGN(3, "handleSteppingInfo  dup of current line offset = " + offs2 );
        Log.LOGN(3, "handleSteppingInfo  offset after current dup = " + offs3 );
        if (table != null) {
            Log.LOGN(3, "handleSteppingInfo current line number = " + table.getLineNumberFromIndex(currentIndex));
        }
        /*
          long offs = table.getCodeIndexBySourceLineNumber( line );
        */

        ps.writeLong( offs );
        ps.writeLong( offs2 );
        ps.writeLong( offs3 );
        
        ps.send();
        ps.waitForReply();
    }

    public String toString() {
        return (new String("KVMListener: "));
    }
} // KVMListener
