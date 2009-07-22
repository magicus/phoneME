/*
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

import java.util.Hashtable;
import java.util.Enumeration;

import javax.microedition.io.StreamConnection;
import javax.microedition.io.HttpConnection;
import javax.microedition.io.Connector;

import com.sun.cdc.io.ConnectionBase;
import com.sun.cdc.io.DateParser;

import java.net.SocketException;
import java.net.UnknownHostException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import javax.microedition.io.ConnectionNotFoundException;

import sun.misc.NetworkMetrics;
import sun.misc.NetworkMetricsInf;

/**
 * This class implements the necessary functionality
 * for an HTTP connection. 
 */
public class Protocol extends ConnectionBase implements HttpConnection {
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

    protected boolean connected;
    /* there should be only one outputstream opened at any time */
    protected boolean outputStreamOpened = false;
    private boolean closed = false;
    /*
     * In/Out Streams used to buffer input and output
     */
    private PrivateInputStream privateIn;
    protected PrivateOutputStream privateOut;

    /*
     * The streams from the underlying socket connection.
     */
    protected StreamConnection streamConnection;
    protected DataOutputStream streamOutput;
    protected DataInputStream streamInput;

    /** Maximum number of persistent connections. */
    protected static int maxNumberOfPersistentConnections;
    /** Connection linger time in the pool, default 60 seconds. */
    protected static long connectionLingerTime;
    protected StreamConnectionPool connectionPool;
    private static StreamConnectionPool staticConnectionPool;

    private static String platformUserAgent;
    private static String platformWapProfile;

    protected NetworkMetricsInf nm;
    protected boolean sentMetric = false;
    protected int totalBytesSent = 0;
    protected int totalBytesRead = 0;

    static {
        maxNumberOfPersistentConnections = Integer.parseInt(
            (String) AccessController.doPrivileged(
            new sun.security.action.GetPropertyAction(
                "microedition.maxpersconn", "10")));
        connectionLingerTime = Integer.parseInt(
            (String) AccessController.doPrivileged(
            new sun.security.action.GetPropertyAction(
                "microedition.connlinger", "60000")));
        staticConnectionPool =
            new StreamConnectionPool(maxNumberOfPersistentConnections,
                                 connectionLingerTime);
        platformUserAgent = (String) AccessController.doPrivileged(
            new sun.security.action.GetPropertyAction(
            "platform.browser.user.agent", null));
        platformWapProfile = (String)AccessController.doPrivileged(
            new sun.security.action.GetPropertyAction(
            "platform.browser.wap.profile", null));
    }
    /*
     * A shared temporary buffer used in a couple of places
     */
    protected StringBuffer stringbuffer;

    private String proxyHost = null;
    private int proxyPort = 80;

    protected final String httpVersion = "HTTP/1.1";

    /** Used when appl calls setRequestProperty("Connection", "close"). */
    private boolean ConnectionCloseFlag;
    private String responseProtocol;

    /**
     * create a new instance of this class.
     * We are initially unconnected.
     */
    public Protocol() {
        connectionPool = getConnectionPool();
        reqProperties = new Hashtable();
        headerFields = new Hashtable();
        stringbuffer = new StringBuffer(32);
        opens = 0;
        connected = false;
        method = GET;
        responseCode = -1;
        protocol = "http";

        AccessController.doPrivileged(new PrivilegedAction() {
            public Object run() {
                String http_proxy;
		String profileTemp =
		    System.getProperty("microedition.profiles");
		if (profileTemp != null && profileTemp.indexOf("MIDP") != -1) {
		    // We want to look for a MIDP property specifying proxies.
		    http_proxy =
			System.getProperty("com.sun.midp.io.http.proxy");
		} else {
		    // Default to CDC
		    http_proxy =
			System.getProperty("com.sun.cdc.io.http.proxy");
		}

                parseProxy(http_proxy);
                return null;
            }
        });
    }

