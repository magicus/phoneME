 /*
 * @(#)HttpURLConnection.java	1.86 06/10/10
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

package sun.net.www.protocol.http;

import java.net.URL;
import java.net.URLConnection;
import java.net.ProtocolException;
import java.net.PasswordAuthentication;
import java.net.Authenticator;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.net.SocketTimeoutException;
import java.io.*;
import java.util.Date;
import java.util.Map;
import java.util.Locale;
import java.util.StringTokenizer;
import sun.net.*;
import sun.net.www.*;
import sun.net.www.http.HttpClient;
import sun.net.www.http.PosterOutputStream;
import sun.net.www.http.ChunkedInputStream;
import java.text.SimpleDateFormat;
import java.util.TimeZone;
import java.net.MalformedURLException;
import sun.misc.NetworkMetrics;
import sun.misc.NetworkMetricsInf;

/**
 * A class to represent an HTTP connection to a remote object.
 */


public class HttpURLConnection extends java.net.HttpURLConnection {
    
    static final String version;
    public static final String userAgent;

    /* max # of allowed re-directs */
    static final int defaultmaxRedirects = 20;
    static final int maxRedirects;

    /* Not all servers support the (Proxy)-Authentication-Info headers.
     * By default, we don't require them to be sent
     */
    static final boolean validateProxy;
    static final boolean validateServer;

    static {
	maxRedirects = ((Integer)java.security.AccessController.doPrivileged(
		new sun.security.action.GetIntegerAction("http.maxRedirects", 
		defaultmaxRedirects))).intValue();
	version = (String) java.security.AccessController.doPrivileged(
                    new sun.security.action.GetPropertyAction("java.version"));
	String agent = (String) java.security.AccessController.doPrivileged(
		    new sun.security.action.GetPropertyAction("http.agent"));
	if (agent == null) {
	    agent = "Java/"+version;
	} else {
	    agent = agent + " Java/"+version;
	}
	userAgent = agent;
	validateProxy = ((Boolean)java.security.AccessController.doPrivileged(
		new sun.security.action.GetBooleanAction(
		    "http.auth.digest.validateProxy"))).booleanValue();
	validateServer = ((Boolean)java.security.AccessController.doPrivileged(
		new sun.security.action.GetBooleanAction(
		    "http.auth.digest.validateServer"))).booleanValue();
    }

    static final String httpVersion = "HTTP/1.1";
    static final String acceptString =
        "text/html, image/gif, image/jpeg, *; q=.2, */*; q=.2";

    // the following http request headers should NOT have their values
    // returned for security reasons.
    private static final String[] EXCLUDE_HEADERS = {
	    "Proxy-Authorization", 
	    "Authorization"
    };
    protected HttpClient http;
    protected Handler handler;


    /* output stream to server */
    protected PrintStream ps = null;

    /* We only have a single static authenticator for now.
     * For backwards compatibility with JDK 1.1.  Should be
     * eliminated for JDK 2.0.
     */
    private static HttpAuthenticator defaultAuth;
    
    /* all the headers we send 
     * NOTE: do *NOT* dump out the content of 'requests' in the 
     * output or stacktrace since it may contain security-sensitive 
     * headers such as those defined in EXCLUDE_HEADERS.
     */
    private MessageHeader requests;

    /* The following two fields are only used with Digest Authentication */
    String domain; 	/* The list of authentication domains */
    DigestAuthentication.Parameters digestparams;

    /* Current credentials in use */
    AuthenticationInfo  currentProxyCredentials = null;
    AuthenticationInfo  currentServerCredentials = null;
    boolean		needToCheck = true;
    private boolean doingNTLM2ndStage = false; /* doing the 2nd stage of an NTLM server authentication */
    private boolean doingNTLMp2ndStage = false; /* doing the 2nd stage of an NTLM proxy authentication */
    Object authObj; 

    /* Progress entry */
    protected ProgressEntry pe;

    /* all the response headers we get back */
    private MessageHeader responses;
    /* the stream _from_ the server */
    private InputStream inputStream = null;
    /* post stream _to_ the server, if any */
    private PosterOutputStream poster = null;

    /* Indicates if the std. request headers have been set in requests. */
    private boolean setRequests=false;

    /* Indicates whether a request has already failed or not */
    private boolean failedOnce=false;

    /* Remembered Exception, we will throw it again if somebody
       calls getInputStream after disconnect */
    private Exception rememberedException = null;

    /* If we decide we want to reuse a client, we put it here */
    private HttpClient reuseClient = null;

    protected int netMetricCode = 0;
    protected NetworkMetricsInf nm;
    private boolean sentMetric = false;

    /*
     * privileged request password authentication 
     *
     */
    private static PasswordAuthentication 
    privilegedRequestPasswordAuthentication(
					    final String host,
					    final InetAddress addr,
					    final int port,
					    final String protocol,
					    final String prompt,
					    final String scheme) {
	return (PasswordAuthentication)
	    java.security.AccessController.doPrivileged( 
		new java.security.PrivilegedAction() {
		public Object run() {
		    return Authenticator.requestPasswordAuthentication(
				       host, addr, port, protocol, prompt, scheme);
		}
	    });
    }

    /* 
     * checks the validity of http message header and throws 
     * IllegalArgumentException if invalid.
     */
    private void checkMessageHeader(String key, String value) {
	char LF = '\n';
	int index = key.indexOf(LF);
	if (index != -1) {
	    throw new IllegalArgumentException(
		"Illegal character(s) in message header field: " + key);
	}
	else {
	    if (value == null) {
                return;
            }

	    index = value.indexOf(LF);
	    while (index != -1) {
		index++;
		if (index < value.length()) {
		    char c = value.charAt(index);
		    if ((c==' ') || (c=='\t')) {
			// ok, check the next occurrence
		        index = value.indexOf(LF, index);
			continue;
		    }
		}
		throw new IllegalArgumentException(
		    "Illegal character(s) in message header value: " + value);
	    }
	}
    }

