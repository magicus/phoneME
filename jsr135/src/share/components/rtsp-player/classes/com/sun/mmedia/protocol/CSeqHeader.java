/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class CSeqHeader {
    private String sequence_number;

    public CSeqHeader(String number) {
        this.sequence_number = number;
    }

    public String getSequenceNumber() {
        return sequence_number;
    }
}



