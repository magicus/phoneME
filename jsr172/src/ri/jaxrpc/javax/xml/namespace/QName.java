/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.xml.namespace;

/** 
 * <p><code>QName</code> represents a <strong>qualified name</strong>
 * as defined in the XML specifications: <a
 * href="http://www.w3.org/TR/xmlschema-2/#QName">XML Schema Part2:
 * Datatypes specification</a>, <a
 * href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">Namespaces
 * in XML</a>, <a
 * href="http://www.w3.org/XML/xml-names-19990114-errata">Namespaces
 * in XML Errata</a>.</p>
 *
 * <p>The value of a <code>QName</code> contains a <strong>Namespace
 * URI</strong>, <strong>local part</strong> and
 * <strong>prefix</strong>.</p>
 *
 * <p>The prefix is included in <code>QName</code> to retain lexical
 * information <strong><em>when present</em></strong> in an {@link
 * javax.xml.transform.Source XML input source}. The prefix is
 * <strong><em>NOT</em></strong> used in {@link #equals(Object)
 * QName.equals(Object)} or to compute the {@link #hashCode()
 * QName.hashCode()}.  Equality and the hash code are defined using
 * only the Namespace URI and local part.</p>
 *
 * <p>If not specified, the Namespace URI is set to "" (the empty string).
 * If not specified, the prefix is set to "" (the empty string).</p>
 *
 * <p><code>QName</code> is immutable.</p>
 *
 * @version 1.1
 * @see <a href="http://www.w3.org/TR/xmlschema-2/#QName">XML Schema
 * Part2: Datatypes specification</a>
 * @see <a
 * href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">Namespaces
 * in XML</a>
 * @see <a
 * href="http://www.w3.org/XML/xml-names-19990114-errata">Namespaces
 * in XML Errata</a>
 */

public class QName {

    /**
     * <p>Namespace URI of this <code>QName</code>.</p>
     */
    private final String namespaceURI;

    /**
     * <p>local part of this <code>QName</code>.</p>
     */
    private final String localPart;

    /**
     * <p>prefix of this <code>QName</code>.</p>
     */
    private final String prefix;

    /** 
     * <p><code>QName</code> constructor specifying the Namespace URI
     * and local part.</p>
     *
     * <p>If the Namespace URI is <code>null</code>, it is set to "".
     * This value represents no
     * explicitly defined Namespace as defined by the <a
     * href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">Namespaces
     * in XML</a> specification.  This action preserves compatible
     * behavior with QName 1.0.</p>
     *
     * <p>If the local part is <code>null</code>, an
     * <code>IllegalArgumentException</code> is thrown.</p>
     *
     * <p>When using this constructor, the prefix is set to "".</p>
     *
     * @param namespaceURI Namespace URI of the <code>QName</code>
     * @param localPart    local part of the <code>QName</code>
     * @see #QName(String namespaceURI, String localPart, String
     * prefix) QName(String namespaceURI, String localPart, String
     * prefix)
     */
    public QName(String namespaceURI, String localPart) {
        this(namespaceURI, localPart, "");
    }

    /** 
     * <p><code>QName</code> constructor specifying the Namespace URI,
     * local part and prefix.</p>
     *
     * <p>If the Namespace URI is <code>null</code>, it is set to "".
     * This value represents no
     * explicitly defined Namespace as defined by the <a
     * href="http://www.w3.org/TR/REC-xml-names/#ns-qualnames">Namespaces
     * in XML</a> specification.  This action preserves compatible
     * behavior with QName 1.0.</p>
     *
     * <p>If the local part is <code>null</code>, an
     * <code>IllegalArgumentException</code> is thrown.</p>
     *
     * <p>If the prefix is <code>null</code>, an
     * <code>IllegalArgumentException</code> is thrown.  Use ""
     * to explicitly indicate that no
     * prefix is present or the prefix is not relevant.</p>
     *
     * @param namespaceURI Namespace URI of the <code>QName<code>
     * @param localPart    local part of the <code>QName<code>
     * @param prefix       prefix of the <code>QName<code>
     */
    public QName(String namespaceURI, String localPart, String prefix) {
        if (namespaceURI == null) {
            this.namespaceURI = "";
        } else {
            this.namespaceURI = namespaceURI;
        }
        
        if (localPart == null) {
            throw new IllegalArgumentException(
                "local part cannot be \"null\" when creating a QName");
        }
        this.localPart = localPart;
        
        if (prefix == null) {
            throw new IllegalArgumentException(
                "prefix cannot be \"null\" when creating a QName");
        }
        this.prefix = prefix;
    }

