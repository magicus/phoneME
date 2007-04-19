/*
 * 	
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
 */

import java.io.*;
import java.util.*;
import java.net.*;
import javax.comm.*;

public class SoSserver {

    static final int FUNC_OPEN_HOST = 100;
    static final int FUNC_OPEN_IPN = 101;
    static final int FUNC_READ = 102;
    static final int FUNC_AVAILABLE = 103;
    static final int FUNC_WRITE = 104;
    static final int FUNC_CLOSE = 105;
    static final int FUNC_SHUTDOWN = 106;
    static final int FUNC_GET_IPNUMBER = 107;

    static final int SUCCESS = 1;
    static final int READ_BUFFER_SIZE = 256;
    static final int MAX_NO_OF_SOCKETS = 5;

    static final int UNKNOWNHOST_EXCEPTION_ERROR = -1;
    static final int PCSL_NET_IOERROR  = -2;
    static final int PCSL_NET_INTERRUPTED = -3;

    static final int SOCKET_ID_NOT_AVAILABLE = -2;
    static final int SOCKET_ID_AVAILABLE = -1;
    static final int SOCKET_ID_IN_USE = 0;

    Socket[] sockets;
    int[] socketid;

    CommPort serialPort;

    public SoSserver(int portno) {
        sockets = new Socket[MAX_NO_OF_SOCKETS];
        
        try {
            serialPort = new CommPort(portno, 9600, SerialPort.DATABITS_8,
                                      SerialPort.STOPBITS_1, SerialPort.PARITY_NONE);
        } catch (IOException e) {
            System.out.println("Open comm port error!");
        }

        socketid = new int[MAX_NO_OF_SOCKETS];

        for (int i=0; i < MAX_NO_OF_SOCKETS; i++) {
            socketid[i] = SOCKET_ID_AVAILABLE;
        }

        processRequestsFromClient();
    }

    public void processRequestsFromClient() {
        int func_type = -1;
        String dataString;

        while (true) {
            try {
                byte[] readin = serialPort.serialReceiveInt();
                func_type = getIntFromByteArray(readin);
                //System.out.println("func_type : " + func_type);
            } catch (IOException ioe) {
                System.out.println("IOException in serial port operations");
                return;
            }

            switch (func_type) {

                case FUNC_OPEN_HOST :  performOpenConnectionByHost();
                                  break;

                case FUNC_OPEN_IPN :  performOpenConnectionByIpn();
                                  break;

                case FUNC_READ :  performReadOperation();
                                  break;

                case FUNC_AVAILABLE :  performAvailableOperation();
                                  break;

                case FUNC_WRITE : performWriteOperation();
                                  break;
     
                case FUNC_GET_IPNUMBER :  performGetIpNumber();
                                  break;

                case FUNC_CLOSE : 
                                  boolean serialPortClosed = performCloseConnection();
                                  if (serialPortClosed) {
                                      System.out.println("End of the story");
                                      return;
                                  } 
                                  
                                  break;

                case FUNC_SHUTDOWN : performShutdownConnection();
                                  break;
     

                default         : break;
            
            }
        }
	}

    private void performOpenConnectionByHost() {
        try {
            performOpenConnectionByHostImpl();
        } catch (IOException ioe) {
            System.out.println("performOpenConnectionByHost : IOException" +
                               " for serial port operations ");
        }
    }
    
    private void performOpenConnectionByHostImpl() throws IOException {
        boolean exceptionThrown = false;
        System.out.println("performOpenConnectionByHostImpl");
        String strHost = null;
        int port = -1;
        int handle;

        //Get the length of host string from client
        byte[] dataFromProxy1 = serialPort.serialReceiveInt();
        int hostLength = getIntFromByteArray(dataFromProxy1);

        byte[] hostBytes = serialPort.serialReceive(hostLength);
        strHost = new String(hostBytes);

        //Get the port id from client
        byte[] dataFromProxy2 = serialPort.serialReceiveInt();
        port = getIntFromByteArray(dataFromProxy2);

        //Get the available socket-id from socketid array
        handle = getFirstAvailableSocketID();
        if ( handle == SOCKET_ID_NOT_AVAILABLE) {
            System.out.println("Limit of no of open connections exceeded");
            serialPort.serialSend(PCSL_NET_IOERROR);
        }

        try {
            //Open the proxy socket
            sockets[handle] = io_openSocketConnectionByHost(strHost, port);
        } catch (UnknownHostException uhe) {
            System.out.println("performOpenConnectionByHostImpl : " +
                               " UnknownHostException ");
            serialPort.serialSend(UNKNOWNHOST_EXCEPTION_ERROR);
            return;
        } catch (IOException ioe) {
            System.out.println("performOpenConnectionByHostImpl : IOException in" +
                                " opening a proxy socket");
            serialPort.serialSend(PCSL_NET_IOERROR);
            return;
        } catch (SecurityException se) {
            System.out.println("SecurityException : IOException in" +
                                " opening a proxy socket");
        } 

        //send the socket-id to the client
        System.out.println("socket= " + sockets[handle]);
        serialPort.serialSend(handle);

        // Mark the socketid status in socketid array
        socketid[handle] = SOCKET_ID_IN_USE;
    }

