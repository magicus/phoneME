/*
 * @(#)URIName.java	1.17 06/10/10
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

package sun.security.x509;

import java.io.IOException;

import sun.security.util.*;

/**
 * This class implements the URIName as required by the GeneralNames
 * ASN.1 object.
 * <p>
 * [RFC2459] When the subjectAltName extension contains a URI, the name MUST be
 * stored in the uniformResourceIdentifier (an IA5String). The name MUST
 * be a non-relative URL, and MUST follow the URL syntax and encoding
 * rules specified in [RFC 1738].  The name must include both a scheme
 * (e.g., "http" or "ftp") and a scheme-specific-part.  The scheme-
 * specific-part must include a fully qualified domain name or IP
 * address as the host.
 * <p>
 * As specified in [RFC 1738], the scheme name is not case-sensitive
 * (e.g., "http" is equivalent to "HTTP").  The host part is also not
 * case-sensitive, but other components of the scheme-specific-part may
 * be case-sensitive. When comparing URIs, conforming implementations
 * MUST compare the scheme and host without regard to case, but assume
 * the remainder of the scheme-specific-part is case sensitive.
 * <p>
 * [RFC2459] Conforming implementations generating new certificates with
 * electronic mail addresses MUST use the rfc822Name in the subject
 * alternative name field (see sec. 4.2.1.7) to describe such
 * identities.  Simultaneous inclusion of the EmailAddress attribute in
 * the subject distinguished name to support legacy implementations is
 * deprecated but permitted.
 * <p>
 * [RFC1738] In general, URLs are written as follows:
 * <pre>
 * <scheme>:<scheme-specific-part>
 * </pre>
 * A URL contains the name of the scheme being used (<scheme>) followed
 * by a colon and then a string (the <scheme-specific-part>) whose
 * interpretation depends on the scheme.
 * <p>
 * While the syntax for the rest of the URL may vary depending on the
 * particular scheme selected, URL schemes that involve the direct use
 * of an IP-based protocol to a specified host on the Internet use a
 * common syntax for the scheme-specific data:
 * <pre>
 * //<user>:<password>@<host>:<port>/<url-path>
 * </pre>
 * [RFC2732] specifies that an IPv6 address contained inside a URL
 * must be enclosed in square brackets (to allow distinguishing the
 * colons that separate IPv6 components from the colons that separate
 * scheme-specific data.
 * <p>
 * @author Amit Kapoor
 * @author Hemma Prafullchandra
 * @version 1.9
 * @see GeneralName
 * @see GeneralNames
 * @see GeneralNameInterface
 */
public class URIName implements GeneralNameInterface {

    //private attributes
    private String name;
    private String scheme;
    private String host;
    private String remainder;
    private IPAddressName hostIP;
    private DNSName hostDNS;

    /**
     * Create the URIName object from the passed encoded Der value.
     *
     * @param derValue the encoded DER URIName.
     * @exception IOException on error.
     */
    public URIName(DerValue derValue) throws IOException {
        name = derValue.getIA5String();
	parseName();
    }

    /**
     * Create the URIName object with the specified name.
     *
     * @param name the URIName.
     * @throws IOException if name is not a proper URIName
     */
    public URIName(String name) throws IOException {
	if (name == null || name.length() == 0) {
	    throw new IOException("URI name must not be null");
	}
        this.name = name;
	parseName();
    }

