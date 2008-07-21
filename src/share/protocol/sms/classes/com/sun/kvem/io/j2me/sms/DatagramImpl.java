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

package com.sun.kvem.io.j2me.sms;

import com.sun.midp.io.j2me.sms.TextEncoder;
import com.sun.midp.io.j2me.datagram.DatagramObject;

import javax.microedition.io.Datagram;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.Vector;


public class DatagramImpl{

    /** Fragment size for large messages. */
    int fragmentsize;

    /** Phone number for the device. */
    protected String phoneNumber;

    public long send(String type, String address, byte[] buffer, 
                     String senderPort, long groupID) 
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

        /** Number of segments that need to be sent. */
        int segments = 1;

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
        if (type.equals("text")) {
            byte[] gsm7bytes = TextEncoder.encode(buffer);
            if (gsm7bytes != null) {
                // 160/152/145
                bitsPerPayloadElement = 7;
                payloadLength = gsm7bytes.length;
                dr.setHeader("Text-Encoding", "gsm7bit");
                buffer = gsm7bytes;
            } else {
                // 140/132/126
                bitsPerPayloadElement = 16;
                payloadLength = buffer.length / 2;
                dr.setHeader("Text-Encoding", "ucs2");
            }
        } else {
            // 140/133/126
            bitsPerPayloadElement = 8;
            payloadLength = buffer.length;
        }

	fragmentsize = calculateFragmentSize(payloadLength, 
                bitsPerPayloadElement, containsPort);
	if (payloadLength > fragmentsize) {
	    segments = (payloadLength + fragmentsize - 1) / fragmentsize;
	}

        /*
         *  Check the maximum allowed SMS message size from
         *  GSM 03.40 Section 9.2.3.24.1. 
         */
	if (payloadLength > (255*fragmentsize)) {
	    throw new IOException("Message is too large");
	}

        if (bitsPerPayloadElement == 16) {
            fragmentsize *= 2;
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

            dr.setHeader("Content-Type", type);
            dr.setHeader("Content-Length", String.valueOf(buffer.length));
            dr.setHeader("Segments", String.valueOf(segments));

            if (segments > 1) {
            offset = i* fragmentsize;
            length =  (i < (segments -1) ? fragmentsize :
                   buffer.length - (fragmentsize * i));
            dr.setHeader("Fragment", String.valueOf(i));
            dr.setHeader("Fragment-Size", String.valueOf(length));
            dr.setHeader("Fragment-Offset", String.valueOf(offset));
            }
            byte[] buf = new byte[length];
            System.arraycopy(buffer, offset, buf, 0, length);
            dr.setData(buf);
            byte [] messdata = dr.getFormattedData();

            send0(md, messdata);
        }
        close0(md);
        return sendtime;
    }

    public long receive(String type, String address, byte[] buffer, 
                        String senderPort, long groupID, String protocol) 
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

        /** Number of segments that need to be sent. */
        int segments = 1;

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
        if (type.equals("text")) {
            byte[] gsm7bytes = TextEncoder.encode(buffer);
            if (gsm7bytes != null) {
                // 160/152/145
                bitsPerPayloadElement = 7;
                payloadLength = gsm7bytes.length;
                dr.setHeader("Text-Encoding", "gsm7bit");
                buffer = gsm7bytes;
            } else {
                // 140/132/126
                bitsPerPayloadElement = 16;
                payloadLength = buffer.length / 2;
                dr.setHeader("Text-Encoding", "ucs2");
            }
        } else {
            // 140/133/126
            bitsPerPayloadElement = 8;
            payloadLength = buffer.length;
        }

        fragmentsize = calculateFragmentSize(payloadLength, 
                bitsPerPayloadElement, containsPort);
        if (payloadLength > fragmentsize) {
            segments = (payloadLength + fragmentsize - 1) / fragmentsize;
        }

        /*
         *  Check the maximum allowed SMS message size from
         *  GSM 03.40 Section 9.2.3.24.1. 
         */
        if (payloadLength > (255*fragmentsize)) {
            throw new IOException("Message is too large");
        }

        if (bitsPerPayloadElement == 16) {
            fragmentsize *= 2;
        }

        length = buffer.length;
        int md = beginReceiving(groupID);
        /* Fragment the data buffer into multiple segments. */
        for (int i = 0; i < segments; i++) {

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

            dr.setHeader("Content-Type", type);
            dr.setHeader("Content-Length", String.valueOf(buffer.length));
            dr.setHeader("Segments", String.valueOf(segments));
            if (segments > 1) {
            offset = i* fragmentsize;
            length =  (i < (segments -1) ? fragmentsize :
                   buffer.length - (fragmentsize * i));
            dr.setHeader("Fragment", String.valueOf(i));
            dr.setHeader("Fragment-Size", String.valueOf(length));
            dr.setHeader("Fragment-Offset", String.valueOf(offset));
            }
            byte[] buf = new byte[length];
            System.arraycopy(buffer, offset, buf, 0, length);
            dr.setData(buf);
            byte [] messdata = dr.getFormattedData();
            mess.setData(messdata, 0, messdata.length);
            received0(md, mess.getData(), mess.getLength());
        }
        close0(md);
        return sendtime;
    }


    /**
     * Maximum size of segment (includes the size of a header and the size of 
     * payload) in bytes. 
     */
    private static final int MAX_USERDATA_SIZE = 140;
    /** The size needed for the Length of User Data Header field. */
    private static final int SIZE_HEADERLEN_FIELD = 1;
    /** The size needed for the port addressing information. */
    private static final int SIZE_ADDRESSING_INFO = 6;
    /** The size needed for the concatenation information. */
    private static final int SIZE_CONCATENTATION_INFO = 6;

    /**
     * Calculates the number of payload elements a protocol fragment can hold.
     *
     * @param payloadLength the length of payload to transfer
     * @param bitsPerPayloadElement the number of bits per a payload element
     * @param containsPort indicates if the message holds port addressing 
     *          information
     * @return the calculated number of payload elements per a protocol fragment
     */
    protected static int calculateFragmentSize(
        int payloadLength,
        int bitsPerPayloadElement,
        boolean containsPort) {

        /** First assume there is no header. */
        int maxFragmentInBytes = MAX_USERDATA_SIZE;

        if (containsPort) {
            /** Reserve space for the Length of User Data Header field. */
            maxFragmentInBytes -= SIZE_HEADERLEN_FIELD;
            /** Reserve space for the port addressing. */
            maxFragmentInBytes -= SIZE_ADDRESSING_INFO;
        }

        /** Calculate free space for the payload. */
        int maxFragmentInElements = (maxFragmentInBytes * 8) / bitsPerPayloadElement;
        if (payloadLength <= maxFragmentInElements) return maxFragmentInElements;


        if (maxFragmentInBytes == MAX_USERDATA_SIZE) {
            /**
             * There was no header before, so reserve space for the Length
             * of User Data Header field now.
             */
            maxFragmentInBytes -= SIZE_HEADERLEN_FIELD;
        }
        /** Reserve space for the concatenation information. */
        maxFragmentInBytes -= SIZE_CONCATENTATION_INFO;
        /** Recalculate free space for the payload. */

        return (maxFragmentInBytes * 8) / bitsPerPayloadElement;
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
