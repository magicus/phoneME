/*
 * @(#)PolicyFile.java	1.13 06/10/11
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
 

package sun.security.provider;

import java.io.*;
import java.lang.RuntimePermission;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import java.util.StringTokenizer;
import java.util.PropertyPermission;
import java.util.ArrayList;

import java.lang.reflect.*;

import java.security.cert.Certificate;
import java.security.*;

import sun.security.util.PropertyExpander;
import sun.security.util.Debug;
import sun.net.www.ParseUtil;


/**
 * The policy for a Java runtime (specifying 
 * which permissions are available for code from various principals)
 * is represented as one or more separate
 * persistent configurations.  Each configuration may be stored as a
 * flat ASCII file, as a serialized binary file of
 * the Policy class, or as a database. <p>
 * 
 * Each Java runtime may have multiple policy files.
 * The default policy files are configured in the security
 * properties file to be:
 * 
 * <pre>
 *   (<i>java.home</i>)/lib/security/java.policy
 *   (<i>user.home</i>)/.java.policy
 * </pre>
 * 
 * <p>where <i>java.home</i> indicates the JDK installation directory 
 * (determined by the value of the "java.home" system property),
 * and
 * <p>where <i>user.home</i> indicates the user's home directory
 * (determined by the value of the "user.home" system property).
 * 
 * <p>The Java runtime creates one global Policy object, which is used to
 * represent the permissions granted in the static policy configuration 
 * file(s).  It is consulted by
 * a ProtectionDomain when the protection domain initializes its set of
 * permissions. <p>
 * 
 * <p>The Policy object is agnostic in that
 * it is not involved in making policy decisions.  It is merely the
 * Java runtime representation of the persistent policy configuration
 * file(s). <p>
 * 
 * <p>When a protection domain needs to initialize its set of
 * permissions, it executes code such as the following
 * to ask the global Policy object to populate a
 * PermissionCollection object with the appropriate permissions:
 * <pre>
 *  policy = Policy.getPolicy();
 *  PermissionCollection perms = policy.getPermissions(MyCodeSource)
 * </pre>
 * 
 * <p>The protection domain passes in a CodeSource
 * object, which encapsulates its codebase (URL) and public key attributes.
 * The Policy object evaluates the global policy in light of who the
 * principal is and returns an appropriate Permissions object. 
 * 
 * @author Roland Schemers
 * @version 1.42, 04/17/00
 * @see java.security.CodeSource
 * @see java.security.Permissions
 * @see java.security.ProtectionDomain 
 */

public class PolicyFile extends java.security.Policy {

    private static final Debug debug = Debug.getInstance("policy");

    private Vector policyEntries;
    private Hashtable aliasMapping;

    private boolean initialized = false;

    private boolean expandProperties = true;
    private boolean ignoreIdentityScope = false;

    // for use with the reflection API

    private static final Class[] PARAMS = { String.class, String.class};

    /** 
     * Creates a Policy object.
     */
    public PolicyFile() {
	// initialize Policy if either the java.security.policy or
	// java.security.manager properties are set
	String prop =(String) java.security.AccessController.doPrivileged(
               new sun.security.action.GetPropertyAction(
					   "java.security.policy"));
	if (prop == null) {
	    prop =(String) java.security.AccessController.doPrivileged(
                new sun.security.action.GetPropertyAction(
 					   "java.security.manager"));
	}
 	if (prop != null)
 	    init();
    }