    /* adds the standard key/val pairs to reqests if necessary & write to
     * given PrintStream
     */
    private void writeRequests() throws IOException {

	/* print all message headers in the MessageHeader 
	 * onto the wire - all the ones we've set and any
	 * others that have been set
	 */
	if (!setRequests) {

	    /* We're very particular about the order in which we
	     * set the request headers here.  The order should not
	     * matter, but some careless CGI programs have been
	     * written to expect a very particular order of the
	     * standard headers.  To name names, the order in which
	     * Navigator3.0 sends them.  In particular, we make *sure*
	     * to send Content-type: <> and Content-length:<> second
	     * to last and last, respectively, in the case of a POST
	     * request.
	     */
	    if (!failedOnce)
		requests.prepend(method + " " + http.getURLFile()+" "  + 
				 httpVersion, null);
	    if (!getUseCaches()) {
		requests.setIfNotSet ("Cache-Control", "no-cache");
		requests.setIfNotSet ("Pragma", "no-cache");
	    }
	    requests.setIfNotSet("User-Agent", userAgent);
	    int port = url.getPort();
	    String host = url.getHost();
	    if (port != -1 && port != 80) {
		host += ":" + String.valueOf(port);
	    }
	    requests.setIfNotSet("Host", host);
	    requests.setIfNotSet("Accept", acceptString);

	    /*
	     * For HTTP/1.1 the default behavior is to keep connections alive.
	     * However, we may be talking to a 1.0 server so we should set
	     * keep-alive just in case, except if we have encountered an error
	     * or if keep alive is disabled via a system property
	     */
	     
	    // Try keep-alive only on first attempt
	    if (!failedOnce && http.getHttpKeepAliveSet()) {
		if (http.usingProxy) {
		    requests.setIfNotSet("Proxy-Connection", "keep-alive");
		} else {
		    requests.setIfNotSet("Connection", "keep-alive");
		}
	    } 
	    // send any pre-emptive authentication
	    if (http.usingProxy) {
		setPreemptiveProxyAuthentication(requests);
	    }
            // Set modified since if necessary
            long modTime = getIfModifiedSince();
            if (modTime != 0 ) {
                Date date = new Date(modTime);
		//use the preferred date format according to RFC 2068(HTTP1.1),
		// RFC 822 and RFC 1123
		SimpleDateFormat fo =
		  new SimpleDateFormat ("EEE, dd MMM yyyy HH:mm:ss 'GMT'", Locale.US);
		fo.setTimeZone(TimeZone.getTimeZone("GMT"));
                requests.setIfNotSet("If-Modified-Since", fo.format(date));
            }
	    // check for preemptive authorization
	    AuthenticationInfo sauth = AuthenticationInfo.getServerAuth(url);
	    if (sauth != null && sauth.supportsPreemptiveAuthorization() ) {
		// Sets "Authorization"
		requests.setIfNotSet(sauth.getHeaderName(), sauth.getHeaderValue(url,method));
		currentServerCredentials = sauth;
	    }

	    if (poster != null) {
		/* add Content-Length & POST/PUT data */
		synchronized (poster) {
		    /* close it, so no more data can be added */
		    poster.close();
		    if (!method.equals("PUT")) {
			String type = "application/x-www-form-urlencoded";
			requests.setIfNotSet("Content-Type", type);
		    }
		    requests.set("Content-Length", 
				 String.valueOf(poster.size()));
		}
	    }
	    setRequests=true;
	}
	final int bytesWritten = http.writeRequests(requests, poster);
        java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {
            public Object run() {
                if (NetworkMetrics.metricsAvailable()) {
                    int methodType =
                        (method.equals("POST") ? NetworkMetricsInf.POST :
                         method.equals("HEAD") ? NetworkMetricsInf.HEAD :
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
                    nm.initReq(NetworkMetricsInf.HTTP, getURL());
                    nm.sendMetricReq(http.getServerSocket(), methodType,
                                     bytesWritten);
                    sentMetric = true;
                }
                return null;
            }
        });

	if (ps.checkError()) {
	    String proxyHost = http.getProxyHostUsed();
	    int proxyPort = http.getProxyPortUsed();
	    disconnectInternal();
	    if (failedOnce) {
		throw new IOException("Error writing to server");
	    } else { // try once more
		failedOnce=true;
		if (proxyHost != null) {
		    setProxiedClient(url, proxyHost, proxyPort);
		} else {
		    setNewClient (url);
		}
		ps = (PrintStream) http.getOutputStream();
		connected=true;
		responses = new MessageHeader();
		setRequests=false;
		writeRequests();
	    }
	}
    }


    /**
     * Create a new HttpClient object, bypassing the cache of
     * HTTP client objects/connections.
     *
     * @param url	the URL being accessed
     */
    protected void setNewClient (URL url)
    throws IOException {
	setNewClient(url, false);
    }

    /**
     * Obtain a HttpsClient object. Use the cached copy if specified. 
     *
     * @param url       the URL being accessed
     * @param useCache  whether the cached connection should be used
     *        if present
     */
    protected void setNewClient (URL url, boolean useCache)
	throws IOException {
	http = HttpClient.New(url, useCache);
    }


    /**
     * Create a new HttpClient object, set up so that it uses
     * per-instance proxying to the given HTTP proxy.  This
     * bypasses the cache of HTTP client objects/connections.
     *
     * @param url	the URL being accessed
     * @param proxyHost	the proxy host to use
     * @param proxyPort	the proxy port to use
     */
    protected void setProxiedClient (URL url, String proxyHost, int proxyPort)
    throws IOException {
	setProxiedClient(url, proxyHost, proxyPort, false); 
    }

    /**
     * Obtain a HttpClient object, set up so that it uses per-instance
     * proxying to the given HTTP proxy. Use the cached copy of HTTP
     * client objects/connections if specified.
     *
     * @param url       the URL being accessed
     * @param proxyHost the proxy host to use
     * @param proxyPort the proxy port to use
     * @param useCache  whether the cached connection should be used
     *        if present
     */
    protected void setProxiedClient (URL url,
					   String proxyHost, int proxyPort,
					   boolean useCache)
	throws IOException {
	proxiedConnect(url, proxyHost, proxyPort, useCache);
    }

