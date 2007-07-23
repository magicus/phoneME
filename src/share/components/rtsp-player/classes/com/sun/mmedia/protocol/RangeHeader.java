/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class RangeHeader {
    private long startPos;

    public RangeHeader(String str) {
        int start = str.indexOf('=') + 1;
        int end = str.indexOf('-');

        String startPosStr = str.substring(start, end);

        if (startPosStr.equals("now"))
        {
            startPos = 0; //IMPL_NOTE: not sure it must actually be zero.
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