    private void performOpenConnectionByIpn() {
        try {
            performOpenConnectionByIpnImpl();
        } catch (IOException ioe) {
            System.out.println("performOpenConnectionByIpn : IOException" +
                               " for serial port operations ");
        }
    }
    
    private void performOpenConnectionByIpnImpl() throws IOException {
        boolean exceptionThrown = false;
        System.out.println("performOpenConnectionByIpnImpl");
        String strHost = null;
        int port = -1;
        int handle;

        //Get the length of host string from client
        byte[] ipn = serialPort.serialReceiveInt();

        //Get the port id from client
        byte[] dataFromProxy2 = serialPort.serialReceiveInt();
        port = getIntFromByteArray(dataFromProxy2);

        //Get the available socket-id from socketid array
        handle = getFirstAvailableSocketID();
        if ( handle == SOCKET_ID_NOT_AVAILABLE) {
            System.out.println("Limit of no of open connections exceeded");
            serialPort.serialSend(PCSL_NET_IOERROR);
        }

        try {
            //Open the proxy socket
            sockets[handle] = io_openSocketConnectionByIpn(ipn, port);
        } catch (UnknownHostException uhe) {
            System.out.println("performOpenConnectionByIpnImpl : " +
                               " UnknownHostException ");
            serialPort.serialSend(UNKNOWNHOST_EXCEPTION_ERROR);
            return;
        } catch (IOException ioe) {
            System.out.println("performOpenConnectionByIpnImpl : IOException in" +
                                " opening a proxy socket");
            serialPort.serialSend(PCSL_NET_IOERROR);
            return;
        } catch (SecurityException se) {
            System.out.println("SecurityException : IOException in" +
                                " opening a proxy socket");
        } 

        //send the socket-id to the client
        System.out.println("socket= " + sockets[handle]);
        serialPort.serialSend(handle);

        // Mark the socketid status in socketid array
        socketid[handle] = SOCKET_ID_IN_USE;
    }

    private void performGetIpNumber() {
        try {
            performGetIpNumberImpl();
        } catch (IOException ioe) {
            System.out.println("performGetIpNumber : IOException for serial" +
                                " port operations ");
        }
    }
    
    private void performGetIpNumberImpl() throws IOException {
        System.out.println("performGetIpNumberImpl");
        String strHost = null;
        byte[] ipn;

        //Get the length of host string from client
        byte[] dataFromProxy1 = serialPort.serialReceiveInt();
        int hostLength = getIntFromByteArray(dataFromProxy1);

        byte[] hostBytes = serialPort.serialReceive(hostLength);
        strHost = new String(hostBytes);

        try {
            // Get the IP address for the host
            ipn = io_getIpNumber(strHost);
        } catch (UnknownHostException ioe) {
            System.out.println("performGetIpNumberImpl : UnknownHostException in" +
                                " io_getIpNumber()");
            serialPort.serialSend(UNKNOWNHOST_EXCEPTION_ERROR);
            return;
        } 

        /* 
        Send the IP-address array to the client. ipn represents the 
        result in network byte-order: the highest order byte of the address
        is in ipn[0]
        */
        //System.out.println("performGetIpNumberImpl : ipn " + ipn);

        serialPort.serialSend(ipn, 4);
    }

    private void performWriteOperation() {
        try {
            performWriteOperationImpl();
        } catch (IOException ioe) {
            System.out.println("performWriteOperation : IOException for serial"  +
                                " port operations ");
        } 
    }

