/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class TeardownMessage extends RequestMessage {
    public TeardownMessage(byte data[]) {
        super(data);
    }
  /*
    public TeardownMessage(String url, int sequenceNumber, int sessionId) {
        String msg = "TEARDOWN " + url + "RTSP/1.0" + "\r\n" + "CSeq: " +
                sequenceNumber + "\r\n" + "Session: " + sessionId + "\r\n";
    }
    */
}







