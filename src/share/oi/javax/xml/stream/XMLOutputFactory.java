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

package javax.xml.stream;


/**
 * Defines an abstract implementation of a factory for getting XMLStreamWriters.
 *
 * The following table defines the standard properties of this specification.  
 * Each property varies in the level of support required by each implementation.
 * The level of support required is described in the 'Required' column.
 *
 *     <table border="2" rules="all" cellpadding="4" width=100%>
 *      <tr>
 *        <th align="center" colspan="2">
 *          Configuration parameters
 *        </th>
 *      </tr>
 *      <tr>
 *        <th width=38%>Property Name</th>
 *        <th width=24%>Behavior</th>
 *        <th width=12%>Return type</th>
 *        <th width=13%>Default Value</th>
 *        <th width=13%>Required</th>
 *       </tr>
 *       <tr>
 *         <td>javax.xml.stream.isRepairingNamespaces</td>
 *         <td>defaults prefixes on the output side</td>
 *         <td>Boolean</td><td>False</td><td>Yes</td>
 *       </tr>
 *   </table>
 *
 * <p>The following paragraphs describe the namespace and prefix repair 
 * algorithm:</p>
 *
 * <p>The property can be set with the following code line:
 * <code>setProperty("javax.xml.stream.isRepairingNamespaces",new Boolean(true|false));</code>
 * </p>
 * 
 * <p>If a writer isRepairingNamespaces it will create a namespace declaration
 * on the current StartElement for any attribute that does not 
 * currently have a namespace declaration in scope.  If the StartElement
 * has a uri but no prefix specified a prefix will be assigned: if the prefix
 * has not been declared in a parent of the current StartElement it will be 
 * declared on the current StartElement.  If the default namespace is bound and 
 * in scope and the default namespace value matches the URI of the attribute
 * or of the StartElement's qualified name, then no prefix will be assigned.</p>
 *
 * <p>If an element or attribute name has a prefix, but is not  bound to any 
 * namespace URI, then the prefix will be removed during serialization.</p> 
 *
 * <p>If element and/or attribute names in the same start or 
 * empty-element tag are bound to different namespace URIs and 
 * are using the same prefix then the element or the first 
 * occurring attribute retains the original prefix and the 
 * following attributes have their prefixes replaced with a 
 * new prefix that is bound to the namespace URIs of those 
 * attributes. </p>
 *
 * <p>If an element or attribute name uses a prefix that is 
 * bound to a different URI than that inherited from the 
 * namespace context of the parent of that element and there 
 * is no namespace declaration in the context of the current 
 * element then such a namespace declaration is added. </p>
 *
 * <p>If an element or attribute name is bound to a prefix and 
 * there is a namespace declaration that binds that prefix 
 * to a different URI then that namespace declaration is 
 * either removed if the correct mapping is inherited from 
 * the parent context of that element, or changed to the 
 * namespace URI of the element or attribute using that prefix.</p> 
 *
 * @version 1.0 
 * @author Copyright (c) 2003 by BEA Systems. All Rights Reserved.
 * @see XMLInputFactory
 * @see XMLStreamWriter
 */
public abstract class XMLOutputFactory {

  /** The default system property name */
  private static final String DEFAULT_SYSPROP_NAME = 
      "javax.xml.stream.XMLOutputFactory";

  /** 
   * Property used to set prefix defaulting on the output side 
   */
  public static final String IS_REPAIRING_NAMESPACES=
      "javax.xml.stream.isRepairingNamespaces";

  protected XMLOutputFactory(){}

  /**
   * Create a new instance of the factory.
   * This static method creates a new factory instance. 
   * This method uses the following ordered lookup procedure to determine 
   * the XMLOutputFactory implementation class to load: 
   * Use the javax.xml.stream.XMLOutputFactory system property. 
   * Platform default XMLOutputFactory instance. 
   * @throws FactoryConfigurationError if an instance of this factory cannot be 
   *   loaded
   */
  public static XMLOutputFactory newInstance() 
      throws FactoryConfigurationError
  {
        try {
            String fname = System.getProperty(DEFAULT_SYSPROP_NAME);
            if(fname == null)
                fname = "com.sun.ukit.xml.XMLOutputFactoryImp";
            return (XMLOutputFactory) Class.forName(fname).newInstance();
        } catch (java.lang.Throwable ex) {
            throw new FactoryConfigurationError(ex.toString());
        }
  }

  /**
   * Create a new XMLStreamWriter that writes to a writer
   * @param stream the writer to write to
   * @throws XMLStreamException
   */
  public abstract XMLStreamWriter createXMLStreamWriter(java.io.Writer stream)
      throws XMLStreamException;

  /**
   * Create a new XMLStreamWriter that writes to a stream
   * @param stream the stream to write to
   * @throws XMLStreamException
   */
  public abstract XMLStreamWriter createXMLStreamWriter(
          java.io.OutputStream stream)
      throws XMLStreamException;

  /**
   * Create a new XMLStreamWriter that writes to a stream
   * @param stream the stream to write to
   * @param encoding the encoding to use
   * @throws XMLStreamException
   */
  public abstract XMLStreamWriter createXMLStreamWriter(
          java.io.OutputStream stream, String encoding)
      throws XMLStreamException;

  /**
   * Allows the user to set specific features/properties on the underlying 
   * implementation. 
   * @param name The name of the property
   * @param value The value of the property
   * @throws java.lang.IllegalArgumentException if the property is not
   * supported
   */
  public abstract void setProperty(java.lang.String name, Object value) 
    throws IllegalArgumentException;

  /**
   * Get a feature/property on the underlying implementation
   * @param name The name of the property
   * @return The value of the property
   * @throws java.lang.IllegalArgumentException if the property is not supported
   */
  public abstract Object getProperty(java.lang.String name)
    throws IllegalArgumentException;

  /**
   * Query the set of properties that this factory supports.
   *
   * @param name The name of the property (may not be null)
   * @return <code>true</code> if the property is supported and 
   *   <code>false</code> otherwise
   */
  public abstract boolean isPropertySupported(String name);
}
