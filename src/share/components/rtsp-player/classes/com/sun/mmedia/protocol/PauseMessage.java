/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class PauseMessage extends RequestMessage {
    public PauseMessage(byte data[]) {
        super(data);
    }
  /*
    public PauseMessage(String url, int sequenceNumber, int sessionId) {
        String msg = "PAUSE " + url + "RTSP/1.0" + "\r\n" + "CSeq: " +
                sequenceNumber + "\r\n" + "Session: " + sessionId + "\r\n";
    }
    */
}


