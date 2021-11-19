/*
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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

package javax.xml;

/**
 * Utility class to contain basic XML values as constants.
 * 
 * @author <a href="http://jcp.org/">JAXP Java Community Process</a>
 * @author <a href="http://java.sun.com/">JAXP Reference Implementation</a>
 * @version 1.0.proposed
 * @see <a href="http://www.w3.org/TR/REC-xml"> Extensible Markup Language (XML)
 *      1.0 (Second Edition)</a>
 * @see <a href="http://www.w3.org/TR/REC-xml-names"> Namespaces in XML</a>
 * @see <a href="http://www.w3.org/XML/xml-names-19990114-errata"> Namespaces in
 *      XML Errata</a>
 */

public class XMLConstants {

    /**
     * Constructor to prevent instantiation.
     */
    private XMLConstants() {
    }

    /**
     * Prefix to use to represent the default XML Namespace.
     * 
     * <p>
     * Defined by the XML specification to be "".
     * </p>
     * 
     * @see <a href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">
     *      Namespaces in XML</a>
     */
    public static final String DEFAULT_NS_PREFIX = "";

    /**
     * The official XML Namespace prefix.
     * 
     * <p>
     * Defined by the XML specification to be "<code>xml</code>".
     * </p>
     * 
     * @see <a href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">
     *      Namespaces in XML</a>
     */
    public static final String XML_NS_PREFIX = "xml";

    /**
     * The official XML Namespace name URI.
     * 
     * <p>
     * Defined by the XML specification to be "<code>http://www.w3.org/XML/1998/namespace</code>".
     * </p>
     * 
     * @see <a href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">
     *      Namespaces in XML</a>
     */
    public static final String XML_NS_URI = "http://www.w3.org/XML/1998/namespace";

    /**
     * The official XML attribute used for specifying XML Namespace
     * declarations.
     * 
     * <p>
     * It is <strong>not</strong> valid to use as a prefix. Defined by the XML
     * specification to be "<code>xmlns</code>".
     * </p>
     * 
     * @see <a href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">
     *      Namespaces in XML</a>
     */
    public static final String XMLNS_ATTRIBUTE = "xmlns";

    /**
     * The official XML attribute used for specifying XML Namespace
     * declarations, {@link #XMLNS_ATTRIBUTE "xmlns"}, Namespace name URI.
     * 
     * <p>
     * Defined by the XML specification to be "<code>http://www.w3.org/2000/xmlns/</code>".
     * </p>
     * 
     * @see <a href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">
     *      Namespaces in XML</a>
     * @see <a href="http://www.w3.org/XML/xml-names-19990114-errata/">
     *      Namespaces in XML Errata</a>
     */
    public static final String XMLNS_ATTRIBUTE_NS_URI = "http://www.w3.org/2000/xmlns/";

    /**
     * <p>Feature for secure processing.</p>
     *
     * <ul>
     *   <li>
     *     <code>true</code> instructs the implementation to process XML securely.
     *     This may set limits on XML constructs to avoid conditions such as denial of service attacks.
     *   </li>
     *   <li>
     *     <code>false</code> instructs the implementation to process XML according the letter of the XML specifications
     *     ignoring security issues such as limits on XML constructs to avoid conditions such as denial of service attacks.
     *   </li>
     * </ul>
     */
    public static final String FEATURE_SECURE_PROCESSING = "http://javax.xml.XMLConstants/feature/secure-processing";

}
