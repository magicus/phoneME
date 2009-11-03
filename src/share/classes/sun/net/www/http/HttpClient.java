/*
 * @(#)HttpClient.java	1.124 06/10/10
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
 *
 */

package sun.net.www.http;

import java.io.*;
import java.net.*;
import java.util.*;
import sun.net.NetworkClient;
import sun.net.ProgressEntry;
import sun.net.ProgressData;
import sun.net.www.MessageHeader;
import sun.net.www.HeaderParser;
import sun.net.www.MeteredStream;
import sun.misc.RegexpPool;

import java.security.*;
/**
 * @version 1.115, 08/30/01
 * @author Herb Jellinek
 * @author Dave Brown
 */
public class HttpClient extends NetworkClient {
    // whether this httpclient comes from the cache
    protected boolean cachedHttpClient = false;

    private boolean inCache;

    // Http requests we send
    MessageHeader requests;

    // Http data we send with the headers
    PosterOutputStream poster = null;

    // if we've had one io error
    boolean failedOnce = false;

    /** regexp pool of hosts for which we should connect directly, not Proxy
     *  these are intialized from a property.
     */
    private static RegexpPool nonProxyHostsPool = null;

    /** The string source of nonProxyHostsPool
     */
    private static String nonProxyHostsSource = null;

    /** Response code for CONTINUE */
    private static final int	HTTP_CONTINUE = 100;

    /** Default port number for http daemons. TODO: make these private */
    static final int	httpPortNumber = 80;

    /** return default port number (subclasses may override) */
    protected int getDefaultPort () { return httpPortNumber; }

    /* The following three data members are left in for binary */
    /* backwards-compatibility.  Unfortunately, HotJava sets them directly */
    /* when it wants to change the settings.  The new design has us not */
    /* cache these, so this is unnecessary, but eliminating the data members */
    /* would break HJB 1.1 under JDK 1.2. */
    /* */
    /* These data members are not used, and their values are meaningless. */
    /* TODO:  Take them out for JDK 2.0! */
    /**
     * @deprecated
     */
    public static String proxyHost = null;
    /**
     * @deprecated
     */
    public static int proxyPort = 80;

    /* instance-specific proxy fields override the static fields if set.
     * Used by FTP.  These are set to the true proxy host/port if
     * usingProxy is true.
     */
    private String instProxy = null;
    private int instProxyPort = -1;

    /* All proxying (generic as well as instance-specific) may be
     * disabled through use of this flag
     */
    protected boolean proxyDisabled;

    // are we using proxy in this instance?
    public boolean usingProxy = false;
    // target host, port for the URL
    protected String host;
    protected int port;

    /* where we cache currently open, persistent connections */
    protected static KeepAliveCache kac = new KeepAliveCache();

    private static boolean keepAliveProp = true;

    volatile boolean keepingAlive = false;     /* this is a keep-alive connection */
    int keepAliveConnections = -1;    /* number of keep-alives left */

    /**Idle timeout value, in milliseconds. Zero means infinity,
     * iff keepingAlive=true.
     * Unfortunately, we can't always believe this one.  If I'm connected
     * through a Netscape proxy to a server that sent me a keep-alive
     * time of 15 sec, the proxy unilaterally terminates my connection
     * after 5 sec.  So we have to hard code our effective timeout to
     * 4 sec for the case where we're using a proxy. *SIGH*
     */
    int keepAliveTimeout = 0;

    /** Url being fetched. */
    protected URL	url;
    
    /* if set, the client will be reused and must not be put in cache */
    public boolean reuse = false; 

    int totalBytesRead;

    /**
     * A NOP method kept for backwards binary compatibility
     * @deprecated -- system properties are no longer cached.
     */
    public static synchronized void resetProperties() {
    }

    int getKeepAliveTimeout() {
	return keepAliveTimeout;
    }
    