    /** 
     * Initializes the Policy object and reads the default policy 
     * configuration file(s) into the Policy object.
     * 
     * The algorithm for locating the policy file(s) and reading their
     * information into the Policy object is:
     * <pre>
     *   loop through the Security Properties named "policy.url.1", 
     *   "policy.url.2", etc, until you don't find one. Each of 
     *   these specify a policy file.
     *   
     *   if none of these could be loaded, use a builtin static policy
     *      equivalent to the default lib/security/java.policy file.
     * 
     *   if the system property "java.policy" is defined (which is the
     *      case when the user uses the -D switch at runtime), and
     *     its use is allowed by the security property file,
     *     also load it.
     * </pre>
     * 
     * Each policy file consists of one or more grant entries, each of
     * which consists of a number of permission entries.
     * <pre>
     *   grant signedBy "<i>alias</i>", codeBase "<i>URL</i>" {
     *     permission <i>Type</i> "<i>name</i>", "<i>action</i>", 
     *         signedBy "<i>alias</i>";
     *     ....
     *     permission <i>Type</i> "<i>name</i>", "<i>action</i>", 
     *         signedBy "<i>alias</i>";
     *   };
     *  	
     * </pre>
     * 
     * All non-italicized items above must appear as is (although case 
     * doesn't matter and some are optional, as noted below).
     * Italicized items represent variable values.
     *
     * <p> A grant entry must begin with the word <code>grant</code>.
     * The <code>signedBy</code> and <code>codeBase</code> name/value 
     * pairs are optional.
     * If they are not present, then any signer (including unsigned code)
     * will match, and any codeBase will match.
     *
     * <p> A permission entry must begin with the word <code>permission</code>. 
     * The word <code><i>Type</i></code> in the template above would actually be
     * a specific permission type, such as <code>java.io.FilePermission</code>
     * or <code>java.lang.RuntimePermission</code>.
     * 
     * <p>The "<i>action</i>" is required for
     * many permission types, such as <code>java.io.FilePermission</code>
     * (where it specifies what type of file access is permitted).
     * It is not required for categories such as 
     * <code>java.lang.RuntimePermission</code>
     * where it is not necessary - you either have the 
     * permission specified by the <code>"<i>name</i>"</code> 
     * value following the type name or you don't.
     * 
     * <p>The <code>signedBy</code> name/value pair for a permission entry 
     * is optional. If present, it indicates a signed permission. That is,
     * the permission class itself must be signed by the given alias in
     * order for it to be granted. For example,
     * suppose you have the following grant entry:
     * 
     * <pre>
     *   grant {
     *     permission Foo "foobar", signedBy "FooSoft";
     *   }
     * </pre>
     * 
     * <p>Then this permission of type <i>Foo</i> is granted if the 
     * <code>Foo.class</code> permission has been signed by the 
     * "FooSoft" alias, or if <code>Foo.class</code> is a 
     * system class (i.e., is found on the CLASSPATH).
     * 
     * <p>Items that appear in an entry must appear in the specified order
     * (<code>permission</code>, <i>Type</i>, "<i>name</i>", and 
     * "<i>action</i>"). An entry is terminated with a semicolon.
     * 
     * <p>Case is unimportant for the identifiers (<code>permission</code>, 
     * <code>signedBy</code>, <code>codeBase</code>, etc.) but is 
     * significant for the <i>Type</i>
     * or for any string that is passed in as a value. <p>
     * 
     * <p>An example of two entries in a policy configuration file is
     * <pre>
     *   //  if the code is signed by "Duke", grant it read/write to all 
     *   // files in /tmp.
     *
     *   grant signedBy "Duke" {
     * 		permission java.io.FilePermission "/tmp/*", "read,write";
     *   };
     * <p>	
     *   // grant everyone the following permission
     *
     *   grant { 
     * 	   permission java.util.PropertyPermission "java.vendor";
     *   };
     *  </pre>
     */
    private synchronized void init() {

	if (initialized)
	    return;

	policyEntries = new Vector();
	aliasMapping = new Hashtable(11);
	
	AccessController.doPrivileged(new java.security.PrivilegedAction() {
	    public Object run() {
		initPolicyFile();
		initialized = true;
		return null;
	    }
	});
    }

    /**
     * Refreshes the policy object. 
     *
     */
    public synchronized void refresh()
    {
	initialized = false;
	init();
    }