    protected void proxiedConnect(URL url,
					   String proxyHost, int proxyPort,
					   boolean useCache)
	throws IOException {
	SecurityManager security = System.getSecurityManager(); 
	if (security != null) { 
	    security.checkConnect(proxyHost, proxyPort); 
	}
	http = HttpClient.New (url, proxyHost, proxyPort, useCache);
    }

    protected HttpURLConnection(URL u, Handler handler)
    throws IOException {
	super(u);
	requests = new MessageHeader();
	responses = new MessageHeader();
	this.handler = handler;
    }
    
    /** this constructor is used by other protocol handlers such as ftp
        that want to use http to fetch urls on their behalf. */
    public HttpURLConnection(URL u, String host, int port)
    throws IOException {
	this(u, new Handler(host, port));
    }

    /** 
     * @deprecated.  Use java.net.Authenticator.setDefault() instead.
     */
    public static void setDefaultAuthenticator(HttpAuthenticator a) {
	defaultAuth = a;
    }

    /**
     * opens a stream allowing redirects only to the same host.
     */
    public static InputStream openConnectionCheckRedirects(URLConnection c)
	throws IOException
    {
        boolean redir;
        int redirects = 0;
        InputStream in = null;

        do {
            if (c instanceof HttpURLConnection) {
                ((HttpURLConnection) c).setInstanceFollowRedirects(false);
            }
 
            // We want to open the input stream before
            // getting headers, because getHeaderField()
            // et al swallow IOExceptions.
            in = c.getInputStream();
            redir = false;
 
            if (c instanceof HttpURLConnection) {
                HttpURLConnection http = (HttpURLConnection) c;
                int stat = http.getResponseCode();
                if (stat >= 300 && stat <= 307 && stat != 306 &&
                        stat != HttpURLConnection.HTTP_NOT_MODIFIED) {
                    URL base = http.getURL();
                    String loc = http.getHeaderField("Location");
                    URL target = null;
                    if (loc != null) {
                        target = new URL(base, loc);
                    }
                    http.disconnect();
                    if (target == null
                        || !base.getProtocol().equals(target.getProtocol())
                        || base.getPort() != target.getPort()
                        || !hostsEqual(base, target)
                        || redirects >= 5)
                    {
                        throw new SecurityException("illegal URL redirect");
		    }
                    redir = true;
                    c = target.openConnection();
                    redirects++;
                }
            }
        } while (redir);
        return in;
    }


    //
    // Same as java.net.URL.hostsEqual
    //
    private static boolean hostsEqual(URL u1, URL u2) {
	final String h1 = u1.getHost();
	final String h2 = u2.getHost();

	if (h1 == null) {
	    return h2 == null;
	} else if (h2 == null) {
	    return false;
	} else if (h1.equalsIgnoreCase(h2)) {
	    return true;
	}
        // Have to resolve addresses before comparing, otherwise
        // names like tachyon and tachyon.eng would compare different
	final boolean result[] = {false};

	java.security.AccessController.doPrivileged(
	    new java.security.PrivilegedAction() {
	    public Object run() {
		try {
		    InetAddress a1 = InetAddress.getByName(h1);
		    InetAddress a2 = InetAddress.getByName(h2);
		    result[0] = a1.equals(a2);
		} catch(UnknownHostException e) {
		} catch(SecurityException e) {
		}
		return null;
	    }
	});

        return result[0];
    }

    // overridden in HTTPS subclass

    public void connect() throws IOException {
	plainConnect();
    }

    private boolean checkReuseConnection () {
	if (connected) {
	    return true;
	}
	if (reuseClient != null) {
	    http = reuseClient;
	    http.reuse = false;
	    reuseClient = null;
	    connected = true;
	    return true;
	}
	return false;
    }

    protected void plainConnect()  throws IOException {
	if (connected) {
	    return;
	}
	try {
	    if ("http".equals(url.getProtocol()) && !failedOnce) {
		http = HttpClient.New(url);
	    } else {
		// make sure to construct new connection if first
		// attempt failed
		http = new HttpClient(url, handler.proxy, handler.proxyPort);
	    }
	    ps = (PrintStream)http.getOutputStream();
	} catch (IOException e) {
	    throw e;
	}
	// constructor to HTTP client calls openserver
	connected = true;
    }

    /*
     * Allowable input/output sequences:
     * [interpreted as POST/PUT]
     * - get output, [write output,] get input, [read input]
     * - get output, [write output]
     * [interpreted as GET]
     * - get input, [read input]
     * Disallowed:
     * - get input, [read input,] get output, [write output]
     */

    public synchronized OutputStream getOutputStream() throws IOException {

	try {
	    if (!doOutput) {
		throw new ProtocolException("cannot write to a URLConnection"
			       + " if doOutput=false - call setDoOutput(true)");
	    }
	    
	    if (method.equals("GET")) {
		method = "POST"; // Backward compatibility
	    }
	    if (!"POST".equals(method) && !"PUT".equals(method) && 
		"http".equals(url.getProtocol())) {
		throw new ProtocolException("HTTP method " + method + 
					    " doesn't support output");
	    }

	    // if there's already an input stream open, throw an exception
	    if (inputStream != null) {
		throw new ProtocolException("Cannot write output after reading input.");
	    }

	    if (!checkReuseConnection())
	    	connect();

	    /* This exists to fix the HttpsURLConnection subclass.
	     * Hotjava needs to run on JDK1.1.  Do proper fix in subclass
	     * for 1.2 and remove this.
	     */
	    ps = (PrintStream)http.getOutputStream();

	    if (poster == null)
		poster = new PosterOutputStream();
	    return poster;
	} catch (RuntimeException e) {
	    disconnectInternal();
	    throw e;
	} catch (IOException e) {
	    disconnectInternal();
	    throw e;
	}
    }

