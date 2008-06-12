/*
 * @(#)Protocol.java	1.50 06/10/16
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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
 *
 */

package com.sun.cdc.io.j2me.http;

import java.io.IOException;
import java.io.InputStream;
import java.io.InterruptedIOException;
import java.io.OutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.ByteArrayOutputStream;

import java.net.URL;
import java.net.MalformedURLException;
import java.net.Socket;

import java.util.Hashtable;
import java.util.Enumeration;

import javax.microedition.io.StreamConnection;
import javax.microedition.io.HttpConnection;
import javax.microedition.io.Connector;
import javax.microedition.io.ConnectionNotFoundException;

import com.sun.cdc.io.ConnectionBase;
import com.sun.cdc.io.DateParser;

/**
 * This class implements the necessary functionality
 * for an HTTP connection. 
 */
public class Protocol extends ConnectionBase implements HttpConnection {
    private int index;          // used by URL parsing functions
    protected String url;
    protected String protocol;
    protected String host;
    private String file;
    private String ref;
    private String query;
    protected int port = 80;
    protected int responseCode;
    protected String responseMsg;
    protected Hashtable reqProperties;
    protected Hashtable headerFields;
    private String[] headerFieldNames;
    private String[] headerFieldValues;
    protected String method;
    protected int opens;
    protected int mode;
    protected Socket socket;

    protected boolean connected;
    /* there should be only one outputstream opened at any time */
    protected boolean outputStreamOpened = false;
    protected boolean requested = false;
    /*
     * In/Out Streams used to buffer input and output
     */
    private PrivateInputStream in;
    private PrivateOutputStream out;

    /*
     * The data streams provided to the application.
     * They wrap up the in and out streams.
     */
    private DataInputStream appDataIn;
    private DataOutputStream appDataOut;

    /*
     * The streams from the underlying socket connection.
     */
    private StreamConnection streamConnection;
    private DataOutputStream streamOutput;
    private DataInputStream streamInput;

    /*
     * A shared temporary buffer used in a couple of places
     */
    private StringBuffer stringbuffer;

    private String proxyHost = null;
    private int proxyPort = 80;

    private String http_version = "HTTP/1.1";

    /**
     * create a new instance of this class.
     * We are initially unconnected.
     */
    public Protocol() {
        reqProperties = new Hashtable();
        headerFields = new Hashtable();
        stringbuffer = new StringBuffer(32);
        opens = 0;
        connected = false;
        method = GET;
        responseCode = -1;
        protocol = "http";
	socket = null;

        // Check for the HTTP proxy only if no security manager exists
        java.lang.SecurityManager sm = System.getSecurityManager();
        if (sm == null){
	    String http_proxy;
	    String profileTemp = System.getProperty("microedition.profiles");
	    if (profileTemp != null && profileTemp.indexOf("MIDP") != -1) {
		// We want to look for a MIDP property specifying proxies.
		http_proxy = System.getProperty("com.sun.midp.io.http.proxy");
	    } else {
		// Default to CDC
		http_proxy = System.getProperty("com.sun.cdc.io.http.proxy");
	    }
	    parseProxy(http_proxy);
        }
    }

    /*
     * Check permission to connect to the indicated host.
     * This should be overriden by the MIDP protocol handler
     * to check the proper MIDP permission.
     */
    protected void checkPermission(String host, int port, String file) {
        // Check for SecurityManager.checkConnect()
        java.lang.SecurityManager sm = System.getSecurityManager();
        if (sm != null) {
            sm.checkConnect(host, port);
        }               
        return;
    }

    /*
     * Check permission when opening an OutputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method. Throw a SecurityException if
     * the connection is not allowed.
     */
    protected void outputStreamPermissionCheck() {
        // Check for SecurityManager.checkConnect()
        java.lang.SecurityManager sm = System.getSecurityManager();
        if (sm != null){
            if (host != null) {
                sm.checkConnect(host, port);
            } else {
                sm.checkConnect("localhost", port);
            }
        }
        return;
    }

