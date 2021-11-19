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
 */


package javax.xml.parsers;

/**
 * Defines a factory API that enables applications to obtain a
 * parser that produces DOM object trees from XML documents.
 */

public abstract class DocumentBuilderFactory 
{

    /** The default property name according to the JAXP spec */
    private static final String DEFAULT_PROPERTY_NAME = 
        "javax.xml.parsers.DocumentBuilderFactory";

    private boolean validating = false;
    private boolean namespaceAware = false;
    private boolean whitespace = false;
    private boolean expandEntityRef = true;
    private boolean ignoreComments = false;
    private boolean coalescing = false;
    
    protected DocumentBuilderFactory ()
    {
    }

    /**
     * Obtain a new instance of a
     * <code>DocumentBuilderFactory</code>. This static method creates
     * a new factory instance.
     * This method uses the following ordered lookup procedure to determine
     * the <code>DocumentBuilderFactory</code> implementation class to
     * load:
     * <ul>
     * <li>
     * Use the <code>javax.xml.parsers.DocumentBuilderFactory</code> system
     * property.
     * </li>
     * <li>
     * Platform default <code>DocumentBuilderFactory</code> instance.
     * </li>
     * </ul>
     *
     * Once an application has obtained a reference to a
     * <code>DocumentBuilderFactory</code> it can use the factory to
     * configure and obtain parser instances.
     * 
     * 
     * <h2>Tip for Trouble-shooting on CDC</h2>
     * <p>Setting the <code>jaxp.debug</code> system property will cause
     * this method to print a lot of debug messages
     * to <tt>System.err</tt> about what it is doing and where it is looking at.</p>
     * 
     * <p> If you have problems loading {@link DocumentBuilder}s, try:</p>
     * <pre>
     * java -Djaxp.debug=1 YourProgram ....
     * </pre>
     * 
     * @return New instance of a <code>DocumentBuilderFactory</code>
     *
     * @exception FactoryConfigurationError if the implementation is not
     * available or cannot be instantiated.
     */

    public static DocumentBuilderFactory newInstance()
    {
        try {
            String fname = System.getProperty(DEFAULT_PROPERTY_NAME);
            if(fname == null)
                fname = "com.sun.ukit.dom.DocBuilderFactory";
            return (DocumentBuilderFactory) Class.forName(fname).newInstance();
        } catch (java.lang.Throwable ex) {
            throw new FactoryConfigurationError(ex.toString());
        }
    }

    /**
     * Creates a new instance of a {@link javax.xml.parsers.DocumentBuilder}
     * using the currently configured parameters.
     *
     * @exception ParserConfigurationException if a DocumentBuilder
     * cannot be created which satisfies the configuration requested.
     * @return A new instance of a DocumentBuilder.
     */

    public abstract DocumentBuilder newDocumentBuilder()
        throws ParserConfigurationException;


    /**
     * Specifies that the parser produced by this code will
     * provide support for XML namespaces. By default the value of this is set
     * to <code>false</code>
     *
     * @param awareness <code>true</code> if the parser produced will provide support
     *                  for XML namespaces; <code>false</code> otherwise.
     */

    public void setNamespaceAware(boolean awareness)
    {
        this.namespaceAware = awareness;
    }

    /**
     * Specifies that the parser produced by this code will
     * validate documents as they are parsed. By default the value of this
     * is set to <code>false</code>.
     * 
     * <p>
     * Note that "the validation" here means
     * <a href="http://www.w3.org/TR/REC-xml#proc-types"> a validating
     * parser</a> as defined in the XML recommendation.
     * In other words, it essentially just controls the DTD validation.
     * </p>
     * 
     * @param validating <code>true</code> if the parser produced will validate documents
     *                   as they are parsed; <code>false</code> otherwise.
     */

    public void setValidating(boolean validating)
    {
        this.validating = validating;
    }

