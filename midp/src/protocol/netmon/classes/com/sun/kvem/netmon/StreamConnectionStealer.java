/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
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
    
    public void disconnect() {
        if (in != null) {
            in.disconnect();
        }
        
        if (out != null) {
            out.disconnect();
        }
    }
    
    /**
     * Set a new URL. Used when a stream reusage is detected
     */
    public void resetURL(String URL) {
        this.URL = URL;
    }

    /**
     * Reset the streams with a new URL. 
     * Used when a stream reusage is detected.
     */
    public void reset(String URL) {
        this.URL = URL;
        groupid = System.currentTimeMillis();

        if (out != null) {
            out.reset(URL, groupid);
        }

        if (in != null) {
            in.reset(URL, groupid);
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