    /*
     * Check permission when opening an InputStream. MIDP
     * versions of the protocol handler should override this
     * with an empty method. A SecurityException will be
     * raised if the connection is not allowed.
     */
    protected void inputStreamPermissionCheck() {
        // Check for SecurityManager.checkConnect()
        java.lang.SecurityManager sm = System.getSecurityManager();
        if (sm != null){
            if (host != null) {
                sm.checkConnect(host, port);
            } else {
                sm.checkConnect("localhost", port);
            }
        }
        return;
    }

    public void open(String url, int mode, boolean timeouts) throws IOException {
        // DEBUG: System.out.println ("open " + url); 
        if (opens > 0) {
            throw new IOException("already connected");
        }

        opens++;

        if (mode != Connector.READ && mode != Connector.WRITE
            && mode != Connector.READ_WRITE) {
            throw new IOException("illegal mode: " + mode);
        }
        
        this.url = url;
        this.mode = mode;
        parseURL();

	if ((host.indexOf('/') != -1) || 
	    (host.indexOf('@') != -1) ||
	    (host.indexOf('?') != -1) ||
	    (host.indexOf(';') != -1)) {
	    throw new IllegalArgumentException("hostname " + 
  		         host +
			 " cannot contain \"?\" , \"@\" , \";\", \":\", or \"/\" character.");
	}

	// Check permission. The permission method wants the URL
	checkPermission(host, port, file);

        // Try to open connection to test for ConnectionNotFoundException
        try {
	    streamConnection=connectSocket();
            connectStream();
            // This is only a test of the connection, so close it again
        } catch (java.net.UnknownHostException e) {
            throw new ConnectionNotFoundException
                ("Could not find "+host+":"+port);
        }

    }

    public void close() throws IOException {
        // DEBUG: System.out.println ("close " + opens + " " + connected ); 
        if (--opens == 0 && (connected || requested))
            disconnect();

    }

    /*
     * Open the input stream if it has not already been opened.
     * @exception IOException is thrown if it has already been
     * opened.
     */
    public InputStream openInputStream() throws IOException {
         // DEBUG: System.out.println ("open input stream");

	inputStreamPermissionCheck();

        /* CR 6226615: opening another stream should not throw IOException
        if (in != null) {
            throw new IOException("already open");
        }
        */
        // If the connection was opened and closed before the 
        // data input stream is accessed, throw an IO exception
        if (opens == 0 ){
            throw new IOException("connection is closed");
        }

        // Check that the connection was opened for reading
        if (mode != Connector.READ && mode != Connector.READ_WRITE) {
            throw new IOException("write-only connection");
        }

        connect();
        opens++;

        in = new PrivateInputStream();
        return in;
    }

    public DataInputStream openDataInputStream() throws IOException {
        // If the connection was opened and closed before the 
        // data input stream is accessed, throw an IO exception
        if (opens == 0 ){
            throw new IOException("connection is closed");
        }

        /* CR 6226615: opening another stream should not throw IOException
        if (appDataIn != null) {
            throw new IOException("already open");
        }
        */

        // TBD: throw in exception if the connection has been closed.
        if (in == null) {
            openInputStream();
        }

        appDataIn = new DataInputStream(in);
        return appDataIn;
    }

    public OutputStream openOutputStream() throws IOException {
        // Delegate to openDataOutputStream
        return openDataOutputStream();
    }

    public DataOutputStream openDataOutputStream() throws IOException {

	outputStreamPermissionCheck();

         // DEBUG: System.out.println ("open data output stream");
        if (mode != Connector.WRITE && mode != Connector.READ_WRITE) {
            throw new IOException("read-only connection");
        }

        // If the connection was opened and closed before the 
        // data output stream is accessed, throw an IO exception
        if (opens == 0 ){
            throw new IOException("connection is closed");
        }

        /* CR 6226615: opening another stream should not throw IOException
        if (out != null) {
            throw new IOException("already open");
        }
        */

        opens++;
        out = new PrivateOutputStream();
        outputStreamOpened = true;
        return new DataOutputStream(out);
    }