    /**
     * Specifies that the parsers created by this  factory must eliminate
     * whitespace in element content (sometimes known loosely as
     * 'ignorable whitespace') when parsing XML documents (see XML Rec
     * 2.10). Note that only whitespace which is directly contained within
     * element content that has an element only content model (see XML
     * Rec 3.2.1) will be eliminated. Due to reliance on the content model
     * this setting requires the parser to be in validating mode. By default
     * the value of this is set to <code>false</code>.
     *
     * @param whitespace <code>true</code> if the parser created must eliminate whitespace
     *                   in the element content when parsing XML documents;
     *                   <code>false</code> otherwise.
     */

    public void setIgnoringElementContentWhitespace(boolean whitespace)
    {
        this.whitespace = whitespace;
    }

    /**
     * Specifies that the parser produced by this code will
     * expand entity reference nodes. By default the value of this is set to
     * <code>true</code>
     *
     * @param expandEntityRef <code>true</code> if the parser produced will expand entity
     *                        reference nodes; <code>false</code> otherwise.
     */

    public void setExpandEntityReferences(boolean expandEntityRef)
    {
        this.expandEntityRef = expandEntityRef;
    }

    /**
     * <p>Specifies that the parser produced by this code will
     * ignore comments. By default the value of this is set to <code>false
     * </code>.</p>
     * 
     * @param ignoreComments <code>boolean</code> value to ignore comments during processing
     */

    public void setIgnoringComments(boolean ignoreComments)
    {
        this.ignoreComments = ignoreComments;
    }

    /**
     * Specifies that the parser produced by this code will
     * convert CDATA nodes to Text nodes and append it to the
     * adjacent (if any) text node. By default the value of this is set to
     * <code>false</code>
     *
     * @param coalescing  <code>true</code> if the parser produced will convert CDATA nodes
     *                    to Text nodes and append it to the adjacent (if any)
     *                    text node; <code>false</code> otherwise.
     */
    
    public void setCoalescing(boolean coalescing)
    {
        this.coalescing = coalescing;
    }

    /**
     * Indicates whether or not the factory is configured to produce
     * parsers which are namespace aware.
     *
     * @return  <code>true</code> if the factory is configured to produce parsers which
     *          are namespace aware; <code>false</code> otherwise.
     */
    
    public boolean isNamespaceAware()
    {
        return namespaceAware;
    }

    /**
     * Indicates whether or not the factory is configured to produce
     * parsers which validate the XML content during parse.
     *
     * @return  <code>true</code> if the factory is configured to produce parsers
     *          which validate the XML content during parse; <code>false</code> otherwise.
     */

    public boolean isValidating() 
    {
        return validating;
    }

    /**
     * Indicates whether or not the factory is configured to produce
     * parsers which ignore ignorable whitespace in element content.
     *
     * @return  <code>true</code> if the factory is configured to produce parsers
     *          which ignore ignorable whitespace in element content;
     *          <code>false</code> otherwise.
     */

    public boolean isIgnoringElementContentWhitespace() 
    {
        return whitespace;
    }

    /**
     * Indicates whether or not the factory is configured to produce
     * parsers which expand entity reference nodes.
     *
     * @return  <code>true</code> if the factory is configured to produce parsers
     *          which expand entity reference nodes; <code>false</code> otherwise.
     */
    
    public boolean isExpandEntityReferences() 
    {
        return expandEntityRef;
    }

    /**
     * Indicates whether or not the factory is configured to produce
     * parsers which ignores comments.
     *
     * @return  <code>true</code> if the factory is configured to produce parsers
     *          which ignores comments; <code>false</code> otherwise.
     */

    public boolean isIgnoringComments() 
    {
        return ignoreComments;
    }

    /**
     * Indicates whether or not the factory is configured to produce
     * parsers which converts CDATA nodes to Text nodes and appends it to
     * the adjacent (if any) Text node.
     *
     * @return  <code>true</code> if the factory is configured to produce parsers
     *          which converts CDATA nodes to Text nodes and appends it to
     *          the adjacent (if any) Text node; <code>false</code> otherwise.
     */

