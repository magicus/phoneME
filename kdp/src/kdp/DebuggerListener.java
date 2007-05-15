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
import java.util.*;
import java.net.*;
import java.io.*;

class DebuggerListener extends ProxyListener implements VMConstants {

    SocketConnection connDebugger;
    ProxyListener KVMListener;
    ClassManager manager = null;
    Packet replyPacket;
    Options options;
    boolean Ready = false;
    ServerSocket serverSocket = null;
    Socket acceptSocket = null;
    boolean stopDebuggerListener = false;
    Thread myThread;
    static boolean options_ready = false;
    int remotePort = 0;

    public DebuggerListener(ServerSocket so, Options opt) {
        super();
        this.options = opt;
        serverSocket = so;
    }

                     
    public void set(ProxyListener KVMListener, ClassManager manager) {
        this.KVMListener = KVMListener;
        this.manager = manager;
    }

    public static void OptionsReady() {
        options_ready = true;
    }

    /*
     * Sends packet to our output socket
     */
    public synchronized void send(Packet p) throws ProxyConnectionException{
        while (!Ready) {
            try {
                wait();
            } catch (InterruptedException e) {
            }
        }
        String id = String.valueOf(p.id);
        synchronized(waitingQueue) {
            if ((p.flags & Packet.Reply) == 0 && p.id < 0) {
                waitingQueue.put(id, p);
            }
        }
        try {
            connDebugger.send(p);
        } catch (IOException e) {
            throw new ProxyConnectionException();
        }
    }

    public void setStop() {
        Log.LOGN(3, "Debuggerlistener: setStop called");
        myThread.interrupt();
        stopDebuggerListener = true;
        synchronized(packetQueue) {
            packetQueue.notify();
        }
        synchronized(waitingQueue) {
            waitingQueue.notify();
        }
        try {
            if (acceptSocket != null) {
                acceptSocket.close();
            }
        } catch (IOException e) {}
    }

    private void sendErrorReply(ProxyListener proxy, PacketStream in,
                                short errorCode) throws ProxyConnectionException {
        PacketStream ps = new PacketStream(proxy, in.id(),
                                           Packet.Reply, errorCode);
        ps.send();
    }

    public void connectToDebugger() {
        try {
            Log.LOGN(3, "Waiting for debugger on port " +
                     serverSocket.getLocalPort());
            acceptSocket = serverSocket.accept();
            connDebugger = new SocketConnection(this,  acceptSocket);
            Log.LOGN(3, "Debugger connection received. " + acceptSocket);
            remotePort = acceptSocket.getPort();
        } catch (IOException e) {
            System.out.println("DebuggerListener0: " + e + " " + e.getMessage());
        } catch (SecurityException e) {
            System.out.println("DebuggerListener1: " + e + " " + e.getMessage());
        }
    }

    public void run() {
        boolean handled;
        Packet p=null;
        String classname=null, methodname=null;
        PacketStream ps=null;
        PacketStream in=null;

        byte [] handshake = new String("JDWP-Handshake").getBytes();
        this.myThread = Thread.currentThread();

        // debugger -> kvm
        try {
            for (int i=0; i < handshake.length; i++)
                connDebugger.receiveByte();
            
            // debugger <- kvm
            for (int i=0; i < handshake.length; i++)
                connDebugger.sendByte(handshake[i]);
        } catch (IOException e) {}

        synchronized(this) {
            Ready = true;
            notify();
        }

        new Thread(connDebugger).start();
        proxyMode = Options.getProxyMode();

        try {
            while (!stopDebuggerListener) {

                handled = false;
                p = waitForPacket();        /* wait for a packet */
                if (p == null) {
                    Log.LOGN(3, "DebuggerListener: quitting");
                    KVMListener.setStop();
                    break;                  // must be time to quit
                }

                if ((p.flags & Packet.Reply) == 1 || proxyMode == false) {
                    Log.LOG(3, "DebuggerListener: ");
                    Log.LOGN(3, p);
                    KVMListener.send(p);
                    continue;
                }

                Log.LOG(3, "DebuggerListener: start: " + remotePort + ": ");
                Log.LOGN(3, p);
 
                in = new PacketStream(this, p);
                switch (p.cmdSet) {
                case VIRTUALMACHINE_CMDSET:
                    handled = doVirtualMachineCmdset(in);
                    break;

                case REFERENCE_TYPE_CMDSET:
                    handled = doReferenceTypeCmdset(in);
                    break;
                        
                case METHOD_CMDSET:
                    handled = doMethodCmdset(in);
                    break;                        

                case THREADREFERENCE_CMDSET:
                    handled = doThreadReferenceCmdset(in);
                    break;                        

                case THREADGROUPREFERENCE_CMDSET:
                    handled = doThreadGroupReferenceCmdset(in);
                    break;

                case STACKFRAME_CMDSET:
                    handled = doStackFrameCmdset(in);
                    break;

                }
               
                if (!handled) {
                    Log.LOG(6, "DebuggerListener:" + remotePort + ": ");
                    Log.LOGN(6, p);
                    KVMListener.send(p);
                }
            }
        } catch (ArrayIndexOutOfBoundsException e) {
            System.out.println(p.cmdSet + "/" + p.cmd + " caused: " + e);
            e.printStackTrace();
            PacketStream error = new PacketStream(this, in.id(), Packet.Reply, NOTFOUND_ERROR);
            error.send();
        } catch (ProxyConnectionException e) {
            // connection has dropped, time to quit
            Log.LOGN(3, "DebuggerListener: caught ProxyConnectionException");
            KVMListener.setStop();
            return;
        }
    }

