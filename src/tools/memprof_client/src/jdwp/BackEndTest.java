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


package com.sun.cldchi.tools.memoryprofiler.jdwp;

import java.io.*;
import java.net.Socket;
import java.util.Enumeration;
import java.util.Vector;
import java.net.*;
/**
 * The <code>BackEndTest</code> class implemtens common debugger features 
 * such as:<br>
 * <ul>
 *   <li>Connects to DebugAgent (either KdpProxy or Java), sends Handshake 
 *       command and waits for VM_INIT event.
 *   <li>Contains a number of helper functions to work with JDWP
 * </ul>
 * This class encapsulates a huge amount of direct work with JDWP.<p>
 * It's a good idea to place all common JDWP operations here.
 *
 * Notice that all <code>BackEndTest</code> methods are declared as static so
 * you must never create instances of this class
 *
 * @see jdwp.jdwp
 * @see jdwp.Reply
 * @see jdwp.Tools
 * @see jdwp.Packet
 * @see jdwp.Command
 * @see jdwp.Transport
 * @see jdwp.ByteBuffer
 * @see jdwp.BoundException
 * @see jdwp.BreakpointInfo
 * @see jdwp.SocketTransport
 * @see jdwp.MethodDescriptor
 *
 */
class BackEndTest extends jdwp implements VMConnection {

    /**
     * Debug time variable. To see a bit more verbose output
     * turn it to <code>true</code>. In non-debug mode <b>must</b>
     * be <code>false</code> otherwise identity of J2SE and KVM outputs
     * is not guarantered.
     */
    public static boolean verbose   = false;
    public boolean is_connected = false;
    /**
     * This object executes all low-level work with JDWP
     * (connection establishing/terminating, data exchange etc). 
     */
    public SocketTransport debug;
        
    public void connect(String hostName, int port) throws ConnectException {
        try{
            openConnections(hostName, port);
            is_connected = true;
        }catch(DebugeeException e){
            throw new ConnectException(e.getMessage());
        }catch(IOException e){
            throw new ConnectException(e.getMessage());
        }
    }
    
   /**
    * Connects to VM being debugged.
    *
    * @param debugee_server host name where VM being debugged is started
    * @param debugee_port TCP/IP port for connection with VM being debugged
    *
    * @throws ConnectException connection establishing failed
    * @throws DebugeeException VM being debugged sent JDWP package with
    * incorrect content
    */ 
    private void openConnections(String debugee_server, int debugee_port)
        throws ConnectException, DebugeeException{

        try {
            Tools.wait(1000);
            debug = new SocketTransport();

            debug.attachToServer(debugee_server, debugee_port);
            debug.Handshake();
        }
        catch (Exception e) {
            System.err.println("* Exception.\n");
            throw new ConnectException("Can't create JDWP connection.");
        }

        System.out.println("* JDWP connection is installed.");
        getIDSizes();
    }

   /**
    * Closes connection. This function is executed prior to
    * KJDB finishing.
    */ 
    public void closeConnections() {
        if (debug != null) {
            try {
                debug.done();
            } catch (Exception e) {};
            debug = null;
        }
        is_connected = false;
    }
   /**
     * Debug time method. Rewrite it if you'd like to see debug output
     * not in the standard output
     *
     * @param s <code>String</code> to be printed
     */
    public static void print(String s) {
        if (s.equals(""))
            return;
        System.out.println(s);
    }

    /**
     * Debug time method. Prints the content of the vector to standard
     * output.
     *
     * @param v <code>java.util.Vector</code> object to be printed
     */
    public static void print(Vector v) throws Exception {
        System.out.println(Tools.listVector(v));
    }

    /**
     * Debug time method. Prints the content of JDWP package in specified manner.
     *
     * @param p JDWP package to be printed
     * @param how mask for proper representing of package content. For example,
     * the mask <code>i(o)</code> will protected parsed as follows: first
     * is <code>int</code> that determines the number of 
     * object references followed. All the follwed elements are object
     * references
     */
    public static void print(Packet p, String how) throws Exception {
        print(p.parse(how));
    }

    /**
     * This method sends JDWP command and does not wait for answer. It's
     * used internally by other <code>BackEndTest</code> methods. Also
     * it's a good idea to use this method in some cases when answer is not
     * guaranteered (for example, if you send <code>VirtualMachine/Resume</code>
     * JDWP command you may receive no answer because of debugging session
     * finishing).
     *
     * @param c command to be sent
     * @return command with ID field filled
     */
    public Command sendCommand(Command c) throws DebugeeException  {
        try{
            debug.sendCommand(c);
        }catch (IOException e){
            throw new DebugeeException(e.getMessage());
        }
        return c;
    }