    /** 
     * <p><code>QName</code> constructor specifying the local part.</p>
     *
     * <p>If the local part is <code>null</code> or
     * <code>.equals("")</code>, an
     * <code>IllegalArgumentException</code> is thrown.</p>
     *
     * <p>When using this constructor, the Namespace URI is set to ""
     * and the prefix is set to "".</p>
     *
     * <p><em>In an XML context, all Element and Attribute names exist
     * in the context of a Namespace.  Making this explicit during the
     * construction of a <code>QName</code> helps to prevent hard to
     * diagnosis XML validity errors.  The constructors {@link
     * #QName(String namespaceURI, String localPart) QName(String
     * namespaceURI, String localPart)} and {@link #QName(String
     * namespaceURI, String localPart, String prefix) QName(String
     * namespaceURI, String localPart, String prefix)} are
     * preferred.</em></p>
     *
     * @param localPart local part of the <code>QName</code>
     * @see #QName(String namespaceURI, String localPart) QName(String
     * namespaceURI, String localPart)
     * @see #QName(String namespaceURI, String localPart, String
     * prefix) QName(String namespaceURI, String localPart, String
     * prefix)
     */
    public QName(String localPart) {
        this("",
             localPart,
             "");
    }

    /** 
     * <p>Get the Namespace URI of this <code>QName</code>.</p>
     *
     * @return Namespace URI of this <code>QName</code>
     */
    public String getNamespaceURI() {
        return namespaceURI;
    }

    /**
     * <p>Get the local part of this <code>QName</code>.</p>
     *
     *  @return local part of this <code>QName</code>
     */
    public String getLocalPart() {
        return localPart;
    }

    /** 
     * <p>Get the prefix of this <code>QName</code>.</p>
     *
     * <p>The prefix assigned to a <code>QName</code> may
     * <strong><em>NOT</em></strong> be valid in a different
     * context. For example, a <code>QName</code> may be assigned a
     * prefix in the context of parsing a document but that prefix may
     * be invalid in the context of a different document.</p>
     *
     *  @return prefix of this <code>QName</code>
     */
    public String getPrefix() {
        return prefix;
    }
   
    /**
     * <p>Test this <code>QName</code> for equality with another
     * <code>Object</code>.</p>
     *
     * <p>If the <code>Object</code> to be tested is not a
     * <code>QName</code> or is <code>null</code>, then this method
     * returns <code>false</code>.</p>
     *
     * <p>Two <code>QName</code>s are considered equal if and only if
     * both the Namespace URI and local part are equal. This method
     * uses <code>String.equals()</code> to check equality of the
     * Namespace URI and local part. The prefix is
     * <strong><em>NOT</em></strong> used to determine equality.</p>
     *
     * <p>This method satisfies the general contract of {@link
     * java.lang.Object#equals(Object) Object.equals(Object)}</p>
     *
     * @param objectToTest the <code>Object</code> to test for
     * equality with this <code>QName</code>
     * @return <code>true</code> if the given <code>Object</code> is
     * equal to this <code>QName</code> else <code>false</code>
     */
    public boolean equals(Object objectToTest) {
        if (objectToTest == null || !(objectToTest instanceof QName)) {
            return false;
        }
    
        QName qName = (QName) objectToTest;
        
        return namespaceURI.equals(qName.namespaceURI)
            && localPart.equals(qName.localPart);
    }

    /**
     * <p>Generate the hash code for this <code>QName</code>.</p>
     *
     * <p>The hash code is calculated using both the Namespace URI and
     * the local part of the <code>QName</code>.  The prefix is
     * <strong><em>NOT</em></strong> used to calculate the hash
     * code.</p>
     *
     * <p>This method satisfies the general contract of {@link
     * java.lang.Object#hashCode() Object.hashCode()}.</p>
     *
     * @return hash code for this <code>QName</code> <code>Object</code>
     */
    public int hashCode() {
        return namespaceURI.hashCode() ^ localPart.hashCode();
    }