    /**
     * @return the proxy host to use, as defined by system properties.
     */
    private String getProxyHost() {
	String host = (String) java.security.AccessController.doPrivileged(
                new sun.security.action.GetPropertyAction("http.proxyHost"));
	if (host == null) {
	    /* maintain compatibility with 1.0.2, before
	     * the properties namespace was so polluted
	     * and these properties were called, respectively,
	     * "proxyHost" "proxyPort".  Hopefully there won't
	     * be any conflicts.
	     */
	    host = (String) java.security.AccessController.doPrivileged(
                    new sun.security.action.GetPropertyAction("proxyHost"));
	}
	if (host != null && host.length() == 0) {
	    /* System.getProp() will give us an empty String, ""
	     * for a defined but "empty" property.
	     */
	    host = null;
	}
	return host;
    }

    /**
     * @return the proxy port to use, as defined by system properties
     */
    private int getProxyPort() {
	final int port[] = {0};
	java.security.AccessController.doPrivileged(
	    new java.security.PrivilegedAction() {
	    public Object run() {
		if (System.getProperty("http.proxyHost") != null) {
		    port[0] =
			Integer.getInteger("http.proxyPort", 80).intValue();
		} else {
		    /* maintain compatibility with 1.0.2, before
		     * the properties namespace was so polluted
		     * and these properties were called, respectively,
		     * "proxyHost" "proxyPort".  Hopefully there won't
		     * be any conflicts.
		     */
		    port[0] = Integer.getInteger("proxyPort", 80).intValue();
		}
		return null;
	    }
	});
	return port[0];
    }

    static {
	String keepAlive = 
	    (String) java.security.AccessController.doPrivileged(
			 new java.security.PrivilegedAction() {
		         public Object run() {
			     return System.getProperty("http.keepAlive");
			 }
	    });
	if (keepAlive != null) {
	    keepAliveProp = Boolean.valueOf(keepAlive).booleanValue(); 
	} else {
	    keepAliveProp = true; 
	}
    }

    /**
     * @return true iff http keep alive is set (i.e. enabled).  Defaults
     *		to true if the system property http.keepAlive isn't set.
     */
    public boolean getHttpKeepAliveSet() {
	return keepAliveProp;
    }

    /** 
     * @return true if host matches a host specified via
     * the http.nonProxyHosts property
     */
    private boolean matchNonProxyHosts(String host) {
	synchronized (getClass()) {
	    String rawList = (String) java.security.AccessController.doPrivileged(
                    new sun.security.action.GetPropertyAction("http.nonProxyHosts"));
	    if (rawList == null) {
	        nonProxyHostsPool = null;
	    } else {
		if (!rawList.equals(nonProxyHostsSource)) {
	            RegexpPool pool = new RegexpPool();
	            StringTokenizer st = new StringTokenizer(rawList, "|", false);
                    try {
                        while (st.hasMoreTokens()) {
                            pool.add(st.nextToken().toLowerCase(), Boolean.TRUE);
                        }
                    } catch (sun.misc.REException ex) {
                        System.err.println("Error in http.nonProxyHosts system property:  " + ex);
		    }
		    nonProxyHostsPool = pool;
                }
	    }
	    nonProxyHostsSource = rawList;
        }

	/*
	 * Match against non-proxy hosts
	 */
	if (nonProxyHostsPool == null) {
	    return false;
	}
	if (nonProxyHostsPool.match(host) != null) {
	    return true;
	} else {
	    return false;
	}
    }

    protected HttpClient() {
    }

    private HttpClient(URL url)
    throws IOException {
	this(url, (String)null, -1, false);
    }

    protected HttpClient(URL url,
			 boolean proxyDisabled) throws IOException {
	this(url, null, -1, proxyDisabled);
    }

    /* This package-only CTOR should only be used for FTP piggy-backed on HTTP
     * HTTP URL's that use this won't take advantage of keep-alive.
     * Additionally, this constructor may be used as a last resort when the
     * first HttpClient gotten through New() failed (probably b/c of a
     * Keep-Alive mismatch).
     *
     * NOTE: That documentation is wrong ... it's not package-private any more
     */
    public HttpClient(URL url, String proxy, int proxyPort)
    throws IOException {
	this(url, proxy, proxyPort, false);
    }