    private boolean doVirtualMachineCmdset(PacketStream in)
        throws ProxyConnectionException {
        boolean handled = false;
        PacketStream ps;
        ClassFile cf;
        int cid;

        switch(in.cmd()) {
        case SENDVERSION_CMD:
            ps = new PacketStream(this,
                                  in.id(), Packet.Reply, Packet.ReplyNoError);
            ps.writeString("Version 1.0");
            ps.writeInt(MAJOR_VERSION);             /* major version */
            ps.writeInt(0);                         /* minor version */
            ps.writeString("1.1.3");
            ps.writeString("KVM/CLDC_VM");
            ps.send(); 
            
            handled = true;
            break;

        case CLASSESBYSIG_CMD:
            String classToMatch = null;
            try {
                ps = new PacketStream(this, in.id(), Packet.Reply,
                                      Packet.ReplyNoError);
                classToMatch = in.readString();
                if (classToMatch == null || classToMatch.length() < 2 ||
                    (classToMatch.length() == 2 &&
                     classToMatch.charAt(0) == 'L')) {
                    Log.LOGN(3, "ClassBySig: null class ");
                    ps.writeInt(0);
                    ps.send();
                    handled = true;
                    break;
                }
                Log.LOGN(3, "ClassBySig: class " + classToMatch);
                Iterator iter =
                    manager.classMap.values().iterator();
                
                while (iter.hasNext()) {
                    cf = (ClassFile)iter.next();
                    if (cf == null) {
                        Log.LOGN(3, "ClassesBySig: Couldn't find classFile object");
                        sendErrorReply(this, in, NOTFOUND_ERROR);
                        
                        handled = true;
                        break;
                    }
                    if (classToMatch.compareTo(cf.getClassSignature()) == 0) {
                        Log.LOGN(3, "ClassBySig matched " + cf.getClassName());
                        Log.LOGN(6, "Class Signature: "+ cf.getClassSignature());
                        ps.writeInt(1);
                        ps.writeByte(cf.getJDWPTypeTag());
                        ps.writeInt(cf.getClassID());
                        ps.writeInt(cf.getClassStatus());
                        ps.send();
                        handled = true;
                        break;
                    }
                }
                if (!handled) {
                    Log.LOGN(3, "ClassBySig: class not found: " + classToMatch);
                    VMCall vmCall = new VMCall();
                    cf = vmCall.callVMForClassBySig(classToMatch, "ClassBySig");
                    if (cf == null) {
                        ps.writeInt(0);
                        ps.send();
                        handled = true;
                        break;
                    }
                    int numClasses = vmCall.getNumClasses();
                    Log.LOGN(3, "ClassBySig: VM: numclasses " + numClasses);
                    ps.writeInt(numClasses);
                    if (numClasses > 0) {
                        ps.writeByte(cf.getJDWPTypeTag());
                        ps.writeInt(cf.getClassID());
                        ps.writeInt(cf.getClassStatus());
                    }
                    ps.send();
                    handled = true;
                }
            } catch(PacketStreamException e) {
                // need revisit : where the exception happened so
                // just send an error back
                Log.LOGN(3, "ClassBySig: VM exception: " + e);
                sendErrorReply(this, in, ILLEGAL_ARGUMENT);
                handled = true;
                break;
            }
            break;

        case ALL_CLASSES_CMD:
            {
                String className;
                int classStatus;
                byte typeTag;

                /* we intercept all the statuses and then just pass it on */
                Log.LOGN(3, "All_Classes command");
                ps = new PacketStream(KVMListener,
                                      VIRTUALMACHINE_CMDSET, ALL_CLASSES_CMD);
                PacketStream ps2 = new PacketStream(this, in.id(),
                                                    Packet.Reply, Packet.ReplyNoError);
                ps.send();
                ps.waitForReply();
                int numClasses = ps.readInt();    // get number of classes
                ps2.writeInt(numClasses);
                for (int i = 0; i < numClasses; i++) {
                    typeTag = ps.readByte();        // typeTag
                    cid = ps.readInt();             // ID
                    className = ps.readString();
                    classStatus = ps.readInt();
                    // pass the info on up to the debugger
                    ps2.writeByte(typeTag);
                    ps2.writeInt(cid);
                    ps2.writeString(className);
                    ps2.writeInt(classStatus);
                    if (typeTag != TYPE_TAG_ARRAY) {
                        // strip off leading 'L' and trailing ';'
                        className =
                            new String(className.substring(1,
                                                           className.length() - 1));
                    }
                    Log.LOGN(3, "AllClasses:  " + className + ", ID = "
                             + Log.intToHex(cid));
                    if ((cf =
                         (ClassFile)manager.classMap.get(new
                                                 Integer(cid))) == null) {
                        cf = manager.findClass(cid, className,
                                               typeTag, classStatus);
                        if (cf == null) {
                            Log.LOGN(3, "ALL_CLASSES_CMD: couldn't find classfile object");
                            //  sendErrorReply(this, in, NOTFOUND_ERROR);
                            //                 handled = true;
                            //  break;
                        }
                    } else {
                        cf.setClassStatus(classStatus);
                    }
                }
                ps2.send();
                handled = true;
            }
            break;

        case TOPLEVELTHREADGROUP_CMD:
            ps = new PacketStream(this,
                                  in.id(), Packet.Reply, Packet.ReplyNoError);
            ps.writeInt(1);
            ps.writeInt(ONLY_THREADGROUP_ID);
            ps.send();
            handled = true;
            break;
                            
        case DISPOSE_CMD:
            Log.LOGN(3, "Dispose");
            if (VM_Version_3) {
                ps = new PacketStream(KVMListener,
                                      VIRTUALMACHINE_CMDSET, DISPOSE_CMD);
                ps.send();
            } else {
                ps = new PacketStream(KVMListener,
                                      VIRTUALMACHINE_CMDSET, EXIT_CMD);
                ps.writeInt(0);
                ps.send();
            }
            PacketStream ps2 =
                new PacketStream(this, in.id(),
                                 Packet.Reply, Packet.ReplyNoError);
            ps2.send();
            handled = true;
            setStop();
            break;
                    
                            
        case IDSIZES_CMD:
            // We have to wait until kdp gets the options from the VM 
            // to determine if we can use 4 or 8 byte method IDs
            while (!options_ready) {
                synchronized(this) {
                    try {
                        wait(100);
                    } catch (InterruptedException e) {
                        throw new ProxyConnectionException();
                    }
                }
            }
            Log.LOGN(3, "IDSizes");
            ps =
                new PacketStream(this,
                                 in.id(), Packet.Reply, Packet.ReplyNoError);
            ps.writeInt(8);             // sizeof field ID in KVM
            if (VM_Version_3) {
                ps.writeInt(8);             // sizeof method ID in KVM
            } else {
                ps.writeInt(4);             // sizeof method ID in KVM
            }
            ps.writeInt(4);             // sizeof object ID in KVM
            ps.writeInt(4);             // sizeof reference ID in KVM
            ps.writeInt(4);             // sizeof frame ID in KVM
            ps.send();
            handled = true;
            break;

        case CAPABILITIES_CMD:
            ps = new PacketStream(this, in.id(), Packet.Reply,
                                  Packet.ReplyNoError);
            ps.writeBoolean(false);
            ps.writeBoolean(false);
            ps.writeBoolean(true);    // can get bytecodes
            ps.writeBoolean(false);
            ps.writeBoolean(false);
            ps.writeBoolean(false);
            ps.writeBoolean(false);
            ps.send();
            handled = true;
            break;
                            
        case CLASSPATHS_CMD:
            ps = new PacketStream(this, in.id(), Packet.Reply,
                                  Packet.ReplyNoError);
            ps.writeString("");
            ps.writeInt(0);             // # of paths in classpath
            ps.writeInt(0);             // # of paths in bootclasspath
            ps.send();
            handled = true;
            break;

        case DISPOSE_OBJECTS_CMD:
            Log.LOGN(3, "Dispose Objects: ");
            ps = new PacketStream(this, in.id(), Packet.Reply,
                                  Packet.ReplyNoError);
            ps.send();
            handled = true;
            break;

        case RESUME_CMD:
            if (options.getNetbeans40compat()) {
                Log.LOGN(3, "Resume Command: ");
                suspendCount--;
                if (suspendCount < 0) {
                    Log.LOGN(3, "Resume Command: not forwarding ");
                    suspendCount = 0;
                    // Don't send to VM, VM isn't suspended
                    handled = true;
                }
            }
            break;
        }
        return handled;
}

