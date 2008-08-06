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
 * Defines an abstract implementation of a factory for getting streams.
 * 
 * The following table defines the standard properties of this specification.  
 * Each property varies in the level of support required by each implementation.
 * The level of support required is described in the 'Required' column.
 *
 * <table border="2" rules="all" cellpadding="4"  width=100%>
 * <tr>
 *        <th align="center" colspan="5">
 *          Configuration parameters
 *        </th>
 *      </tr>
 *      <tr>
 *        <th width=39%>Property Name</th>
 *        <th width=24%>Behavior</th>
 *        <th width=10%>Return type</th>
 *        <th width=13%>Default Value</th>
 *        <th width=14%>Required</th>
 * </tr>
 * <tr>
 *   <td>javax.xml.stream.isValidating</td>
 *   <td>Turns on/off implementation specific DTD validation</td>
 *   <td>Boolean</td><td>False</td>
 *   <td>No</td>
 * </tr>
 * <tr>
 *   <td>javax.xml.stream.isNamespaceAware</td>
 *   <td>Turns on/off namespace processing for XML 1.0 support</td>
 *   <td>Boolean</td>
 *   <td>True</td>
 *   <td>True (required) / False (optional)</td>
 * </tr>
 * <tr>
 *   <td>javax.xml.stream.isCoalescing</td>
 *   <td>Requires the processor to coalesce adjacent character data</td>
 *   <td>Boolean</td>
 *   <td>False</td>
 *   <td>Yes</td>
 * </tr>
 * <tr>
 *   <td>javax.xml.stream.isReplacingEntityReferences</td>
 *   <td>replace internal entity references with their replacement text and 
 *       report them as characters</td><td>Boolean</td>
 *   <td>True</td>
 *   <td>Yes</td>
 * </tr>
 * <tr>
 *   <td>javax.xml.stream.isSupportingExternalEntities</td>
 *   <td>Resolve external parsed entities</td>
 *   <td>Boolean</td>
 *   <td>Unspecified</td>
 *   <td>Yes</td>
 * </tr>
 * <tr>
 *   <td>javax.xml.stream.supportDTD</td>
 *   <td>Use this property to request processors that do not support DTDs</td>
 *   <td>Boolean</td><td>True</td>
 *   <td>Yes</td>
 * </tr>
 * <tr>
 *   <td>javax.xml.stream.resolver</td>
 *   <td>sets/gets the impl of the XMLResolver interface</td>
 *   <td>javax.xml.stream.XMLResolver</td>
 *   <td>Null</td>
 *   <td>Yes</td>
 * </tr>
 * </table>
 *
 *
 * @version 1.0
 * @author Copyright (c) 2003 by BEA Systems. All Rights Reserved.
 * @see XMLOutputFactory
 * @see XMLStreamReader
 */

public abstract class XMLInputFactory {
  /** 
   * The property used to turn on/off namespace support, 
   * this is to support XML 1.0 documents,
   * only the <code>true</code> setting must be supported
   */
  public static final String IS_NAMESPACE_AWARE=
    "javax.xml.stream.isNamespaceAware";

  /** 
   * The property used to turn on/off implementation specific validation 
   */
  public static final String IS_VALIDATING=
    "javax.xml.stream.isValidating";
  
  /** 
   * The property that requires the parser to coalesce adjacent character data 
   * sections 
   */
  public static final String IS_COALESCING=
    "javax.xml.stream.isCoalescing";
  
  /** 
   * Requires the parser to replace internal 
   * entity references with their replacement 
   * text and report them as characters
   */
  public static final String IS_REPLACING_ENTITY_REFERENCES=
    "javax.xml.stream.isReplacingEntityReferences";
  
  /** 
   *  The property that requires the parser to resolve external parsed entities
   */
  public static final String IS_SUPPORTING_EXTERNAL_ENTITIES=
    "javax.xml.stream.isSupportingExternalEntities";

  /** 
   *  The property that requires the parser to support DTDs
   */
  public static final String SUPPORT_DTD=
    "javax.xml.stream.supportDTD";

  /*
   * The property used to set/get the implementation of the XMLResolverr
   */
  public static final String RESOLVER=
    "javax.xml.stream.resolver";


  protected XMLInputFactory(){}

  /**
   * Create a new instance of the factory.
   * This static method creates a new factory instance. 
   * This method uses the following ordered lookup procedure to determine 
   * the XMLInputFactory implementation class to load: 
   * Use the javax.xml.stream.XMLInputFactory system property. 
   * Platform default XMLInputFactory instance. 
   * Once an application has obtained a reference to a XMLInputFactory 
   * it can use the factory to configure and obtain stream instances. 
   *
   * @throws FactoryConfigurationError if an instance of this factory cannot be 
   *   loaded
   */
  public static XMLInputFactory newInstance()
    throws FactoryConfigurationError
  {
    XMLInputFactory fobj = null;
    try {
      String fname = System.getProperty("javax.xml.stream.XMLInputFactory");
      if (fname == null)
        fname = "com.sun.ukit.xml.XMLInputFactoryImp";

      fobj = (XMLInputFactory) Class.forName(fname).newInstance();
    } catch (Throwable ex) {
      throw new FactoryConfigurationError("");
    }
    return fobj;
  }

  /**
   * Create a new XMLStreamReader from a reader
   * @param reader the XML data to read from, may not be null
   * @throws XMLStreamException 
   */
  public abstract XMLStreamReader createXMLStreamReader(java.io.Reader reader) 
    throws XMLStreamException;

  /**
   * Create a new XMLStreamReader from a java.io.InputStream
   * @param stream the InputStream to read from
   * @throws XMLStreamException 
   */
  public abstract XMLStreamReader createXMLStreamReader(
      java.io.InputStream stream) 
    throws XMLStreamException;

  /**
   * Create a new XMLStreamReader from a java.io.InputStream
   * @param stream the InputStream to read from
   * @param encoding the character encoding of the stream
   * @throws XMLStreamException 
   */
  public abstract XMLStreamReader createXMLStreamReader(
      java.io.InputStream stream, String encoding)
    throws XMLStreamException;

  /**
   * The resolver that will be set on any XMLStreamReader created 
   * by this factory instance.
   */
  public abstract XMLResolver getXMLResolver();

  /**
   * The resolver that will be set on any XMLStreamReader created 
   * by this factory instance.
   * @param resolver the resolver to use to resolve references
   */
  public abstract void  setXMLResolver(XMLResolver resolver);

  /**
   * Allows the user to set specific feature/property on the underlying 
   * implementation. The underlying implementation is not required to support 
   * every setting of every property in the specification and may use 
   * IllegalArgumentException to signal that a property is unsupported,
   * or that the specified property may not be set with the specified value.
   * @param name The name of the property (may not be <code>null</code>)
   * @param value The value of the property
   * @throws java.lang.IllegalArgumentException if the property is not 
   *   supported
   */
  public abstract void setProperty(java.lang.String name, Object value) 
    throws java.lang.IllegalArgumentException;  

  /**
   * Get the value of a feature/property from the underlying implementation
   * @param name The name of the property (may not be <code>null</code>)
   * @return The value of the property
   * @throws IllegalArgumentException if the property is not supported
   */
  public abstract Object getProperty(java.lang.String name) 
    throws java.lang.IllegalArgumentException;  


  /**
   * Query the set of properties that this factory supports.
   *
   * @param name The name of the property (may not be <code>null</code>)
   * @return <code>true</code> if the property is supported and 
   *    <code>false</code> otherwise
   */
  public abstract boolean isPropertySupported(String name);


}

