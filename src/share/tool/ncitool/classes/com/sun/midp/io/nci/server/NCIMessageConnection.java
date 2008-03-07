package com.sun.midp.io.nci.server;

import com.sun.tck.wma.*;
import java.io.*;
import java.net.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.charset.*;
import java.util.*;
import java.util.regex.*;

public class NCIMessageConnection implements MessageConnection {
    private static boolean running = true;
    private static final int PORT_NUMBER = 7777;

    private static final Charset UTF8 = Charset.forName("UTF-8");
    private static final Pattern SPACE = Pattern.compile("\\s+");
    private static final String LINE_SEP = "\r\n";
    private static final String HTTP_HDR =
    "HTTP/1.0 200 OK" + LINE_SEP +
    "Server: NCI test server" + LINE_SEP +
    "Content-Type: text/plain" + LINE_SEP;
    
    private static final ByteBuffer STATIC_HEADER = ByteBuffer.wrap(HTTP_HDR.getBytes());
    
    private static final Server serverInstance = new Server();
    private static volatile List pendingMessages = new ArrayList();
    private volatile List receivedMessages = new ArrayList();
    private static Map listeningURLs = new HashMap();
    private String url;
    private String appID;
    
    public NCIMessageConnection(String url) {
	 String port = "INVALID";
        this.url = url;
	
        int pos = url.lastIndexOf(':');

        if(pos > -1) {
     		port = url.substring(pos + 1);
        }
        synchronized(listeningURLs) {

	     System.out.println("Add listening port: "+port);
      	     listeningURLs.put(port, this);
        }
    }
    
    /**
     * Constructs a new message object of a given type. When the
     * string <code>text</code> is passed in, the created
     * object implements the <code>TextMessage</code> interface.
     * When the <code>binary</code> constant is passed in, the
     * created object implements the <code>BinaryMessage</code>
     * interface. Adapter definitions for messaging protocols can define
     * new constants and new subinterfaces for the <code>Message</code>s.
     * The type strings are case-sensitive.
     *
     * <p>For adapter definitions that are not defined within the JCP
     * process, the strings used <strong>must</strong> begin with
     * an inverted domain
     * name controlled by the defining organization, as is
     * used for Java package names. Strings that do not contain a
     * full stop character "." are reserved for specifications done
     * within the JCP process and <strong>must not</strong> be used by
     * other organizations
     * defining adapter specification.
     * </p>
     * <p>When this method is called from a <em>client</em> mode connection,
     * the newly created <code>Message</code> has the destination address
     * set to the address identified when this <code>Connection</code>
     * was created.
     * </p>
     * <p>When this method is called from a <em>server</em> mode connection,
     * the newly created Message does not have the destination
     * address set. It must be set by the application before
     * trying to send the message.
     * </p>
     * @param type the type of message to be created. There are
     * constants for basic types defined in
     * this interface
     * @throws java.lang.IllegalArgumentException if the message
     * type is not <code>TEXT_MESSAGE</code> or
     * <code>BINARY_MESSAGE</code>
     * @return Message object for a given type of message
     */
    public Message newMessage(String type) {
        
        if(TEXT_MESSAGE.equals(type)) {
            return new NCITextMessage();
        } else if (BINARY_MESSAGE.equals(type)) {
            return new NCIBinaryMessage();
        } else {
            return new NCIMultipartMessage();
        }
    }
    
    /**
     * Constructs a new message object of a given type and
     * initializes it with the given destination address.
     * The semantics related to the parameter <code>type</code>
     * are the same as for the method signature with just the
     * <code>type</code> parameter.
     *
     * @param type the type of message to be created. There are
     * constants for basic types defined in
     * this interface.
     * @param address destination address for the new message
     * @return <code>Message</code> object for a given type of message
     * @throws java.lang.IllegalArgumentException if the message
     * type is not <code>TEXT_MESSAGE</code> or
     * <code>BINARY_MESSAGE</code>
     * @see #newMessage(String type)
     */
    public Message newMessage(String type, String address) {
        Message m = newMessage(type);
        m.setAddress(address);
        return m;
    }
    