    /**
     * parse the URIName into scheme, host, and remainder
     * Host includes only the fully-qualified domain name
     * or IP address.  Remainder includes everything except the
     * scheme and host.
     * <p>
     * Since RFC2459 specifies that email addresses must be specified
     * using emailaddress= attribute in X500Name or using RFC822Name,
     * we do not need to support the "mailto:user@company.com" scheme
     * that has no //.
     * <p>
     * Format:, where
     * <ul>
     *   <li>[] encloses optional elements
     *   <li>'[' and ']' are literal square brackets
     *   <li>{} encloses a choice
     *   <li>| separates elements in a choice
     *   <li>&lt;&gt; encloses an element
     * <p>
     * &lt;scheme&gt; :// [ &lt;username&gt; @ ] { '['&lt;IPv6 addr&gt;']' | &lt;IPv4 addr&gt; | &lt;DNSName&gt; } [: &lt;port&gt;] [ / [ &lt;directory&gt; ] ]
     * <p>
     * @throws IOException if name is not a proper URIName
     */
    private void parseName() throws IOException {
	//parse out <scheme>:
	int colonAfterSchemeNdx = name.indexOf(':');
	if (colonAfterSchemeNdx < 0)
	    throw new IOException("Name " + name + " does not include a <scheme>");

	//Verify presence of // immediately following <scheme>:
	int slashSlashNdx = name.indexOf("//", colonAfterSchemeNdx);
	if (slashSlashNdx != colonAfterSchemeNdx + 1)
	    throw new IOException("name does not include scheme-specific portion starting with host");

	//look for optional / preceding directory, first parsing past any ] enclosing an IPv6 address
	//(since an IPv6 address can end with  "/ <prefix length>")
	//This slash (end of name string) delimits end of scheme-specific portion
	int startSlashSearchNdx = slashSlashNdx + 2;
	if (startSlashSearchNdx == name.length())
	    throw new IOException("Name " + name + " doesn't include a <host>");  
	int rightSquareBracketNdx = name.indexOf(']', startSlashSearchNdx);
	if (rightSquareBracketNdx >= 0)
	    startSlashSearchNdx = rightSquareBracketNdx;
	int endSchemeSpecificNdx = name.indexOf('/', startSlashSearchNdx);
	if (endSchemeSpecificNdx < 0)
	    endSchemeSpecificNdx = name.length();

	//parse past any user:password portion
	int startHostNameNdx = name.indexOf('@', slashSlashNdx+2) + 1;
	if (startHostNameNdx <= 0 || startHostNameNdx >= endSchemeSpecificNdx)
	    startHostNameNdx = slashSlashNdx + 2;

	//parse to any :port portion, allowing for [] around IPv6 name
	int endHostNameNdx = -1;
	if (name.charAt(startHostNameNdx) == '[') {
	    endHostNameNdx = name.indexOf(']', startHostNameNdx);
	    if (endHostNameNdx < 0)
		throw new IOException("Invalid IPv6 address as host: missing ]");
	
	    if (endHostNameNdx < name.length() -1) {
		char nextChar = name.charAt(endHostNameNdx + 1);
		if (nextChar != ':' && nextChar != '/')
		    throw new IOException("Invalid host[:port][/] boundary");
		else
		    endHostNameNdx = endHostNameNdx + 1;
	    } else {
		endHostNameNdx = endSchemeSpecificNdx;
	    }
	} else {
	    endHostNameNdx = name.indexOf(':', startHostNameNdx);
	    if (endHostNameNdx < 0 || endHostNameNdx >= endSchemeSpecificNdx)
		endHostNameNdx = endSchemeSpecificNdx;
	}

	//extract scheme
	scheme = name.substring(0, colonAfterSchemeNdx);

	//extract host
	host = name.substring(startHostNameNdx, endHostNameNdx);
	
	if (host.length() > 0) {
	    //verify host portion is a valid IPAddress or DNS name
	    if (host.charAt(0) == '[') {
		//Verify host is a valid IPv6 address name
		String ipV6Host = host.substring(1, host.length()-1);
		try {
		    hostIP = new IPAddressName(ipV6Host);
		} catch (IOException ioe) {
		    throw new IOException
		    	("Host portion is not a valid IPv6 address: "
			+ ioe.getMessage());
		}
	    } else {
		try {
		    if (host.charAt(0) == '.') {
			hostDNS = new DNSName(host.substring(1));
		    } else
			hostDNS = new DNSName(host);
		} catch (IOException ioe) {
		    //Not a valid DNS Name; see if it is a valid IPv4 IPAddressName
		    try {
			hostIP = new IPAddressName(host);
		    } catch (Exception ioe2) {
			throw new IOException("Host portion is not a valid "
				+ "DNS name, IPv4 address, or IPv6 address");
		    }
		}
	    }
	}

	//piece together remainder
	remainder = name.substring(colonAfterSchemeNdx, startHostNameNdx);
	if (endHostNameNdx < name.length())
	    remainder += name.substring(endHostNameNdx);
    }

    /**
     * Return the type of the GeneralName.
     */
    public int getType() {
        return (GeneralNameInterface.NAME_URI);
    }

    /**
     * Encode the URI name into the DerOutputStream.
     *
     * @param out the DER stream to encode the URIName to.
     * @exception IOException on encoding errors.
     */
    public void encode(DerOutputStream out) throws IOException {
        out.putIA5String(name);
    }

    /**
     * Convert the name into user readable string.
     */
    public String toString() {
        return ("URIName: " + name);
    }

    /**
     * Compares this name with another, for equality.
     *
     * @return true iff the names are equivalent
     * according to RFC2459.
     */
    public boolean equals(Object obj) {
	if (this == obj)
	    return true;

	if (!(obj instanceof URIName))
	    return false;

	URIName other = (URIName)obj;

	if (name.equalsIgnoreCase(other.getName())) {

	    //Compare any remainders with case-sensitive compare

	    String otherRemainder = other.getRemainder();

	    if ((remainder == null) ^ (otherRemainder == null))
		return false;
	    
	    if (remainder != null && otherRemainder != null)
		return remainder.equals(otherRemainder);

	    // both are null
	    return true;

	} else {
	    return false;
	}
    }

    /**
     * Returns this URI name.
     */
    public String getName() {
        return name;
    }