    private boolean doReferenceTypeCmdset(PacketStream in)
        throws ProxyConnectionException {
        boolean handled = false;
        ClassFile cf;
        int cid;

        PacketStream ps =
            new PacketStream(this, in.id(), Packet.Reply,
                             Packet.ReplyNoError);

        switch(in.cmd()) {
                            
        case SIGNATURE_CMD:
            /*
             * Implementation note:  If you fail to find the
             * ClassFile object here, it could be because the
             * debugger has not requested ClassPrepare events.
             * In that case the classes loaded in the VM may be
             * out of synch with the classes that the debug agent
             * thinks are loaded.  One way to potentially
             * fix this would be to do an AllClasses command to
             * the VM in the event of failure to refresh our list
             * of classes.
             */
            try {
                cid = in.readInt();
            } catch(PacketStreamException e) {
                Log.LOGN(3, "Signature cmd: exception: " + e);
                sendErrorReply(this, in, INVALID_OBJECT);
                handled = true;
                break;
            }
            Log.LOGN(3, "Signature cmd: class id = " + Log.intToHex(cid));
            if (cid == ONLY_THREADGROUP_ID) {
                ps.writeString("Lkvm_threadgroup;");
                ps.send();
                handled = true;
                break;
            }
            if ((cf = (ClassFile)manager.classMap.get(new Integer(cid))) == null) {
                VMCall vmCall = new VMCall();
                cf = vmCall.callVMForClass(cid, "Signature");
                if (cf == null) {
                    Log.LOGN(3, "Signature cmd: couldn't find classfile object");
                    sendErrorReply(this, in, INVALID_OBJECT);
                    handled = true;
                    break;
                }
            }
            Log.LOGN(3, "Signature cmd: returning " + cf.getClassSignature());
            ps.writeString(cf.getClassSignature());
            ps.send();
            handled = true;
            break;
            
        case CLASSLOADER_CMD:
            try {
                in.readInt();       // ignore the reference ID
            } catch (PacketStreamException e) {
                // ignore it, return 0
            }
            ps.writeInt(0);     // the null classloader object
            ps.send();
            handled = true;
            break;
            
        case MODIFIERS_CMD:
            try {
                cid = in.readInt();
            } catch (PacketStreamException e) {
                System.out.println("Modifiers cmd: exception: " + e);
                sendErrorReply(this, in, INVALID_OBJECT);
                handled = true;
                break;
            }
            if ((cf = (ClassFile)manager.classMap.get(new Integer(cid))) == null) {
                VMCall vmCall = new VMCall();
                cf = vmCall.callVMForClass(cid, "Modifiers");
                if (cf == null) {
                    Log.LOGN(3, "Modifiers cmd: Couldn't get ClassFile object");
                    sendErrorReply(this, in, INVALID_OBJECT);
                    handled = true;
                    break;
                }
            }
            ps.writeInt(cf.getRawAccessFlags());
            ps.send();
            handled = true;
            break;
            
        case FIELDS_CMD:
            try {
                cid = in.readInt();
            } catch (PacketStreamException e) {
                System.out.println("Fields cmd: exception: " + e);
                sendErrorReply(this, in, INVALID_OBJECT);
                handled = true;
                break;
            }
            Log.LOGN(3, "field command: cid = " + Log.intToHex(cid));
            if ((cf = (ClassFile)manager.classMap.
                 get(new Integer(cid))) == null) {
                VMCall vmCall = new VMCall();
                cf = vmCall.callVMForClass(cid, "Fields");
                if (cf == null) {
                    Log.LOGN(3, "field_cmd: cf == null");
                    ps.writeInt(0);
                    ps.send();
                    handled = true;
                    break;
                }
            }
            if (processFields(cf, ps, in)) {
                ps.send();
            }
            handled = true;
            break;
            
        case METHODS_CMD:
            try {
                cid = in.readInt();
            } catch (PacketStreamException e) {
                System.out.println("Methods cmd: exception: " + e);
                sendErrorReply(this, in, INVALID_OBJECT);
                handled = true;
                break;
            }

            Log.LOGN(3, "methods command: cid = " + Log.intToHex(cid));
            if ((cf = (ClassFile)manager.classMap.
                 get(new Integer(cid))) == null) {
                VMCall vmCall = new VMCall();
                cf = vmCall.callVMForClass(cid, "Methods");
                if (cf == null) {
                    Log.LOGN(3, "method_cmd: cf == null");
                    ps.writeInt(0);
                    ps.send();
                    handled = true;
                    break;
                }
            }
            if (cf.getJDWPTypeTag() == TYPE_TAG_ARRAY) {
                Log.LOGN(3, "methods_cmd: cf == arrayclass");
                ps.writeInt(0);
                ps.send();
                handled = true;
                break;
            }
            Log.LOGN(3, "methods for " + cf.getClassFileName());
            MethodInfo mi;
            int index = method_index_base;
            MethodID mid = new MethodID(cf.getClassID(), index);
            List miList = cf.getAllMethodInfo();
            Iterator iter = miList.iterator();
            Log.LOGN(5, "Methods: " + miList.size() + " methods");
            ps.writeInt(miList.size());
            while (iter.hasNext()) {
                mi = (MethodInfo)iter.next();
                if (mi == null) {
                    Log.LOGN(3, "Methods cmd: MethodInfo is null ");
                    sendErrorReply(this, in, NOTFOUND_ERROR);
                    return true;
                }
                mid.setMethodPart(index++);
                Log.LOGN(5, "Method: id = " + mid.toString());
                mid.writeMethodID(ps);
                Log.LOGN(5, "Method: name = " + mi.getName());
                ps.writeString(mi.getName());
                Log.LOGN(5, "Method: sig = " + mi.getSignatureRaw());
                ps.writeString(mi.getSignatureRaw());
                Log.LOGN(5, "Method: flags = " + mi.getAccessFlags());
                ps.writeInt(mi.getAccessFlags());
            }
            ps.send();
            handled = true;
            break;
              
        case SOURCEFILE_CMD:
            SourceFileAttribute sfAttr;
            Log.LOGN(3, "Source file cmd");
            try {
                cid = in.readInt();
            } catch (PacketStreamException e) {
                System.out.println("Sourcefile cmd: exception: " + e);
                sendErrorReply(this, in, INVALID_OBJECT);
                handled = true;
                break;
            }

            if ((cf = (ClassFile)manager.classMap.get(new Integer(cid))) == null) {
                VMCall vmCall = new VMCall();
                cf = vmCall.callVMForClass(cid, "SourceFile");
                if (cf == null) {
                    Log.LOGN(3, "Sourcefile cmd: Couldn't get ClassFile object");
                    sendErrorReply(this, in, INVALID_OBJECT);
                    handled = true;
                    break;
                }
            }
            if ((sfAttr = cf.getSourceAttribute()) != null) {
                String s = sfAttr.getSourceFileName();
                Log.LOGN(3, "Returning from attribute: " + s);
                ps.writeString(s);
            } else {
                Log.LOGN(3, "Creating source name: " + cf.getBaseName() + ".java");
                ps.writeString(cf.getBaseName() + ".java");
            }
            ps.send();
            handled = true;
            break;

        case STATUS_CMD:
            try {
                cid = in.readInt();
            } catch (PacketStreamException e) {
                Log.LOGN(3, "status cmd: exception: " + e);
                sendErrorReply(this, in, INVALID_OBJECT);
                handled = true;
                break;
            }
            {
                VMCall vmCall = new VMCall();
                cf = vmCall.callVMForClass(cid, "Status");
            }
            if (cf == null) {
                Log.LOGN(3, "Status cmd: Couldn't get ClassFile object ");
                sendErrorReply(this, in, INVALID_OBJECT);
                handled = true;
                break;
            }
            ps.writeInt(cf.getClassStatus());
            ps.send();
            handled = true;
            break;

        case INTERFACES_CMD:
            try {
                cid = in.readInt();
            } catch (PacketStreamException e) {
                System.out.println("Interfaces cmd: exception: " + e);
                sendErrorReply(this, in, INVALID_OBJECT);
                handled = true;
                break;
            }

            Log.LOGN(3, "Interface cmd: class id " + Log.intToHex(cid));
            if ((cf = (ClassFile)manager.classMap.get(new Integer(cid))) == null) {
                VMCall vmCall = new VMCall();
                cf = vmCall.callVMForClass(cid, "Interfaces");
                if (cf == null) {
                    Log.LOGN(3, "Interfaces cmd: Couldn't get ClassFile object for id = " + Log.intToHex(cid));
                    sendErrorReply(this, in, INVALID_OBJECT);
                    handled = true;
                    break;
                }
            }
            List iList = cf.getAllInterfaces();
            Iterator iIter = iList.iterator();
            Log.LOGN(3, "Interfaces: class " + cf.getClassName() +
                     " " + iList.size() + " interfaces");
            ps.writeInt(iList.size());
            while (iIter.hasNext()) {
                String className = (String)iIter.next();
                Log.LOGN(3, "interfaces: interfacename: " + className);
                if (className == null) {
                    Log.LOGN(3, "Interfaces cmd: interface name is null");
                    sendErrorReply(this, in, INVALID_OBJECT);
                    return true;
                }
                if ((cf =
                     manager.findClass((byte)'L', className)) ==
                    null) {
                    VMCall vmCall = new VMCall();
                    String sig = "L" + className + ";";
                    cf = vmCall.callVMForClassBySig(sig, "Interfaces");
                    if (cf == null) {
                        Log.LOGN(3, "interfaces: classname: null ClassFile");
                        ps.writeInt(0);
                    } else {
                        Log.LOGN(3, "interfaces: classID: " + cf.getClassID());
                        ps.writeInt(cf.getClassID());
                    }
                } else {
                    Log.LOGN(3, "interfaces: classID: " + cf.getClassID());
                    ps.writeInt(cf.getClassID());
                }
            }
            ps.send();
            handled = true;
            break;
        }
        return handled;
    }