    /**
     * PrivateInputStream to handle chunking for HTTP/1.1.
     */
    class PrivateInputStream extends InputStream {
        int bytesleft;          // Number of bytes left in current chunk
        boolean chunked;        // true if Transfer-Encoding: chunked
        boolean eof;            // true if eof seen

        PrivateInputStream() throws IOException {
            bytesleft = 0;
            chunked = false;
            eof = false;
            // Determine if this is a chunked datatransfer and setup
            String te = (String)headerFields.get("transfer-encoding");
            if (te != null && te.equals("chunked")) {
                chunked = true;
                bytesleft = readChunkSize();
                eof = (bytesleft == 0);
            }
            if (te == null) {
                // CR 6211256
                // If there is no Transfer-Encoding header, a Content-Length
                // header may tell us how much to read. We will treat this
                // as a big "logical chunk" and set the number of bytes left
                // to the header's value. If we cannot parse the
                // value, throw an IOException as we would if we couldn't
                // read a chunk size; according to the spec, the User Agent
                // should notify the user that the value is bad. This CR
                // was filed against the Https procotol handler specifically,
                // but is included here because we should pay attention
                // to the content-length header, and to keep the two
                // PrivateInputStream classes in sync.
                String cl = (String)headerFields.get("content-length");
                if (cl != null) {
                    try {
                        // Parse the content-length. If it fails to parse or
                        // is < 0 it is invalid.
                        bytesleft = Integer.parseInt(cl);
                    } catch (NumberFormatException nfe) {
                        // Deliberately set bytesleft to a bogus value
                        bytesleft = -1;
                    } finally {
                        if (bytesleft < 0) {
                            throw new IOException( "Bad Content-Length value" );
                        }
                        eof = (bytesleft == 0);
                    }
                }
            }
        }

        /** 
         * Returns the number of bytes that can be read (or skipped over) 
         * from this input stream without blocking by the next caller of
         * a method for this input stream. 
         *
         * This method simply returns the number of bytes left from a 
         * chunked response from an HTTP 1.1 server, or the remainder
         * of the Content-Length value if the response is not chunked.
         */
        public int available()
            throws IOException {
             // DEBUG: System.out.println ("available " + bytesleft + " " + connected );

            if (connected) {
                if (bytesleft > 0) {
                    return bytesleft;
                } else {
                    return streamInput.available();
                }
            } else {
                throw new IOException("connection is not open");
            }
        }

        /**
         * Reads the next byte of data from the input stream. The value byte is
         * returned as an <code>int</code> in the range <code>0</code> to
         * <code>255</code>. If no byte is available because the end of the stream
         * has been reached, the value <code>-1</code> is returned. This method
         * blocks until input data is available, the end of the stream is detected,
         * or an exception is thrown.
         *
         * <p> A subclass must provide an implementation of this method.
         *
         * @return     the next byte of data, or <code>-1</code> if the end of the
         *             stream is reached.
         * @exception  IOException  if an I/O error occurs.
         */
        public int read() throws IOException {
            // Be consistent about returning EOF once encountered.
            if (eof) {
                return -1;
            }

            /* If all the current chunk has been read and this
             * is a chunked transfer then read the next chunk length.
             */      
            if (bytesleft <= 0 && chunked) {
                readCRLF();     // Skip trailing \r\n

                bytesleft = readChunkSize();
                if (bytesleft == 0) {
                    eof = true;
                    return -1;
                }
            }

            int ch = streamInput.read();
            bytesleft--;
            // CR 6211256
            // If we read an EOF, or if we are not chunked but
            // we've read all we expect to see (i.e., the Content-Length
            // header was set), then note that we've hit EOF.
            eof = (ch == -1) || (!chunked && bytesleft == 0);
            return ch;
        }

