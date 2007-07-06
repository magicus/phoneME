/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class PlayMessage extends RequestMessage {
    public PlayMessage(byte data[]) {
        super(data);
    }
  /*
    public PlayMessage(String url, int sequenceNumber, int sessionId,
            int range_lo, int range_hi) {
        String msg = "PLAY " + url + "RTSP/1.0" + "\r\n" + "CSeq: " +
                sequenceNumber + "\r\n" + "Session: " + sessionId +
                "\r\n" + "Range: npt=" + range_lo + "-" + range_hi;
    }
    */
}