    public boolean isCoalescing() 
    {
        return coalescing;
    }

    /**
     * Allows the user to set specific attributes on the underlying
     * implementation.
     * @param name The name of the attribute.
     * @param value The value of the attribute.
     * @exception IllegalArgumentException thrown if the underlying
     * implementation doesn't recognize the attribute.
     */

    public abstract void setAttribute(String name, Object value)
                throws IllegalArgumentException;

    /**
     * Allows the user to retrieve specific attributes on the underlying
     * implementation.
     * @param name The name of the attribute.
     * @return value The value of the attribute.
     * @exception IllegalArgumentException thrown if the underlying
     * implementation doesn't recognize the attribute.
     */

    public abstract Object getAttribute(String name)
                throws IllegalArgumentException;

    /**
     * <p>Set a feature for this <code>DocumentBuilderFactory</code> and 
     * <code>DocumentBuilder</code>s created by this factory.</p>
     * 
     * <p>
     * Feature names are fully qualified URIs.
     * Implementations may define their own features.
     * An {@link ParserConfigurationException} is thrown if this 
     * <code>DocumentBuilderFactory</code> or the
     * <code>DocumentBuilder</code>s it creates cannot support the feature.
     * It is possible for an <code>DocumentBuilderFactory</code> to expose
     * a feature value but be unable to change its state. A null
     * feature name parameter causes the XML reader to throw a 
     * <code>NullPointerException</code>. </p>
     * 
     * <p>
     * All implementations are required to support the 
     * {@link javax.xml.XMLConstants#FEATURE_SECURE_PROCESSING} feature.
     * When the feature is:</p>
     * <ul>
     *   <li>
     *     <code>true</code>: the implementation will limit XML processing to 
     *     conform to implementation limits. Examples include entity expansion 
     *     limits and XML Schema constructs that would consume large amounts of 
     *     resources. If XML processing is limited for security reasons, it will 
     *     be reported via a call to the registered
     *     {@link org.xml.sax.ErrorHandler#fatalError(SAXParseException exception)}.
     *     See {@link DocumentBuilder#setErrorHandler(org.xml.sax.ErrorHandler errorHandler)}.
     *   </li>
     *   <li>
     *     <code>false</code>: the implementation will processing XML according 
     *     to the XML specifications without regard to possible implementation 
     *     limits.
     *   </li>
     * </ul>
     * 
     * @param name Feature name.
     * @param value Is feature state <code>true</code> or <code>false</code>.
     *  
     * @throws ParserConfigurationException if this <code>DocumentBuilderFactory</code> 
     *   or the <code>DocumentBuilder</code>s it creates cannot support this 
     *   feature.
     * @throws NullPointerException If the <code>name</code> parameter is null.
     */

    public abstract void setFeature(String name, boolean value)
                throws ParserConfigurationException;

    /**
     * <p>Get the state of the named feature.</p>
     * 
     * <p>
     * Feature names are fully qualified URIs.
     * Implementations may define their own features.
     * A {@link ParserConfigurationException} is thrown if this 
     * <code>DocumentBuilderFactory</code> or the
     * <code>DocumentBuilder</code>s it creates cannot support the feature.
     * It is possible for an <code>DocumentBuilderFactory</code> to expose 
     * a feature value but be unable to change its state.
     * A <code>NullPointerException</code> is thrown if the feature name
     * parameter is null.
     * </p>
     * 
     * @param name Feature name.
     * 
     * @return State of the named feature.
     * 
     * @throws ParserConfigurationException if this 
     * <code>DocumentBuilderFactory</code> or the 
     * <code>DocumentBuilder</code>s it creates cannot support this feature.
     */

    public abstract boolean getFeature(String name)
                throws ParserConfigurationException;
}