        /*
         * Reads up to <code>len</code> bytes of data from the input stream into
         * an array of bytes.  An attempt is made to read as many as
         * <code>len</code> bytes, but a smaller number may be read, possibly
         * zero. The number of bytes actually read is returned as an integer.
         *
         * This method allows direct consumer-supplier connection 
         * to avoid default byte-by-byte reading behaviour.
         */
        public int read(byte[] b, int off, int len) throws IOException {
            /* Need to check parameters here, because len may be changed
             * and streamInput.read() will not notice invalid argument.
             */
            if (b == null) {
                throw new NullPointerException();
            } else if ((off < 0) || (off > b.length) || (len < 0) ||
                   ((off + len) > b.length) || ((off + len) < 0)) {
                throw new IndexOutOfBoundsException();
            } else if (len == 0) {
                return 0;
            }

            // Be consistent about returning EOF once encountered.
            if (eof)
                return -1;

            if ((chunked) && (bytesleft <= 0)) {
                readCRLF();     // Skip trailing \r\n

                bytesleft = readChunkSize();
                if (bytesleft == 0) {
                    eof = true;
                    return -1;
                }
            }

            /* Don't read more than was specified as available .
             * len will remain > 0, because 
             *  if bytesleft is 0, than eof was also true.
             */
            if (len > bytesleft) {
                len = bytesleft;
            }

            int bytesRead = streamInput.read(b, off, len);
            if (bytesRead < 0) {
                eof = true;
            } else {
                bytesleft -= bytesRead;
                eof = (!chunked) && (bytesleft <= 0);
            }
            return bytesRead;
        }

        /* Read the chunk size from the input.
         * It is a hex length followed by optional headers (ignored).
         * and terminated with <cr><lf>.
         */
        private int readChunkSize() throws IOException {
            int size = -1;
            try {
                String chunk = readLine(streamInput);
                if (chunk == null) {
                    throw new IOException("No Chunk Size");
                }
                int i;
                for (i=0; i < chunk.length(); i++) {
                    char ch = chunk.charAt(i);
                    if (Character.digit(ch, 16) == -1)
                        break;
                }
                /* look at extensions?.... */
                size = Integer.parseInt(chunk.substring(0, i), 16);
            } catch (NumberFormatException e) {
                throw new IOException("Bogus chunk size");
            }

            return size;
        }

        /*
         * Read <cr><lf> from the InputStream.
         * @exception IOException is thrown if either <CR> or <LF>
         * is missing.
         */
        private void readCRLF() throws IOException { 
            int ch;
            ch = streamInput.read();
            if (ch != '\r') 
                throw new IOException("missing CRLF"); 
            ch = streamInput.read();
            if (ch != '\n')
                throw new IOException("missing CRLF");
        } 
        
        public void close() throws IOException {
            // DEBUG:  System.out.println ("close input stream " + opens + " " + connected );
            if (--opens == 0 && connected) disconnect();
        }
    }

    /**
     * Private OutputStream to allow the buffering of output
     * so the "Content-Length" header can be supplied.
     */
    class PrivateOutputStream extends OutputStream {
        private ByteArrayOutputStream output;

        public PrivateOutputStream() {
            output = new ByteArrayOutputStream();
        }

        public void write(int b) throws IOException {
            output.write(b);
            // CR 6216611: set the content-length. Note: we shouldn't set
            // content-length to the size of the current bytes that we are
            // writing. The length should be number of all valid bytes in the 
            // output buffer.
            reqProperties.put("Content-Length", "" + output.size());
        }

        /* Override this method from OutputStream class, this is either called
         * directly or by writeUTF(), to set the header field "content-length" to 
         * "len". CR 6216611 
         */
        public void write(byte b[], int off, int len) throws IOException {
            output.write(b, off, len);
            // Update Content-Length. Note: we should't set
            // content-length to the size of the current bytes that we are
            // writing. The length should be number of all valid bytes in the 
            // output buffer.
            reqProperties.put("Content-Length", "" + output.size());
        }

