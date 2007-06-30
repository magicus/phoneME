/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

import java.io.*;
import java.util.*;

public class Header extends Parser {
    public int type; // the header type
    public Object parameter; // parameter entries
    public int contentLength;

    public final static int TRANSPORT = 1;
    public final static int CSEQ = 2;
    public final static int SESSION = 3;
    public final static int DURATION = 4;
    public final static int RANGE = 5;
    public final static int DATE = 6;
    public final static int SERVER = 7;
    public final static int CONTENT_TYPE = 8;
    public final static int CONTENT_BASE = 9;
    public final static int CONTENT_LENGTH = 10;

    public Header(String input) {
        ByteArrayInputStream bin =
                new ByteArrayInputStream(input.getBytes());

        String id = getToken(bin).toUpperCase();

        // changed equalsIgnoreCase to toUpperCase

        if (id.equals("CSEQ:")) {
            type = CSEQ;

            String number = getStringToken(bin).trim();

            parameter = new CSeqHeader(number);
        } else if (id.equals("TRANSPORT:")) {
            type = TRANSPORT;

            String tx = getToken(bin);

            parameter = new TransportHeader(tx);
        } else if (id.equals("SESSION:")) {
            type = SESSION;

            String tx = getToken(bin);

            parameter = new SessionHeader(tx);
        } else if (id.equals("DURATION:")) {
            type = DURATION;

            String tx = getToken(bin);

            System.out.println("Duration : " + tx);

            parameter = new DurationHeader(tx);
        } else if (id.equals("RANGE:")) {
            type = RANGE;

            String tx = getToken(bin);

            parameter = new RangeHeader(tx);
        } else if (id.equals("DATE:")) {
            type = DATE;

            String date = getStringToken(bin);

            // Debug.println( "Date : " + date);
        } else if (id.equals("ALLOW:")) {
            type = DATE;

            String entries = getStringToken(bin);

            // Debug.println( "Allow : " + entries);
        } else if (id.equals("SERVER:")) {
            type = SERVER;

            String server = getStringToken(bin);

            // Debug.println ( "Server   : " + server);
        } else if (id.equals("CONTENT-TYPE:")) {
            type = CONTENT_TYPE;

            String content_type = getStringToken(bin);

            // Debug.println( "Content-Type : " + content_type);
        } else if (id.equals("CONTENT-BASE:")) {
            type = CONTENT_BASE;
            String content_base = getStringToken(bin);

            parameter = new ContentBaseHeader(content_base);
        } else if (id.equals("CONTENT-LENGTH:")) {
            type = CONTENT_LENGTH;

            String content_length = getStringToken(bin);

            // System.out.println( "Content-Length : " + content_length);

            contentLength = Integer.parseInt(content_length);
        } else if (id.equals("LAST-MODIFIED:")) {
            String date = getStringToken(bin);

            // Debug.println( "Last-Modified: " + date);
        } else if (id.equals("RTP-INFO:")) {
            String rtpInfo = getStringToken(bin);

            // Debug.println( "RTP-Info: " + rtpInfo);
        } else if (id.length() > 0) {
            System.out.println("unknown id : <" + id + ">");

            String tmp = getStringToken(bin);
        }
    }
}