    /**
     * Prints all JDWP replies that were not demanded yet. This method is
     * invoked when no expected reply is received that is considered as
     * fatal error.
     */
    public void printReplies() throws IOException {

        debug.receive();

        if (debug.Replies.size() == 0)
            return;

           print("\nReplies:\n");
        for (int i = 0; i < debug.Replies.size(); i++)
            print("\n" + (Reply) debug.Replies.elementAt(i) + "\n");
    }
    
    /**
     * Sends JDWP command and waits for reply. If reply is not received in
     * 100 seconds <code>DebugeeException</code> is raised. So large timeout
     * is choosen because of possibility of using extremely slow 
     * or busy systems where VM being debugged is run. This method is
     * used internally by other <code>BackEndTest</code> methods. Also
     * it's a good idea to explicity invoke this method when non-zero
     * error code in considered as correct or allowable answer.
     *
     * @param c JDWP command to be sent
     * @return received reply packet
     * @throws DebugeeException the command can't be sent or reply packet is
     * not received in 100 seconds
     * @throws IOException a generic I/O error occured
     */
    public Reply checkReply(Command c)
    throws IOException, DebugeeException{
        sendCommand(c);
        Reply r = debug.receiveReply(c.getID(), 100000);
        if (r.getErrorCode() == Reply.errNotAvailable) {
            print("\nCommand:\n" + c);
            printReplies();
            throw new DebugeeException("Reply packet is not received.");
        }
        return r;
    }

    /**
     * Sends JDWP command and waits for reply. The method expects that
     * JDWP reply packet will contain expected error code otherwise
     * <code>DebugeeException</code> is raised. Currently this method is used
     * internally by othe methods of <code>BackEndTest</code>.
     *
     * @param c JDWP command to be sent
     * @param ErrorCode expected error code in reply packet
     * @param Description if unexpected error code (in particular,
     * <code>NONE</code>)is received this string will be included in
     * the error message
     * @return received reply packet
     *
     * @see #checkReplyF(Command, int, String)
     */
    public Reply checkReply(Command c, int ErrorCode, String Description)
    throws IOException, DebugeeException{
        Reply r = checkReply(c);
        if (r.getErrorCode() != ErrorCode) {
            print("\nCommand:\n" + c + "\nReply:\n" + r);
            String m = "Unexpected error code";
            if (! Description.equals(""))
                Description = "Unexpected error code (" + Description + ").";
            else
                Description = "Unexpected error code.";
            throw new DebugeeException (Description);
        }
        return r;
    }

    /**
     * Determines the sizes (in bytes ) of field IDs,
     * method IDs, object reference IDs and reference type IDs.
     * These values are VM specific and are obtained via
     * VirtualMachine/IDSizes JDWP command. This method is invoked by
     * <code>openConnections(String, int)</code> method immediately after
     * establishing JDWP connection.
     *
     * @see #openConnections(String, int)
     */
    public void getIDSizes() throws DebugeeException{
        try{
            Command c;
            Reply r;
            c = new Command(0x0107);
            r = checkReply(c);
            r.resetDataParser();
            fieldIDSize = r.getInt();
            methodIDSize = r.getInt();
            objectIDSize = r.getInt();
            referenceTypeIDSize = r.getInt();
            frameIDSize = r.getInt();
        }catch(Exception e){
            throw new DebugeeException("Exception occured in IDSizes function");
        } 
    }


    //VMConnection implementation
    public VMReply sendReplyCommand(int command, int[] params) throws DebugeeException {
      try {
        Command cmd = new Command(command);
        for (int i = 0; i < params.length; i++) {
          cmd.addInt(params[i]);
        }
        Reply reply = checkReply(cmd);
        reply.resetDataParser(); 
        return reply;
      } catch (IOException e) {
        throw new DebugeeException(e.getMessage());
      }
    }

    //VMConnection implementation
    public VMReply sendReplyCommand(int command) throws DebugeeException {
      try {
        Reply reply = checkReply(new Command(command));
        reply.resetDataParser(); 
        return reply;
      } catch (IOException e) {
        throw new DebugeeException(e.getMessage());
      }
    }

    public void sendCommand(int command) throws DebugeeException {
      sendCommand(new Command(command));
    }    

    public boolean isConnected() {
      return is_connected;
    }
}