    public synchronized InputStream getInputStream() throws IOException {

	if (!doInput) {
	    throw new ProtocolException("Cannot read from URLConnection"
		   + " if doInput=false (call setDoInput(true))");
	}

	if (rememberedException != null) {
	    if (rememberedException instanceof RuntimeException)
		throw new RuntimeException(rememberedException);
	    else {
		IOException exception;
		try {
		    exception = new IOException();
		    exception.initCause(rememberedException);
		} catch (Exception t) {
		    exception = (IOException) rememberedException;
		}
		throw exception;
	    }
	}

	if (inputStream != null) {
	    return inputStream;
	}

	int redirects = 0;
	int respCode = 0;
	AuthenticationInfo serverAuthentication = null;
	AuthenticationInfo proxyAuthentication = null;
	AuthenticationHeader srvHdr = null; 
	try {
	    do {

		pe = new ProgressEntry(url.getFile(), null);
		ProgressData.pdata.register(pe);
		if (!checkReuseConnection())
		    connect();

		/* This exists to fix the HttpsURLConnection subclass.
		 * Hotjava needs to run on JDK1.1.  Do proper fix once a
		 * proper solution for SSL can be found.
		 */
		ps = (PrintStream)http.getOutputStream();

		writeRequests();
		http.parseHTTP(responses, pe);
		inputStream = new HttpInputStream (http.getInputStream());

		respCode = netMetricCode = getResponseCode();
		if (respCode == HTTP_PROXY_AUTH) {
		    AuthenticationHeader authhdr = new AuthenticationHeader (
			"Proxy-Authenticate", responses
		    );
		    if (!doingNTLMp2ndStage) {
 		        proxyAuthentication =
		            resetProxyAuthentication(proxyAuthentication, authhdr);
		        if (proxyAuthentication != null) {
			    redirects++;
			    disconnectInternal();
			    continue;
		        }
		    } else {
			/* in this case, only one header field will be present */
		    	String raw = responses.findValue ("Proxy-Authenticate");
			reset ();
			if (!proxyAuthentication.setHeaders(this, 
							authhdr.headerParser(), raw)) {
			    disconnectInternal();
			    throw new IOException ("Authentication failure");
			}
			if (serverAuthentication != null && srvHdr != null &&
				!serverAuthentication.setHeaders(this, 
							srvHdr.headerParser(), raw)) {
			    disconnectInternal ();
			    throw new IOException ("Authentication failure");
			}
			authObj = null; 
			doingNTLMp2ndStage = false;
			continue;
		    }
		}

		// cache proxy authentication info
		if (proxyAuthentication != null) {
		    // cache auth info on success, domain header not relevant.
		    proxyAuthentication.addToCache();
		}

		if (respCode == HTTP_UNAUTHORIZED) {
    		    srvHdr = new AuthenticationHeader (
	    	   	 "WWW-Authenticate", responses
    		    );
		    String raw = srvHdr.raw();
		    if (!doingNTLM2ndStage) {
		        if (serverAuthentication != null) { 
			    if (serverAuthentication.isAuthorizationStale (raw)) {
			        /* we can retry with the current credentials */
			        disconnectInternal();
			        redirects++;
			        requests.set(serverAuthentication.getHeaderName(), 
				            serverAuthentication.getHeaderValue(url, method));
		    	        currentServerCredentials = serverAuthentication;
			        continue;
			    } else {
			        serverAuthentication.removeFromCache();
			    }
		        }
		        serverAuthentication = getServerAuthentication(srvHdr);
		        currentServerCredentials = serverAuthentication;
    
		        if (serverAuthentication != null) {
		            disconnectInternal();
		            redirects++; // don't let things loop ad nauseum
		            continue;
		        }
		    } else {
			reset ();
			/* header not used for ntlm */
			if (!serverAuthentication.setHeaders(this, null, raw)) {
			    disconnectInternal();
			    throw new IOException ("Authentication failure");
			}
			doingNTLM2ndStage = false;
			authObj = null; 
			continue;
		    }
		}
		// cache server authentication info
		if (serverAuthentication != null) {
		    // cache auth info on success
		    if (!(serverAuthentication instanceof DigestAuthentication) ||
			(domain == null)) {
			if (serverAuthentication instanceof BasicAuthentication) {
			    // check if the path is shorter than the existing entry
			    String npath = AuthenticationInfo.reducePath (url.getPath());
			    String opath = serverAuthentication.path;
			    if (!opath.startsWith (npath) || npath.length() >= opath.length()) {
				/* npath is longer, there must be a common root */
				npath = BasicAuthentication.getRootPath (opath, npath);
			    }
			    // remove the entry and create a new one 
			    BasicAuthentication a = 
			        (BasicAuthentication) serverAuthentication.clone();
			    serverAuthentication.removeFromCache();
			    a.path = npath;
			    serverAuthentication = a;
			}
			serverAuthentication.addToCache();
		    } else {
			// what we cache is based on the domain list in the request
			DigestAuthentication srv = (DigestAuthentication)
			    serverAuthentication;
			StringTokenizer tok = new StringTokenizer (domain," ");
			String realm = srv.realm;
			PasswordAuthentication pw = srv.pw;
			digestparams = srv.params;
			while (tok.hasMoreTokens()) {
			    String path = tok.nextToken();
			    try {
				/* path could be an abs_path or a complete URI */
				URL u = new URL (url, path);
				DigestAuthentication d = new DigestAuthentication (
						   false, u, realm, "Digest", pw, digestparams);
				d.addToCache ();
			    } catch (Exception e) {}
			}
		    }
		}
		
		if (respCode == HTTP_OK) {
		    checkResponseCredentials (false);
		} else {
		    needToCheck = false;
		}

		if (followRedirect()) {
		    /* if we should follow a redirect, then the followRedirects()
		     * method will disconnect() and re-connect us to the new
		     * location
		     */
		    redirects++;
		    continue;
		}

		int cl = -1;
		try {
		    cl = Integer.parseInt(responses.findValue("content-length"));
		} catch (Exception exc) { };

		if (method.equals("HEAD") || method.equals("TRACE") || cl == 0 ||
		    respCode == HTTP_NOT_MODIFIED ||
		    respCode == HTTP_NO_CONTENT) {

		    if (pe != null) {
			ProgressData.pdata.unregister(pe);
		    }
		    http.finished();
		    http = null;
		    inputStream = new EmptyInputStream();
		    if ( respCode < 400) {
			connected = false;
			return inputStream;
		    }
		}
		
		if (respCode >= 400) {
		    if (respCode == 404 || respCode == 410) {
			throw new FileNotFoundException(url.toString());
		    } else {
			throw new java.io.IOException("Server returned HTTP" +
			      " response code: " + respCode + " for URL: " +
			      url.toString());
		    }
		}

		return inputStream;
	    } while (redirects < maxRedirects);

	    throw new ProtocolException("Server redirected too many " +
					" times ("+ redirects + ")");
	} catch (RuntimeException e) {
	    disconnectInternal();
	    rememberedException = e;
	    throw e;
	} catch (IOException e) {
	    rememberedException = e;
	    throw e;
	} finally {
	    if (respCode == HTTP_PROXY_AUTH && proxyAuthentication != null) {
		proxyAuthentication.endAuthRequest();
	    } 
	    else if (respCode == HTTP_UNAUTHORIZED && serverAuthentication != null) {
		serverAuthentication.endAuthRequest();
	    }
	}
    }