        public void write(byte[] b) throws IOException{
            // Create the headers
            String reqLine = method + " " + getFile()
                + (getRef() == null ? "" : "#" + getRef())
                + (getQuery() == null ? "" : "?" + getQuery())
                + " " + http_version + "\r\n";
            write((reqLine).getBytes(), 0, reqLine.length());
 
            // HTTP 1/1 requests require the Host header to
            // distinguish virtual host locations.
            reqProperties.put("Host" ,  host + ":" + port );
 
            Enumeration reqKeys = reqProperties.keys();
            while (reqKeys.hasMoreElements()) {
                String key = (String)reqKeys.nextElement();
                String reqPropLine = key + ": " + reqProperties.get(key) + 
                    "\r\n";
                write((reqPropLine).getBytes(), 0, reqPropLine.length());
            }
            write("\r\n".getBytes(), 0, "\r\n".length());
            write(b, 0, b.length);
            // Update Content-Length. Note: we should't set
            // content-length to the size of the current bytes that we are
            // writing. The length should be number of all valid bytes in the 
            // output buffer.
            reqProperties.put("Content-Length", "" + output.size());
        }


        public void flush() throws IOException {
            if (output.size() > 0) {
                connect();
            }
        }

        public byte[] toByteArray() {
            return output.toByteArray();
        }

        public int size() {
            return output.size();
        }

        public void close() throws IOException {
             // DEBUG: System.out.println ("close output stream" + opens + " " + connected );
            // CR 6216611: If the connection is already closed, just return
            if (opens == 0) return;
            flush();

            if (--opens == 0 && connected) disconnect();
            outputStreamOpened = false;
        }
    }

    protected void ensureOpen() throws IOException {
        if (opens == 0) throw new IOException("Connection closed");
    }

    public String getURL() {
        // RFC:  Add back protocol stripped by Content Connection.
        return protocol + ":" + url;
    }

    public String getProtocol() {
        return protocol;
    }

    public String getHost() {
        return  (host.length() == 0 ? null : host);
    }

    public String getFile() {
        return (file.length() == 0 ? null : file);
    }

    public String getRef() {
        return  (ref.length() == 0 ? null : ref);
    }

    public String getQuery() {
        return (query.length() == 0 ? null : query);
    }

    public int getPort() {
        return port;
    }

    public String getRequestMethod() {
        return method;
    }

    public void setRequestMethod(String method) throws IOException {
        ensureOpen();
        if (connected) throw new IOException("connection already open");

        if (!method.equals(HEAD) && !method.equals(GET) && !method.equals(POST)) {
            throw new IOException("unsupported method: " + method);
        }
        /* ignore the request if the outputstream is already open */
        if (outputStreamOpened) return;
        this.method = new String(method);
    }

    public String getRequestProperty(String key) {
        return (String)reqProperties.get(key);
    }

    public void setRequestProperty(String key, String value) throws IOException {
        ensureOpen();
        if (connected) throw new IOException("connection already open");
        if (outputStreamOpened) return;
        reqProperties.put(key, value);
    }

    public int getResponseCode() throws IOException {
        ensureOpen();
        connect();
        return responseCode;
    }

    public String getResponseMessage() throws IOException {
        ensureOpen();
        connect();
        return responseMsg;
    }

    public long getLength() {
        try {connect();}
        catch (IOException x) {return -1;}
        try {
             return getHeaderFieldInt("content-length", -1);
        } catch (IOException e) {
             return -1 ;
        }
    }

    public String getType() {
        try {connect();}
        catch (IOException x) {return null;}
        try {
             return getHeaderField("content-type");
        } catch (IOException e) { return null; }
    }

    public String getEncoding() {
        try {connect();}
        catch (IOException x) {return null;}
        try {
             return getHeaderField("content-encoding");
        } catch (Exception e) { return null; }
    }

    public long getExpiration() throws IOException {
        return getHeaderFieldDate("expires", 0);
    }

    public long getDate() throws IOException {
        return getHeaderFieldDate("date", 0);
    }

    public long getLastModified() throws IOException {
        return getHeaderFieldDate("last-modified", 0);
    }

    public String getHeaderField(String name) throws IOException {
        ensureOpen();
        try {connect();}
        catch (IOException x) {return null;}
        return (String)headerFields.get(toLowerCase(name));
    }
    
    public String getHeaderField(int index) throws IOException {
        ensureOpen();
        try {connect();}
        catch (IOException x) {return null;}

        if (headerFieldValues == null){
            makeHeaderFieldValues ();
        }

        if (index >= headerFieldValues.length)
            return null;

        return headerFieldValues[index];
    }