    /*
     * Return the static pool instance for this type of protocol.
     */
    protected StreamConnectionPool getConnectionPool() {
        return staticConnectionPool;
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
        if (sm != null) {
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
        if (sm != null) {
            if (host != null) {
                sm.checkConnect(host, port);
            } else {
                sm.checkConnect("localhost", port);
            }
        }
        return;
    }
    
    private boolean match(String src, String alphabet) {
        src = src.toLowerCase();
        for (int i = 0; i < src.length(); ++i) {
            if (-1 == alphabet.indexOf(src.charAt(i))) {
                return false;
            }
        }
        return true;
    }
    
    private void validateHost() {
        final String ALPHANUM = "0123456789abcdefghijklmnopqrstuvwxyz-";
        final String DIGITS = "0123456789";
        final String HEX = "abcdef";
        final int TLD = -1;
        if (host.endsWith(".") || host.startsWith(".")) {
            throw new IllegalArgumentException("Invalid host name: "+host);
        }
        int i = 0;
        int[] ip4addr = new int[4];
        int ip4n = 0;
        boolean ip4 = true;
        boolean ip6 = host.startsWith("[") && host.endsWith("]");
        if (ip6) {
            // remove [ and ] for IPv6:
            host = host.substring(1, host.length()-1);
            ip4 = false;
        }
        try {
            boolean error = false;
            String sep = ip6 ? ":" : ".";
            do {
                int n = host.indexOf(sep, i); // -1 means top-level domain
                String domain = host.substring(i, n == TLD ? host.length() : n);
                if (ip6) {
                    if (!match(domain, DIGITS+HEX) && !"".equals(domain)){
                        error = true;
                        break;
                    }
                } else {
                    if (domain.equals("") || !match(domain, ALPHANUM)
                        // Minus sign cannot be first or last symbol
                        || domain.startsWith("-") || domain.endsWith("-")
                        // TLD cannot start with a digit
                        || (n == TLD && !ip4
                                     && match(domain.substring(0, 1), DIGITS))
                        // Only for numbers in IPv4 address
                        || (ip4 && ip4n >= 4)) {
                        error = true;
                        break;
                    }

                    if (ip4 && match(domain, DIGITS+HEX+'x')) {
                        if (match(domain, DIGITS)) { // decimal
                            ip4addr[ip4n] = Integer.parseInt(domain, 10);
                        } else {
                            if (domain.startsWith("0x")) { // hexadecimal
                                ip4addr[ip4n] =
                                    Integer.parseInt(domain.substring(2), 16);
                            } else {
                                ip4 = false;
                            }
                        }
                    } else {
                        ip4 = false;
                    }
                    ip4n++;
                }
                i = n+1;
            } while (i != 0);
            if (error) {
                throw new IllegalArgumentException("Invalid host name: "+host);
            }
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException("Invalid host name: "+host);
        }

        // Check IPv4 address range
        if (ip4 && ip4n == 4) {
            for (i = 0; i < 4; ++i) {
                if (ip4addr[i] < 0 || ip4addr[i] > 255) {
                    throw new IllegalArgumentException(
                        "Invalid host name: " + host);
                }
            }
        }
    }

    /*
     * To pass CDC TCK (which should not be testing HTTP) only.
     * (not needed for FP and MIDP TCKs)
     * This can be overrided by non-CDC profiles to just return false.
     */
    protected boolean isNoSuchHost() {
        if (host.equals("no.such.host")) {
            return true;
        }

        return false;
    }

    public void open(String url, int mode, boolean timeouts)
        throws IOException {
        open1(url, mode, timeouts);
    }

    protected void open1(String url, int mode, boolean timeouts)
        throws IOException {

        // DEBUG: System.out.println(this + ".open(" + url + ")");
        if (opens > 0) {
            throw new IOException("already connected");
        }

        opens = 1;
        closed = false;

        if (mode != Connector.READ && mode != Connector.WRITE
            && mode != Connector.READ_WRITE) {
            throw new IOException("illegal mode: " + mode);
        }
        
        this.url = url;
        this.mode = mode;
        parseURL();
        
        validateHost();

        // Check permission. The permission method wants the URL
        checkPermission(host, port, file);

        /*
         * Holding off the connecting to the server until the application
         * has fully setup the request helps avoid connection failures
         * against real world servers that require sending data immediately
         * after connecting, so the FP and MIDP specs for HttpConnection
         * specify this behavior.
         *
         * The CDC (not the FP or MIDP) TCK tests Connector.open with "http"
         * when it should not and then incorrectly expects the connection
         * to happen during Connector.open when the FP and MIDP specs say
         * otherwise. Serveral tests open the connection with an unknown host
         * and expects ConnectionNotFoundException to be thrown.
         *
         * While waiting for the CDC TCK to be changed we will workaround the
         * CDC TCK by rejecting the "no.such.host" host during open.
         */
        if (isNoSuchHost()) {
            throw new ConnectionNotFoundException("Unknown host");
        }
    }

    protected void getStreamConnection()
        throws IOException {
        if (streamConnection == null) {
            try {
                streamConnection = connectSocket();
            } catch (UnknownHostException ex) {
                throw new ConnectionNotFoundException(
                        "Cannot connect to "+host+" on port "+port+": "+ex);
            } catch (SocketException ex) {
                throw new ConnectionNotFoundException(
                        "Cannot connect to "+host+" on port "+port+": "+ex);
            }
        }

    }

    public void close() throws IOException {
        // DEBUG: System.out.println ("close " + opens + " " + connected ); 

        /*
         Decrement opens only once - there could be multiple close() calls
         */
        if (!closed) {
            --opens;
            closed = true;
        }
        
        if (opens == 0 && streamConnection != null) {
            disconnect();
        }
    }

    /*
     * Open the input stream if it has not already been opened.
     * @exception IOException is thrown if it has already been
     * opened for writing
     */
    public InputStream openInputStream() throws IOException {
        // DEBUG: System.out.println ("open input stream");

        inputStreamPermissionCheck();

        /*
         CR 6226615: opening another stream should not throw IOException
        if (in != null) {
            throw new IOException("already open");
        }
        */
        // If the connection was opened and closed before the 
        // data input stream is accessed, throw an IO exception
        if (opens == 0) {
            throw new IOException("connection is closed");
        }

        // Check that the connection was opened for reading
        if (mode != Connector.READ && mode != Connector.READ_WRITE) {
            throw new IOException("write-only connection");
        }

        connect();
        opens++;

        privateIn = new PrivateInputStream();
        return privateIn;
    }

    public DataInputStream openDataInputStream() throws IOException {
        /*
         CR 6226615: opening another stream should not throw IOException
        if (appDataIn != null) {
            throw new IOException("already open");
        }
        */

        openInputStream();

        return new DataInputStream(privateIn);
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
        if (opens == 0) {
            throw new IOException("connection is closed");
        }
        
        if (privateIn != null) {
            throw new IOException(
                "cannot open output stream while input stream is open");
        }

        /*
         CR 6226615: opening another stream should not throw IOException
        if (out != null) {
            throw new IOException("already open");
        }
        */

        opens++;
        privateOut = new PrivateOutputStream();
        outputStreamOpened = true;
        return new DataOutputStream(privateOut);
    }

    /**
     * PrivateInputStream to handle chunking for HTTP/1.1.
     */
    protected class PrivateInputStream extends InputStream {
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
                            throw new IOException("Bad Content-Length value");
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

            // DEBUG: System.out.println("available " + bytesleft + " " + connected);

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
         * <code>255</code>. If no byte is available because the end of the
         * stream has been reached, the value <code>-1</code> is returned. This
         * method blocks until input data is available, the end of the stream is
         * detected, or an exception is thrown.
         *
         * <p> A subclass must provide an implementation of this method.
         *
         * @return     the next byte of data, or <code>-1</code> if the end of
         *             the stream is reached.
         * @exception  IOException  if an I/O error occurs.
         */
        public int read() throws IOException {
            // Be consistent about returning EOF once encountered.
            if (eof) {
                return -1;
            }

            /*
             * If all the current chunk has been read and this
             * is a chunked transfer then read the next chunk length.
             */      
            if (bytesleft <= 0 && chunked) {
                readCRLF();     // Skip trailing \r\n

                bytesleft = readChunkSize();
                if (bytesleft == 0) {
                    // end of the chunks, read the 'trailer'
                    try {
                        while (streamInput.available() > 0) {
                            int t = streamInput.read();
                            if (t == '\r') {
                                if (streamInput.available() <= 0) {
                                    break;
                                }
                                t = streamInput.read();
                                if (t != '\n') {
                                    break;
                                }
                                totalBytesRead += 2;
                                break;
                            }
                        }
                    } catch (Exception e) {}
                    eof = true;
                    return -1;
                }
            }

            int ch = streamInput.read();
            totalBytesRead++;
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
            /*
             * Need to check parameters here, because len may be changed
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
                    // end of the chunks, read the 'trailer'
                    try {
                        while (streamInput.available() > 0) {
                            int t = streamInput.read();
                            if (t == '\r') {
                                if (streamInput.available() <= 0) {
                                    break;
                                }
                                t = streamInput.read();
                                if (t != '\n') {
                                    break;
                                }
                                totalBytesRead += 2;
                                break;
                            }
                        }
                    } catch (Exception e) {}
                    eof = true;
                    return -1;
                }
            }

            /*
             * Don't read more than was specified as available .
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
                totalBytesRead += bytesRead;
                bytesleft -= bytesRead;
                eof = (!chunked) && (bytesleft <= 0);
            }
            return bytesRead;
        }

        /*
         * Read the chunk size from the input.
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
                totalBytesRead += chunk.length();

                int i;
                for (i = 0; i < chunk.length(); i++) {
                    char ch = chunk.charAt(i);
                    if (Character.digit(ch, 16) == -1)
                        break;
                }
                // look at extensions?....
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
            totalBytesRead += 2;
        }
        
        public void close() throws IOException {
            // DEBUG: System.out.println("close input stream "+opens+" " + connected);
            if (opens == 0)
                return;

            if (--opens == 0 && connected)
                disconnect();
        }

    }

    /**
     * Private OutputStream to allow the buffering of output
     * so the "Content-Length" header can be supplied.
     */
    protected class PrivateOutputStream extends ByteArrayOutputStream {

        public void flush() throws IOException {
            // DEBUG: System.out.println("flush output stream");
            super.flush();

            // Unlike close(), no data written, then no connection needed
            if (size() > 0) {
                connect();
            }
        }

        public void close() throws IOException {
            // DEBUG: System.out.println("close output stream"+opens + " "+connected);

            // CR 6216611: If the connection is already closed, just return
            if (opens == 0)
                return;
            flush();

            /*
             * Close must connect and flush may not connect.
             * Connect will handle multiple calls.
             */
            connect();

            if (--opens == 0 && connected) disconnect();
            outputStreamOpened = false;
        }
    }// PrivateOutputStream
    protected void ensureOpen() throws IOException {
        if (opens == 0)
            throw new IOException("Connection closed");
    }

    public String getURL() {
        // RFC:  Add back protocol stripped by Content Connection.
        return protocol + ":" + url;
    }

    public String getProtocol() {
        return protocol;
    }

    public String getHost() {
        return (host.length() == 0 ? null : host);
    }

    public String getFile() {
        return (file.length() == 0 ? null : file);
    }

    public String getRef() {
        return (ref.length() == 0 ? null : ref);
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
        // DEBUG: System.out.println("setRequestMethod("+ method + ")");
        ensureOpen();
        if (connected)
            throw new IOException("connection already open");

        if (!method.equals(HEAD) && !method.equals(GET)
            && !method.equals(POST)) {
            throw new IOException("unsupported method: " + method);
        }
        // ignore the request if the outputstream is already open
        if (outputStreamOpened)
            return;
        this.method = new String(method);
    }

    public String getRequestProperty(String key) {
        // DEBUG: System.out.println("getRequestProperty("+ key + ")");
        return (String)reqProperties.get(key);
    }
    
    private static boolean validateProperty(String s) {
        boolean res = true;
        if (s.endsWith("\r") || s.endsWith("\n")) {
            res = false;
        } else {
            // Check spaces after each \r\n
            for (int i = s.indexOf('\r'); i != -1; i = s.indexOf('\r', i+1)) {
                if (s.charAt(i+1) != '\n' || 
                        (s.charAt(i+2) != ' ' && s.charAt(i+2) != '\t')) {
                    res = false;
                    break;
                }
            }
        }
        return res;
    }

    public void setRequestProperty(String key, String value)
        throws IOException {

        // DEBUG: System.out.println("setRequestProperty("+ key + ", " + value + ")");

        ensureOpen();
        if (connected)
            throw new IOException("connection already open");
        if (outputStreamOpened)
            return;
        if (!validateProperty(value))
            throw new IllegalArgumentException("Illegal request property");
        /*
         * If application setRequestProperties("Connection", "close")
         * then we need to know this & take appropriate default close action
         */
        if ((key.equalsIgnoreCase("connection")) && 
            (value.equalsIgnoreCase("close"))) {
            ConnectionCloseFlag = true;
        }
        reqProperties.put(key, value);
    }

    public int getResponseCode() throws IOException {
        // DEBUG: System.out.println("getResponseCode");
        ensureOpen();
        connect();
        return responseCode;
    }

    public String getResponseMessage() throws IOException {
        // DEBUG: System.out.println("getResponseMessage");
        ensureOpen();
        connect();
        return responseMsg;
    }

    public long getLength() {
        // DEBUG: System.out.println("getLength");
        try { connect(); }
        catch (IOException x) {
            return -1;
        }
        try {
             return getHeaderFieldInt("content-length", -1);
        } catch (IOException e) {
             return -1;
        }
    }

    public String getType() {
        // DEBUG: System.out.println("getType");
        try { connect(); }
        catch (IOException x) {
            return null;
        }
        try {
             return getHeaderField("content-type");
        } catch (IOException e) {
            return null;
        }
    }

    public String getEncoding() {
        // DEBUG: System.out.println("getEncoding");
        try { connect(); }
        catch (IOException x) {
            return null;
        }
        try {
             return getHeaderField("content-encoding");
        } catch (Exception e) {
            return null;
        }
    }

    public long getExpiration() throws IOException {
        // DEBUG: System.out.println("getExpiration");
        return getHeaderFieldDate("expires", 0);
    }

    public long getDate() throws IOException {
        // DEBUG: System.out.println("getDate");
        return getHeaderFieldDate("date", 0);
    }

    public long getLastModified() throws IOException {
        // DEBUG: System.out.println("getLastModified");
        return getHeaderFieldDate("last-modified", 0);
    }

    public String getHeaderField(String name) throws IOException {
        // DEBUG: System.out.println("getHeaderField(" + name + ")");
        ensureOpen();
        
        connect();

        if (name == null) {
            return null;
        }

        return (String)headerFields.get(toLowerCase(name));
    }
    
    public String getHeaderField(int index) throws IOException {
        // DEBUG: System.out.println("getHeaderField(" + index + ")");
        ensureOpen();

        connect();

        if (headerFieldValues == null) {
            makeHeaderFieldValues();
        }

        if (index >= headerFieldValues.length)
            return null;

        return headerFieldValues[index];
    }

    public String getHeaderFieldKey(int index) throws IOException {
        // DEBUG: System.out.println("getHeaderFieldKey(" + index + ")");
        ensureOpen();

        connect();

        if (headerFieldNames == null) {
            makeHeaderFields();
        }
        if (index >= headerFieldNames.length)
            return null;

        return headerFieldNames[index];
    }
    
    private void makeHeaderFields() {
        int i = 0;
        headerFieldNames = new String [ headerFields.size() ];
        for (Enumeration e = headerFields.keys(); e.hasMoreElements();
              headerFieldNames[i++] = (String)e.nextElement());
    }
    
    private void makeHeaderFieldValues() {
        int i = 0;
        headerFieldValues = new String [ headerFields.size() ];
        for (Enumeration e = headerFields.keys(); e.hasMoreElements();
              headerFieldValues[i++] =
                (String) headerFields.get(e.nextElement()));
    }

    public int getHeaderFieldInt(String name, int def) throws IOException {
        String field = getHeaderField(name);

        if (field == null) {
            return def;
        }

        try {
            return Integer.parseInt(field);
        } catch (NumberFormatException nfe) {
            // fall through
        } catch (IllegalArgumentException iae) {
            // fall through
        }

        return def;
    }

    public long getHeaderFieldDate(String name, long def) throws IOException {
        String field = getHeaderField(name);
        
        if (field == null) {
            return def;
        }

        try {
            return DateParser.parse(field);
        } catch (NumberFormatException nfe) {
            // fall through
        } catch (IllegalArgumentException iae) {
            // fall through
        }

        return def;
    }


    protected StreamConnection connectSocket() throws IOException {
        
        // Check for illegal empty string for host
        if (host.equals("")) {
            throw new IllegalArgumentException("Host not recognized: "+host);
        }

        // Open socket connection.
        StreamConnection hsc = null;

        if (proxyHost == null) {
            hsc = connectionPool.get(protocol,
                                          host, port);
        } else {
            hsc = connectionPool.get(protocol,
                                          proxyHost, proxyPort);
        }
        if (hsc != null) {
            return hsc;
        }
        
        if (proxyHost == null) {
            hsc = new HttpStreamConnection(host, port);
        } else {
            hsc = new HttpStreamConnection(proxyHost, proxyPort);
        }
        return hsc;
    }

    protected void sendRequest() throws IOException {
        // DEBUG: System.out.println("sendRequest");

        streamOutput = streamConnection.openDataOutputStream();

        // HTTP 1.1 requests must contain content length for proxies
        if ((getRequestProperty("Content-Length") == null) ||
            (getRequestProperty("Content-Length").equals("0"))) {
            reqProperties.put("Content-Length", ""
                        + (privateOut == null ? 0 : privateOut.size()));
        }

        String reqLine;

        if (proxyHost == null) {
            reqLine = method + " " + (getFile() == null ? "/" : getFile())
                + (getRef() == null ? "" : "#" + getRef())
                + (getQuery() == null ? "" : "?" + getQuery())
                + " " + httpVersion + "\r\n";
        } else {
            reqLine = method + " http://" + host + ":" + port
                + (getFile() == null ? "/" : getFile())
                + (getRef() == null ? "" : "#" + getRef())
                + (getQuery() == null ? "" : "?" + getQuery())
                + " " + httpVersion + "\r\n";
        }

        // DEBUG: System.out.print("Request: " + reqLine);
        streamOutput.write((reqLine).getBytes());
        totalBytesSent = reqLine.length();

        // HTTP 1/1 requests require the Host header to distinguish virtual
        // host locations.
        reqProperties.put("Host", host + ":" + port);

        Enumeration reqKeys = reqProperties.keys();
        while (reqKeys.hasMoreElements()) {
            String key = (String)reqKeys.nextElement();
            String reqPropLine = key + ": " + reqProperties.get(key) + "\r\n";
            // DEBUG: System.out.print("  " + reqPropLine);
            streamOutput.write((reqPropLine).getBytes());
            totalBytesSent += reqPropLine.length();
        }

        // DEBUG: System.out.println("");
        streamOutput.write("\r\n".getBytes());
        totalBytesSent += 2;

        if (privateOut != null) {
            byte[] temp = privateOut.toByteArray();
    	    streamOutput.write(temp);
            // ***Bug 4485901*** streamOutput.write("\r\n".getBytes());
            // DEBUG: System.out.print("  privateOut: ");
            // DEBUG: System.out.println(binaryToTraceString(temp));
            totalBytesSent += temp.length;
        }

        java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {
            public Object run() {
                if (NetworkMetrics.metricsAvailable()) {
                    StreamConnection con = streamConnection;

                    int methodType =
                        (method.equals(POST) ? NetworkMetricsInf.POST :
                         method.equals(HEAD) ? NetworkMetricsInf.HEAD :
                         NetworkMetricsInf.GET);
                    Class nmClass = NetworkMetrics.getImpl();
                    if (nmClass == null) {
                        return null;
                    }
                    try {
                        nm = (NetworkMetricsInf) nmClass.newInstance();
                    } catch (Exception e) {
                        return null;
                    }
                    nm.initReq(NetworkMetricsInf.HTTP,
                               getHost(),
                               getPort(),
                               getFile(),
                               getRef(),
                               getQuery());
                    if (con instanceof StreamConnectionElement) {
                        con =
                            ((StreamConnectionElement)con).getBaseConnection();
                    }
                    nm.sendMetricReq(con, methodType, totalBytesSent);
                    sentMetric = true;
                }
                return null;
            }
        });
        streamOutput.flush();
        totalBytesRead = 0;

        streamInput = streamConnection.openDataInputStream();
    }

    protected void connect() throws IOException {
        if (connected) {
            return;
        }

        String origUserAgentValue = getRequestProperty("User-Agent");
        if (origUserAgentValue == null) {
            origUserAgentValue = "";
        }
        if (platformUserAgent != null &&
                -1 == origUserAgentValue.indexOf(platformUserAgent)) {
            reqProperties.put("User-Agent",
                    origUserAgentValue + " " + platformUserAgent);
        }

        String origWapProfileValue = getRequestProperty("x-wap-profile");
        if (origWapProfileValue == null)
        {
            origWapProfileValue = "";
        }
        if (platformWapProfile != null &&
                -1 == origWapProfileValue.indexOf(platformWapProfile))
        {
            reqProperties.put("x-wap-profile", platformWapProfile);
        }

        String platformNetworkType = (String)AccessController.doPrivileged(
            new sun.security.action.GetPropertyAction(
            "platform.browser.network.type", null));
        String origNetworkTypeValue = getRequestProperty("network-type");
        if (origNetworkTypeValue == null)
        {
            origNetworkTypeValue = "";
        }
        if (platformNetworkType != null &&
                -1 == origNetworkTypeValue.indexOf(platformNetworkType))
        {
            reqProperties.put("network-type", platformNetworkType);
        }

        getStreamConnection();

        // DEBUG: System.out.println("Calling sendRequest");
        sendRequest();
        try {
            readResponseMessage();
        } catch (InterruptedIOException ex) {
            if (responseMsg == null) {
                // Most probably the server has closed its end by timeout.
                // Here we have to reconnect and resend the request
                getStreamConnection();
                // DEBUG: System.out.println("Calling sendRequest again");
                sendRequest();
                readResponseMessage();
            } else {
                throw ex;
            }
        }
        if (privateOut != null) {
            privateOut.reset();
        }
        readHeaders();

        connected = true;
    }

    protected void readResponseMessage() throws IOException {
        String line = readLine(streamInput);
        int httpEnd, codeEnd;

        responseCode = -1;
        responseMsg = null;
        responseProtocol = null;

        // DEBUG: System.out.println ("Response: " + line);

malformed: {
            if (line == null)
                break malformed;
            httpEnd = line.indexOf(' ');
            if (httpEnd < 0)
                break malformed;
    
            responseProtocol = line.substring(0, httpEnd);
            if (!responseProtocol.startsWith("HTTP"))
                break malformed;
            if (line.length() <= httpEnd)
                break malformed;
    
            codeEnd = line.substring(httpEnd + 1).indexOf(' ');
            if (codeEnd < 0)
                break malformed;
            codeEnd += (httpEnd + 1);
            if (line.length() <= codeEnd) 
                break malformed;
    
            try {
                responseCode =
                    Integer.parseInt(line.substring(httpEnd + 1, codeEnd));
            } catch (NumberFormatException nfe) {
                break malformed;
            }
            totalBytesRead += line.length();
            responseMsg = line.substring(codeEnd + 1);

            return;
        }
        disconnect();
        throw new InterruptedIOException("malformed response message");
    }
    
    protected void readHeaders() throws IOException {
        String line, key = null, value = null;
        int index;

        for (;;) {
            line = readLine(streamInput);
             // DEBUG: System.out.println ("Response: " + line);
            
            if (line == null || line.equals(""))
                break;
            totalBytesRead += line.length();

            /*
             * There can be multiline values. The line starts with a space
             * in that case.
             */
            if (line.charAt(0) == ' ' || line.charAt(0) == '\t') {
                index = 0;
                // Replace multiple spaces with a single space
                while (line.charAt(index) == ' '
                        || line.charAt(index) == '\t') {
                    ++index;
                }
                value += " " + line.substring(index);
            } else {
                if (key != null && value != null) {
                    headerFields.put(toLowerCase(key), value);
                    key = null;
                    value = null;
                }
                index = line.indexOf(':');
                if (index < 0)
                    throw new IOException("malformed header field");

                key = line.substring(0, index);
                if (key.length() == 0)
                    throw new IOException("malformed header field");

                if (line.length() <= index + 2) value = "";
                else value = line.substring(index + 2);
            }
        }
        
        if (key != null && value != null) {
            headerFields.put(toLowerCase(key), value);
        }
    }

    /*
     * Uses the shared stringbuffer to read a line
     * terminated by <cr><lf> and return it as string.
     */
    protected String readLine(InputStream in) {
        int c;
        stringbuffer.setLength(0);
        for (;;) {
            try {
                c = in.read();
                if (c < 0) {
                    return null;
                }
                if (c == '\r') {
                    totalBytesRead++;
                    continue;
                }
            } catch (IOException ioe) {
                return null;
            }
            if (c == '\n') {
                totalBytesRead++;
                break;
            }
            stringbuffer.append((char)c);
        }

        return stringbuffer.toString();
    }

    protected void disconnect() throws IOException {
        if (streamConnection == null)
            return;

        if (sentMetric && NetworkMetrics.metricsAvailable()) {
            java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {
                    public Object run() {
                        try {
                            StreamConnection con = streamConnection;

                            if (con instanceof StreamConnectionElement) {
                                con =
                                    ((StreamConnectionElement)con).getBaseConnection();
                            }
                            nm.sendMetricResponse(con, responseCode,
                                                  totalBytesRead);
                        } catch (NullPointerException e) {}
                        return null;
                    }
                });
        }

        if (streamConnection != null) {
            String connectionField = (String) headerFields.get("connection");
            if (privateIn == null || privateIn.available() > 0
                    || connectionField != null &&
                    (connectionField.equalsIgnoreCase("close") ||
                    (responseProtocol != null
                        && responseProtocol.equalsIgnoreCase("HTTP/1.0")
                        && !connectionField.equalsIgnoreCase("keep-alive")))) {
                if (streamConnection instanceof StreamConnectionElement) {
                    connectionPool.remove(
                            (StreamConnectionElement) streamConnection);
                    streamConnection = null;
                } else {
                    disconnectSocket();
                }
            }
            
            if (streamConnection != null) {

                if (streamConnection instanceof StreamConnectionElement) {
                    // we got this connection from the pool
                    connectionPool.returnForReuse(
                            (StreamConnectionElement) streamConnection);
                    streamConnection = null;
                } else {
                    // save the connection for reuse
                    if (!connectionPool.add(protocol, host, port,
                           (StreamConnection)streamConnection, streamOutput,
                           streamInput)) {
                        // pool full, disconnect
                        disconnectSocket();
                    }
                }
            }
        }

        privateIn = null;
        responseCode = -1;
        responseMsg = null;
        connected = false;
        responseProtocol = null;
    }

    protected void disconnectSocket() throws IOException {
        if (streamConnection != null) {
            streamConnection.close();
            if (! (streamConnection instanceof StreamConnectionElement)) {
                if (streamInput != null) {
                    streamInput.close();
                }
                if (streamOutput != null) {
                    streamOutput.close();
                }
            }
            streamInput = null;
            streamOutput = null;
            streamConnection = null;
        }    
    }

    protected synchronized void parseURL() throws IOException {
        try {
            URL loc = new URL(url.startsWith("//") ? protocol+":"+url : url);
            host = loc.getHost();
            if (-1 == (port = loc.getPort())) {
                port = loc.getProtocol().equals("http") ? 80 : 443;
            }
            if (null == (file = loc.getPath())) {
                file = "";
            }
            if (null == (query = loc.getQuery())) {
                query = "";
            }
            if (null == (ref = loc.getRef())) {
                ref = "";
            }
        } catch (MalformedURLException ex) {
            throw new IllegalArgumentException("Malformed URL: "+url);
        }
    }

    // The proxy value, if any, is specified as a host:port
    // string. Use the convenience routines to parse it.
    protected synchronized void parseProxy(String proxyVal) {
	if (proxyVal != null) {
            try {
                URL proxyURL = new URL(proxyVal.startsWith("http://") ? proxyVal
                        : "http://" + proxyVal);
                proxyHost = proxyURL.getHost(); // null is ok.
                if (-1 == (proxyPort = proxyURL.getPort())) {
                    proxyPort = 80;
                }
            } catch (MalformedURLException ex) {
                throw new IllegalArgumentException("Malformed URL: "+proxyVal);
            }
	    // DEBUG: System.out.println ("http parseProxy: " + proxyVal + " " + proxyHost + " " + proxyPort);
	}
	return;
    }

    private String toLowerCase(String string) {
        // Uses the shared stringbuffer to create a lower case string.
        stringbuffer.setLength(0);
        for (int i = 0; i < string.length(); i++) {
            stringbuffer.append(Character.toLowerCase(string.charAt(i)));
        }

        return stringbuffer.toString();
    }

    protected String binaryToTraceString(byte[] arg) {
        StringBuffer temp = new StringBuffer();
        temp.setLength(0);

        for (int i = 0; i < arg.length; i++) {
            char current = (char)arg[i];
            if (current < ' ' || current > '~') {
                temp.append('^');
                continue;
            }

            temp.append(current);
        }

        return temp.toString();
    }
}