    private void performWriteOperationImpl() throws IOException {
        String dataString;
        int handle = -1;
        byte[] buf = null;

        System.out.println("performWriteOperation ");

        //Get the socketid from client
        byte[] dataFromProxy1 = serialPort.serialReceiveInt();
        handle = getIntFromByteArray(dataFromProxy1); 
        //System.out.println("handle : " + handle);

        //IMPL_NOTE: check if socket-id is invalid

        //Get the length of data to be read
        byte[] dataFromProxy2 = serialPort.serialReceiveInt();
        int numbytes = getIntFromByteArray(dataFromProxy2);
        //System.out.println("numbytes : " + numbytes);

        //Get the data to be written at the socket
        buf = serialPort.serialReceive(numbytes);

        //Perform the write operation at proxy socket
        System.out.println("Data from client : " + new String(buf));

        try {
            io_writeDataToSocket(sockets[handle], buf, numbytes);
        } catch (InterruptedIOException ioe) {
            System.out.println("performWriteOperation : InterruptedIOException"
                                + " in writing to a proxy socket");
            serialPort.serialSend(PCSL_NET_INTERRUPTED);
            return;
        } catch (IOException ioe) {
            System.out.println("performWriteOperation : IOException in"  +
                                " writing to a proxy socket");
            serialPort.serialSend(PCSL_NET_IOERROR);
            return;
        } 

        /* 
         * io_writeDataToSocket() does not return how many bytes it has 
         * actually written to the socket. According to the Javadoc for 
         * write() of an outputstream, it writes all the bytes one by one 
         */
        // Return the no of bytes written
        serialPort.serialSend(numbytes);
    }

    private void performReadOperation() {
        try {
            performReadOperationImpl();
        } catch (IOException ioe) {
            System.out.println("performReadOperation : IOException for serial"  +
                                " port operations ");
        }
    }

    private void performReadOperationImpl() throws IOException {
        String dataString;
        int handle = -1;
        int numbytes;
        int bytesRead;

        System.out.println("performReadOperation");

        //Get the socketid from client
        byte[] dataFromProxy1 = serialPort.serialReceiveInt();
        handle = getIntFromByteArray(dataFromProxy1);
        //System.out.println("handle : " + handle);
        //IMPL_NOTE: check if socket-id is invalid
       
        //Get the number of bytes to be read 
        byte[] dataFromProxy2 = serialPort.serialReceiveInt();
        numbytes = getIntFromByteArray(dataFromProxy2);
		byte[] buf = new byte[numbytes];

        try { 
            //Perform the read operation
            bytesRead = io_readDataFromSocket(sockets[handle], buf, numbytes);
        } catch (InterruptedIOException ioe) {
            System.out.println("performReadOperation : InterruptedIOException"
                                + " in reading from a proxy socket");
            serialPort.serialSend(PCSL_NET_INTERRUPTED);
            return;
        } catch (IOException ioe) {
            System.out.println("performReadOperation : IOException in" + 
                                " reading from a proxy socket");
            serialPort.serialSend(PCSL_NET_IOERROR);
            return;
        }
        
        //send the number of bytes read from socket
        serialPort.serialSend(bytesRead);
        //System.out.println("bytesRead : " + bytesRead);
        //write the actual data
        serialPort.serialSend(buf, bytesRead);

        //System.out.println("End of performReadOperation");
    }

    private void performAvailableOperation() {
        try {
            performAvailableOperationImpl();
        } catch (IOException ioe) {
            System.out.println("performAvailableOperation : IOException for serial"  +
                                " port operations ");
        }
    }

    private void performAvailableOperationImpl() throws IOException {
        String dataString;
        int handle = -1;
        int numbytes;
        int availableBytes;

        System.out.println("performAvailableOperation");

        //Get the socketid from client
        byte[] dataFromProxy1 = serialPort.serialReceiveInt();
        handle = getIntFromByteArray(dataFromProxy1);
        System.out.println("handle : " + handle);
        //IMPL_NOTE: check if socket-id is invalid
       
        try { 
            //Perform the read operation
            availableBytes = io_availableBytesFromSocket(sockets[handle]);
        } catch (IOException ioe) {
            System.out.println("performAvailableOperation : IOException in" + 
                                " available operation in a proxy socket");
            serialPort.serialSend(PCSL_NET_IOERROR);
            return;
        }

        //System.out.println("availableBytes : " + availableBytes);
        
        //send the number of bytes available to read from socket
        serialPort.serialSend(availableBytes);
    }

    private boolean performCloseConnection() {
        boolean serialPortClosed= false;
        try {
            serialPortClosed = performCloseConnectionImpl();
        } catch (IOException ioe) {
            System.out.println("performCloseConnection : IOException for serial"  +
                                " port operations ");
        }

        return serialPortClosed;
    }

