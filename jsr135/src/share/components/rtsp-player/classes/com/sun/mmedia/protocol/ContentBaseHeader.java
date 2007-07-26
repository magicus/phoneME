/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia.rtsp.protocol;

public class ContentBaseHeader {
    private String contentBase;

    public ContentBaseHeader(String contentBase) {
        // Debug.println( "Content-Base : " + contentBase);

        this.contentBase = contentBase;
    }

    public String getContentBase() {
        return contentBase;
    }
}
