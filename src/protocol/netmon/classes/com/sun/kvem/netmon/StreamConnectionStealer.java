/*
 * Copyright © 2007 Sun Microsystems, Inc. All rights reserved
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
*/
package com.sun.kvem.netmon;

import javax.microedition.io.StreamConnection;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;


/**
 * Wrapps a StreamConnection and steal the data from its input and output streams
 * 
 *
 *@author ah123546
 *@created December 25, 2001
 *@version 
 * @see InputStreamStealer
 * @see OutputStreamStealer
 */
public class StreamConnectionStealer
    implements StreamConnection {

    StreamConnection con;
    StreamAgent inputNetAgent;
    StreamAgent outputNetAgent;
    InputStreamStealer in;
    OutputStreamStealer out;
    String URL;
    long groupid;

    boolean closed;
    /**
     * Constructor for the StreamConnectionStealer object
     */
    public StreamConnectionStealer(String url, StreamConnection con, 
                                   StreamAgent agent) {
        this(url, con, agent, agent);
    }

    /**
     * Constructor for the StreamConnectionStealer object
     */
    public StreamConnectionStealer(String url, StreamConnection con, 
                                   StreamAgent inAgent, StreamAgent outAgent) {
        this.con = con;
        this.inputNetAgent = inAgent;
        this.outputNetAgent = outAgent;
        this.URL = url;
        groupid = System.currentTimeMillis();
    }

    public void setStreams(InputStream streamInput, OutputStream streamOutput)
                    throws IOException {
        in = new InputStreamStealer(URL, streamInput, inputNetAgent, groupid);
        out = new OutputStreamStealer(URL, streamOutput, outputNetAgent, 
                                      groupid);
    }

    public InputStream openInputStream()
                                throws IOException {

        if (in == null) {
            in = new InputStreamStealer(URL, con.openInputStream(), 
                                        inputNetAgent, groupid);
        }

        return in;
    }

    public DataInputStream openDataInputStream()
                                        throws IOException {

        return new DataInputStream(openInputStream());
    }

    public OutputStream openOutputStream()
                                  throws IOException {

        if (out == null) {
            out = new OutputStreamStealer(URL, con.openOutputStream(), 
                                          outputNetAgent, groupid);
        }
        
        if (out.isClosed()) {
            throw new IOException("stream closed");
        }

        return out;
    }

    public DataOutputStream openDataOutputStream()
                                          throws IOException {

        return new DataOutputStream(openOutputStream());
    }

    public void close()
               throws IOException {
        if (!closed) {           
            closed = true;
            con.close();

            if (in != null) {
                in.close();
            }

            if (out != null) {
                out.close();
            }
        }
    }

    /**
    * This method is called when a new message is about to start.
    * It is called even for a reused connection.
    */
    public void newMsg()
                throws IOException {

        if (out != null) {
            out.newMsg();
        }

        if (in != null) {
            in.newMsg();
        }
    }

    public int available() throws IOException {
        return in.available();
    }    
}
