/*
 * @(#)AuthenticationInfo.java	1.30 06/10/10
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

import java.io.IOException;
import java.net.URL;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.ListIterator;
import java.util.Enumeration;
import java.util.HashMap;

import sun.net.www.HeaderParser;


/**
 * AuthenticationInfo: Encapsulate the information needed to
 * authenticate a user to a server.
 *
 * @version 1.19, 11/06/00 
 */
//	It would be nice if this class understood about partial matching.
//	If you're authorized for foo.com, chances are high you're also
//	authorized for baz.foo.com.
// NB:  When this gets implemented, be careful about the uncaching
//	policy in HttpURLConnection.  A failure on baz.foo.com shouldn't
//	uncache foo.com!
abstract
class AuthenticationInfo implements Cloneable {

    // Constants saying what kind of authroization this is.  This determines
    // the namespace in the hash table lookup.
    static final char SERVER_AUTHENTICATION = 's';
    static final char PROXY_AUTHENTICATION = 'p';

    /**
     * Caches authentication info entered by user.  See cacheKey()
     */
    static private PathMap cache = new PathMap();

    /** 
     * If true, then simultaneous authentication requests to the same realm/proxy
     * are serialized, in order to avoid a user having to type the same username/passwords
     * repeatedly, via the Authenticator. Default is false, which means that this
     * behavior is switched off.
     */
    static boolean serializeAuth;

    static {
	serializeAuth = ((Boolean)java.security.AccessController.doPrivileged(
		new sun.security.action.GetBooleanAction(
		    "http.auth.serializeRequests"))).booleanValue();
    }

    /**
     * requests is used to ensure that interaction with the
     * Authenticator for a particular realm is single threaded.
     * ie. if multiple threads need to get credentials from the user
     * at the same time, then all but the first will block until
     * the first completes its authentication.
     */
    static private HashMap requests = new HashMap ();

    /* check if a request for this destination is in progress
     * return false immediately if not. Otherwise block until
     * request is finished and return true
     */
    static private boolean requestIsInProgress (String key) {
	if (!serializeAuth) {
	    /* behavior is disabled. Revert to concurrent requests */
	    return false;
	}
	synchronized (requests) {
	    Thread t, c;
	    c = Thread.currentThread();
            if ((t=(Thread)requests.get(key))==null) {
	    	requests.put (key, c);
		return false;
            }
	    if (t == c) {
		return false;
	    }
            while (requests.containsKey(key)) {
		try {
                    requests.wait ();
		} catch (InterruptedException e) {}
            }
	}
        /* entry may be in cache now. */
	return true;
    }

    /* signal completion of an authentication (whether it succeeded or not) 
     * so that other threads can continue. 
     */
    static private void requestCompleted (String key) {
	synchronized (requests) {
	    boolean waspresent = requests.remove (key) != null;
	    // Uncomment this line if assertions are always
	    //    turned on for libraries
	    //assert waspresent;
	    requests.notifyAll();
	}
    }

    static void printCache () {cache.print(); }

    //public String toString () {
	//return ("{"+type+":"+authType+":"+protocol+":"+host+":"+port+":"+realm+":"+path+"}");
    //}

    // TODO:  This cache just grows forever.  We should put in a bounded
    //	      cache, or maybe something using WeakRef's.

    /** The type (server/proxy) of authentication this is.  Used for key lookup */
    char type;

    /** The authentication type (basic/digest). Also used for key lookup */
    char authType;

    /** The protocol/scheme (i.e. http or https ). Need to keep the caches
     *  logically separate for the two protocols. This field is only used
     *  when constructed with a URL (the normal case for server authentication)
     *  For proxy authentication the protocol is not relevant.
     */
    String protocol;

    /** The host we're authenticating against. */
    String host;

    /** The port on the host we're authenticating against. */
    int port;

    /** The realm we're authenticating against. */
    String realm;

    /** The shortest path from the URL we authenticated against. */
    String path;

    /** Use this constructor only for proxy entries */
    AuthenticationInfo(char type, char authType, String host, int port, String realm) {
	this.type = type;
    	this.authType = authType;
	this.protocol = "";
	this.host = host.toLowerCase();
	this.port = port;
	this.realm = realm;
	this.path = null;
    }

    public Object clone() {
	try {
	    return super.clone ();
	} catch (CloneNotSupportedException e) {
	    // Cannot happen because Cloneable implemented by AuthenticationInfo
	    return null;
	}
    }

    /*
     * Constructor used to limit the authorization to the path within
     * the URL. Use this constructor for origin server entries.
     */
    AuthenticationInfo(char type, char authType, URL url, String realm) {
    	this.type = type;
    	this.authType = authType;
	this.protocol = url.getProtocol().toLowerCase();
	this.host = url.getHost().toLowerCase();
	this.port = url.getPort();
	if (this.port == -1) {
	    this.port = url.getDefaultPort();
	}
	this.realm = realm;

	String urlPath = url.getPath();
	if (urlPath.length() == 0)
	    this.path = urlPath;
	else {
	    this.path = reducePath (urlPath);
	}

    }

