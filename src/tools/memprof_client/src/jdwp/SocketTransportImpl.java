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


/*
 * SocketTransportImpl.java
 *
 * Created on April 20, 2001, 6:26 PM
 */

package com.sun.cldchi.tools.memoryprofiler.jdwp;

import java.net.*;
import java.io.*;
import java.util.*;

/**
 * This class implements JDWP socket connection and works in the
 * background thread.
 *
 * @see jdwp.SocketTransport
 *
 * @version 
 */
class SocketTransportImpl extends Thread {
    
    /**
     * A synchronisation object. It's used for synchronising access
     * to <code>finished</code> field.
     *
     * @see #finished
     */
    private static Object sync = new Object();
    
    /**
     * A synchronication object. Using this object a reading
     * loop waits for invoking <code>receive()</code> method.
     *
     * @see#run()
     * @see#receive()
     */
    private static Object sync1 = new Object();
    
    /**
     * A synchronisation object. It's used for synchronising access
     * to the buffer that contains data that already read and to the
     * <code>cycleStarted</code> field.
     *
     * @see #privateBuffer
     * @see #cycleStarted
     */
    private static Object sync2 = new Object();
    
    /**
     * A host name where VM being debugged is running.
     */
    private String serverName;
    
    /**
     * A TCP/IP port number that is used for connecting to VM being debugged.
     */
    private int portNumber;
    
    /**
     * Indicates that it's time to finish this thread. May be initiated inside
     * and outside the thread.
     */
    private boolean finished = true;
    
    /**
     * A stream that reads data from the socket.
     */
    private DataInputStream inputStream;
    
    /**
     * A stream that writes data to the socket.
     */
    private DataOutputStream outputStream;
    
    /**
     * A socket that provides JDWP connection.
     */
    private Socket socket;
    
    /**
     * A reference to the outer object ahere is to put
     * an information about received JDWP replies.
     *
     * @see jdwp.Transport#Replies
     */
    private Vector replies;
    
    /**
     * An internal structure for temporary keeping of received JDWP
     * replies. Later this data is copied to <code>replies</code> vector.
     *
     * @see #receive()
     * @see #replies
     */
    private Vector repliesInternal = new Vector();
    
    /**
     * If this field is set to <code>-1</code> a normal JDWP packet
     * is expected. Otherwise it indicates that handshake is expected and
     * is value equals to expected size of handshake string.
     *
     * @see #run()
     * @see #startHandShake(int)
     */
    private int expectedSize = -1;
    
    /**
     * A current position in the buffer that contains received handshake
     * string.
     *
     * @see #startHandShake(int)
     */
    private int currentInPrivateBuffer = 0;
    
    /**
     * An expected size of received handshake string.
     *
     * @see #startHandShake(int)
     */
    private int handShakeSize;
    
    /**
     * A buffer that contains a received handshake string.
     *
     * @see #startHandShake(int)
     */
    private byte[] privateBuffer = new byte[1024];
    
    /**
     * Indicates that the reading thread doesn't sleep but processes reading
     * of data. Used by handshake procedure to wait until receiving handshake
     * string.
     *
     * @see #startHandShake(int)
     */
    private boolean cycleStarted = false;

    /**
     * Creates a new <code>SocketTransportImpl</code> object.
     */
    public SocketTransportImpl() {
        super();
    }
    
    /**
     * Intializes some internal fields of the object. Need to be invoked
     * immediately after constructor.
     *
     * @param serverName a hostname where VM being debugged is running
     * @param portNumber a TCP/IP port number that should be used for connecting
     * with VM being debugged
     * @param replies a vector where it's necessary to put received JDWP
     * replies
     */
    public void initAsClient(String serverName, int portNumber, Vector replies)
    throws IOException{
        this.serverName = serverName;
        this.portNumber = portNumber;
        this.replies = replies;
    }
    
    /**
     * It's an entry point of background thread. This method processes
     * reading of handshake and normal JDWP packets and stores the received 
     * values in <code>repliesInternal</code> vector.
     * Also this method establishes JDWP connection.
     * The method works in a
     * loop until <code>finished</code> field becomes <code>true</code>.
     * After this the JDWP connections terminates.
     */
    public void run(){
        //setPriority(MIN_PRIORITY);
        synchronized(sync){
            finished = false;
        }
        byte[] buf = new byte[100000];
        try{
            attachToServer();
            boolean timeToQuit = false;
            synchronized(sync){
                timeToQuit = finished;
            }
            synchronized(this){
                notifyAll();
            }
            while(!timeToQuit){
                synchronized(sync1){
                    sync1.wait();
                }
                synchronized(sync2){
                    cycleStarted = true;
                }
                
                if(expectedSize != -1){
                    //read handshake
                    int bytesRead = 0;
                    while(bytesRead != expectedSize){
                        bytesRead += inputStream.read(buf, bytesRead, expectedSize - bytesRead);
                    }
                    synchronized(sync2){
                        for(int i=0 ; i<expectedSize ; i++){
                            privateBuffer[i] = buf[i];
                        }
                        expectedSize = -1;
                    }
                    continue;
                }
                
                //System.out.println("read packet");
                
                //Read header
                int bytesRead = 0;
                while(bytesRead != Packet.PacketHeaderSize){
                    int bytesReadDuringLastRead = inputStream.read(buf,                    
                    bytesRead, Packet.PacketHeaderSize - bytesRead);
                    bytesRead += bytesReadDuringLastRead;
                    if(bytesReadDuringLastRead  == -1){
                        throw new IOException("Connection closed");
                    }
                }
                
                //System.out.println("read data");
                
                //Read data
                int size = (((((int)buf[0]) & 0xFF) << 24) |
                ((((int)buf[1]) & 0xFF ) << 16) |
                ((((int)buf[2]) & 0xFF ) << 8) |
                (((int)buf[3]) & 0xFF));
                bytesRead = 0;
                while(bytesRead != size - Packet.PacketHeaderSize){
                    int bytesReadDuringLastRead =
                    inputStream.read(buf, bytesRead + Packet.PacketHeaderSize, 
                    size - Packet.PacketHeaderSize - bytesRead);
                    bytesRead += bytesReadDuringLastRead;
                    if(bytesReadDuringLastRead  == -1){
                        throw new IOException("Connection closed");
                    }
                }
                
                //System.out.println("data read");
                
                Reply r = new Reply();
                r.resetBuffer();
                r.addBytes(buf,0,size);
                synchronized(repliesInternal){
                    repliesInternal.add(r);
                }
                
                synchronized(sync){
                    timeToQuit = finished;
                }
            }
        }catch(IOException e){
            synchronized(sync){
                finished = true;
            }
            synchronized(this){
                notifyAll();
            }
        }catch(InterruptedException e){
            synchronized(sync){
                finished = true;
            }
            synchronized(this){
                notifyAll();
            }
        }
        synchronized(sync){
            finished = true;
        }
        doneInternal();
    }
    