    private boolean performCloseConnectionImpl() throws IOException {
        String dataString;
        int handle;

        System.out.println("performCloseOperation");

        //Get the socketid from client
        byte[] dataFromProxy1 = serialPort.serialReceiveInt();
        handle = getIntFromByteArray(dataFromProxy1);
        //IMPL_NOTE: check if socket-id is invalid

        try {
            //Perform the close operation
            io_closeSocketConnection(sockets[handle]);
        } catch (IOException ioe) {
            System.out.println("performCloseConnectionImpl : IOException in" + 
                                " closing the proxy socket");
            serialPort.serialSend(PCSL_NET_IOERROR);
            return false;  //serial port is still open
        }

        // Mark the socketid status in socketid array
        socketid[handle] = SOCKET_ID_AVAILABLE;

        serialPort.serialSend(SUCCESS);
      
        return false;
    }

    private void performShutdownConnection() {
        try {
            performShutdownImpl();
        } catch (IOException ioe) {
            System.out.println("performShutdownConnection : IOException for serial"  +
                                " port operations ");
        }
    }

    private void performShutdownImpl() throws IOException {
        String dataString;
        int handle;

        System.out.println("performShutdownImpl");

        //Get the socketid from client
        byte[] dataFromProxy1 = serialPort.serialReceiveInt();
        handle = getIntFromByteArray(dataFromProxy1);
        //IMPL_NOTE: check if socket-id is invalid

        try {
            //Perform the shutdown operation
            io_shutdownSocketConnection(sockets[handle]);
        } catch (IOException ioe) {
            System.out.println("performShutdownImpl : IOException in" + 
                                " closing the proxy socket");
            serialPort.serialSend(PCSL_NET_IOERROR);
            return;
        }

        // Mark the socketid status in socketid array
        socketid[handle] = SOCKET_ID_AVAILABLE;

        serialPort.serialSend(SUCCESS);
    }

	private Socket io_openSocketConnectionByHost(String host, int port) 
            throws IOException, UnknownHostException, SecurityException {
		Socket socket = null;
        //System.out.println("Inside io_openSocketConnection(): host: " + host +
         //                   "port : " + port);

        socket = new Socket(host, port);

        //System.out.println("socket opened successfully");

		return socket;
	}

	private Socket io_openSocketConnectionByIpn(byte[] ipn, int port) 
            throws IOException, UnknownHostException, SecurityException {
		Socket socket = null;
        InetAddress ipAddress = InetAddress.getByAddress(ipn);

        //System.out.println("Inside io_openSocketConnection(): host: " + host +
         //                   "port : " + port);

        socket = new Socket(ipAddress, port);

        //System.out.println("socket opened successfully");

		return socket;
	}

    private byte[] io_getIpNumber(String host) throws UnknownHostException {
        byte[] ip = InetAddress.getByName(host).getAddress();    
        return ip;
    }

    /** 
     * This function returns number of bytes read at proxy socket
     * Returns -1 in case of an error in read operation
     */
	private int io_readDataFromSocket(Socket socket, byte[] buf, 
                            int numbytes) throws IOException {
		int num = 0;

        InputStream in = socket.getInputStream();
        if (in.available() != 0) {
            num = in.read(buf, 0, numbytes);
        }

        return num;
	} 

	private int io_availableBytesFromSocket(Socket socket) 
                throws IOException {
		int num = 0;

        InputStream in = socket.getInputStream();
        num = in.available();

        return num;
	}
 
    /** 
     * This function does NOT return number of bytes written at proxy 
     * socket. It always writes all the bytes per the request. Write()
     * operation for an Outputstream is a blocking operation
     */
	private void io_writeDataToSocket(Socket socket, byte[] buf, 
                                   int numbytes) throws IOException {
        OutputStream out = socket.getOutputStream();
        out.write(buf, 0, numbytes);
	}

	private void io_closeSocketConnection(Socket socket) throws IOException {
        socket.close();
	}

	private void io_shutdownSocketConnection(Socket socket) throws IOException {
        socket.shutdownInput();
        socket.shutdownOutput();
	}

    public int getIntFromByteArray(byte[] buffer) {
        int n = 0;
        for(int i = 0; i < 4; i++){
            n += ((0xFF & buffer[i]) << (8*(3 - i)));
        }
        return n;
    }

    public int getFirstAvailableSocketID() {
        for (int index = 0; index < MAX_NO_OF_SOCKETS; index++) {
            if (socketid[index] == SOCKET_ID_AVAILABLE) {
                return index;
            }
        }

        return SOCKET_ID_NOT_AVAILABLE;
    }

    public static void main(String[] args) {
        SoSserver sosserver = new SoSserver(1);
    }

}