    /*
     * reduce the path to the root of where we think the
     * authorization begins. This could get shorter as
     * the url is traversed up following a successful challenge.
     */
    static String reducePath (String urlPath) {
        int sepIndex = urlPath.lastIndexOf('/');
        int targetSuffixIndex = urlPath.lastIndexOf('.');
        if (sepIndex != -1)
	    if (sepIndex < targetSuffixIndex)
	        return urlPath.substring(0, sepIndex+1);
	    else
	        return urlPath;
        else
	    return urlPath;
    }

    /**
     * Returns info for the URL, for an HTTP server auth.  Used when we 
     * don't yet know the realm
     * (i.e. when we're preemptively setting the auth).
     */
    static AuthenticationInfo getServerAuth(URL url) {
	int port = url.getPort();
	if (port == -1) {
	    port = url.getDefaultPort();
	}
	String key = SERVER_AUTHENTICATION + ":" + url.getProtocol().toLowerCase()
		+ ":" + url.getHost().toLowerCase() + ":" + port;
	return getAuth(key, url);
    }

    /**
     * Returns info for the URL, for an HTTP server auth.  Used when we 
     * do know the realm (i.e. when we're responding to a challenge).
     * In this case we do not use the path because the protection space
     * is identified by the host:port:realm only
     */
    static AuthenticationInfo getServerAuth(URL url, String realm, char atype) {
	int port = url.getPort();
	if (port == -1) {
	    port = url.getDefaultPort();
	}
	String key = SERVER_AUTHENTICATION + ":" + atype + ":" + url.getProtocol().toLowerCase()
		     + ":" + url.getHost().toLowerCase() + ":" + port + ":" + realm;
	AuthenticationInfo cached = getAuth(key, null);
	if ((cached == null) && requestIsInProgress (key)) {
	    /* check the cache again, it might contain an entry */
	    cached = getAuth(key, null);
	}
	return cached;
    }


    /**
     * Return the AuthenticationInfo object from the cache if it's path is
     * a substring of the supplied URLs path.
     */
    static AuthenticationInfo getAuth(String key, URL url) {
	if (url == null) {
	    return cache.get (key, null);
	} else {
	    return cache.get (key, url.getPath());
	}
    }

    /**
     * Returns a firewall authentication, for the given host/port.  Used
     * for preemptive header-setting. Note, the protocol field is always
     * blank for proxies.
     */
    static AuthenticationInfo getProxyAuth(String host, int port) {
	String key = PROXY_AUTHENTICATION + "::" + host.toLowerCase() + ":" + port;
	AuthenticationInfo result = (AuthenticationInfo) cache.get(key, null);
	return result;
    }

    /**
     * Returns a firewall authentication, for the given host/port and realm.
     * Used in response to a challenge. Note, the protocol field is always
     * blank for proxies.
     */
    static AuthenticationInfo getProxyAuth(String host, int port, String realm, char atype) {
	String key = PROXY_AUTHENTICATION + ":" + atype + "::" + host.toLowerCase() 
			+ ":" + port + ":" + realm;
	AuthenticationInfo cached = cache.get(key, null);
	if ((cached == null) && requestIsInProgress (key)) {
	    /* check the cache again, it might contain an entry */
	    cached = cache.get(key, null);
	}
	return cached;
    }


    /**
     * Add this authentication to the cache
     */
    void addToCache() {
	cache.put (cacheKey(true), this);
	if (supportsPreemptiveAuthorization()) {
	    cache.put (cacheKey(false), this);
	}
	endAuthRequest();
    }

    void endAuthRequest () {
	if (!serializeAuth) {
	    return;
	}
    	synchronized (requests) {
	    requestCompleted (cacheKey(true));
	}
    }

    /**
     * Remove this authentication from the cache
     */
    void removeFromCache() {
	cache.remove(cacheKey(true), this);
	if (supportsPreemptiveAuthorization()) {
	    cache.remove(cacheKey(false), this);
	}
    }

    /**
     * @return true if this authentication supports preemptive authorization
     */
    abstract boolean supportsPreemptiveAuthorization();

    /**
     * @return the name of the HTTP header this authentication wants set.
     *		This is used for preemptive authorization.
     */
    abstract String getHeaderName();

    /**
     * Calculates and returns the authentication header value based
     * on the stored authentication parameters. If the calculation does not depend
     * on the URL or the request method then these parameters are ignored.
     * @param url The URL
     * @param method The request method
     * @return the value of the HTTP header this authentication wants set.
     *		Used for preemptive authorization.
     */
    abstract String getHeaderValue(URL url, String method);