    /**
     * Receives a message.
     *
     * <p>If there are no <code>Message</code>s for this
     * <code>MessageConnection</code> waiting,
     * this method will block until a message for this <code>Connection</code>
     * is received, or the <code>MessageConnection</code> is closed.
     * </p>
     * @return a <code>Message</code> object representing the
     * information in the received message
     * @throws java.io.IOException if an error occurs while receiving
     * a message
     * @throws java.io.InterruptedIOException if this
     * <code>MessageConnection</code> object
     * is closed during this receive method call
     * @throws java.lang.SecurityException if the application does not
     * have permission to receive messages using the given port
     * number
     * @see #send(Message)
     */
    public Message receive() {
        System.out.println("NCI:: receive(): Waiting for a new SMS");
        while (running) {
            synchronized(serverInstance) {
                if(receivedMessages.size() > 0) {
                    System.out.println("NCI:: receive(): Got new SMS");
                    return (Message)receivedMessages.remove(0);
                }
                try {
                   serverInstance. wait(50);
                } catch(InterruptedException ignore) {}
            }
        }
        return null;
    }
    
    /**
     * Sends a message.
     *
     * @param msg the message to be sent
     * @throws java.io.IOException if the message could not be sent
     * or because of network failure
     * @throws java.lang.IllegalArgumentException if the message is
     * incomplete or contains invalid information
     * This exception
     * is also thrown if the payload of the message exceeds
     * the maximum length for the given messaging protocol.
     * @throws java.io.InterruptedIOException if a timeout occurs while
     * trying to send the message or if this <code>Connection</code>
     * object is closed during this send operation
     * @throws java.lang.NullPointerException if the parameter is null
     * @throws java.lang.SecurityException if the application does not
     * have permission to send the message
     * @see #receive()
     */
    public void send(Message msg) {
        System.out.println("NCI:: send(): " + msg);
        synchronized(serverInstance) {
            pendingMessages.add(msg);
        }
    }
    
    /**
     * Close the connection.
     * When the connection has been closed access to all methods except this one
     * will cause an an IOException to be thrown. Closing an already closed
     * connection has no effect. Streams derived from the connection may be open
     * when method is called. Any open streams will cause the
     * connection to be held open until they themselves are closed.
     * @throws java.io.IOException - If an I/O error occurs
     */
    public void close() {
        System.out.println("NCI:: close()");
        // server is static it is never shut down...
/*        synchronized(serverInstance) {
            try {
                serverInstance.wait();
            } catch(InterruptedException err) {}
        }*/
    }
    
    /**
     * This is an HTTP server running in the background
     */
    static class Server extends Thread {
        public Server() {
            start();
        }
        