    private boolean doMethodCmdset(PacketStream in)
        throws ProxyConnectionException {
        boolean  handled = false;

        /* Method */
        switch(in.cmd()) {

        case METHOD_LINETABLE_CMD:  // LineTable
            lineTable(in);
            handled = true;
            break;
                            
        case METHOD_VARIABLETABLE_CMD:  // VariableTable
            variableTable(in);
            handled = true;
            break;
                            
        case METHOD_BYTECODES_CMD:   // Bytecodes
            byteCode(in);
            handled = true;
            break;
        }
        return handled;
    }

    private boolean doThreadReferenceCmdset(PacketStream in)
        throws ProxyConnectionException {
        boolean handled = false;
        PacketStream ps = new PacketStream(this, in.id(), Packet.Reply,
                                           Packet.ReplyNoError);

        switch(in.cmd()) {
                            
        case THREADGROUP_CMD:
            Log.LOGN(3, "Threadreference: threadgroup");
            ps.writeInt(ONLY_THREADGROUP_ID);
            ps.send();
            handled = true;
            break;
        }
        return handled;
    }

    private boolean doThreadGroupReferenceCmdset(PacketStream in)
        throws ProxyConnectionException{

        boolean handled = false;
        PacketStream ps = new PacketStream(this, in.id(), Packet.Reply,
                                           Packet.ReplyNoError);

        int tgID = 0;
        try {
            tgID = in.readInt();       // threadgroup ID
        } catch (PacketStreamException e) {
            Log.LOGN(3, "ThreadGroup cmd: exception: " + e);
        }

        switch(in.cmd()) {

        case THREADGROUP_NAME_CMD:
            Log.LOGN(3, "ThreadGroup: name");
            ps.writeString(KVM_THREADGROUP_NAME);
            ps.send();
            handled = true;
            break;
                            
        case THREADGROUP_PARENT_CMD:
            Log.LOGN(3, "ThreadGroup: parent");
            ps.writeInt(0);             // we're the top level
            ps.send();
            handled = true;
            break;
                            
        case THREADGROUP_CHILDREN_CMD:
            Log.LOGN(3, "ThreadGroup: children");
            if (tgID == ONLY_THREADGROUP_ID) {
                PacketStream ps2 = new PacketStream(KVMListener,
                                      VIRTUALMACHINE_CMDSET, ALL_THREADS_CMD);
                ps2.send();
                try {
                    ps2.waitForReply();
                } catch (ProxyConnectionException e) {
                    ps.writeInt(0);
                    ps.writeInt(0);
                    ps.send();
                    handled = true;
                    break;
                }
                int numThreads = ps2.readInt();
                Log.LOGN(3, "threadgroup: " + numThreads + " children");
                ps.writeInt(numThreads);
                while (numThreads-- > 0) {
                    ps.writeInt(ps2.readInt());
                }
            } else {
                ps.writeInt(0);        // number of child threads;
            }
            ps.writeInt(0);    // number of child threadgroups
            ps.send();
            handled = true;
            break;
        }
        return handled;
    }