    /*
     * This constructor gives "ultimate" flexibility, including the ability
     * to bypass implicit proxying.  Sometimes we need to be using tunneling
     * (transport or network level) instead of proxying (application level),
     * for example when we don't want the application level data to become
     * visible to third parties.
     *
     * @param url		the URL to which we're connecting
     * @param proxy		proxy to use for this URL (e.g. forwarding)
     * @param proxyPort		proxy port to use for this URL
     * @param proxyDisabled	true to disable default proxying
     */
    private HttpClient(URL url, String proxy, int proxyPort,
		       boolean proxyDisabled)
	throws IOException {

	this.proxyDisabled = proxyDisabled;
	if (!proxyDisabled) {
	    this.instProxy = proxy;
	    this.instProxyPort = (proxyPort < 0)
		? getDefaultPort()
		: proxyPort;
	}
	/* try to set host to "%d.%d.%d.%d" string if
	 * visible - customer bug - brown */
	try {
	    InetAddress addr = InetAddress.getByName(url.getHost());
	    this.host = addr.getHostAddress();
	} catch (UnknownHostException ignored) {
	    this.host = url.getHost();
	}
	this.url = url;
	port = url.getPort();
	if (port == -1) {
	    port = getDefaultPort();
	}
	openServer();
    }

    /* This class has no public constructor for HTTP.  This method is used to
     * get an HttpClient to the specifed URL.  If there's currently an
     * active HttpClient to that server/port, you'll get that one.
     */
    public static HttpClient New(URL url)
    throws IOException {
	return HttpClient.New(url, true);
    }

    public static HttpClient New(URL url, boolean useCache)
	throws IOException {
	return HttpClient.New(url, (String)null, -1, useCache);
    }

    public static HttpClient New(URL url, String proxy, int proxyPort,
				 boolean useCache)
	throws IOException {
	HttpClient ret = null;
	if (useCache) {
	    /* see if one's already around */
	    ret = (HttpClient) kac.get(url, null);
	    if (ret != null) {
		synchronized (ret) {
		    ret.cachedHttpClient = true;
		    // Uncomment this line if assertions are always
		    //    turned on for libraries
		    //assert ret.inCache;
	    	    ret.inCache = false;
		}
	    }
	}
	if (ret == null) {
	    ret = new HttpClient(url, proxy, proxyPort);
	} else {
	    SecurityManager security = System.getSecurityManager();
	    if (security != null) {
		security.checkConnect(url.getHost(), url.getPort());
	    }
	    ret.url = url;
	}
	
	return ret;
    }

    /* return it to the cache as still usable, if:
     * 1) It's keeping alive, AND
     * 2) It still has some connections left, AND
     * 3) It hasn't had a error (PrintStream.checkError())
     * 4) It hasn't timed out
     *
     * If this client is not keepingAlive, it should have been
     * removed from the cache in the parseHeaders() method.
     */

    public void finished() {
	if (reuse) /* will be reused */
	    return;
	keepAliveConnections--;
	if (keepAliveConnections > 0 && isKeepingAlive() &&
	       !(serverOutput.checkError())) {
	    /* This connection is keepingAlive && still valid.
	     * Return it to the cache.
	     */
	    putInKeepAliveCache();
	} else {
	    closeServer();
	}
    }

    protected synchronized void putInKeepAliveCache() {
	if (inCache) {
	    // Uncomment this line if assertions are always
	    //    turned on for libraries
	    //assert false : "Duplicate put to keep alive cache";
	    return;
	}
	inCache = true;
	kac.put(url, null, this);
    }

    /*
     * Close an idle connection to this URL (if it exists in the
     * cache).
     */
    public void closeIdleConnection() {
	HttpClient http = (HttpClient) kac.get(url, null);
	if (http != null) {
	    http.closeServer();
	}
    }

    /* We're very particular here about what our InputStream to the server
     * looks like for reasons that are apparent if you can decipher the
     * method parseHTTP().  That's why this method is overidden from the
     * superclass.
     */
    public void openServer(String server, int port) throws IOException {
	serverSocket = doConnect(server, port);
	try {
 	    serverOutput = new PrintStream(
 	        new BufferedOutputStream(serverSocket.getOutputStream()), 
 					 false, encoding);
 	} catch (UnsupportedEncodingException e) {
 	    throw new InternalError(encoding+" encoding not found");
 	}
	serverSocket.setTcpNoDelay(true);
    }