    /**
     * This method sets the <code>finished</code> field that
     * indicates that the background thread is to be finished.
     * After this it closes JDWP connection.
     */
    public void done(){
        synchronized(sync){
            finished = true;
        }
        doneInternal();
    }
    
    /**
     * Internal method that terminates JDWP connection in fact.
     */
    private synchronized void doneInternal(){
        try{
            if(socket == null){
                return;
            }
            socket.close();
        }catch(IOException e){
        }
    }
    
    /**
     * Stores a portion of received replies in the outer buffer. If no replies
     * are received since the previous call of this method it does nothing.
     *
     * @see #replies
     * @see #repliesInternal
     * @see jdwp.Transport#Replies
     */
    public void receive()
    throws IOException{
        //System.out.println("receive");
        synchronized(sync){
            if(finished){
                throw new SocketException("Connection closed");
            }
        }
        synchronized(sync1){
            sync1.notifyAll();
        }
        synchronized(repliesInternal){
            replies.addAll(repliesInternal);
            repliesInternal.clear();
        }
    }
    
    /**
     * Writes a specified byte into the socket.
     *
     * @param b a byte to be written
     */
    public void write(int b) throws IOException {
        outputStream.write(b);
        //System.out.println("Written byte " + b);
    }
    
    /**
     * Establishes TCP/IP connection to the VM being debugged.
     * <p>
     * NOTE: It's one of few methods in KJDB that produces non-debug
     * output directly. It's not good. For non-debug output classes
     * must use <code>kdb.Log</code> class. So may be it's a good idea
     * to remove the message this method prints.
     */
    public void attachToServer()
    throws IOException, UnknownHostException{
        socket = new Socket(serverName, portNumber);
        Tools.wait(100);
        System.out.println("Socket Created.");
        socket.setTcpNoDelay(true);
        inputStream = new DataInputStream(socket.getInputStream());
        outputStream = new DataOutputStream(socket.getOutputStream());
    }
    
    /**
     * Initiates handshake procedure that is physically performed
     * by <code>run()</code> method and waits for <code>run()</code>
     * method's sterting the waiting for reply. After this it exits.
     * This procedure is performed immediately after establishing TCP/IP
     * connection and consist on sending and receiving "JDWP-Handshake" string.
     *
     * @param an expected length of received handshake string
     */
    public void startHandShake(int len){
        currentInPrivateBuffer = 0;
        expectedSize = len;
        handShakeSize = len;
        boolean b = true;
        do{
            synchronized(sync1){
                sync1.notifyAll();
            }
            synchronized(sync2){
                b = !cycleStarted;
            }
        }while(b);
        synchronized(sync2){
            cycleStarted = false;
        }
    }
    
    /**
     * Reads next byte from the buffer that contains received
     * handshake string.
     *
     * @return next byte from the buffer that contains received
     * handshake string
     */
    public int readNextFromPrivateBuffer()
    throws IOException{
        synchronized(sync){
            if(finished){
                throw new SocketException("Connection closed");
            }
        }
        //System.out.println("read");
        synchronized(sync2){
            return privateBuffer[currentInPrivateBuffer++];
        }
    }
    
    /**
     * Returns the number of bytes that are available in the buffer
     * that that contains received handshake string.
     *
     * @return the number of bytes that are available in the buffer
     * that that contains received handshake string
     */
    public int availableInPrivateBuffer()
    throws IOException{
        synchronized(sync){
            if(finished){
                throw new SocketException("Connection closed");
            }
        }
        //System.out.println("available");
        synchronized(sync2){
            if(expectedSize == -1){
                return handShakeSize;
            }else{
                return 0;
            }
        }
    }
    
    /**
     * Checks if the background thread is initialized and is not received
     * request for terminating yet.
     *
     * @return <code>true</code> if the background thread is initialized and
     * is not received request for terminating yet, <code>false</code>
     * otherwise
     */
    public boolean isStarted(){
        synchronized(sync){
            return !finished;
        }
    }
}