    private boolean doStackFrameCmdset(PacketStream in)
        throws ProxyConnectionException {
        boolean handled = false;
        int frame_index_adjust = 1;
        PacketStream ps = new PacketStream(this, in.id(),
                                           Packet.Reply, Packet.ReplyNoError);

        switch(in.cmd()) {

        case STACKFRAME_THISOBJECT_CMD:
            Log.LOGN(3, "Stackframe: thisobject");
            int tID = in.readInt();         // get thread id;
            int fID = in.readInt();         // get frame id;
            // We need to get the method of this frame so we can check to
            // see if it is a static method.  This is so we can pass the
            // test suite.  A normal debugger should never try to get the
            // 'this' object of a static method
            // need to query VM to get method ID of the method of this frame

            // First we need to find out if the VM sends us 0 or 1 based
            // Frame ID's.  Unfortunately, this isn't covered by a minor
            // version number.
            PacketStream ps2 = new PacketStream(KVMListener,
                               THREADREFERENCE_CMDSET, FRAMES_CMD);
            ps2.writeInt(tID);
            ps2.writeInt(0);
            ps2.writeInt(1);      // just want one frame
            ps2.send();
            try {
                ps2.waitForReply();
                ps2.readInt();   // framecount   
                int id = ps2.readInt();
                if (id == 0) {
                    // VM sent 0 back as ID, must be older KVM
                    frame_index_adjust = 0;
                }
            } catch (ProxyConnectionException e) {
                // What to do?  Nothing, just use frame index as it it
                // set already
            }

            ps2 = new PacketStream(KVMListener,
                                  THREADREFERENCE_CMDSET, FRAMES_CMD);

            ps2.writeInt(tID);
            ps2.writeInt(fID - frame_index_adjust);
            ps2.writeInt(1);      // just want one frame
            ps2.send();
            try {
                ClassFile cf = null;
                MethodInfo mi;
                int typeTag;
                int fID2;
                int cid;

                ps2.waitForReply();

                ps2.readInt();           // get number of frames, should be 1
                fID2 = ps2.readInt();    // get frame ID
                if (fID2 != fID) {
                    Log.LOGN(3, "Stackframe: thisobject: mismatched frames");
                    throw new ProxyConnectionException();
                }
                typeTag = ps2.readByte();
                if (typeTag != TYPE_TAG_CLASS) {
                    Log.LOGN(3, "Stackframe: thisobject: wrong type tag");
                    throw new ProxyConnectionException();
                }
                cid = ps2.readInt();
                MethodID mid = new MethodID();
                int methodIndex = mid.readMethodPart(ps2) - method_index_base;
                if ((cf = (ClassFile)manager.classMap.get(new Integer(cid))) == null) {
                    VMCall vmCall = new VMCall();
                    cf = vmCall.callVMForClass(cid, "ThisObject");
                    if (cf == null) {
                        Log.LOGN(3, "Stackframe: thisobject: could not find class object");
                        throw new ProxyConnectionException();
                    }
                }
                mi = cf.getMethodInfoByIndex(methodIndex);
                if (mi == null) {
                    Log.LOGN(3, "Stackframe: thisobject: could not find method object");
                    throw new ProxyConnectionException();
                }
                if (mi.is_static()) {
                    // we're done, no 'this' in a static method
                    ps.writeByte((byte)'L');
                    ps.writeInt(0);
                    ps.send();
                    handled = true;
                    break;
                }
                            
            } catch (ProxyConnectionException e) {
                // What to do?  Nothing, just fall through and try to get
                // object in slot '0'.
            }

            ps2 = new PacketStream(KVMListener,
                                  STACKFRAME_CMDSET, STACKFRAME_GETVALUES_CMD);
            ps2.writeInt(tID);
            ps2.writeInt(fID);
            ps2.writeInt(1);         // just want the 'this' object
            ps2.writeInt(0);         // it's at slot 0
            ps2.writeByte((byte)'L');    // it's an object type
            ps2.send();
            try {
                ps2.waitForReply();
            } catch (ProxyConnectionException e) {
                ps.writeByte((byte)'L');
                ps.writeInt(0);
                ps.send();
                handled = true;
                break;
            }
            int num = ps2.readInt();         // get number of values
            byte tag = ps2.readByte();       // get tag type
            int objectID = ps2.readInt();    // and get object ID
            Log.LOGN(3, "Stackframe: thisobject tag: " + tag +
                     " objectID " + Log.intToHex(objectID));
            ps.writeByte(tag);
            ps.writeInt(objectID);
            ps.send();
            handled = true;
            break;
        }
        return handled;
    }