    /*
     * Returns true if the http request should be tunneled through proxy.
     * An example where this is the case is Https.
     */
    public boolean needsTunneling() {
	return false;
    }

    /*
     * Returns true if this httpclient is from cache
     */
    public boolean isCachedConnection() {
	return cachedHttpClient;
    }

    /*
     * Finish any work left after the socket connection is
     * established.  In the normal http case, it's a NO-OP. Subclass
     * may need to override this. An example is Https, where for
     * direct connection to the origin server, ssl handshake needs to
     * be done; for proxy tunneling, the socket needs to be converted
     * into an SSL socket before ssl handshake can take place.
     */
    public void afterConnect() throws IOException, UnknownHostException {
	// NO-OP. Needs to be overwritten by HttpsClient
    }

    /*
     * call openServer in a privileged block
     */
    private synchronized void privilegedOpenServer(final String proxyHost,
						   final int proxyPort)
	 throws IOException
    {
	try {
	    java.security.AccessController.doPrivileged(
		new java.security.PrivilegedExceptionAction() {
		public Object run() throws IOException {
		    openServer(proxyHost, proxyPort);
		    return null;
		}
	    });
	} catch (java.security.PrivilegedActionException pae) {
	    throw (IOException) pae.getException();
	}
    }

    /*
     * call super.openServer
     */
    private void superOpenServer(final String proxyHost,
				 final int proxyPort)
	throws IOException, UnknownHostException
    {
	super.openServer(proxyHost, proxyPort);
    } 
 
    /*
     * call super.openServer in a privileged block
     */
    private synchronized void privilegedSuperOpenServer(final String proxyHost,
						        final int proxyPort)
	throws IOException
    {
	try {
	    java.security.AccessController.doPrivileged(
		new java.security.PrivilegedExceptionAction() {
		public Object run() throws IOException
		{
		    superOpenServer(proxyHost, proxyPort);
		    return null;
		}
	    });
	} catch (java.security.PrivilegedActionException pae) {
	    throw (IOException) pae.getException();
	}
    }

    /**
     * Determines if the given host address is a loopback address or
     * not. Any IP address starting with 127 is a loopback address
     * although typically this is set to 127.0.0.1. A request for
     * the address "localhost" will also be treated as loopback.
     */
    private boolean isLoopback(String host) {

	if (host == null || host.length() == 0)
	    return false;

	if (host.equalsIgnoreCase("localhost"))
	    return true;

        if (!Character.isDigit(host.charAt(0))) {
	    return false;
	} else {
	    /* The string (probably) represents a numerical IP address.
	     * Parse it into int's. Compare the first int
	     * component to 127 and return if it is not
	     * 127. Otherwise, just make sure the rest of the
	     * address is a valid address.
	     */

	    boolean firstInt = true;

	    int hitDots = 0;
	    char[] data = host.toCharArray();

	    for(int i = 0; i < data.length; i++) {
		char c = data[i];
		if (c < 48 || c > 57) { // !digit
		    return false;
		}
		int b = 0x00; // the value of one integer component
		while(c != '.') {
		    if (c < 48 || c > 57) { // !digit
			return false;
		    }
		    b = b*10 + c - '0';

		    if (++i >= data.length)
			break;
		    c = data[i];
		}
		if(b > 0xFF) { /* invalid - bigger than a byte */
		    return false;
		}


		/* 
		 * If the first integer component is not 127 we may
		 * as well stop.
		 */
		if (firstInt) {
		    firstInt = false;
		    if (b != 0x7F)
			return false;
		}
		
		hitDots++;
	    }
	    
	    if(hitDots != 4 || host.endsWith(".")) {
		return false;
	    }
	}

	return true;
    }

