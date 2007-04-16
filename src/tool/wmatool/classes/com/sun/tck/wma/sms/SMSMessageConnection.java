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

package com.sun.tck.wma.sms;

import com.sun.tck.wma.PropLoader;
import com.sun.tck.wma.BinaryMessage;
import com.sun.tck.wma.Message;
import com.sun.tck.wma.MessageConnection;
import com.sun.tck.wma.MessageTransportConstants;
import com.sun.tck.wma.TextMessage;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.Properties;
import com.sun.midp.io.j2me.sms.TextEncoder;

import java.io.IOException;


/**
 * SMS message connection handler.
 */
public class SMSMessageConnection extends PropLoader 
    implements MessageConnection {

    /** Machine name - the parsed target address from the URL. */
    protected String host = null;

    /** Port number from the URL connection string. */
    protected String port = null;

    /** Datagram host for sending/receiving. */
    protected String clientHost;

    /** Datagram transport for sending. */
    protected int portOut;

    /** Datagram transport for receiving. */
    protected int portIn;

    /** Phone number of the message sender. */
    protected String phoneNumber;

    /** Fragment size for large messages. */
    int fragmentsize;

    /** Datagram server connection. */
    DatagramSocket dgc; 

    /** Datagram buffer. */
    byte [] buf = new byte[MessageTransportConstants.DATAGRAM_PACKET_SIZE];

    /** Datagram envelope for sending or receiving messages. */
    DatagramPacket mess =
        new DatagramPacket(buf, MessageTransportConstants.DATAGRAM_PACKET_SIZE);

    /**
     * Open flag indicates when the connection is closed. When a connection is
     * closed, subsequent operations throw an exception.
     */
    protected boolean open;

    /** Constructor for SMS message connection handling. */
    public SMSMessageConnection() {

        /* 
         * Configurable parameters for low level transport.
         * e.g. sms://+5551234:54321 maps to datagram://129.148.70.80:123
         */

        clientHost = "localhost";
        portOut = 11100;
        portIn = 11101;
        phoneNumber = "+5551234";

        /* 
         * Check for overrides in the "connections.prop"
         * configuration file.
         */

        clientHost = getProp("localhost", "JSR_120_DATAGRAM_HOST",
            "connections.prop", "DatagramHost");
        
        portOut = getIntProp(11100, "JSR_120_SMS_OUT_PORT",
            "connections.prop", "SMSDatagramPortOut");

        portIn = getIntProp(11101, "JSR_120_SMS_PORT",
            "connections.prop", "SMSDatagramPortIn");

        phoneNumber = getProp("+5551234", "JSR_120_PHONE_NUMBER",
            "connections.prop", "PhoneNumber");
                
    }

    /**
     * Opens a connection.
     * <p>
     * This method is called from <code>Connector.open()</code> method to obtain
     * the destination address given in the <code>name</code> parameter.
     * <p>
     * The format for the <code>name</code> string for this method is:
     * <code>sms://<em>phone_number</em>:<em>port</em></code>
     * where the <em>phone_number:</em> is optional. If the
     * <em>phone_number</em> parameter is present, the connection is being
     * opened in "client" mode. This means that messages can be sent. If the
     * parameter is absent, the connection is being opened in "server" mode.
     * This means that messages can be sent and received.
     * <p>
     * The connection that is opened is to a low-level transport mechanism which
     * can be any of the following:
     * <ul>
     * <li>A datagram Short Message Peer to Peer (SMPP) to a service 
     * center.
     * <li>A <code>comm</code> connection to a phone device with AT-commands.
     * </ul>
     *
     * @param name the target of the connection
     * @return this connection
     * @throws IOException if the connection is closed or unavailable.
     */
    public MessageConnection openPrim(String name)
        throws IOException {
        /*
         * If <code>host == null</code>, then we are a server endpoint at
         * the supplied <code>port</code>.
         *
         * If <code>host != null</code> we are a client endpoint at a port
         * decided by the system and the default address for
         * SMS messages to be sent is <code>sms://host:port</code> .
         */
        
        if (name.charAt(0) != '/' || name.charAt(1) != '/') {
            throw new IllegalArgumentException(
                           "Missing protocol separator.");
        }
        
        int colon = name.indexOf(':');
        if (colon > 0) {
            if (colon != 2) {
                host = name.substring(2, colon);
            }
            port = name.substring(colon + 1);
        } else {
            if (name.length() > 2) {
                host = name.substring(2); 
            }
        }

        /* Open the inbound server datagram connection. */
        dgc =  new DatagramSocket(portIn);
        
        open = true;
        return this;
    }

    /**
     * Constructs a new message object of a text or binary type.
     * <p>
     * When the <code>TEXT_MESSAGE</code> constant is passed in, the created
     * object implements the <code>TextMessage</code> interface. When the
     * <code>BINARY_MESSAGE</code> constant is passed in, the created object
     * implements the <code>BinaryMessage</code> interface. 
     * <p>
     * If this method is called in a sending mode, a new <code>Message</code>
     * object is requested from the connection. For example:
     * <p>
     * <code>Message msg = conn.newMessage(TEXT_MESSAGE);</code>
     * <p>
     * The newly created <code>Message</code> does not have the destination
     * address set. It must be set by the application before the message is
     * sent.
     * <p>
     * If it is called in receiving mode, the <code>Message</code> object does
     * have its address set. The application can act on the object to extract
     * the address and message data. 
     * <p>
     * <!-- The <code>type</code> parameter indicates the number of bytes 
     * that should be
     * allocated for the message. No restrictions are placed on the application 
     * for the value of <code>size</code>.
     * A value of <code>null</code> is permitted and creates a 
     * <code>Message</code> object 
     * with a 0-length message. -->
     * 
     * @param  type either TEXT_MESSAGE or BINARY_MESSAGE.
     * @return      a new message
     */
    public Message newMessage(String type) {
        return newMessage(type, null);
    }

    /**
     * Constructs a new message object of a text or binary type and specifies
     * a destination address.
     * When the 
     * <code>TEXT_MESSAGE</code> constant is passed in, the created 
     * object implements the <code>TextMessage</code> interface.
     * When the <code>BINARY_MESSAGE</code> constant is passed in, the 
     * created object implements the <code>BinaryMessage</code> 
     * interface. 
     * <p>
     * The destination address <code>addr</code> has the following format:
     * <code>sms://</code><em>phone_number</em>:<em>port</em>.
     *
     * @param  type either TEXT_MESSAGE or BINARY_MESSAGE.
     * @param  addr the destination address of the message.
     * @return      a new <code>Message</code> object.
     */
    public Message newMessage(String type, String addr)  {
        /* Return the appropriate type of sub message. */
        if (type == MessageConnection.TEXT_MESSAGE) {
            return  new TextObject(addr); 
        } else if (type == MessageConnection.BINARY_MESSAGE) {
            return  new BinaryObject(addr);
        }
        return null; /* message type not supported */
    }

    /**
     * Sends a message over the connection. This method extracts
     * the data payload from 
     * the <code>Message</code> object so that it
     * can be sent as a datagram. 
     *
     * @param     dmsg a <code>Message</code> object.
     * @exception ConnectionNotFoundException  if the address is 
     *    invalid or if no address is found in the message.
     * @exception IOException  if an I/O error occurs.
     */
    public void send(Message dmsg) throws IOException {

        /** Saved timestamp for use with multiple segment records. */
        long sendtime = System.currentTimeMillis();

        byte[] buffer = null;
        String type = null;
        if (dmsg instanceof TextMessage) {
            type = MessageConnection.TEXT_MESSAGE;
            buffer = ((TextObject)dmsg).getPayloadData();
        } else if (dmsg instanceof BinaryMessage) {
            type = MessageConnection.BINARY_MESSAGE;
            buffer = ((BinaryObject)dmsg).getPayloadData();
        }

        /*
         * For text messages choose between UCS-2 or GSM 7-bit
         * encoding.
         */
        int encodingType = MessageTransportConstants.GSM_BINARY;
        if (type.equals(MessageConnection.TEXT_MESSAGE)) {
            byte[] gsm7bytes = TextEncoder.encode(buffer);
            if (gsm7bytes != null) {
                encodingType = MessageTransportConstants.GSM_TEXT;
                buffer = gsm7bytes;
            } else {
                encodingType = MessageTransportConstants.GSM_UCS2;
            }
        }


        mess = new DatagramPacket(buf,
                   MessageTransportConstants.DATAGRAM_PACKET_SIZE);
        mess.setAddress(InetAddress.getByName(clientHost));
        mess.setPort(portOut);

        SMSPacket smsPacket = new SMSPacket();
        smsPacket.setEncodingType(encodingType);
	if (port != null) {
            smsPacket.setPort(Integer.parseInt(port));
	} else {
            smsPacket.setPort(0);
	}
        smsPacket.setTimeStamp(sendtime);

        /*
         * Set target address.
         */
        smsPacket.setAddress(dmsg.getAddress());

        /*
         * Set sender's phone number
         */
        smsPacket.setPhoneNumber(phoneNumber);

        smsPacket.setMessageLength(buffer.length);
        smsPacket.setMessage(buffer);

        debug("SMS PACKET: clientHost = " + InetAddress.getByName(clientHost));
        debug("SMS PACKET: portOut = " + portOut);

        debug("SMS PACKET: encoding type = " + encodingType);
        debug("SMS PACKET: port = " + port);
        debug("SMS PACKET: timestamp = " + sendtime);
        debug("SMS PACKET: address = " + dmsg.getAddress());
        debug("SMS PACKET: sender's phone number = " + phoneNumber);
        debug("SMS PACKET: message length = " + buffer.length);
        debug("SMS PACKET: message:" + new String(buffer));
        byte[] buf = smsPacket.getData();

        mess.setData(buf, 0, buf.length);
        dgc.send(mess);
    }
    
    /**
     * Receives the bytes that have been sent over the connection, constructs a
     * <code>Message</code> object, and returns it. 
     * <p>
     * If there are no <code>Message</code>s waiting on the connection, this
     * method will block until a message is received, or the
     * <code>MessageConnection</code> is closed.
     *
     * @return a <code>Message</code> object
     * @exception IOException  if an I/O error occurs.
     */
    public synchronized Message receive()
        throws IOException {

        dgc.receive(mess);

        /* get message buffer and extract info */
        byte[] buf = mess.getData();
        SMSPacket packet = new SMSPacket(buf);
        int msgType = packet.getEncodingType();
        int smsPort = packet.getPort();
        long time = packet.getTimeStamp();
        String address = packet.getAddress();
        String phoneNumber = packet.getPhoneNumber();
        int msgLen = packet.getMessageLength();
        byte[] messg = packet.getMessage(msgLen);

        debug("SMS PACKET: encodingType = " + msgType);
        debug("SMS PACKET: port = " + smsPort);
        debug("SMS PACKET: time = " + time);
        debug("SMS PACKET: address = " + address);
        debug("SMS PACKET: Sender's phone number = " + phoneNumber);
        debug("SMS PACKET: message length = " + msgLen);
        debug("SMS PACKET: message:" + new String(messg));

        Message msg = null;
        if (msgType == MessageTransportConstants.GSM_TEXT ||
            msgType == MessageTransportConstants.GSM_UCS2) {

            TextMessage textmsg = (TextMessage) 
                newMessage(MessageConnection.TEXT_MESSAGE,
                           address);

            /* Always store text messages as UCS 2 bytes. */
            if (msgType == MessageTransportConstants.GSM_TEXT) {
                messg = TextEncoder.decode(messg);
            }
            ((TextObject)textmsg).setPayloadData(messg);
             msg = textmsg;
         } else {
             BinaryMessage binmsg = (BinaryMessage) 
                 newMessage(MessageConnection.BINARY_MESSAGE,
                            address);
             ((BinaryObject)binmsg).setPayloadData(messg);
             msg = binmsg;
         }
         ((MessageObject) msg).setTimeStamp(time);

        return msg; 
    }


    /**
     * Closes the connection. Reset the connection is open flag
     * so methods can be checked to throws an appropriate exception
     * for operations on a closed connection.
     *
     * @exception IOException  if an I/O error occurs.
     */
    public void close() throws IOException {

        if (open) {
            dgc.close();
            dgc = null;
            open = false;
        }
    }

    /**
     * Show a debug message.
     *
     * @param text  The text to be displayed after a mandatory prefix.
     */
    private void debug(String text) {
        System.out.println("SMSMessageConnection: " + text);
    }

}