    public InputStream getErrorStream() {
	if (connected && responseCode >= 400) {
	    // Client Error 4xx and Server Error 5xx
	    if (inputStream != null) {
		return inputStream;
	    }
	}
	return null;
    }
    
    /**
     * set or reset proxy authentication info in request headers
     * after receiving a 407 error. In the case of NTLM however,
     * receiving a 407 is normal and we just skip the stale check
     * because ntlm does not support this feature.
     */
    private AuthenticationInfo
	resetProxyAuthentication(AuthenticationInfo proxyAuthentication, AuthenticationHeader auth) {
	if (proxyAuthentication != null ) { 
	    String raw = auth.raw();
	    if (proxyAuthentication.isAuthorizationStale (raw)) {
		/* we can retry with the current credentials */
		requests.set (proxyAuthentication.getHeaderName(), 
			      proxyAuthentication.getHeaderValue(
						     url, method));
		currentProxyCredentials = proxyAuthentication;
		return  proxyAuthentication;
	    } else {
		proxyAuthentication.removeFromCache();
	    }
	}
	proxyAuthentication = getHttpProxyAuthentication(auth);
	currentProxyCredentials = proxyAuthentication;
	return  proxyAuthentication;
    }

    /**
     * establish a tunnel through proxy server
     */
    protected synchronized void doTunneling() throws IOException {
	int retryTunnel = 0;
	String statusLine = "";
	int respCode = 0;
	AuthenticationInfo proxyAuthentication = null;
	String proxyHost = null;
	int proxyPort = -1;
	try {
	    do {
	        if (!checkReuseConnection()) {
		    proxiedConnect(url, proxyHost, proxyPort, false);
	        }
	        // send the "CONNECT" request to establish a tunnel
	        // through proxy server
	        sendCONNECTRequest();
	        responses.reset();
	        http.parseHTTP(responses, new ProgressEntry(url.getFile(), null));
	        statusLine = responses.getValue(0);
	        StringTokenizer st = new StringTokenizer(statusLine);
	        st.nextToken();
	        respCode = Integer.parseInt(st.nextToken().trim());
	        if (respCode == HTTP_PROXY_AUTH) {
	    	    AuthenticationHeader authhdr = new AuthenticationHeader (
		        "Proxy-Authenticate", responses
	    	    );
	            if (!doingNTLMp2ndStage) {
		        proxyAuthentication =
		            resetProxyAuthentication(proxyAuthentication, authhdr);
		        if (proxyAuthentication != null) {
			    proxyHost = http.getProxyHostUsed();
			    proxyPort = http.getProxyPortUsed();
		            disconnectInternal();
		            retryTunnel++;
		            continue;
		        }
		    } else {
		        String raw = responses.findValue ("Proxy-Authenticate");
		        reset ();
		        if (!proxyAuthentication.setHeaders(this, 
						authhdr.headerParser(), raw)) {
			    proxyHost = http.getProxyHostUsed();
			    proxyPort = http.getProxyPortUsed();
		            disconnectInternal();
		            throw new IOException ("Authentication failure");
		        }
			authObj = null;
		        doingNTLMp2ndStage = false;
		        continue;
		    }
	        }
	        // cache proxy authentication info
	        if (proxyAuthentication != null) {
		    // cache auth info on success, domain header not relevant.
		    proxyAuthentication.addToCache();
	        }
    
	        if (respCode == HTTP_OK) {
		    break;
	        }
	        // we don't know how to deal with other response code
	        // so disconnect and report error
	        disconnectInternal();
	        break;
	    } while (retryTunnel < maxRedirects);
    
	    if (retryTunnel >= maxRedirects || (respCode != HTTP_OK)) {
	        throw new IOException("Unable to tunnel through proxy."+
				      " Proxy returns \"" +
				      statusLine + "\"");
	    }
	} finally  {
	    if (respCode == HTTP_PROXY_AUTH && proxyAuthentication != null) {
		proxyAuthentication.endAuthRequest();
	    } 
	}
	// remove tunneling related requests
	int i;
	if ((i = requests.getKey("Proxy-authorization")) >= 0)
	    requests.set(i, null, null);

	// reset responses
	responses.reset();
    }

    /**
     * send a CONNECT request for establishing a tunnel to proxy server
     */
    private void sendCONNECTRequest() throws IOException {
	int port = url.getPort();
	if (port == -1) {
	    port = url.getDefaultPort();
	}
	requests.prepend("CONNECT " + url.getHost() + ":"
			 + port + " " + httpVersion, null);
	requests.setIfNotSet("User-Agent", userAgent);

	String host = url.getHost();
	if (port != -1 && port != 80) {
	    host += ":" + String.valueOf(port);
	}
	requests.setIfNotSet("Host", host);

	// Not really necessary for a tunnel, but can't hurt
	requests.setIfNotSet("Accept", acceptString);

	setPreemptiveProxyAuthentication(requests);
	http.writeRequests(requests, null);
	// remove CONNECT header
	requests.set(0, null, null);
    }

    /**
     * Sets pre-emptive proxy authentication in header
     */
    private void setPreemptiveProxyAuthentication(MessageHeader requests) {
	AuthenticationInfo pauth
	    = AuthenticationInfo.getProxyAuth(http.getProxyHostUsed(),
					      http.getProxyPortUsed());
	if (pauth != null && pauth.supportsPreemptiveAuthorization()) {
	    // Sets "Proxy-authorization"
	    requests.setIfNotSet(pauth.getHeaderName(), 
				 pauth.getHeaderValue(url,method));
	    currentProxyCredentials = pauth;
	}
    }

