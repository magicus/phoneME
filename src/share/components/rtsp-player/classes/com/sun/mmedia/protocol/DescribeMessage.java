/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class DescribeMessage extends RequestMessage {
    public DescribeMessage(byte data[]) {
        super(data);
    }
  /*
    public DescribeMessage(String url, int sequenceNumber) {
        String msg = "DESCRIBE " + url + "RTSP/1.0" + "\r\n" + "CSeq: " +
                sequenceNumber + "\r\n\r\n";
    }
    */
}