    /** 
     * <p><code>String</code> representation of this
     * <code>QName</code>.</p>
     *
     * <p><em>There is <strong>NO</strong> standard specification for
     * representing a <code>QName</code> as a <code>String</code>.
     * The returned <code>String</code> is not portable across
     * implementations and will change when a standard
     * <code>String</code> representation is defined.  This
     * implementation currently represents a <code>QName</code> as:
     * "{" + Namespace URI + "}" + local part.  If the Namespace URI
     * <code>.equals("")</code>, only the
     * local part is returned.  An appropriate use of this method is
     * for debugging or logging for human consumption.</em></p>
     *
     * <p>Note the prefix value is <strong><em>NOT</em></strong>
     * returned as part of the <code>String</code> representation.</p>
     *  
     * <p>This method satisfies the general contract of {@link
     * java.lang.Object#toString() Object.toString()}.</p>
     *
     *  @return <code>String</code> representation of this <code>QName</code>
     */
    public String toString() {
        if (namespaceURI.equals("")) {
            return localPart;
        } else {
            return "{" + namespaceURI + "}" + localPart;
        }
    }

    /** 
     * <p><code>QName</code> derived from parsing the formatted
     * <code>String</code>.</p>
     *
     * <p>If the <code>String</code> is <code>null</code>
     * or does not conform to {@link #toString() QName.toString()} formatting,
     * an <code>IllegalArgumentException</code> is thrown.</p>
     *  
     * <p><em>The <code>String</code> <strong>MUST</strong> be in the
     * form returned by {@link #toString() QName.toString()}. There is
     * <strong>NO</strong> standard specification for representing a
     * <code>QName</code> as a <code>String</code>.  The
     * <code>String</code> format is <strong>NOT</strong> portable
     * across implementations and will change when a standard
     * <code>String</code> representation is defined.  This
     * implementation currently parses a <code>String</code> formatted
     * as: "{" + Namespace URI + "}" + local part.  If the Namespace
     * URI <code>.equals("")</code>, only the
     * local part should be provided.</em></p>
     *
     * <p>The prefix value <strong><em>CANNOT</em></strong> be
     * represented in the <code>String</code> and will be set to
     * ""</p>
     *
     * <p>This method does not do full validation of the resulting
     * <code>QName</code>.  In particular, the local part is not
     * validated as a <a
     * href="http://www.w3.org/TR/REC-xml-names/#NT-NCName">NCName</a>
     * as specified in <a
     * href="http://www.w3.org/TR/REC-xml-names/">Namespaces in
     * XML</a>.</p>
     *
     * @param qNameAsString <code>String</code> representation
     * of the <code>QName</code>
     * @return <code>QName</code> corresponding to the given <code>String</code>
     * @see #toString() QName.toString()
     */
    public static QName valueOf(String qNameAsString) {
        if (qNameAsString == null) {
            throw new IllegalArgumentException(
              "cannot create QName from \"null\"");
        }

        // added this in 1.1 so that valueOf() can read back any QName
        // serialized using toString()
        if (qNameAsString.length() == 0) {
            return new QName("");
        }

        // local part only?
        if (qNameAsString.charAt(0) != '{') {
            return new QName("",
                             qNameAsString,
                             "");
        }

        // specifies Namespace URI and local part
        int endOfNamespaceURI = qNameAsString.indexOf('}');
        if (endOfNamespaceURI == -1) {
            throw new IllegalArgumentException(
                "cannot create QName from \""
                + qNameAsString + "\", missing closing \"}\"");
        }
        if (endOfNamespaceURI == qNameAsString.length() - 1) {
            throw new IllegalArgumentException(
                "cannot create QName from \""
                + qNameAsString + "\", missing local part");
        }
        return new QName(qNameAsString.substring(1, endOfNamespaceURI),
                         qNameAsString.substring(endOfNamespaceURI + 1),
                         "");
    }
}