    /*
     */
    protected synchronized void openServer() throws IOException {

	SecurityManager security = System.getSecurityManager();
	if (security != null) {
	    security.checkConnect(host, port);
	}

	if (keepingAlive) { // already opened
	    return;
	}

	/* Try to open connections in this order, return on the
	 * first one that's successful:
	 * 1) if (instProxy != null)
	 *        connect to instProxy
	 *        _raise_exception_ if failure
	 *        usingProxy=true
	 * 2) if (proxyHost != null) and !(proxyDisabled || host is on dontProxy list)
	 *        connect to proxyHost;
	 *        usingProxy=true;
	 * 3) connect locally
	 *    usingProxy = false;
	 */

	String urlHost = url.getHost().toLowerCase();
	boolean loopback = isLoopback(urlHost);

	if (url.getProtocol().equals("http") ||
	    url.getProtocol().equals("https") ) {
	    
	    if ((instProxy != null) && !loopback) {
		privilegedOpenServer(instProxy, instProxyPort);
		usingProxy = true;
		return;
	    }

	    String proxyHost = getProxyHost();
	    if ((proxyHost != null) &&
		(!(proxyDisabled ||
		   loopback ||
		   matchNonProxyHosts(urlHost) ||
		   matchNonProxyHosts(host)))) {
		try {
		    int proxyPort = getProxyPort();
		    privilegedOpenServer(proxyHost, proxyPort);
		    instProxy = proxyHost;
		    instProxyPort = proxyPort;
		    usingProxy = true;
		    return;
		} catch (IOException e) {
		    // continue in this case - try locally
		}
	    }
	    // try locally - let Exception get raised
	    openServer(host, port);
	    usingProxy = false;
	    return;

	} else {
	    /* we're opening some other kind of url, most likely an
	     * ftp url. In this case:
	     * 1) try instProxy (== FTP proxy)
	     * 2) try HttpProxy
	     * 3) directly
	     */
	    if ((instProxy != null) && !loopback) {
		privilegedSuperOpenServer(instProxy, instProxyPort);
		usingProxy = true;
		return;
	    }
		
	    String proxyHost = getProxyHost();
	    if ((proxyHost != null) &&
		(!(proxyDisabled ||
		   loopback ||
		   matchNonProxyHosts(urlHost) ||
		   matchNonProxyHosts(host)))) {
		try {
		    int proxyPort = getProxyPort();
		    privilegedSuperOpenServer(proxyHost, proxyPort);
		    instProxy = proxyHost;
		    instProxyPort = proxyPort;
		    usingProxy = true; 
		    return;
		} catch (IOException e) {
		    // continue in this case - try locally
		}
	    }
	    // try locally
	    super.openServer(host, port);
	    usingProxy = false;
	    return;
	}
    }

    public String getURLFile() throws IOException {

	String fileName = url.getFile();
	if ((fileName == null) || (fileName.length() == 0))
	    fileName = "/";

	if (usingProxy) {
	    fileName = url.toExternalForm();
	}
	if (fileName.indexOf('\n') == -1)
	    return fileName;
	else
	    throw new java.net.MalformedURLException("Illegal character in URL");
    }

    /**
     * @deprecated 
     */

    public void writeRequests(MessageHeader head) {
	requests = head;
	requests.print(serverOutput);
	serverOutput.flush();
    }

    public int writeRequests(MessageHeader head, 
			      PosterOutputStream pos) throws IOException {
        int bytesWritten;
	requests = head;
	bytesWritten = requests.print(serverOutput);
	poster = pos;
	if (poster != null) {
	    poster.writeTo(serverOutput);
            bytesWritten += poster.size();
        }
	serverOutput.flush();
        return bytesWritten;
    }

    /** Parse the first line of the HTTP request.  It usually looks
	something like: "HTTP/1.0 <number> comment\r\n". */

    public boolean parseHTTP(MessageHeader responses, ProgressEntry pe)
    throws IOException {
	/* If "HTTP/*" is found in the beginning, return true.  Let
         * HttpURLConnection parse the mime header itself.
	 *
	 * If this isn't valid HTTP, then we don't try to parse a header
	 * out of the beginning of the response into the responses,
	 * and instead just queue up the output stream to it's very beginning.
	 * This seems most reasonable, and is what the NN browser does.
	 */

	try {
	    serverInput = serverSocket.getInputStream();
	    serverInput = new HttpClientInputStream(serverInput);
            return (parseHTTPHeader(responses, pe));
	} catch (IOException e) {
	    closeServer();
            if (!failedOnce && requests != null) {
                // try once more
		failedOnce = true;
		openServer();
		writeRequests(requests, poster);
		return parseHTTP(responses, pe);
	    } else {
		throw e;
	    }
	}

    }