    public String getHeaderFieldKey(int index) throws IOException {
        ensureOpen();
        try {connect();}
        catch (IOException x) {return null;}

        if (headerFieldNames == null){
            makeHeaderFields ();
        }
        if (index >= headerFieldNames.length)
            return null;

        return headerFieldNames[index];
    }
    
    private void makeHeaderFields () {
        int i = 0;
        headerFieldNames = new String [ headerFields.size() ] ;
        for ( Enumeration e = headerFields.keys() ; e.hasMoreElements() ; 
              headerFieldNames[i++] = (String) e.nextElement()) ;
    }
    
    private void makeHeaderFieldValues () {
        int i = 0;
        headerFieldValues = new String [ headerFields.size() ] ;
        for ( Enumeration e = headerFields.keys() ; e.hasMoreElements() ; 
              headerFieldValues[i++] = (String) headerFields.get(e.nextElement()));
    }

    public int getHeaderFieldInt(String name, int def) throws IOException {
        ensureOpen();
        try {connect();}
        catch (IOException x) {return def;}
        try {
            return Integer.parseInt(getHeaderField(name));
        } catch(Throwable t) {}
        return def;
    }

    public long getHeaderFieldDate(String name, long def) throws IOException {
        ensureOpen();
        try {connect();}
        catch (IOException x) {return def;}

        try {
            return DateParser.parse(getHeaderField(name));
        } catch(Throwable t) {}

        return def;
    }


    protected StreamConnection connectSocket() throws IOException {
        
        // Check for illegal empty string for host
        if (host.equals("")) {
            throw new IllegalArgumentException("Host not recognized."+host);
        }

        // Open socket connection.
        HttpStreamConnection hsc = null;
        if (proxyHost == null) {
            hsc = new HttpStreamConnection(host, port);
        } else {
            hsc = new HttpStreamConnection(proxyHost, proxyPort);
        }
        return hsc;
    }
    
    protected void connectStream() throws IOException {

	if (streamConnection==null) {
	    streamConnection=connectSocket();        
        }

        streamOutput = streamConnection.openDataOutputStream();

        // HTTP 1.1 requests must contain content length for proxies
        if ((getRequestProperty("Content-Length") == null) ||
            (getRequestProperty("Content-Length").equals("0"))) {
            reqProperties.put("Content-Length",
                               "" + (out == null ? 0 : out.size()));
        }

        String reqLine ;

        if (proxyHost == null) {
            reqLine = method + " " + getFile()
                + (getRef() == null ? "" : "#" + getRef())
                + (getQuery() == null ? "" : "?" + getQuery())
                + " " + http_version + "\r\n";
        } else {
            reqLine = method + " "  
                + "http://" + host + ":" + port 
                + getFile()
                + (getRef() == null ? "" : "#" + getRef())
                + (getQuery() == null ? "" : "?" + getQuery())
                + " " + http_version + "\r\n";
        }

        // DEBUG:  System.out.print("Request: " + reqLine);
        // we should not write to the streamoutput as we are in set up state and not connected
        //streamOutput.write((reqLine).getBytes());

        requested = true;

    }

