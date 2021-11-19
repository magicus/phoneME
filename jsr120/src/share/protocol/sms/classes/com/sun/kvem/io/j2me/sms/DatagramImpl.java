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

package com.sun.kvem.io.j2me.sms;

import com.sun.midp.io.j2me.sms.TextEncoder;
import com.sun.midp.io.j2me.datagram.DatagramObject;

import javax.microedition.io.Datagram;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.Vector;

/**
 * Sending the information about cbs and sms actions.
 * to network monitor window.
 * 
 */
public class DatagramImpl{

    /** Fragment size for large messages. */
    int fragmentsize;

    /** Phone number for the device. */
    protected String phoneNumber;

    /**
     * Notify the network monitor about sms send action.
     *
     * @param address recipient's address of message
     * @param buffer content of message
     * @param senderPort port value placed after "sms://"
     * @param messageType encoding value for text sms
     * @param segments number of segments
     * @param groupId unique ID of sending action
     * @return time in milliseconds of receiving action
     */
    public long send(String address, byte[] buffer, 
                     String senderPort, int messageType,
                     int segments, long groupID) throws IOException {

        /** The length of the payload in elements. */
        int payloadLength;
        /** The number of bits per a payload element. */
        int bitsPerPayloadElement;
        /** Indicates if the address contains a port number. */
        boolean containsPort = false;

        /** Formatted datagram record. */
        DatagramRecord dr = new DatagramRecord();

        /** Saved timestamp for use with multiple segment records. */
        long sendtime = System.currentTimeMillis();

        /** Offset in the sending buffer for the current fragment. */
        int offset = 0;

        /* Total length of the multisegment transmission. */
        int length;

        /* Check if the target address includes a port number. */
        int colon = address.indexOf(":");
        colon = address.indexOf(":", colon + 1);
        if (colon != -1) {
            containsPort = true;
        }

        if (buffer == null) {
            /* 
             * Allow sending messages with empty buffer.
             */
            buffer = new byte[0];
        } 

        /*
         * For text messages choose between UCS-2 or GSM 7-bit
         * encoding.
         */
        String currValue = getTextEncoding(messageType);
        if (currValue != null) {
            dr.setHeader("Text-Encoding", currValue);
            if (messageType == com.sun.midp.io.j2me.sms.Protocol.GSM_TEXT) {
                byte[] gsm7bytes = TextEncoder.encode(buffer);
                if (gsm7bytes != null) {
                    buffer = gsm7bytes;
                }
            }
        }

        length = buffer.length;
        int md = beginSending(address, groupID);
        /* Fragment the data buffer into multiple segments. */
        for (int i = 0; i < segments; i++) {

            dr.setHeader("Date", String.valueOf(sendtime));
            dr.setHeader("Address", address);
            if (phoneNumber == null) {
                phoneNumber = System.getProperty("com.sun.midp.io.j2me.sms.PhoneNumber");
            }
            if ((senderPort == null) || senderPort.equals("-1")) {
                dr.setHeader("SenderAddress", "sms://" + phoneNumber);
            } else {
                dr.setHeader("SenderAddress", "sms://" + phoneNumber + ":" 
                             + senderPort);
            }

            if (messageType == com.sun.midp.io.j2me.sms.Protocol.GSM_BINARY) {
                currValue = "binary";
            } else {
                currValue = "text";
            }
            dr.setHeader("Content-Type", currValue);
            dr.setHeader("Content-Length", String.valueOf(buffer.length));
            dr.setHeader("Segments", String.valueOf(segments));

            /* IMPL_NOTE - implementation has no API for getting fragment information
            if (segments > 1) {
                offset = i* fragmentsize;
                length =  (i < (segments -1) ? fragmentsize :
                   buffer.length - (fragmentsize * i));
                dr.setHeader("Fragment", String.valueOf(i));
                dr.setHeader("Fragment-Size", String.valueOf(length));
                dr.setHeader("Fragment-Offset", String.valueOf(offset));
            }
             */
            byte[] buf = new byte[length];
            System.arraycopy(buffer, offset, buf, 0, length);
            dr.setData(buf);
            byte [] messdata = dr.getFormattedData();

            send0(md, messdata);
        }
        close0(md);
        return sendtime;
    }