    public int setTimeout (int timeout) throws SocketException {
	int old = serverSocket.getSoTimeout ();
	serverSocket.setSoTimeout (timeout);
	return old;
    }

    private boolean parseHTTPHeader(MessageHeader responses, ProgressEntry pe)
    throws IOException {
	/* If "HTTP/*" is found in the beginning, return true.  Let
         * HttpURLConnection parse the mime header itself.
	 *
	 * If this isn't valid HTTP, then we don't try to parse a header
	 * out of the beginning of the response into the responses,
	 * and instead just queue up the output stream to it's very beginning.
	 * This seems most reasonable, and is what the NN browser does.
	 */

	keepAliveConnections = -1;
	keepAliveTimeout = 0;

	boolean ret = false;
	byte[] b = new byte[8];

	try {
	    int nread = 0;
	    serverInput.mark(10);
	    while (nread < 8) {
		int r = serverInput.read(b, nread, 8 - nread);
		if (r < 0) {
		    break;
		}
		nread += r;
	    }
	    String keep=null;
	    ret = b[0] == 'H' && b[1] == 'T'
		    && b[2] == 'T' && b[3] == 'P' && b[4] == '/' &&
		b[5] == '1' && b[6] == '.';
	    serverInput.reset();
	    if (ret) { // is valid HTTP - response started w/ "HTTP/1."
		responses.parseHeader(serverInput);
		/* decide if we're keeping alive:
		 * Note:  There's a spec, but most current
		 * servers (10/1/96) that support this differ in dialects.
		 * If the server/client misunderstand each other, the
		 * protocol should fall back onto HTTP/1.0, no keep-alive.
		 */
		if (usingProxy) { // not likely a proxy will return this
		    keep = responses.findValue("Proxy-Connection");
		}
		if (keep == null) {
		    keep = responses.findValue("Connection");
		}
		if (keep != null && keep.toLowerCase().equals("keep-alive")) {
		    /* some servers, notably Apache1.1, send something like:
		     * "Keep-Alive: timeout=15, max=1" which we should respect.
		     */
		    HeaderParser p = new HeaderParser(
			    responses.findValue("Keep-Alive"));
		    if (p != null) {
			/* default should be larger in case of proxy */
			keepAliveConnections = p.findInt("max", usingProxy?50:5);
			keepAliveTimeout = p.findInt("timeout", usingProxy?60:5);
		    }
		} else if (b[7] != '0') {
		    /*
		     * We're talking 1.1 or later. Keep persistent until
		     * the server says to close.
		     */
		    if (keep != null) {
			/*
			 * The only Connection token we understand is close.
			 * Paranoia: if there is any Connection header then
			 * treat as non-persistent.
			 */
		        keepAliveConnections = 1;
		    } else {
		        keepAliveConnections = 5;
		    }
		}
	    } else if (nread != 8) {
                if (!failedOnce && requests != null) {
		    failedOnce = true;
		    closeServer();
		    openServer();
		    writeRequests(requests, poster);
		    return parseHTTP(responses, pe);
		}
		throw new SocketException("Unexpected end of file from server");
	    } else {
		// we can't vouche for what this is....
		responses.set("Content-type", "unknown/unknown");
	    }
	} catch (IOException e) {
	    throw e;
	}

	int code = -1;
	try {
	    String resp;
	    resp = responses.getValue(0);
            /* should have no leading/trailing LWS
             * expedite the typical case by assuming it has
             * form "HTTP/1.x <WS> 2XX <mumble>"
             */
            int ind;
            ind = resp.indexOf(' ');
            while(resp.charAt(ind) == ' ')
                ind++;
            code = Integer.parseInt(resp.substring(ind, ind + 3));
	} catch (Exception e) {}

	if (code == HTTP_CONTINUE) {
	    responses.reset();
	    return parseHTTPHeader(responses, pe);
	}

   	int cl = -1;

	/*
	 * Set things up to parse the entity body of the reply.
	 * We should be smarter about avoid pointless work when
	 * the HTTP method and response code indicate there will be
	 * no entity body to parse.
	 */
	String te = null;
	try {
	    te = responses.findValue("Transfer-Encoding");
	} catch (Exception e) {}
	if (te != null && te.equalsIgnoreCase("chunked")) {
	    serverInput = new ChunkedInputStream(serverInput, this, responses);

	    /*
  	     * If keep alive not specified then close after the stream
	     * has completed.
	     */
	    if (keepAliveConnections < 0) {
		keepAliveConnections = 1; 
	    }
	    keepingAlive = true;
	    failedOnce = false;
	} else {

	    /* 
	     * If it's a keep alive connection then we will keep 
	     * (alive if :-
	     * 1. content-length is specified, or 
	     * 2. "Not-Modified" or "No-Content" responses - RFC 2616 states that  
	     *    204 or 304 response must not include a message body. 
	     */
	    try {
	        cl = Integer.parseInt(responses.findValue("content-length"));
	    } catch (Exception e) {}

	    if (keepAliveConnections > 1 && 
		(cl >= 0 ||
		 code == HttpURLConnection.HTTP_NOT_MODIFIED || 
		 code == HttpURLConnection.HTTP_NO_CONTENT)) { 
	        keepingAlive = true;
	    } else if (keepingAlive) {
	        /* Previously we were keeping alive, and now we're not.  Remove
	         * this from the cache (but only here, once) - otherwise we get
	         * multiple removes and the cache count gets messed up.
	         */
	        keepingAlive=false;
	    }
	}

	/* finally wrap a KeepAlive/MeteredStream around it if appropriate */
	if (cl > 0) {
	    pe.setType(url.getFile(), responses.findValue("content-type"));
	    pe.update(0, cl);
	    if (isKeepingAlive()) {
		serverInput = new KeepAliveStream(serverInput, pe, this);
		failedOnce = false;
	    } else {
		serverInput = new MeteredStream(serverInput,pe);
	    }
	} else {
	    ProgressData.pdata.unregister(pe);
	}
	return ret;
    }