    protected void connect() throws IOException {
        if (connected) {
            return;
        }

        if (streamConnection==null) {
            streamConnection=connectSocket();        
        }

        streamOutput = streamConnection.openDataOutputStream();
        
        // HTTP 1.1 requests must contain content length for proxies
        if ((getRequestProperty("Content-Length") == null) ||
            (getRequestProperty("Content-Length").equals("0"))) {
            reqProperties.put("Content-Length",
                               "" + (out == null ? 0 : out.size()));
        }

        String reqLine ;
        
	if (proxyHost == null) {
            reqLine = method + " " + getFile()
                + (getRef() == null ? "" : "#" + getRef())
                + (getQuery() == null ? "" : "?" + getQuery())
                + " " + http_version + "\r\n";
        } else {
            reqLine = method + " "  
                + "http://" + host + ":" + port 
                + (getFile() == null ? "/" : getFile())
                + (getRef() == null ? "" : "#" + getRef())
                + (getQuery() == null ? "" : "?" + getQuery())
                + " " + http_version + "\r\n";
        }
        // DEBUG:  System.out.print("Request: " + reqLine);
        streamOutput.write((reqLine).getBytes());

        
        // HTTP 1/1 requests require the Host header to distinguish virtual host locations.
        reqProperties.put ("Host" ,  host + ":" + port );

        Enumeration reqKeys = reqProperties.keys();
        while (reqKeys.hasMoreElements()) {
            String key = (String)reqKeys.nextElement();
            String reqPropLine = key + ": " + reqProperties.get(key) + "\r\n";
            // DEBUG: System.out.print("Request: " + reqPropLine);
	    streamOutput.write((reqPropLine).getBytes());
        }
	streamOutput.write("\r\n".getBytes());

        if (out != null) {
	    streamOutput.write(out.toByteArray());
            //***Bug 4485901*** streamOutput.write("\r\n".getBytes());
            // DEBUG: System.out.println("Request: " + new String(out.toByteArray()));  
        }

        streamOutput.flush();

        streamInput = streamConnection.openDataInputStream();
        readResponseMessage(streamInput);
        readHeaders(streamInput);
        
        connected = true;

    }


    private void readResponseMessage(InputStream in) throws IOException {
        String line = readLine(in);
        int httpEnd, codeEnd;

        responseCode = -1;
        responseMsg = null;

        // DEBUG: System.out.println ("Response: " + line); 

 malformed: {
            if (line == null)
                break malformed;
            httpEnd = line.indexOf(' ');
            if (httpEnd < 0)
                break malformed;
    
            String httpVer = line.substring(0, httpEnd);
            if (!httpVer.startsWith("HTTP"))
                break malformed;
            if(line.length() <= httpEnd)
                break malformed;
    
            codeEnd = line.substring(httpEnd + 1).indexOf(' ');
            if (codeEnd < 0)
                break malformed;
            codeEnd += (httpEnd + 1);
            if(line.length() <= codeEnd) 
                break malformed;
    
            try {responseCode = Integer.parseInt(line.substring(httpEnd + 1, codeEnd));}
            catch (NumberFormatException nfe) {
                break malformed;
            }
    
            responseMsg = line.substring(codeEnd + 1);
            return;
        }  

        throw new InterruptedIOException("malformed response message");
    }

    private void readHeaders(InputStream in) throws IOException {
        String line, key, value;
        int index;

        for (;;) {
            line = readLine(in);
            // DEBUG: System.out.println ("Response: " + line);   

            if (line == null || line.equals(""))
                return;

            index = line.indexOf(':');
            if (index < 0)
                throw new IOException("malformed header field");

            key = line.substring(0, index);
            if (key.length() == 0)
                throw new IOException("malformed header field");

            if (line.length() <= index + 2) value = "";
            else value = line.substring(index + 2);
            headerFields.put(toLowerCase(key), value);
        }
    }

    /*
     * Uses the shared stringbuffer to read a line
     * terminated by <cr><lf> and return it as string.
     */
    private String readLine(InputStream in) {
        int c;
        stringbuffer.setLength(0);
        for (;;) {
            try {
                c = in.read();
                if (c < 0) {
                    return null;
                }
                if (c == '\r') {
                    continue;
                }
            } catch (IOException ioe) {
                return null;
            }
            if (c == '\n') {
                break;
            }
            stringbuffer.append((char)c);
        }

        return stringbuffer.toString();
    }

    protected void disconnect() throws IOException {

        if (streamConnection != null) {
            if (streamInput != null) {
                streamInput.close();
            }
            if (streamOutput != null) {
                streamOutput.close();
            }
            disconnectSocket();
            streamConnection = null;
        }

        responseCode = -1;
        responseMsg = null;
        connected = false;

    }

    protected void disconnectSocket() throws IOException {
        if (streamConnection != null) {
            streamConnection.close();
        }    
    }
    
    
    private String parseProtocol() throws IOException {
        int n = url.indexOf(':');
        if (n <= 0) throw new IOException("malformed URL");
        String token = url.substring(0, n);
        if (!token.equals("http")) {
            throw new IOException("protocol must be 'http'");
        }
        index = n + 1;
        return token;
    }