    private void initPolicyFile() {

	// No need to put this into a begin/endPrivileged block, because
	// this is already called from privileged code in Policy.java
	String prop = Security.getProperty("policy.expandProperties");

	if (prop != null) expandProperties = prop.equalsIgnoreCase("true");

	String iscp = Security.getProperty("policy.ignoreIdentityScope");

	if (iscp != null) ignoreIdentityScope = iscp.equalsIgnoreCase("true");

	// No need to put this into a begin/endPrivileged block, because
	// this is already called from privileged code in Policy.java
	String allowSys  = Security.getProperty("policy.allowSystemProperty");

	if ((allowSys!=null) && allowSys.equalsIgnoreCase("true")) {
	    // No need to put this into a begin/endPrivileged block, because
	    // this is already called from privileged code in Policy.java
	    String extra_policy = System.getProperty("java.security.policy");
	    if (extra_policy != null) {
		boolean overrideAll = false;
		if (extra_policy.startsWith("=")) {
		    overrideAll = true;
		    extra_policy = extra_policy.substring(1);
		}
		try {
		    extra_policy = PropertyExpander.expand(extra_policy);
		    URL policyURL;
		    File policyFile = new File(extra_policy);
		    if (policyFile.exists()) {
			policyURL = policyFile.toURL();
		    } else {
			policyURL = new URL(extra_policy);
		    }
		    if (debug != null)
			    debug.println("reading "+policyURL);
		    init(policyURL);
		} catch (Exception e) {
		    // ignore. 
		    if (debug != null) {
			debug.println("caught exception: "+e);
		    }

		}
		if (overrideAll) {
		    if (debug != null) {
			debug.println("overriding other policies!");
		    }
		    return;
		}
	    }
	} else {
	}

	int n = 1;
	boolean loaded_one = false;
	String policy_url;

	// No need to put this into a begin/endPrivileged block, because
	// this is already called from privileged code in Policy.java
	while ((policy_url = Security.getProperty("policy.url."+n)) != null) {
	    try {
		policy_url = PropertyExpander.expand(policy_url).replace
						(File.separatorChar, '/');
		if (debug != null)
		    debug.println("reading "+policy_url);
		if (init(new URL(policy_url))) {
		    loaded_one = true;
		}
	    } catch (Exception e) {
		if (debug != null) {
		    debug.println("error reading policy "+e);
		    e.printStackTrace();
		}
		// ignore that policy
	    }
	    n++;
	}

	if (loaded_one == false) {
	    // use static policy if all else fails
	    initStaticPolicy();
	}
    }

    private void initStaticPolicy() {
	PolicyEntry pe = new PolicyEntry(new CodeSource(null, null));
	pe.add(new java.net.SocketPermission("localhost:1024-", "listen"));
	pe.add(new PropertyPermission("java.version","read"));
	pe.add(new PropertyPermission("java.vendor","read"));
	pe.add(new PropertyPermission("java.vendor.url","read"));
	pe.add(new PropertyPermission("java.class.version","read"));
	pe.add(new PropertyPermission("os.name","read"));
	pe.add(new PropertyPermission("os.version","read"));
	pe.add(new PropertyPermission("os.arch","read"));
	pe.add(new PropertyPermission("file.separator","read"));
	pe.add(new PropertyPermission("path.separator","read"));
	pe.add(new PropertyPermission("line.separator","read"));
	pe.add(new PropertyPermission("java.specification.version", "read"));
	pe.add(new PropertyPermission("java.specification.vendor", "read"));
	pe.add(new PropertyPermission("java.specification.name", "read"));
	pe.add(new PropertyPermission("java.vm.specification.version", "read"));
	pe.add(new PropertyPermission("java.vm.specification.vendor", "read"));
	pe.add(new PropertyPermission("java.vm.specification.name", "read"));
	pe.add(new PropertyPermission("java.vm.version", "read"));
	pe.add(new PropertyPermission("java.vm.vendor", "read"));
	pe.add(new PropertyPermission("java.vm.name", "read"));
	policyEntries.addElement(pe);
	try {
	    String extdir = 
		PropertyExpander.expand("file://${java.home}/lib/ext/*");
	    pe = new PolicyEntry(new CodeSource(new URL(extdir), null));
	    pe.add(new java.security.AllPermission());
	    policyEntries.addElement(pe);
	} catch (Exception e) {
	    // this is probably bad (though not dangerous). What should we do?
	}
    }

