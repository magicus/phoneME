/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class RangeHeader {
    public final long NOW = -1;
    private long startPos;
    public RangeHeader(String str) {
        int start = str.indexOf('=') + 1;
        int end = str.indexOf('-');

        String startPosStr = str.substring(start, end);

        if (startPosStr.equals("now"))
        {
            startPos = NOW;
        }
        else
        {
            startPos = Long.parseLong(startPosStr);
        }
    }

    public long getStartPos() {
        return startPos;
    }
}
