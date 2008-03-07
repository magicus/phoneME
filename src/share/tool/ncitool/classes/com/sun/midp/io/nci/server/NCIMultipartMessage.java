package com.sun.midp.io.nci.server;

import com.sun.tck.wma.*;

/**
 * There is a stub implementation to avoid compiler errors for pure JSR120 build.
 */
public class NCIMultipartMessage extends NCIMessage {

    public NCIMultipartMessage() {
        throw new IllegalArgumentException("Multipart message are not supported");
    }

    public void setTimeStamp(long timestamp) {
        throw new IllegalArgumentException("Multipart message are not supported");
    }

    public java.util.Date getTimestamp() {
        throw new IllegalArgumentException("Multipart message are not supported");
    }

    public byte[] getAsByteArray() {
        throw new IllegalArgumentException("Multipart message are not supported");
    }

    public static NCIMultipartMessage createFromByteArray(byte[] data) {
        throw new IllegalArgumentException("Multipart message are not supported");
    }
}