    /** 
     * Reads a policy configuration into the Policy object using a
     * Reader object.
     * 
     * @param policyFile the policy Reader object.
     *
     * @return true iff the policy was parsed successfully.
     */
    private boolean init(URL policy) {
	boolean success = false;
	PolicyParser pp = new PolicyParser(expandProperties);
	try {
	    InputStreamReader isr
		= new InputStreamReader(getInputStream(policy)); 
	    pp.read(isr);
	    isr.close();
	    Enumeration enum_ = pp.grantElements();
	    while (enum_.hasMoreElements()) {
		PolicyParser.GrantEntry ge =
		    (PolicyParser.GrantEntry) enum_.nextElement();
		addGrantEntry(ge, null);
	    }
	    success = true;
	} catch (PolicyParser.ParsingException pe) {
	    System.err.println("java.security.Policy: error parsing "+policy);
	    System.err.println("java.security.Policy: " + pe.getMessage());
	    if (debug != null) 
		pe.printStackTrace();

	} catch (Exception e) {
	    if (debug != null) {
		debug.println("error parsing "+policy);
		debug.println(e.toString());
		e.printStackTrace();
	    }
	}
	return success;
    }

    /*
     * Fast path reading from file urls in order to avoid calling
     * FileURLConnection.connect() which can be quite slow the first time
     * it is called. We really should clean up FileURLConnection so that
     * this is not a problem but in the meantime this fix helps reduce
     * start up time noticeably for the new launcher. -- DAC
     */
    private InputStream getInputStream(URL url) throws IOException {
	if ("file".equals(url.getProtocol())) {
	    String path = url.getFile().replace('/', File.separatorChar);
	    path = ParseUtil.decode(path);
	    return new FileInputStream(path);
	} else {
	    return url.openStream();
	}
    }

    /**
     * Given a PermissionEntry, create a codeSource.
     *
     * @return null if signedBy alias is not recognized
     */
    CodeSource getCodeSource(PolicyParser.GrantEntry ge, Object keyStore) 
	throws java.net.MalformedURLException
    {
	Certificate[] certs = null;
	if (ge.signedBy != null) {
	    certs = getCertificates(null, ge.signedBy);
	    if (certs == null) {
		// we don't have a key for this alias,
		// just return
		if (debug != null) {
		    debug.println(" no certs for alias " +
				       ge.signedBy + ", ignoring.");
		}
		return null;
	    }
	}
	
	URL location;

	if (ge.codeBase != null)
	    location = new URL(ge.codeBase);
	else
	    location = null;
	
	return (canonicalizeCodebase(new CodeSource(location, certs),
				     false));
    }

    /**
     * Add one policy entry to the vector. 
     */
    private void addGrantEntry(PolicyParser.GrantEntry ge,
			       Object keyStore) {

	if (debug != null) {
	    debug.println("Adding policy entry: ");
	    debug.println("  signedBy " + ge.signedBy);
	    debug.println("  codeBase " + ge.codeBase);
	    debug.println();
	}

	try {
	    CodeSource codesource = getCodeSource(ge, null);
	    // skip if signedBy alias was unknown...
	    if (codesource == null) return;

	    PolicyEntry entry = new PolicyEntry(codesource);

	    Enumeration enum_ = ge.permissionElements();
	    while (enum_.hasMoreElements()) {
		PolicyParser.PermissionEntry pe =
		    (PolicyParser.PermissionEntry) enum_.nextElement();
		try { 
		    Permission perm = getInstance(pe.permission,
							 pe.name,
							 pe.action);
		    entry.add(perm);
		    if (debug != null) {
			debug.println("  "+perm);
		    }
		} catch (ClassNotFoundException cnfe) {
		    Certificate certs[];
		    if (pe.signedBy != null) 
			certs = getCertificates(null, pe.signedBy);
		    else 
			certs = null;

		    // only add if we had no signer or we had a
		    // a signer and found the keys for it.
		    if (certs != null || pe.signedBy == null) {
			    Permission perm = new UnresolvedPermission(
					     pe.permission,
					     pe.name,
					     pe.action,
					     certs);
			    entry.add(perm);
			    if (debug != null) {
				debug.println("  "+perm);
			    }
		    }
		} catch (java.lang.reflect.InvocationTargetException ite) {
		    System.err.println(
			   "java.security.Policy: error adding Permission " +
			       pe.permission + " "+ ite.getTargetException());
		} catch (Exception e) {
		    System.err.println(
			   "java.security.Policy: error adding Permission " +
				       pe.permission + " "+ e);
		}
	    }
	    policyEntries.addElement(entry);
	} catch (Exception e) {
	    System.err.println(
			  "java.security.Policy: error adding Entry " +
			  ge + " " +e);
	}

	if (debug != null)
	    debug.println();
    }