        /**
         * This is the webserver thread
         */
        public void run() {
			
			System.out.println("\n\n\nNCI:: ===================================================");
			System.out.println("NCI::  =======WAIT FOR PARTNER MIDLET ON PORT " + PORT_NUMBER + "========");
			System.out.println("NCI::  =======WAIT FOR PARTNER MIDLET ON PORT " + PORT_NUMBER + "========");
			System.out.println("NCI:: ===================================================\n\n\n");
			
			ServerSocketChannel ssc = null;
            SocketChannel socket = null;;
            try {
                ssc = ServerSocketChannel.open();
                ByteBuffer temp = ByteBuffer.allocate(1024 * 10);
                ssc.socket().bind(new InetSocketAddress(PORT_NUMBER));
                
                while (running) {

					
                    socket = ssc.accept();
                    temp.clear();
                    socket.read(temp);
                    
                    temp.flip();
                    CharBuffer cb = UTF8.decode(temp);
                    
                    String [] tokens = SPACE.split(cb, 3);
                    
                        /*for(int iter = 0 ; iter < tokens.length ; iter++) {
                            System.out.println("Token " + iter + ": " + tokens[iter]);
                        }*/
                    
                    ByteArrayOutputStream outputArray = new ByteArrayOutputStream();
                    DataOutputStream outputData = new DataOutputStream(outputArray);
                    
                    // the client is polling the server whether something is available
                    if(tokens [1].endsWith("/read")) {
                        Message message = null;
                        synchronized(this) {
                            if(pendingMessages.size() > 0) {
                                message = (Message)pendingMessages.remove(0);
                            }
                        }
                        
                        if(message != null) {
							outputData.writeBoolean(true);
                            outputData.writeUTF(message.getAddress());
                            if(message instanceof TextMessage) {
                                //System.out.println("NCI:: Sending SMS message: " + ((TextMessage)message).getPayloadText());
                                outputData.writeInt(0);
                                outputData.writeUTF(((TextMessage)message).getPayloadText());
                            } else if (message instanceof BinaryMessage){
                                //System.out.println("NCI:: Sending SMS binary message");
                                outputData.writeInt(1);
                                byte[] data = ((BinaryMessage)message).getPayloadData();
                                outputData.writeInt(data.length);
                                for(int iter = 0 ;iter < data.length ; iter++) {
                                    outputData.writeByte(data[iter]);
                                }
                            } else {
                                //System.out.println("NCI:: Sending MMS message");
                                //TODO:
                                outputData.writeInt(2);
				    byte[] data = ((NCIMultipartMessage)message).getAsByteArray();
				    outputData.writeInt(data.length);
				    for(int iter = 0 ;iter < data.length ; iter++) {
                                    outputData.writeByte(data[iter]);
                                }
                            }
                        } else {
                            outputData.writeBoolean(false);
                            outputData.writeInt(0);
                            
                            // this doesn't work due to synchronization between client and server
                            /*outputData.writeInt(listeningURLs.size());
                            Iterator iter = listeningURLs.keySet().iterator();
                            while(iter.hasNext()) {
                                outputData.writeUTF((String)iter.next());
                            }*/
                        }
                    } else {
                        //System.out.println("NCI:: Got read request");
                        tokens = tokens[1].split(";");
                        // if this is a binary message
                        Message current;
                        String url = URLDecoder.decode(tokens[3], "iso8859-1");
                        if(tokens[1].equals("b")) {
                            System.out.println("NCI:: Received BINARY SMS on:" + url);
                            NCIBinaryMessage message = new NCIBinaryMessage();
                            message.setPayloadData(URLDecoder.decode(tokens[2], "iso8859-1").getBytes("iso8859-1"));
                            current = message;
                        } else if (tokens[1].equals("t")){
                            System.out.println("NCI:: Received TEXT SMS on:" + url);
                            NCITextMessage message = new NCITextMessage();
                            message.setPayloadText(URLDecoder.decode(tokens[2], "UTF-8"));
                            current = message;
                        } else {
                            System.out.println("NCI:: Received MultiPart Message on:" + url);
                            current = NCIMultipartMessage.createFromByteArray(URLDecoder.decode(tokens[2], "iso8859-1").getBytes("iso8859-1"));
                        }
                        
                        synchronized(this) {
				NCIMessageConnection  connection;
                            int pos = url.lastIndexOf(':');
                            String port = "INVALID";
                            if(pos > -1) {
                                port = url.substring(pos + 1);
                            }
    				System.out.println("port = "+port);
                            connection = (NCIMessageConnection)listeningURLs.get(port);

				if (connection != null) {
				     connection.receivedMessages.add(current);
				} else {
				     System.out.println("WARNING====Ignored a received message");
				}
                        }
                        outputData.writeBoolean(true);                        
                    }
                    
                    sendFile(outputArray.toByteArray(), socket, "no/mime-type");
                    outputData.close();
                    
                    socket.close();
                }
                
                synchronized(this) {
                    notify();
                }
            } catch(IOException error) {
                error.printStackTrace();
                try {
                    socket.close();
                } catch(Throwable ignore1) {}
                try {
                    ssc.close();
                } catch(Throwable ignore2) {}
            }
        }
        
        public void sendFile(byte[] fileArray, GatheringByteChannel out,
        String contentType)
        throws IOException {
            ByteBuffer fileData = MappedByteBuffer.wrap(fileArray);
            
            sendBuffer(fileData, out, contentType);
        }
        
        private CharBuffer cbtemp = CharBuffer.allocate(1024);
        private ByteBuffer dynHdr = ByteBuffer.allocate(1024);
        
        private void sendBuffer(ByteBuffer data,
        GatheringByteChannel channel, String contentType)
        throws IOException {
            ByteBuffer [] buffers = { STATIC_HEADER, dynHdr, data };
            
            STATIC_HEADER.rewind();
            
            cbtemp.clear();
            cbtemp.put("Content-Length: " + data.limit());
            cbtemp.put(LINE_SEP);
            cbtemp.put("Content-Type: ");
            cbtemp.put(contentType);
            cbtemp.put(LINE_SEP);
            cbtemp.put(LINE_SEP);
            cbtemp.flip();
            
            buffers [1] = UTF8.encode(cbtemp);
            
            while (channel.write(buffers) != 0) {
                // nothing
            }
        }
    }
}