    public String toString() {
        return (new String("DebuggerListener: "));
    }

    /**
     * Retrieves the name of the class associated
     * with the specified id in a KVM defined manner
     */
    protected String getClassName(byte[] classid) {

        return new String("");
   
    }

    /**
     * Retrieves the name of the method associated
     * with the specified id in a KVM defined manner
     */
    protected String getMethodName(byte[] methodid) {

        return new String("");
    }

    protected boolean processFields(ClassFile cf, PacketStream ps,
                                    PacketStream in) {

        ClassFile scf;

        Log.LOGN(3, "processFields for " + cf.getClassFileName());
        FieldInfo fi;
        List fiList = cf.getAllFieldInfo();
        if (fiList == null) {
            ps.writeInt(0);
            return true;
        }
        Iterator fiIter = fiList.iterator();
        long fieldID = ((long)cf.getClassID()) << 32;
        Log.LOGN(5, "field class id is " +
                 Log.intToHex(cf.getClassID()) +
                 " fieldid is " + Log.longToHex(fieldID));
        ps.writeInt(fiList.size());
        while (fiIter.hasNext()) {
            fi = (FieldInfo)fiIter.next();
            if (fi == null) {
                Log.LOGN(3, "Fields cmd: fieldinfo null ");
                sendErrorReply(this, in, INVALID_OBJECT);
                return false;
            }
            Log.LOGN(5, "Field: id = " + Log.longToHex(fieldID));
            ps.writeLong(fieldID++);
            Log.LOGN(5, "Field: name = " + fi.getName());
            ps.writeString(fi.getName());
            Log.LOGN(5, "Field: sig = " + fi.getType());
            ps.writeString(fi.getType());
            Log.LOGN(5, "Field: flags = " + fi.getAccessFlags());
            ps.writeInt(fi.getAccessFlags());
        }
        return true;
    }