    /**
     * Returns a new Permission object of the given Type. The Permission is
     * created by getting the 
     * Class object using the <code>Class.forName</code> method, and using 
     * the reflection API to invoke the (String name, String actions) 
     * constructor on the
     * object.
     *
     * @param type the type of Permission being created.
     * @param name the name of the Permission being created.
     * @param actions the actions of the Permission being created.
     *
     * @exception  ClassNotFoundException  if the particular Permission
     *             class could not be found.
     *
     * @exception  IllegalAccessException  if the class or initializer is
     *               not accessible.
     *
     * @exception  InstantiationException  if getInstance tries to
     *               instantiate an abstract class or an interface, or if the
     *               instantiation fails for some other reason.
     *
     * @exception  NoSuchMethodException if the (String, String) constructor
     *               is not found.
     *
     * @exception  InvocationTargetException if the underlying Permission 
     *               constructor throws an exception.
     *               
     */

    private static final Permission getInstance(String type,
				    String name,
				    String actions)
	throws ClassNotFoundException,
	       InstantiationException,
	       IllegalAccessException,
	       NoSuchMethodException,
	       InvocationTargetException
    {
	//we might want to keep a hash of created factories...
	Class pc = Class.forName(type);
	Constructor c = pc.getConstructor(PARAMS);
	return (Permission) c.newInstance(new Object[] { name, actions });
    }

    /**
     * Fetch all certs associated with this alias. 
     */
    Certificate[] getCertificates(
				    Object keyStore, String aliases) {

	Vector vcerts = null;

	StringTokenizer st = new StringTokenizer(aliases, ",");
	int n = 0;

	while (st.hasMoreTokens()) {
	    String alias = st.nextToken().trim();
	    n++;
	    Certificate cert = null;
	    //See if this alias's cert has already been cached
	    cert = (Certificate) aliasMapping.get(alias);
	    if (cert == null && keyStore != null) {

		if (cert != null) {
		    aliasMapping.put(alias, cert);
		    aliasMapping.put(cert, alias);
		} 
	    }

	    if (cert != null) {
		if (vcerts == null)
		    vcerts = new Vector();
		vcerts.addElement(cert);
	    }
	}

	// make sure n == vcerts.size, since we are doing a logical *and*
	if (vcerts != null && n == vcerts.size()) {
	    Certificate[] certs = new Certificate[vcerts.size()];
	    vcerts.copyInto(certs);
	    return certs;
	} else {
	    return null;
	}
    }

    /**
     * Enumerate all the entries in the global policy object. 
     * This method is used by policy admin tools.   The tools
     * should use the Enumeration methods on the returned object
     * to fetch the elements sequentially. 
     */
    private final synchronized Enumeration elements(){
	return policyEntries.elements();
    }

    /**
     * Examines the global policy for the specified CodeSource, and
     * creates a PermissionCollection object with
     * the set of permissions for that principal's protection domain.
     *
     * @param CodeSource the codesource associated with the caller.
     * This encapsulates the original location of the code (where the code
     * came from) and the public key(s) of its signer.
     *
     * @return the set of permissions according to the policy.  
     */
    public PermissionCollection getPermissions(CodeSource codesource) {
 	if (initialized)
 	    return getPermissions(new Permissions(), codesource);
 	else
 	    return new PolicyPermissions(this, codesource);
    }

    /**
     * Examines the global policy for the specified CodeSource, and
     * creates a PermissionCollection object with
     * the set of permissions for that principal's protection domain.
     *
     * @param permissions the permissions to populate
     * @param codesource the codesource associated with the caller.
     * This encapsulates the original location of the code (where the code
     * came from) and the public key(s) of its signer.
     *
     * @return the set of permissions according to the policy.  
     */
    public Permissions getPermissions(final Permissions perms,
			       final CodeSource cs)
    {
	if (!initialized) {
	    init();
	}

	final CodeSource codesource[] = {null};

	AccessController.doPrivileged(new java.security.PrivilegedAction() {
	    public Object run() {
		int i, j;
		codesource[0] = canonicalizeCodebase(cs, true);

		if (debug != null) {
		    debug.println("evaluate("+codesource[0]+")");
		}
	    
		// needs to be in a begin/endPrivileged block because
		// codesource.implies calls URL.equals which does an
		// InetAddress lookup

		for (i = 0; i < policyEntries.size(); i++) {

		   PolicyEntry entry = (PolicyEntry)policyEntries.elementAt(i);
		   if (entry.codesource.implies(codesource[0])) {
		       for (j = 0; j < entry.permissions.size(); j++) {
			   Permission p = 
			       (Permission) entry.permissions.elementAt(j);
			   if (debug != null) {
			       debug.println("  granting " + p);
			   }
			   perms.add(p);
		       }
		   }
		}
		return null;
	    }
	});

	// J2ME CDC reductions: remove deprecated 1.1 security model support.

	return perms;
    }