    /**
     * Notify the network monitor about sms and cbs receive action.
     *
     * @param messageType message type id (GSM_TEXT, GSM_BINARY or GSM_UCS2)
     * @param address sender's address of message
     * @param buffer content of message
     * @param senderPort port value placed after "cbs(sms)://"
     * @param segments number of segments
     * @param groupId unique ID of receiving action
     * @param protocol the protocol name ("cbs" or "sms")
     * @return time in milliseconds of receiving action
     */
    public long receive(int messageType, String address, byte[] buffer, 
                        String senderPort, int segments, long groupID, String protocol) 
                throws IOException {

        /** The length of the payload in elements. */
        int payloadLength;
        /** The number of bits per a payload element. */
        int bitsPerPayloadElement;
        /** Indicates if the address contains a port number. */
        boolean containsPort = false;

        /** Formatted datagram record. */
        DatagramRecord dr = new DatagramRecord();

        /** Saved timestamp for use with multiple segment records. */
        long sendtime = System.currentTimeMillis();

        /** Offset in the sending buffer for the current fragment. */
        int offset = 0;

        /* Total length of the multisegment transmission. */
        int length;

        /* Check if the target address includes a port number. */
        int colon = address.indexOf(":");
        colon = address.indexOf(":", colon + 1);
        if (colon != -1) {
            containsPort = true;
        }

        if (buffer == null) {
            /* 
             * Allow sending messages with empty buffer.
             */
            buffer = new byte[0];
        } 

        /*
         * For text messages choose between UCS-2 or GSM 7-bit
         * encoding.
         */
        String currValue = getTextEncoding(messageType);
        if (currValue != null) {
            dr.setHeader("Text-Encoding", currValue);
            if (messageType == com.sun.midp.io.j2me.sms.Protocol.GSM_TEXT) {
                byte[] gsm7bytes = TextEncoder.encode(buffer);
                if (gsm7bytes != null) {
                    buffer = gsm7bytes;
                }
            }
        }

        length = buffer.length;
        int md = beginReceiving(groupID);

        Datagram mess = new DatagramObject(buffer, buffer.length);

        dr.setHeader("Date", String.valueOf(sendtime));
        dr.setHeader("SenderAddress", address);
        if (phoneNumber == null) {
            phoneNumber = System.getProperty("com.sun.midp.io.j2me.sms.PhoneNumber");
        }
        String  addrHeader= protocol + "://" + phoneNumber;
        if (senderPort != null) {
            addrHeader += ":" + senderPort;
        }
        if (protocol.equals("sms")) {
            dr.setHeader("Address", addrHeader);
        } else if (protocol.equals("cbs")) {
            dr.setHeader("CBSAddress", addrHeader);
        }

        if (messageType == com.sun.midp.io.j2me.sms.Protocol.GSM_BINARY) {
            currValue = "binary";
        } else {
            currValue = "text";
        }
        dr.setHeader("Content-Type", currValue);
        dr.setHeader("Content-Length", String.valueOf(buffer.length));
        dr.setHeader("Segments", String.valueOf(segments));

        /*  IMPL_NOTE - implementation has no API for getting fragment information
        if (segments > 1) {
            offset = i* fragmentsize;
            length =  (i < (segments -1) ? fragmentsize :
                   buffer.length - (fragmentsize * i));
            dr.setHeader("Fragment", String.valueOf(i));
            dr.setHeader("Fragment-Size", String.valueOf(length));
            dr.setHeader("Fragment-Offset", String.valueOf(offset));
        }
        */
        byte[] buf = new byte[length];
        System.arraycopy(buffer, offset, buf, 0, length);
        dr.setData(buf);
        byte [] messdata = dr.getFormattedData();
        mess.setData(messdata, 0, messdata.length);
        received0(md, mess.getData(), mess.getLength());
        close0(md);
        return sendtime;
    }

    /**
     * Getting the text encoding name.
     *
     * @param messageType id of encoding
     * @return name of encoding
     */
    private String getTextEncoding(int messageType) {
        String retName = null;
        switch (messageType) {
            case com.sun.midp.io.j2me.sms.Protocol.GSM_TEXT:
                retName = "gsm7bit";
                break;
            case com.sun.midp.io.j2me.sms.Protocol.GSM_UCS2:
                retName = "ucs2";
                break;
        }
        return retName;
    }


    private static int beginSending(String url, long groupID) {
        int md = open0(url, 0, groupID);
        return md;
    }

    private static int beginReceiving(long groupID) {
        return open0("", 1, groupID);
    }

    private static native int open0(String name, int mode, long groupid);
    private static native void close0(int md);
    private static native void send0(int md, byte datagram[]);
    private static native void received0(int md, byte datagram[], int length);

}