    public void lineTable(PacketStream in) {
        int cid;
        String classname, methodname, methodsig;
        ClassFile   cf = null;
        MethodInfo  mi;
        int code[][] = null;
        long startingOffset, endingOffset;
        int lineCount;

        startingOffset = endingOffset = -1;
        PacketStream ps = new PacketStream(this, in.id(),
                                           Packet.Reply, Packet.ReplyNoError);

        // we need to know the class and method to which the id's refer 

        cid = in.readInt();
        MethodID mid = new MethodID();
        int methodIndex = mid.readMethodPart(in) - method_index_base;
        // Get local variable table from VM if it supports it
        int num_entries;
        if (VM_Version_3) {
            // The bytecode offsets in ROMized system classes may change
            // due to various optimizations.  We get the modified
            // linenumber table from the VM if it
            // supports this command
            PacketStream ps2 = new PacketStream(KVMListener,
                                  KVM_CMDSET, KVM_GET_LINE_TABLE_CMD);

            ps2.writeInt(cid);
            mid.writeMethodID(ps2);
            ps2.send();
            try {
                ps2.waitForReply();
                num_entries = ps2.readInt();     // get number of entries
                Log.LOGN(3, "linenumber table: VM returned " + num_entries + " entries.");
                if (num_entries > 0) {
                    startingOffset = ps2.readLong();
                    endingOffset = ps2.readLong();
                    code = new int[num_entries][2];
                    for (int i = 0; i < num_entries; i++) {
                        code[i][0] = ps2.readShort();  // pc
                        code[i][1] = ps2.readShort();  // line num
                    }
                }
            } catch (ProxyConnectionException e) {
                // just use agent line number table
                // fall through...
                code = null;
            }
        }
        if (code == null) {
            Log.LOGN(4, "linetable: class id= " + Log.intToHex(cid) +
                     ", method id= " + mid.toString());

            if ((cf = (ClassFile)manager.classMap.
                 get(new Integer(cid))) == null) {
                VMCall vmCall = new VMCall();
                cf = vmCall.callVMForClass(cid, "LineTable");
                if (cf == null) {
                    Log.LOGN(3, "Linetable cmd: not found");
                    sendErrorReply(this, in, NOTFOUND_ERROR);
                    return;
                }
            }
            Log.LOGN(4, "linetable: class: " + cf.getClassFileName());
            mi = cf.getMethodInfoByIndex(methodIndex);
            if (mi == null) {
                Log.LOGN(1, "Couldn't find methodinfo for index " + mid +
                         method_index_base);
                sendErrorReply(this, in, INVALID_METHODID);
                return;
            }
            CodeAttribute ca = mi.getCodeAttribute();
            if (ca == null) {
                /*
                 * I don't think this can happen; no code, return -1 as start
                 * and end
                 */
                startingOffset = endingOffset = -1;
            } else {
                startingOffset = 0;
                endingOffset = ca.getCodeLength() - 1;
            }
            code = mi.getBreakableLineNumbers();
        }
        if (code == null) {
            Log.LOGN(1, "No linenumber table found for class " +
                     cf.getClassName());
            lineCount = 0;
        } else {
            lineCount = code.length;
        }

        ps.writeLong(startingOffset);  // starting offset
        ps.writeLong(endingOffset);

        Log.LOGN(5, "Starting code offset = " + startingOffset +
                 ", Ending code offset = " + endingOffset);
        Log.LOGN(5, "Code Length = " + lineCount);

        ps.writeInt(lineCount);
        for (int i=0; i < lineCount; i++) {
            ps.writeLong(code[i][0]); 
            ps.writeInt(code[i][1]); 
            Log.LOGN(5, "  index=" + code[i][0] + " l#=" + code[i][1]);
        } 
        ps.send();
    }

    public void variableTable(PacketStream in) {

        int cid;
        List table = null;
        MethodInfo mi = null;
        ClassFile cf;
        boolean use_agent_table = true;
        PacketStream ps = new PacketStream(this, in.id(),
                                           Packet.Reply, Packet.ReplyNoError);

        cid = in.readInt(); // class id - we won't need it
        Log.LOGN(3, "variable: class id = " + Log.intToHex(cid));

        MethodID mid = new MethodID();
        int methodIndex = mid.readMethodPart(in) - method_index_base;

        if ((cf = (ClassFile)manager.classMap.get(new
            Integer(cid))) == null) {
            VMCall vmCall = new VMCall();
            cf = vmCall.callVMForClass(cid, "VariableTable");
            if (cf == null) {
                Log.LOGN(3, "Variabletable cmd: not found");
                sendErrorReply(this, in, NOTFOUND_ERROR);
                return;
            }
        }
        Log.LOGN(3, "variable: method id = " +  mid.toString());
        table = cf.getVariableTableForMethodIndex(methodIndex);
        mi = cf.getMethodInfoByIndex(methodIndex);
        if (mi == null) {
            Log.LOGN(3, "HandleVariableTable: Couldn't find method info for class " + cf.getClassName());
            sendErrorReply(this, in, INVALID_OBJECT);
            return;
        }

        // Get local variable table from VM if it supports it
        int num_entries;
        short[][] VM_var_table = null;
        if (VM_Version_3) {
            // The bytecode offsets in ROMized system classes may change
            // due to various optimizations.  We get the modified
            // variable offset/length/slot table from the VM if it
            // supports this command
            PacketStream ps2 = new PacketStream(KVMListener,
                                  KVM_CMDSET, KVM_GET_VAR_TABLE_CMD);

            ps2.writeInt(cid);
            mid.writeMethodID(ps2);
            ps2.send();
            try {
                ps2.waitForReply();
                num_entries = ps2.readInt();     // get number of entries
                Log.LOGN(3, "variable: VM returned " + num_entries + " entries.");
                if (num_entries > 0) {
                    use_agent_table = false;
                    VM_var_table = new short[num_entries][3];
                    for (int i = 0; i < num_entries; i++) {
                        VM_var_table[i][0] = ps2.readShort();  // start_pc
                        VM_var_table[i][1] = ps2.readShort();  // code_length
                        VM_var_table[i][2] = ps2.readShort();  // slot
                    }
                }
            } catch (ProxyConnectionException e) {
                // just use agent copy
                use_agent_table = true;
            } catch (PacketStreamException e) {
                use_agent_table = true;
            }
        }

        int argCount = mi.getArgCount();
        Log.LOGN(3, "variable: argcount is " + argCount);
        ps.writeInt(argCount);
        int i = 0;
        if (table != null) {
            Log.LOGN(3, "variable: table size is " + table.size());
            ps.writeInt(table.size());
            Iterator iter = table.iterator();
            while (iter.hasNext()) {
                LocalVariable var = (LocalVariable)iter.next();
                if (use_agent_table) {
                    ps.writeLong(var.getCodeIndex());
                } else {
                    ps.writeLong(VM_var_table[i][0]);;
                }
                ps.writeString(var.getName());
                ps.writeString(getJNISignature(var.getType()));
                if (use_agent_table) {
                    ps.writeInt(var.getLength());
                } else {
                    ps.writeInt(VM_var_table[i][1]);
                }
                if (use_agent_table) {
                    ps.writeInt(var.getSlot());
                } else {
                    ps.writeInt(VM_var_table[i][2]);
                }
                i++;
            }
        } else {
            ps.writeInt(0);
        }
        ps.send();
    }