    /*
     * Returns the signer certificates from the list of certificates associated
     * with the given code source.
     *
     * The signer certificates are those certificates that were used to verify
     * signed code originating from the codesource location.
     *
     * This method assumes that in the given code source, each signer
     * certificate is followed by its supporting certificate chain
     * (which may be empty), and that the signer certificate and its
     * supporting certificate chain are ordered bottom-to-top (i.e., with the
     * signer certificate first and the (root) certificate authority last).
     */
    protected Certificate[] getSignerCertificates(CodeSource cs) {
	Certificate[] certs = null;
	if ((certs = cs.getCertificates()) == null)
	    return null;
	// J2ME CDC reduction: Don't check for X509Certificate

	// J2ME CDC reduction: Skip counting certs

	ArrayList userCertList = new ArrayList();

	// J2ME CDC reduction: Skip building array list of certs

	Certificate[] userCerts = new Certificate[userCertList.size()];
	userCertList.toArray(userCerts);
	return userCerts;
    }

    private CodeSource canonicalizeCodebase(CodeSource cs,
					    boolean extractSignerCerts) {
	CodeSource canonCs = cs;
	if (cs.getLocation() != null &&
	    cs.getLocation().getProtocol().equalsIgnoreCase("file")) {
	    try {
		String path = cs.getLocation().getFile().replace
							('/',
							File.separatorChar);
		path = ParseUtil.decode(path);
		URL csUrl = null;
		if (path.endsWith("*")) {
		    // remove trailing '*' because it causes canonicalization
		    // to fail on win32
		    path = path.substring(0, path.length()-1);
		    boolean appendFileSep = false;
		    if (path.endsWith(File.separator))
			appendFileSep = true;
		    if (path.equals("")) {
			path = (String)
			    java.security.AccessController.doPrivileged(
			    new sun.security.action.GetPropertyAction(
								 "user.dir"));
		    }
		    File f = new File(path);
		    path = f.getCanonicalPath();
		    StringBuffer sb = new StringBuffer(path);
		    // reappend '*' to canonicalized filename (note that
		    // canonicalization may have removed trailing file
		    // separator, so we have to check for that, too)
		    if (!path.endsWith(File.separator) &&
			(appendFileSep || f.isDirectory()))
			sb.append(File.separatorChar);
		    sb.append('*');
		    path = sb.toString();
		} else {
		    path = new File(path).getCanonicalPath();
		}
		csUrl = ParseUtil.fileToEncodedURL(new File(path));

		if (extractSignerCerts) {
		    canonCs = new CodeSource(csUrl, getSignerCertificates(cs));
		} else {
		    canonCs = new CodeSource(csUrl, cs.getCertificates());
		}
	    } catch (IOException ioe) {
		// leave codesource as it is, unless we have to extract its
		// signer certificates
		if (extractSignerCerts) {
		    canonCs = new CodeSource(cs.getLocation(),
					     getSignerCertificates(cs));
		}
	    }
	} else {
	    if (extractSignerCerts) {
		canonCs = new CodeSource(cs.getLocation(),
					 getSignerCertificates(cs));
	    }
	}
	return canonCs;
    }