    /**
     * Gets the authentication for an HTTP proxy, and applies it to
     * the connection.
     */
    private AuthenticationInfo getHttpProxyAuthentication (AuthenticationHeader authhdr) {
	/* get authorization from authenticator */
	AuthenticationInfo ret = null;
	String raw = authhdr.raw();
	String host = http.getProxyHostUsed();
	int port = http.getProxyPortUsed();
	if (host != null && authhdr.isPresent()) {
	    HeaderParser p = authhdr.headerParser();
	    String realm = p.findValue("realm");
	    String scheme = authhdr.scheme();
	    char schemeID;
	    if ("basic".equalsIgnoreCase(scheme)) {
		schemeID = BasicAuthentication.BASIC_AUTH;
	    } else if ("digest".equalsIgnoreCase(scheme)) {
		schemeID = DigestAuthentication.DIGEST_AUTH;

	    } else {
		schemeID = 0;
	    }
	    if (realm == null)
		realm = "";
	    ret = AuthenticationInfo.getProxyAuth(host, port, realm, schemeID);
	    if (ret == null) {
	    	if (schemeID == BasicAuthentication.BASIC_AUTH) {
		    InetAddress addr = null;
		    try {
		        final String finalHost = host;
		        addr = (InetAddress)
			    java.security.AccessController.doPrivileged
			        (new java.security.PrivilegedExceptionAction() {
			        public Object run()
				    throws java.net.UnknownHostException {
				    return InetAddress.getByName(finalHost);
			        }
			    });
		    } catch (java.security.PrivilegedActionException ignored) {
		        // User will have an unknown host.
		    }
		    PasswordAuthentication a =
		        privilegedRequestPasswordAuthentication(
					    host, addr, port, "http", realm, scheme);
		    if (a != null) {
		        ret = new BasicAuthentication(true, host, port, realm, a);
		    }
	        } else if (schemeID == DigestAuthentication.DIGEST_AUTH) {
		    PasswordAuthentication a = 
		        privilegedRequestPasswordAuthentication(
					    host, null, port, url.getProtocol(),
					    realm, scheme);
		    if (a != null) {
		        DigestAuthentication.Parameters params = 
			    new DigestAuthentication.Parameters();
		        ret = new DigestAuthentication(true, host, port, realm, 
							    scheme, a, params);
		    }
	        } 
	    }
	    // For backwards compatibility, we also try defaultAuth

	    if (ret == null && defaultAuth != null
		&& defaultAuth.schemeSupported(scheme)) {
		try {
		    URL u = new URL("http", host, port, "/");
		    String a = defaultAuth.authString(u, scheme, realm);
		    if (a != null) {
			ret = new BasicAuthentication (true, host, port, realm, a);
			// not in cache by default - cache on success
		    }
		} catch (java.net.MalformedURLException ignored) {
		}
	    }
	    if (ret != null) {
		if (!ret.setHeaders(this, p, raw)) {
		    ret = null;
		}
	    }
	}
	return ret;
    }

    /**
     * Gets the authentication for an HTTP server, and applies it to
     * the connection.
     */
    private AuthenticationInfo getServerAuthentication (AuthenticationHeader authhdr) {
	/* get authorization from authenticator */
	AuthenticationInfo ret = null;
	String raw = authhdr.raw();
	/* When we get an NTLM auth from cache, don't set any special headers */
	if (authhdr.isPresent()) {
	    HeaderParser p = authhdr.headerParser();
	    String realm = p.findValue("realm");
	    String scheme = authhdr.scheme();
	    char schemeID;
	    if ("basic".equalsIgnoreCase(scheme)) {
		schemeID = BasicAuthentication.BASIC_AUTH;
	    } else if ("digest".equalsIgnoreCase(scheme)) {
		schemeID = DigestAuthentication.DIGEST_AUTH;
 
	    } else {
		schemeID = 0;
	    }
	    domain = p.findValue ("domain");
	    if (realm == null)
		realm = "";
	    ret = AuthenticationInfo.getServerAuth(url, realm, schemeID);
	    InetAddress addr = null;
	    if (ret == null) {
		try {
		    addr = InetAddress.getByName(url.getHost());
		} catch (java.net.UnknownHostException ignored) {
		    // User will have addr = null
		}
	    }
	    // replacing -1 with default port for a protocol
	    int port = url.getPort();
	    if (port == -1) {
		port = url.getDefaultPort();
	    }
	    if (ret == null) {
	        if (schemeID == BasicAuthentication.BASIC_AUTH) {
		    PasswordAuthentication a = 
		        privilegedRequestPasswordAuthentication(
					    url.getHost(), addr, port, url.getProtocol(),
					    realm, scheme);
		    if (a != null) {
		        ret = new BasicAuthentication(false, url, realm, a);
		    }
	        }
    
	        if (schemeID == DigestAuthentication.DIGEST_AUTH) {
		    PasswordAuthentication a = 
		        privilegedRequestPasswordAuthentication(
					    url.getHost(), addr, port, url.getProtocol(),
					    realm, scheme);
		    if (a != null) {
		        digestparams = new DigestAuthentication.Parameters();
		        ret = new DigestAuthentication(false, url, realm, scheme, a, digestparams);
		    }
	        }
	    }

	    // For backwards compatibility, we also try defaultAuth

	    if (ret == null && defaultAuth != null
		&& defaultAuth.schemeSupported(scheme)) {
		String a = defaultAuth.authString(url, scheme, realm);
		if (a != null) {
		    ret = new BasicAuthentication (false, url, realm, a); 
		    // not in cache by default - cache on success
		}
	    }

	    if (ret != null ) {
		if (!ret.setHeaders(this, p, raw)) {
		    ret = null;
		}
	    }
	}
	return ret;
    }