    /**
     * return the scheme name portion of a URIName
     *
     * @param name full URIName
     * @returns scheme portion of full name
     */
    public String getScheme() {
	return scheme;
    }

    /**
     * return the host name or IP address portion of the URIName
     *
     * @param name full URIName
     * @returns host name or IP address portion of full name
     */
    public String getHost() {
	return host;
    }

    /**
     * return the host object type; if host name is a
     * DNSName, then this host object does not include any
     * initial "." on the name.
     *
     * @returns host name as DNSName or IPAddressName
     */
    public Object getHostObject() {
	if (hostIP != null)
	    return hostIP;
	else
	    return hostDNS;
    }

    /**
     * return the remainder (not scheme name or host part) of the URIName
     *
     * @param name full URIName
     * @returns remainder portion of full name
     */
    public String getRemainder() {
	return remainder;
    }

    /**
     * Returns the hash code value for this object.
     * 
     * @return a hash code value for this object.
     */
    public int hashCode() {
	return name.toUpperCase().hashCode();
    }

    /**
     * Return type of constraint inputName places on this name:<ul>
     *   <li>NAME_DIFF_TYPE = -1: input name is different type from name (i.e. does not constrain).
     *   <li>NAME_MATCH = 0: input name matches name.
     *   <li>NAME_NARROWS = 1: input name narrows name (is lower in the naming subtree)
     *   <li>NAME_WIDENS = 2: input name widens name (is higher in the naming subtree)
     *   <li>NAME_SAME_TYPE = 3: input name does not match or narrow name, but is same type.
     * </ul>.
     * These results are used in checking NameConstraints during
     * certification path verification.
     * <p>
     * RFC2459: For URIs, the constraint applies to the host part of the name. The
     * constraint may specify a host or a domain.  Examples would be
     * "foo.bar.com";  and ".xyz.com".  When the the constraint begins with
     * a period, it may be expanded with one or more subdomains.  That is,
     * the constraint ".xyz.com" is satisfied by both abc.xyz.com and
     * abc.def.xyz.com.  However, the constraint ".xyz.com" is not satisfied
     * by "xyz.com".  When the constraint does not begin with a period, it
     * specifies a host.
     * <p>
     * @param inputName to be checked for being constrained
     * @returns constraint type above
     * @throws UnsupportedOperationException if name is not exact match, but narrowing and widening are
     *          not supported for this name type.
     */
    public int constrains(GeneralNameInterface inputName) throws UnsupportedOperationException {
	int constraintType;
	if (inputName == null)
	    constraintType = NAME_DIFF_TYPE;
	else if (inputName.getType() != NAME_URI)
	    constraintType = NAME_DIFF_TYPE;
	else {
	    String otherScheme = ((URIName)inputName).getScheme();
	    String otherHost = ((URIName)inputName).getHost();

	    if (!(scheme.equalsIgnoreCase(otherScheme)))
		constraintType = NAME_SAME_TYPE;
	    else if (otherHost.equals(host))
		constraintType = NAME_MATCH;
	    else if ((((URIName)inputName).getHostObject() instanceof IPAddressName) &&
		     hostIP != null) {
		return hostIP.constrains((IPAddressName)((URIName)inputName).getHostObject());
	    } else {
		//At least one host is not an IPAddressName
		if (otherHost.charAt(0) == '.' || host.charAt(0) == '.') {
		    //DNSName constraint semantics specify subdomains without an initial
		    //period on the constraint.  URIName constraint semantics specify subdomains
		    //only when the constraint host name starts with a period.
		    DNSName hostDNS;
		    DNSName otherDNS;
		    try {
			if (host.charAt(0) == '.')
			    hostDNS = new DNSName(host.substring(1));
			else
			    hostDNS = new DNSName(host);
			if (otherHost.charAt(0) == '.')
			    otherDNS = new DNSName(otherHost.substring(1));
			else
			    otherDNS = new DNSName(otherHost);
			constraintType = hostDNS.constrains(otherDNS);
		    } catch (IOException ioe2) {
			constraintType = NAME_SAME_TYPE;
		    } catch (UnsupportedOperationException uoe) {
			//At least one of the hosts is not a valid DNS name
			constraintType = NAME_SAME_TYPE;
		    }
		} else {
		    constraintType = NAME_SAME_TYPE;
		}
	    }
	}
	return constraintType;
    }

    /**
     * Return subtree depth of this name for purposes of determining
     * NameConstraints minimum and maximum bounds and for calculating
     * path lengths in name subtrees.
     *
     * @returns distance of name from root
     * @throws UnsupportedOperationException if not supported for this name type
     */
    public int subtreeDepth() throws UnsupportedOperationException {
	DNSName dnsName = null;
	try {
	    dnsName = new DNSName(host);
	} catch (IOException ioe) {
	    throw new UnsupportedOperationException(ioe.getMessage());
	}
	int i = dnsName.subtreeDepth();
	return i;
    }

}
