package com.sun.midp.io.j2me.http;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;

import java.io.IOException;

public class Protocol extends com.sun.cdc.io.j2me.http.Protocol {
    
    /**
     * This class overrides the openXXputStream() methods to restrict the number of opened 
     * input or output streams to 1 since the MIDP GCF Spec allows only 1 opened input/output stream.
     */
    
    /** Number of input streams that were opened. */
    protected int iStreams = 0;
    /**
     * Maximum number of open input streams. Set this
     * to zero to prevent openInputStream from giving out a stream in
     * write-only mode.
     */
    protected int maxIStreams = 1;
    /** Number of output streams were opened. */
    protected int oStreams = 0;
    /**
     * Maximum number of output streams. Set this
     * to zero to prevent openOutputStream from giving out a stream in
     * read-only mode.
     */
    protected int maxOStreams = 1;

    /*
     * Open the input stream if it has not already been opened.
     * @exception IOException is thrown if it has already been
     * opened.
     */
    public InputStream openInputStream() throws IOException {
        if (maxIStreams == 0) {
            throw new IOException("no more input streams available");
        }
        InputStream i = super.openInputStream();
        maxIStreams--;
        iStreams++;
        return i;
    }
    
    
    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    /*
     * Open the output stream if it has not already been opened.
     * @exception IOException is thrown if it has already been
     * opened.
     */
    public OutputStream openOutputStream() throws IOException {
        if (maxOStreams == 0) {
            throw new IOException("no more output streams available");
        }
        OutputStream o = super.openDataOutputStream();
        maxOStreams--;
        oStreams++;
        return o;
    }
    
    public DataOutputStream openDataOutputStream() throws IOException {
        return new DataOutputStream(openOutputStream());
    }

    /*
     * throws SecurityException if MIDP permission check fails 
    */
    protected void checkMIDPPermission(String url) {
        //The actual MIDP permission check happens here
    }
}