    /* inclose will be true if called from close(), in which case we
     * force the call to check because this is the last chance to do so.
     * If not in close(), then the authentication info could arrive in a trailer
     * field, which we have not read yet.
     */
    private void checkResponseCredentials (boolean inClose) throws IOException {
	try {
	    if (!needToCheck)
	        return;
	    if (validateProxy && currentProxyCredentials != null) {
	        String raw = responses.findValue ("Proxy-Authentication-Info");
		if (inClose || (raw != null)) {
	            currentProxyCredentials.checkResponse (raw, method, url);
	            currentProxyCredentials = null;
		}
	    }
	    if (validateServer && currentServerCredentials != null) {
	        String raw = responses.findValue ("Authentication-Info");
		if (inClose || (raw != null)) {
	            currentServerCredentials.checkResponse (raw, method, url);
	            currentServerCredentials = null;
		}
	    }
	    if ((currentServerCredentials==null) && (currentProxyCredentials == null)) {
		needToCheck = false;
	    }
	} catch (IOException e) {
	    disconnectInternal();
	    connected = false;
	    throw e;
	}
    }

    /* Tells us whether to follow a redirect.  If so, it
     * closes the connection (break any keep-alive) and
     * resets the url, re-connects, and resets the request
     * property.
     */
    private boolean followRedirect() throws IOException {
	if (!getInstanceFollowRedirects()) {
	    return false;
	}

	int stat = getResponseCode();
	if (stat < 300 || stat > 307 || stat == 306 
				|| stat == HTTP_NOT_MODIFIED) {
	    return false;
	}
	String loc = getHeaderField("Location");
	if (loc == null) { 
	    /* this should be present - if not, we have no choice
	     * but to go forward w/ the response we got
	     */
	    return false;
	}
	URL locUrl;
	try {
	    locUrl = new URL(loc);
	    if (!url.getProtocol().equalsIgnoreCase(locUrl.getProtocol())) {
		return false;
	    }

	} catch (MalformedURLException mue) {
	  // treat loc as a relative URI to conform to popular browsers
	  locUrl = new URL(url, loc);
	}
	disconnectInternal();
	// clear out old response headers!!!!
	responses = new MessageHeader();
	if (stat == HTTP_USE_PROXY) {
	    /* This means we must re-request the resource through the
	     * proxy denoted in the "Location:" field of the response.
	     * Judging by the spec, the string in the Location header
	     * _should_ denote a URL - let's hope for "http://my.proxy.org"
	     * Make a new HttpClient to the proxy, using HttpClient's
	     * Instance-specific proxy fields, but note we're still fetching
	     * the same URL.
	     */
	    setProxiedClient (url, locUrl.getHost(), locUrl.getPort());
	    requests.set(0, method + " " + http.getURLFile()+" "  + 
			     httpVersion, null);
	    connected = true;
	} else {
	    // maintain previous headers, just change the name
	    // of the file we're getting
	    url = locUrl;
	    if (method.equals("POST") && !Boolean.getBoolean("http.strictPostRedirect") && (stat!=307)) {
		/* The HTTP/1.1 spec says that a redirect from a POST 
		 * *should not* be immediately turned into a GET, and
		 * that some HTTP/1.0 clients incorrectly did this.
		 * Correct behavior redirects a POST to another POST.
		 * Unfortunately, since most browsers have this incorrect
		 * behavior, the web works this way now.  Typical usage
		 * seems to be:
		 *   POST a login code or passwd to a web page.
		 *   after validation, the server redirects to another
		 *     (welcome) page
		 *   The second request is (erroneously) expected to be GET
		 * 
		 * We will do the incorrect thing (POST-->GET) by default.
		 * We will provide the capability to do the "right" thing
		 * (POST-->POST) by a system property, "http.strictPostRedirect=true"
		 */

		requests = new MessageHeader();
		setRequests = false;
		setRequestMethod("GET");
		poster = null;
		if (!checkReuseConnection())
		    connect();
	    } else {
		if (!checkReuseConnection())
		    connect();
		requests.set(0, method + " " + http.getURLFile()+" "  + 
			     httpVersion, null);
		requests.set("Host", url.getHost() + ((url.getPort() == -1 || url.getPort() == 80) ?
			                             "": ":"+String.valueOf(url.getPort())));
	    }
	}
	return true;
    }

    /* dummy byte buffer for reading off socket prior to closing */
    byte[] cdata = new byte [128];

    /**
     * Reset (without disconnecting the TCP conn) in order to do another transaction with this instance
     */
    private void reset() throws IOException {
	http.reuse = true;
	/* must save before calling close */
	reuseClient = http;
	InputStream is = http.getInputStream();
	try {
	    /* we want to read the rest of the response without using the
	     * hurry mechanism, because that would close the connection
  	     * if everything is not available immediately
	     */
	    if ((is instanceof ChunkedInputStream) || 
		(is instanceof MeteredStream)) {
		/* reading until eof will not block */
	        while (is.read (cdata) > 0) {}
	    } else { 
		/* raw stream, which will block on read, so only read
		 * the expected number of bytes, probably 0
		 */
		int cl = 0, n=0;
		try {
	            cl = Integer.parseInt (responses.findValue ("Content-Length"));
		} catch (Exception e) {}
	        for (int i=0; i<cl; ) {
		    if ((n = is.read (cdata)) == -1) {
		        break;
		    } else {
		        i+= n;
		    }
	        }
	    }
	} catch (IOException e) {
	    http.reuse = false;
	    reuseClient = null;
	    disconnectInternal();
	    return;
	} 
	try {
	    if (is instanceof MeteredStream) {
		is.close();
	    }
	} catch (IOException e) { }
	responseCode = -1;
	responses = new MessageHeader();
	connected = false;
    }

    /**
     * Disconnect from the server (for internal use)
     */
    private void disconnectInternal() {
	responseCode = -1;
        if (pe != null) {
            ProgressData.pdata.unregister(pe);
        }
	if (http != null) {
	    http.closeServer();
            http = null;
            connected = false;
        }
    }