    public synchronized InputStream getInputStream() {
	return serverInput;
    }

    public OutputStream getOutputStream() {
	return serverOutput;
    }

    public String toString() {
	return getClass().getName()+"("+url+")";
    }

    public final boolean isKeepingAlive() {
	return getHttpKeepAliveSet() && keepingAlive;
    }

    protected void finalize() throws Throwable {
	// This should do nothing.  The stream finalizer will
	// close the fd.
    }

    /* Use only on connections in error. */
    public void closeServer() {
	try {
	    keepingAlive = false;
	    serverSocket.close();
	} catch (Exception e) {}
    }

    /**
     * @return the proxy host being used for this client, or null
     *		if we're not going through a proxy
     */
    public String getProxyHostUsed() {
	if (!usingProxy) {
	    return null;
	} else {
	    return instProxy;
	}
    }

    /**
     * @return the proxy port being used for this client.  Meaningless
     *		if getProxyHostUsed() gives null.
     */
    public int getProxyPortUsed() {
	return instProxyPort;
    }

    public Socket getServerSocket() {
        return serverSocket;
    }

    public int getBytesRead() {
        return totalBytesRead;
    }

    class HttpClientInputStream extends BufferedInputStream {

        public HttpClientInputStream (InputStream is) {
	    super (is);
            totalBytesRead = 0;
        }

        public int read() throws IOException {
            int ret = super.read();
            totalBytesRead ++;
            return ret;
        }

        public synchronized void mark(int readlimit) {
            super.mark(readlimit);
            markpos = totalBytesRead;
        }

        public synchronized void reset() throws IOException {
            super.reset();
            totalBytesRead = markpos;
        }

        public int read(byte b[], int off, int len) throws IOException {
            int ret = super.read(b, off, len);
            totalBytesRead += ret;
            return ret;
        }
    }
}