    private String parseHostname() throws IOException {
        String buf = url.substring(index);
        if (buf.startsWith("//")) {
            buf = buf.substring(2);
            index += 2;
        }
        int n = buf.indexOf(':');
        if (n < 0) n = buf.indexOf('/');
        if (n < 0) n = buf.length();

        int beginIndex = buf.indexOf("[");
        int endIndex = buf.indexOf("]");
        
        /* IPv6 addresses are enclosed within [] */
        if (beginIndex > endIndex) {
            throw new IllegalArgumentException("invalid host name " + buf);
        }
        if ((beginIndex ==0) && (endIndex >0)) {
            return parseIPv6Address(buf, endIndex);
        } else {
            /* parse IPv4 Address */
            String token = buf.substring(0, n);
            index += n;
            return token;
        }
    }

    private String parseIPv6Address(String address, int closing) {
        index += closing+1;
        /* beginning '[' and closing ']' should be included in the hostname*/
        return address.substring(0, closing+1);
    }

    private int parsePort(int defaultPort) throws IOException {
        int p = defaultPort;
        String buf = url.substring(index);
        if (!buf.startsWith(":")) return p;
        buf = buf.substring(1);
        index++;
        int n = buf.indexOf('/');
        if (n < 0) n = buf.indexOf('?');
        if (n < 0) n = buf.length();
        try { 
            p = Integer.parseInt(buf.substring(0, n));
            if (p <= 0) {
                throw new NumberFormatException();
            }
        }catch (NumberFormatException nfe) {
            throw new IllegalArgumentException("invalid port");
        }
        index += n;
        return p;
    }

    private String parseFile() throws IOException {
        String token = "";
        String buf = url.substring(index);
        if (buf.length() == 0) return token;
        if (!buf.startsWith("/") && !buf.startsWith("?")) {
            throw new IOException("invalid path");
        }
        int n = buf.indexOf('#');
        int m = buf.indexOf('?');
        if (n < 0 && m < 0){
            n = buf.length();    // Url does not contain any query or frag id.
        } else if (n < 0 || (m > 0 && m < n)){
            n = m ;              // Use query loc if no frag id is present
                                 // or if query comes before frag id.
                                 // otherwise just strip the frag id.
        }
        token = buf.substring(0, n);
        index += n;
        return token;
    }

    private String parseRef() throws IOException {
        String buf = url.substring(index);
        if (buf.length() == 0 || buf.charAt(0) == '?') return "";
        if (!buf.startsWith("#")) {
            throw new IOException("invalid ref");
        }
        int n = buf.indexOf('?');
        if (n < 0) n = buf.length();
        index += n;
        return buf.substring(1, n);
    }

    private String parseQuery() throws IOException {
        String buf = url.substring(index);
        if (buf.length() == 0) return "";
        if (buf.startsWith("?")) {
            String token = buf.substring(1);
            int n = buf.indexOf('#');
            if (n > 0) {
                token = buf.substring(1, n);
                index += n;
            }
            return token;
        }
        return "" ;
    }

    protected synchronized void parseURL() throws IOException {
        index = 0;
        host = parseHostname();
        if (protocol.equals("http")) {
            port = parsePort(80);
        } else {
            port = parsePort(443);
        }
        file = parseFile();
        query = parseQuery();
        ref = parseRef();
    }

    // The proxy value, if any, is specified as a host:port
    // string. Use the convenience routines to parse it.
    protected synchronized void parseProxy(String proxyVal) {
	if (proxyVal != null) {
	    index = 0;
	    try {
		proxyHost = parseHostname();
		proxyPort = parsePort(80);
	    } catch (IOException ioe) {
		// We cannot interpret the proxy.
	    }
	}
	return;
    }

    private String toLowerCase(String string) {
        // Uses the shared stringbuffer to create a lower case string.
        stringbuffer.setLength(0);
        for (int i=0; i<string.length(); i++) {
            stringbuffer.append(Character.toLowerCase(string.charAt(i)));
        }

        return stringbuffer.toString();
    }
}