    /**
     * Set header(s) on the given connection.  Subclasses must override
     * This will only be called for
     * definitive (i.e. non-preemptive) authorization.
     * @param conn The connection to apply the header(s) to
     * @param p A source of header values for this connection, if needed.
     * @param raw The raw header field (if needed)
     * @return true if all goes well, false if no headers were set.
     */
    abstract boolean setHeaders(HttpURLConnection conn, HeaderParser p, String raw);

    /**
     * Check if the header indicates that the current auth. parameters are stale.
     * If so, then replace the relevant field with the new value
     * and return true. Otherwise return false.
     * returning true means the request can be retried with the same userid/password
     * returning false means we have to go back to the user to ask for a new
     * username password.
     */
    abstract boolean isAuthorizationStale (String header);

    /**
     * Check for any expected authentication information in the response
     * from the server
     */
    abstract void checkResponse (String header, String method, URL url) 
					throws IOException;

    /** 
     * Give a key for hash table lookups.
     * @param includeRealm if you want the realm considered.  Preemptively
     *		setting an authorization is done before the realm is known.
     */
    String cacheKey(boolean includeRealm) {
     	// This must be kept in sync with the getXXXAuth() methods in this
	// class.
	if (includeRealm) {
	    return type + ":" + authType + ":" + protocol + ":"
			+ host + ":" + port + ":" + realm;
	} else {
	    return type + ":" + protocol + ":" + host + ":" + port;
	}
    }
}

/*
 * Pathmap stores AuthenticationInfo instances, which are keyed
 * by the combination of:
 *
 *  1) the protocol+host+port field of the URL where the resource is located
 *     plus optionally the name of the authentication realm.
 *
 *  2) the "known" abs_path root of the protection space. For digest
 *     authentication the root is generally known because it is returned
 *     explicitly by the server. For basic authentication, this is not the
 *     case and we start with the abs_path in the request. If a subsequent
 *     authentication succeeds for a path higher in the same hierarchy then
 *     the shorter pathname replaces the longer one.
 *
 *  If the scheme does not support pre-emptive authentication, then
 *  only one entry is created (per successful authentication), which 
 *  does not inlclude the realm in the primary 1) key. 
 *
 *  If the scheme does support pre-emptive authentication (which is the
 *  case for both basic and digest) then two entries are created for
 *  each successful authentication. One, with the realm and one without.
 *  The one without the realm is used for pre-emptive header setting
 *  because at this time the realm is not known.
 */

class PathMap {
    Hashtable hashtable;
    PathMap () {
	hashtable = new Hashtable ();
    }

    // put a value in map according to primary key + secondary key which
    // is the path field of AuthenticationInfo

    void print () {
	Enumeration keys = hashtable.keys ();
	while (keys.hasMoreElements()) {
	    String key = (String) keys.nextElement();
	    LinkedList list = (LinkedList)(hashtable.get(key));
	    System.out.print ("pkey = " + key + " ");
	    ListIterator iter = list.listIterator();
	    while (iter.hasNext()) {
    	    	AuthenticationInfo inf = (AuthenticationInfo)iter.next();
		System.out.print (inf.path + " ");
	    }
	    System.out.println (" ");
	}
    }

    synchronized void put (String pkey, AuthenticationInfo value) {
	LinkedList list = (LinkedList) hashtable.get (pkey);
	String skey = value.path;
	if (list == null) {
	    list = new LinkedList ();
	    hashtable.put (pkey, list);
	}
	// Check if the path already exists or a super-set of it exists
	ListIterator iter = list.listIterator();
    	while (iter.hasNext()) {
    	    AuthenticationInfo inf = (AuthenticationInfo)iter.next();
    	    if (inf.path == null || inf.path.startsWith (skey)) {
	    	iter.remove ();
    	    }
	}
	iter.add (value);
    }

    // get a value from map checking both primary
    // and secondary (urlpath) key

    synchronized AuthenticationInfo get (String pkey, String skey) {
	LinkedList list = (LinkedList) hashtable.get (pkey);
	if (list == null || list.size() == 0) {
	    return null;
	}
	if (skey == null) { 
	    // list should contain only one element
	    return (AuthenticationInfo)list.get (0);
	}
	ListIterator iter = list.listIterator();
    	while (iter.hasNext()) {
    	    AuthenticationInfo inf = (AuthenticationInfo)iter.next();
    	    if (skey.startsWith (inf.path)) {
	    	return inf;
    	    }
	}
	return null;
    }

    synchronized void remove (String pkey, AuthenticationInfo entry) {
	LinkedList list = (LinkedList) hashtable.get (pkey);
	if (list == null) {
	    return;
	}
	if (entry == null) {
	    list.clear();
	    return;
	}
	ListIterator iter = list.listIterator ();
	while (iter.hasNext()) {
	    AuthenticationInfo inf = (AuthenticationInfo)iter.next();
	    if (entry.equals(inf)) {
	    	iter.remove ();
	    }
	}
    }
}