    /**
     * Disconnect from the server (public API)
     */
    public void disconnect() {

	responseCode = -1;
	if (pe != null) {
	    ProgressData.pdata.unregister(pe);
	}
	if (http != null) {
	    /*
	     * If we have an input stream this means we received a response
	     * from the server. That stream may have been read to EOF and
	     * dependening on the stream type may already be closed or the
	     * the http client may be returned to the keep-alive cache.
	     * If the http client has been returned to the keep-alive cache
	     * it may be closed (idle timeout) or may be allocated to 
	     * another request.
	     *
	     * In other to avoid timing issues we close the input stream
	     * which will either close the underlying connection or return
	     * the client to the cache. If there's a possibility that the
	     * client has been returned to the cache (ie: stream is a keep
	     * alive stream or a chunked input stream) then we remove an
	     * idle connection to the server. Note that this approach
	     * can be considered an approximation in that we may close a
  	     * different idle connection to that used by the request.
	     * Additionally it's possible that we close two connections
	     * - the first becuase it wasn't an EOF (and couldn't be
	     * hurried) - the second, another idle connection to the
	     * same server. The is okay because "disconnect" is an
	     * indication that the application doesn't intend to access
	     * this http server for a while.
	     */

	    if (inputStream != null) {
		HttpClient hc = http;

		// un-synchronized 
		boolean ka = hc.isKeepingAlive();

		try {
		    inputStream.close();
		} catch (IOException ioe) { }

		// if the connection is persistent it may have been closed
		// or returned to the keep-alive cache. If it's been returned
		// to the keep-alive cache then we would like to close it
		// but it may have been allocated

		if (ka) {
		    hc.closeIdleConnection();
		}
		

	    } else {
	        http.closeServer();
	    }

	    //	    poster = null;
	    http = null;
	    connected = false;
	}
    }

    public boolean usingProxy() {
	if (http != null) {
	    return (http.getProxyHostUsed() != null);
	}
	return false;
    }

    /**
     * Gets a header field by name. Returns null if not known.
     * @param name the name of the header field
     */
    public String getHeaderField(String name) {
	try {
	    getInputStream();
	} catch (IOException e) {}
	return responses.findValue(name);
    }

    /**
     * Returns an unmodifiable Map of the header fields.
     * The Map keys are Strings that represent the
     * response-header field names. Each Map value is an
     * unmodifiable List of Strings that represents 
     * the corresponding field values.
     *
     * @return a Map of header fields
     * @since 1.4
     */
    public Map getHeaderFields() {
	try {
	    getInputStream();
	} catch (IOException e) {}
        return responses.getHeaders();
    }

    /**
     * Gets a header field by index. Returns null if not known.
     * @param n the index of the header field
     */
    public String getHeaderField(int n) {
	try {
	    getInputStream();
	} catch (IOException e) {}
	return responses.getValue(n);
    }

    /**
     * Gets a header field by index. Returns null if not known.
     * @param n the index of the header field
     */
    public String getHeaderFieldKey(int n) {
	try {
	    getInputStream();
	} catch (IOException e) {}
	return responses.getKey(n);
    }

    /**
     * Sets request property. If a property with the key already
     * exists, overwrite its value with the new value.
     * @param value the value to be set
     */
    public void setRequestProperty(String key, String value) {
	super.setRequestProperty(key, value);
	checkMessageHeader(key, value);
	requests.set(key, value);
    }

    /**
     * Adds a general request property specified by a
     * key-value pair.  This method will not overwrite
     * existing values associated with the same key.
     *
     * @param   key     the keyword by which the request is known
     *                  (e.g., "<code>accept</code>").
     * @param   value  the value associated with it.
     * @see #getRequestProperties(java.lang.String)
     * @since 1.4
     */
    public void addRequestProperty(String key, String value) {
	super.addRequestProperty(key, value);
	checkMessageHeader(key, value);
	requests.add(key, value);
    }

    //
    // Set a property for authentication.  This can safely disregard
    // the connected test.
    //
    void setAuthenticationProperty(String key, String value) {
	checkMessageHeader(key, value);
	requests.set(key, value);
    }

    public String getRequestProperty (String key) {
	// don't return headers containing security sensitive information
	if (key != null) {
	    for (int i=0; i < EXCLUDE_HEADERS.length; i++) {
		if (key.equalsIgnoreCase(EXCLUDE_HEADERS[i])) {
		    return null;
		}
	    }
	}
	return requests.findValue(key);
    }

    /**
     * Returns an unmodifiable Map of general request
     * properties for this connection. The Map keys
     * are Strings that represent the request-header
     * field names. Each Map value is a unmodifiable List 
     * of Strings that represents the corresponding 
     * field values.
     *
     * @return  a Map of the general request properties for this connection.
     * @throws IllegalStateException if already connected
     * @since 1.4
     */
    public Map getRequestProperties() {
        if (connected)
            throw new IllegalStateException("Already connected");

	// exclude headers containing security-sensitive info
	return requests.getHeaders(EXCLUDE_HEADERS);
    }

    public void finalize() {
	// this should do nothing.  The stream finalizer will close 
	// the fd
    }

    String getMethod() {
	return method;
    }

    /* The purpose of this wrapper is just to capture the close() call
     * so we can check authentication information that may have
     * arrived in a Trailer field
     */
    class HttpInputStream extends FilterInputStream {

        public HttpInputStream (InputStream is) {
	    super (is);
        }

        public int read() throws IOException {
            int ret = super.read();
            return ret;
        }

        public int read(byte b[], int off, int len) throws IOException {
            int ret = super.read(b, off, len);
            return ret;
        }

        public void close () throws IOException {
	    try {
		super.close ();
                if (sentMetric && NetworkMetrics.metricsAvailable()) {
                    java.security.AccessController.doPrivileged(new java.security.PrivilegedAction() {
                        public Object run() {
                            int totalBytesRead = http.getBytesRead();
                            nm.sendMetricResponse(http.getServerSocket(),
                                          netMetricCode, totalBytesRead);
                            return null;
                        }
                    });
                    netMetricCode = 0;
                    sentMetric = false;
                }
	    } finally {
		HttpURLConnection.this.http = null;
	    	checkResponseCredentials (true);
	    } 
        }
    }
}

/** An input stream that just returns EOF.  This is for
 * HTTP URLConnections that are KeepAlive && use the
 * HEAD method - i.e., stream not dead, but nothing to be read.
 */

class EmptyInputStream extends InputStream {

    public int available() {
	return 0;
    }

    public int read() {
	return -1;
    }
}