    /**
     * Each entry in the policy configuration file is represented by a
     * PolicyEntry object.  <p>
     *
     * A PolicyEntry is a (CodeSource,Permission) pair.  The
     * CodeSource contains the (URL, PublicKey) that together identify
     * where the Java bytecodes come from and who (if anyone) signed
     * them.  The URL could refer to localhost.  The URL could also be
     * null, meaning that this policy entry is given to all comers, as
     * long as they match the signer field.  The signer could be null,
     * meaning the code is not signed. <p>
     * 
     * The Permission contains the (Type, Name, Action) triplet. <p>
     * 
     * For now, the Policy object retrieves the public key from the
     * X.509 certificate on disk that corresponds to the signedBy
     * alias specified in the Policy config file.  For reasons of
     * efficiency, the Policy object keeps a hashtable of certs already
     * read in.  This could be replaced by a secure internal key
     * store.
     * 
     * <p>
     * For example, the entry
     * <pre>
     * 		permission java.io.File "/tmp", "read,write",
     *		signedBy "Duke";	
     * </pre>
     * is represented internally 
     * <pre>
     * 
     * FilePermission f = new FilePermission("/tmp", "read,write");
     * PublicKey p = publickeys.get("Duke");
     * URL u = InetAddress.getLocalHost();
     * CodeBase c = new CodeBase( p, u );
     * pe = new PolicyEntry(f, c);
     * </pre>
     * 
     * @author Marianne Mueller
     * @author Roland Schemers
     * @version 1.6, 03/04/97
     * @see java.security.CodeSource
     * @see java.security.Policy
     * @see java.security.Permissions
     * @see java.security.ProtectionDomain
     */

    private static class PolicyEntry {

	CodeSource codesource;
	Vector permissions;

	/**
	 * Given a Permission and a CodeSource, create a policy entry.
	 * 
	 * TODO: Decide if/how to add validity fields and "purpose" fields to
	 *       policy entries 
	 * 
	 * @param cs the CodeSource, which encapsulates the URL and the public
	 *        key
	 *        attributes from the policy config file.   Validity checks are
	 *        performed on the public key before PolicyEntry is called. 
	 * 
	 */
	PolicyEntry(CodeSource cs)
	{
	    this.codesource = cs;
	    this.permissions = new Vector();
	}

	/**
	 * add a Permission object to this entry.
	 */
	void add(Permission p) {
	    permissions.addElement(p);
	}

	/**
	 * Return the CodeSource for this policy entry
	 */
	CodeSource getCodeSource() {
	    return this.codesource;
	}

	public String toString(){
	    StringBuffer sb = new StringBuffer();
	    sb.append("(");
	    sb.append(getCodeSource());
	    sb.append("\n");
	    for (int j = 0; j < permissions.size(); j++) {
		Permission p = (Permission) permissions.elementAt(j);
		sb.append("  ");
		sb.append(p);
		sb.append("\n");
	    }
	    sb.append(")\n");
	    return sb.toString();
	}

    }
}

class PolicyPermissions extends PermissionCollection {

    private CodeSource codesource;
    private Permissions perms;
    private PolicyFile policy;
    private boolean notInit; // have we pulled in the policy permissions yet?
    private Vector additionalPerms;

    PolicyPermissions(PolicyFile policy,
		      CodeSource codesource)
    {
	this.codesource = codesource;
	this.policy = policy;
	this.perms = null;
	this.notInit = true;
	this.additionalPerms = null;
    }

    public void add(Permission permission) {
	if (isReadOnly())
	    throw new SecurityException(
            "attempt to add a Permission to a readonly PermissionCollection");

	if (perms == null) {
	    if (additionalPerms == null)
		additionalPerms = new Vector();
	    additionalPerms.add(permission);
	} else {
	    perms.add(permission);
	}
    }

    private synchronized void init() {
	if (notInit) {
	    if (perms == null)
		perms = new Permissions();

	    // optimization to put the most likely to be
	    // used permissions first (4301064)
	    policy.getPermissions(perms,codesource);
	    if (additionalPerms != null) {
		Enumeration e = additionalPerms.elements();
		while (e.hasMoreElements()) {
		    perms.add((Permission)e.nextElement());
		}
		additionalPerms = null;
	    }
	    notInit=false;
	}
    }

    public boolean implies(Permission permission) {
	if (notInit)
	    init();
	return perms.implies(permission);
    }

    public Enumeration elements() {
	if (notInit)
	    init();
	return perms.elements();
    }

    public String toString() {
	if (notInit)
	    init();
	return perms.toString();
    }
}