   private void byteCode(PacketStream in) {
        int cid;
        MethodInfo mi = null;
        ClassFile cf;
        PacketStream ps = new PacketStream(this, in.id(),
                                           Packet.Reply, Packet.ReplyNoError);

        Log.LOGN(1, "Method: Bytecodes");
        String sig=null;

        cid = in.readInt(); // class id
        Log.LOGN(3, "class id=0x" + Log.intToHex(cid));

        MethodID mid = new MethodID();
        int methodIndex = mid.readMethodPart(in) - method_index_base;

        //
        // now we will build a packet to send 
        // to the debugger
        //

        if ((cf = (ClassFile)manager.classMap.get(new
            Integer(cid))) == null) {
            VMCall vmCall = new VMCall();
            cf = vmCall.callVMForClass(cid, "Fields");
            if (cf == null) {
                Log.LOGN(3, "Bytecode cmd: not found");
                sendErrorReply(this, in, INVALID_OBJECT);
                return;
            }
        }
        Log.LOGN(3, "method id=0x" + mid.toString());

        mi = cf.getMethodInfoByIndex(methodIndex);
        if (mi == null) {
            Log.LOGN(3, "HandleByteCode: couldn't find method info for class " +
                               cf.getClassFileName());
            sendErrorReply(this, in, INVALID_METHODID);
            return;
        }

        // grab the byte codes
        //
        CodeAttribute ca = mi.getCodeAttribute();
        int code_length;
        byte[] code = null;
        if (ca == null) {
            // Either no code here or a native method
            code_length = 0;
        } else {
            code = mi.getCodeAttribute().getByteCodes();
            code_length = code.length;
        }
        //
        // build the reply packet and send it off
        //
        ps.writeInt(code_length);
        if (code_length > 0) {
            ps.writeByteArray(code);
        }
        ps.send(); 
    }

    private String getJNISignature(String type) {
        int index = type.length();
        int preindex = index;
        String ret = new String();
        String str;

        Log.LOGN(6, "getJNISignature()  type == " + type);
        while ((index = type.lastIndexOf("[]", index)) != -1) {
            ret += "[";
            preindex = index;
            index--;
        }

        str = type.substring(0, preindex);

        if      ("int".equalsIgnoreCase(str))     ret += "I";
        else if ("boolean".equalsIgnoreCase(str)) ret += "Z";
        else if ("short".equalsIgnoreCase(str))   ret += "S";
        else if ("byte".equalsIgnoreCase(str))    ret += "B";
        else if ("char".equalsIgnoreCase(str))    ret += "C";
        else if ("long".equalsIgnoreCase(str))    ret += "J";
        else if ("float".equalsIgnoreCase(str))   ret += "F";
        else if ("double".equalsIgnoreCase(str))  ret += "D";
        else ret += "L" + str.replace('.', '/') + ";";

        return ret;
    }

    class VMCall {
        int numClasses;

        protected ClassFile callVMForClass(int cid, String command) {
            ClassFile cf;
            PacketStream ps;

            if (!VM_Version_3) {
                return null;
            }
            Log.LOGN(3, command + " cmd: calling VM");
            try {
                ps = new PacketStream(KVMListener,
                                      REFERENCE_TYPE_CMDSET, SIGNATURE_CMD);
                ps.writeInt(cid);
                ps.send();
                ps.waitForReply();
            } catch (PacketStreamException e) {
                Log.LOGN(3, command + " cmd: Couldn't get ClassFile object");
                return null;
            }
            String className = ps.readString();
            Log.LOGN(3, command + " cmd: VM returned " + className);
            // We now have the signature, call down to VM and get
            // all the class info
            try {
                ps = new PacketStream(KVMListener,
                                      VIRTUALMACHINE_CMDSET, CLASSESBYSIG_CMD);
                ps.writeString(className);
                ps.send();
                ps.waitForReply();
            } catch (PacketStreamException e) {
                Log.LOGN(3, command + " cmd: Couldn't get Class info");
                return null;
            }
            ps.readInt();   // number of classes, must be one for KVMs
            byte typeTag = ps.readByte();
            int newCid = ps.readInt();  // Must match original one
            int classStatus = ps.readInt();
            if (cid != newCid) {
                Log.LOGN(3, command + " cmd: class id mismatch: old " + cid
                         + " new " + newCid);
            }
            Log.LOGN(3, command + " cmd: VM returned: tag:  " + typeTag +
                     ", cid " + newCid + ", status " + classStatus);
            String newClassName = className;
            if (typeTag != TYPE_TAG_ARRAY) {
                // strip off leading 'L' and trailing ';'
                newClassName =
                    new String(className.substring(1, className.length() - 1));
            }
            cf = manager.findClass(newCid, newClassName, typeTag, classStatus);
            return cf;
        }

        protected ClassFile callVMForClassBySig(String sig, String command) {
            ClassFile cf;
            PacketStream ps;
            if (!VM_Version_3) {
                return null;
            }
            ps = new PacketStream(KVMListener,
                                  VIRTUALMACHINE_CMDSET, CLASSESBYSIG_CMD);
            ps.writeString(sig);
            ps.send();
            ps.waitForReply();
            numClasses = ps.readInt();
            Log.LOGN(3, command + ": VM: numclasses " + numClasses);

            if (numClasses > 0) {
                int classStatus;
                byte typeTag;
                int cid;

                typeTag = ps.readByte();        // typeTag
                cid = ps.readInt();             // ID
                classStatus = ps.readInt();
                String className = new String(sig);
                if (typeTag != TYPE_TAG_ARRAY) {
                    // strip off leading 'L' and trailing ';'
                    className =
                        new String(className.substring(1,
                                                       className.length() - 1));
                }
            
                Log.LOGN(3, command + ": VM class found: " +
                         Log.intToHex(cid));
                cf = manager.findClass(cid, className,
                                       typeTag, classStatus);
                if (cf == null) {
                    Log.LOGN(3, command + ": couldn't find classfile object");
                }
                return cf;
            }
            return null;
        }

        protected int getNumClasses() {
            return numClasses;
        }
    }
} // DebuggerListener
