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
 *  The XMLStreamReader interface allows forward, read-only access to XML.
 *  It is designed to be the lowest level and most efficient way to
 *  read XML data.
 *
 * <p> The XMLStreamReader is designed to iterate over XML using
 * next() and hasNext().  The data can be accessed using methods such as 
 * getEventType(), getNamespaceURI(), getLocalName() and getText();
 *
 * <p> The <a href="#next()">next()</a> method causes the reader to read the 
 * next parse event. The next() method returns an integer which identifies the 
 * type of event just read.
 * <p> The event type can be determined using <a href="#getEventType()">
 * getEventType()</a>.
 * <p> Parsing events are defined as the XML Declaration, a DTD,
 * start tag, character data, white space, end tag, comment,
 * or processing instruction.  An attribute or namespace event may be 
 * encountered at the root level of a document as the result of a query 
 * operation.
 *
 * <p>For XML 1.0 compliance an XML processor must pass the
 * identifiers of declared unparsed entities, notation declarations and their
 * associated identifiers to the application.  This information was
 * provided through the property API on this interface.
 * The following two properties allowed access to this information:
 * "javax.xml.stream.notations" and "javax.xml.stream.entities".
 * These properties are not supported in the JSR 280 subset of StAX;
 * calls to <code>getProperty("javax.xml.stream.notations");</code>
 * or <code>getProperty("javax.xml.stream.entities");</code> will
 * always return <code>null</code>.
 *
 * <p>The following table describes which methods are valid in what state.
 * If a method is called in an invalid state the method will throw a
 * java.lang.IllegalStateException.
 *
 * <table border="2" rules="all" cellpadding="4" width=100%>
 *     <tr>
 *       <th align="center" colspan="2">
 *         Valid methods for each state
 *       </th>
 *     </tr>
 *     <tr>
 *       <th width=13%>Event Type</th>
 *       <th width=28%>Valid Methods</th>
 *     </tr>
 *     <tr>
 *       <td> All States  </td>
 *       <td> getProperty(), hasNext(), require(), close(),
 *            getNamespaceURI(), isWhiteSpace(),
 *            getEventType(),getLocation()</td>
 *     </tr>
 *     <tr>
 *       <td> START_ELEMENT  </td>
 *       <td> next(), getLocalName(), getPrefix(),
 *            getAttributeXXX(), isAttributeSpecified(),
 *            getNamespaceXXX(), getElementText(),
 *            nextTag()
 *       </td>
 *     </tr>
 *       <td> ATTRIBUTE  </td>
 *       <td> next(), nextTag()
 *            getAttributeXXX(), isAttributeSpecified()
 *       </td>
 *     </tr>
 *     </tr>
 *       <td> NAMESPACE  </td>
 *       <td> next(), nextTag()
 *            getNamespaceXXX()
 *       </td>
 *     </tr>
 *     <tr>
 *       <td> END_ELEMENT  </td>
 *       <td> next(), getLocalName(), getPrefix(),
 *            getNamespaceXXX(), nextTag()
 *      </td>
 *     </tr>
 *     <tr>
 *       <td> CHARACTERS  </td>
 *       <td> next(), getTextXXX(), nextTag() </td>
 *     </tr>
 *     <tr>
 *       <td> CDATA  </td>
 *       <td> next(), getTextXXX(), nextTag() </td>
 *     </tr>
 *     <tr>
 *       <td> COMMENT  </td>
 *       <td> next(), getTextXXX(), nextTag() </td>
 *     </tr>
 *     <tr>
 *       <td> SPACE  </td>
 *       <td> next(), getTextXXX(), nextTag() </td>
 *     </tr>
 *     <tr>
 *       <td> START_DOCUMENT  </td>
 *       <td> next(), getEncoding(), getVersion(), isStandalone(), 
 *            standaloneSet(), getCharacterEncodingScheme(), nextTag()</td>
 *     </tr>
 *     <tr>
 *       <td> END_DOCUMENT  </td>
 *       <td> close()</td>
 *     </tr>
 *     <tr>
 *       <td> PROCESSING_INSTRUCTION  </td>
 *       <td> next(), getPITarget(), getPIData(), nextTag() </td>
 *     </tr>
 *     <tr>
 *       <td> ENTITY_REFERENCE  </td>
 *       <td> next(), getLocalName(), getText(), nextTag() </td>
 *     </tr>
 *     <tr>
 *       <td> DTD  </td>
 *       <td> next(), getText(), nextTag() </td>
 *     </tr>
 *  </table>
 *
 * @version 1.0
 * @author Copyright (c) 2003 by BEA Systems. All Rights Reserved.
 * @see XMLInputFactory
 * @see XMLStreamWriter
 */

public interface XMLStreamReader extends XMLStreamConstants {
  /**
   * Get the value of a feature/property from the underlying implementation
   * @param name The name of the property, may not be <code>null</code>
   * @return The value of the property
   * @throws IllegalArgumentException if name is <code>null</code>
   */
  public Object getProperty(java.lang.String name)
      throws java.lang.IllegalArgumentException;

  /**
   * Get next parsing event - a processor may return all contiguous
   * character data in a single chunk, or it may split it into several chunks.
   * If the property javax.xml.stream.isCoalescing is set to <code>true</code>
   * element content must be coalesced and only one CHARACTERS event
   * must be returned for contiguous element content or
   * CDATA Sections.  
   *
   * By default entity references must be 
   * expanded and reported transparently to the application.
   * An exception will be thrown if an entity reference cannot be expanded.
   * If element content is empty (i.e. content is "") then no CHARACTERS event 
   * will be reported.
   *
   * <p>Given the following XML:<br>
   * &lt;foo>&lt;!--description-->content text&lt;![CDATA[&lt;greeting>Hello&lt;
   * /greeting>]]>other content&lt;/foo><br>
   * The behavior of calling next() when being on foo will be:<br>
   * 1- the comment (COMMENT)<br>
   * 2- then the characters section (CHARACTERS)<br>
   * 3- then the CDATA section (another CHARACTERS)<br>
   * 4- then the next characters section (another CHARACTERS)<br>
   * 5- then the END_ELEMENT<br>
   *
   * <p><b>NOTE:</b> empty element (such as &lt;tag/>) will be reported
   *  with  two separate events: START_ELEMENT, END_ELEMENT - This preserves
   *   parsing equivalency of empty element to &lt;tag>&lt;/tag>.
   *
   * @return the integer code corresponding to the current parse event
   * @throws NoSuchElementException if this is called when hasNext() returns 
   *   <code>false</code>
   * @throws XMLStreamException  if there is an error processing the underlying 
   *   XML source
   */
  public int next() throws XMLStreamException;

  /**
   * Test if the current event is of the given type and if the namespace and 
   * name match the current namespace and name of the current event.  If the 
   * namespaceURI is <code>null</code> it is not checked for equality, if the 
   * localName is <code>null</code> it is not checked for equality.
   * @param type the event type
   * @param namespaceURI the uri of the event, may be <code>null</code>
   * @param localName the localName of the event, may be <code>null</code>
   * @throws XMLStreamException if the required values are not matched.
   */
  public void require(int type, String namespaceURI, String localName)
      throws XMLStreamException;

  /**
   * Reads the content of a text-only element, an exception is thrown if this 
   * is not a text-only element.
   * Regardless of value of javax.xml.stream.isCoalescing this method always 
   * returns coalesced content.
   * <br /> Precondition: the current event is START_ELEMENT.
   * <br /> Postcondition: the current event is the corresponding END_ELEMENT.
   *
   * <br />The method does the following (implementations are free to optimize 
   * but must do equivalent processing):
   * <pre>
   * if(getEventType() != XMLStreamConstants.START_ELEMENT) {
   *     throw new XMLStreamException(
   *         "parser must be on START_ELEMENT to read next text", 
   *         getLocation());
   * }
   * int eventType = next();
   * StringBuffer content = new StringBuffer();
   * while(eventType != XMLStreamConstants.END_ELEMENT ) {
   *     if(eventType == XMLStreamConstants.CHARACTERS
   *             || eventType == XMLStreamConstants.CDATA
   *             || eventType == XMLStreamConstants.SPACE
   *             || eventType == XMLStreamConstants.ENTITY_REFERENCE) {
   *         buf.append(getText());
   *     } else if(eventType == XMLStreamConstants.PROCESSING_INSTRUCTION
   *             || eventType == XMLStreamConstants.COMMENT) {
   *             // skipping
   *     } else if(eventType == XMLStreamConstants.END_DOCUMENT) {
   *         throw new XMLStreamException(
   *             "unexpected end of document when reading element text content",
   *             this);
   *     } else if(eventType == XMLStreamConstants.START_ELEMENT) {
   *         throw new XMLStreamException(
   *             "element text content may not contain START_ELEMENT", 
   *             getLocation());
   *     } else {
   *         throw new XMLStreamException(
   *             "Unexpected event type "+eventType, getLocation());
   *     }
   *     eventType = next();
   * }
   * return buf.toString();
   * </pre>
   *
   * @throws XMLStreamException if the current event is not a START_ELEMENT 
   * or if a non text element is encountered
   */
  public String getElementText() throws XMLStreamException;

  /**
   * Skips any white space (isWhiteSpace() returns <code>true</code>), COMMENT,
   * or PROCESSING_INSTRUCTION,
   * until a START_ELEMENT or END_ELEMENT is reached.
   * If other than white space characters, COMMENT, PROCESSING_INSTRUCTION, 
   * START_ELEMENT, END_ELEMENT are encountered, an exception is thrown. This 
   * method should be used when processing element-only content separated by 
   * white space. 
   *
   * <br /> Precondition: none
   * <br /> Postcondition: the current event is START_ELEMENT or END_ELEMENT
   * and cursor may have moved over any whitespace event.
   *
   * <br />Essentially it does the following (implementations are free to 
   * optimize but must do equivalent processing):
   * <pre>
   * int eventType = next();
   * // skip whitespace
   * while((eventType == XMLStreamConstants.CHARACTERS 
   *         &amp;&amp; isWhiteSpace())
   *         || (eventType == XMLStreamConstants.CDATA &amp;&amp; isWhiteSpace()) 
   *         // skip whitespace
   *         || eventType == XMLStreamConstants.SPACE
   *         || eventType == XMLStreamConstants.PROCESSING_INSTRUCTION
   *         || eventType == XMLStreamConstants.COMMENT) {
   *     eventType = next();
   * }
   * if (eventType != XMLStreamConstants.START_ELEMENT 
   *         &amp;&amp; eventType != XMLStreamConstants.END_ELEMENT) {
   *     throw new String XMLStreamException(
   *         "expected start or end tag", getLocation());
   * }
   * return eventType;
   * </pre>
   *
   * @return the event type of the element read (START_ELEMENT or END_ELEMENT)
   * @throws XMLStreamException if the current event is not white space, 
   *   COMMENT, PROCESSING_INSTRUCTION, START_ELEMENT or END_ELEMENT
   * @throws NoSuchElementException if this is called when hasNext() returns 
   *   <code>false</code>
   */
  public int nextTag() throws XMLStreamException;

  /**
   * Returns <code>true</code> if there are more parsing events and 
   * <code>false</code> if there are no more events.  This method will return
   * <code>false</code> if the current state of the XMLStreamReader is
   * END_DOCUMENT
   * @return <code>true</code> if there are more events, <code>false</code> 
   *   otherwise
   * @throws XMLStreamException if there is a fatal error detecting the next 
   *   state
   */
  public boolean hasNext() throws XMLStreamException;
  
  /**
   * Frees any resources associated with this Reader.  This method does not 
   * close the underlying input source.
   * @throws XMLStreamException if there are errors freeing associated resources
   */
  public void close() throws XMLStreamException;

  /**
   * Return the uri for the given prefix.
   * The uri returned depends on the current state of the processor.
   *
   * <p><strong>NOTE:</strong>The 'xml' prefix is bound as defined in
   * <a href="http://www.w3.org/TR/REC-xml-names/#ns-using">
   * Namespaces in XML</a> specification to 
   * "http://www.w3.org/XML/1998/namespace".
   *
   * <p><strong>NOTE:</strong> The 'xmlns' prefix must be resolved to following
   * namespace
   * <a href="http://www.w3.org/2000/xmlns/">http://www.w3.org/2000/xmlns/</a>
   * @param prefix The prefix to lookup, may not be <code>null</code>
   * @return the uri bound to the given prefix or <code>null</code> if it is 
   *   not bound
   * @throws IllegalArgumentException if the prefix is <code>null</code>
   */
  public String getNamespaceURI(String prefix);

  /**
   * Returns <code>true</code> if the cursor points to a character data event
   * that consists of all whitespace
   * @return <code>true</code> if the cursor points to all whitespace, 
   *   <code>false</code> otherwise
   */
  public boolean isWhiteSpace();

  /**
   * Returns the normalized attribute value of the attribute with the namespace
   * and localName. If the namespaceURI is <code>null</code> it is not checked
   * for equality, i.e. only the localName part of an attribute name will be 
   * compared. A value of <code>""</code> (empty String) is interpreted
   * to mean 'no namespace', i.e. any matching attribute must have no 
   * namespace specified. 
   * @param namespaceURI the namespace of the attribute
   * @param localName the local name of the attribute, cannot be 
   *   <code>null</code>
   * @return returns the value of the attribute , returns <code>null</code> if 
   *   not found
   * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
   */
  public String getAttributeValue(String namespaceURI, String localName);

  /**
   * Returns the count of attributes on this START_ELEMENT,
   * this method is only valid on a START_ELEMENT or ATTRIBUTE.  This
   * count excludes namespace definitions when <code>isNamespaceAware</code>
   * is true; when <code>isNamespaceAware</code> is false, the attribute
   * count includes <code>xmlns</code> and <code>xmlns.*</code> attributes. 
   * Attribute indices are zero-based.
   * @return returns the number of attributes
   * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
   */
  public int getAttributeCount();

  /**
   * Returns the namespace of the attribute at the provided
   * index
   * @param index the position of the attribute
   * @return the namespace URI (can be <code>null</code>)
   * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
   * @throws IndexOutOfBoundsException if the index is invalid or if there is
   *  no attribute at the specified index.
   */
  public String getAttributeNamespace(int index);

  /**
   * Returns the localName of the attribute at the provided
   * index
   * @param index the position of the attribute
   * @return the localName of the attribute
   * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
   * @throws IndexOutOfBoundsException if the index is invalid or if there is
   *  no attribute at the specified index.
   */
  public String getAttributeLocalName(int index);

  /**
   * Returns the prefix of this attribute at the
   * provided index
   * @param index the position of the attribute
   * @return the prefix of the attribute
   * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
   * @throws IndexOutOfBoundsException if the index is invalid or if there is
   *  no attribute at the specified index.
   */
  public String getAttributePrefix(int index);

  /**
   * Returns the XML type of the attribute at the provided
   * index (see XML 1.0 Specification, section 3.3.1)
   * @param index the position of the attribute
   * @return the XML type of the attribute
   * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
   * @throws IndexOutOfBoundsException if the index is invalid or if there is
   *  no attribute at the specified index.
   */
  public String getAttributeType(int index);

  /**
   * Returns the value of the attribute at the
   * index
   * @param index the position of the attribute
   * @return the attribute value
   * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
   * @throws IndexOutOfBoundsException if the index is invalid or if there is
   *  no attribute at the specified index.
   */
  public String getAttributeValue(int index);

  /**
   * Returns a boolean which indicates if this
   * attribute was created by default
   * @param index the position of the attribute
   * @return <code>true</code> if this is a default attribute
   * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
   */
  public boolean isAttributeSpecified(int index);

  /**
   * Returns the count of namespaces declared on this START_ELEMENT or 
   * END_ELEMENT, this method is only valid on a START_ELEMENT, END_ELEMENT or 
   * NAMESPACE. On an END_ELEMENT the count is of the namespaces that are about 
   * to go out of scope.  This is the equivalent of the information reported
   * by SAX callback for an end element event.
   * @return returns the number of namespace declarations on this specific 
   *   element
   * @throws IllegalStateException if this is not a START_ELEMENT, END_ELEMENT 
   *   or NAMESPACE
   */
  public int getNamespaceCount();

  /**
   * Returns the prefix for the namespace declared at the index. Returns 
   * <code>null</code> if this is the default namespace declaration
   *
   * @param index the position of the namespace declaration
   * @return returns the namespace prefix
   * @throws IllegalStateException if this is not a START_ELEMENT, END_ELEMENT 
   *   or NAMESPACE
   * @throws IndexOutOfBoundsException if the index is invalid or if there is
   *  no namespace at the specified index
   */
  public String getNamespacePrefix(int index);
  
  /**
   * Returns the uri for the namespace declared at the
   * index.
   *
   * @param index the position of the namespace declaration
   * @return returns the namespace uri or <code>null</code> if none exists
   * @throws IllegalStateException if this is not a START_ELEMENT, END_ELEMENT 
   *   or NAMESPACE
   */
  public String getNamespaceURI(int index);

  /**
   * Returns an integer code that indicates the type
   * of the event the cursor is pointing to.
   */
  public int getEventType();

  /**
   * Returns the current value of the parse event as a string, this returns the
   * string value of a CHARACTERS event, returns the value of a COMMENT, the 
   * replacement value for an ENTITY_REFERENCE, the string value of a CDATA 
   * section, the string value for a SPACE event, or the String value of the 
   * internal subset of the DTD. If an ENTITY_REFERENCE has been resolved, any 
   * character data
   * will be reported as CHARACTERS events.
   * @return the current text or <code>null</code>
   * @throws java.lang.IllegalStateException if this state is not
   * a valid text state.
   */
  public String getText();

  /**
   * Returns an array which contains the characters from this event.
   * This array should be treated as read-only and transient. I.e. the array 
   * will contain the text characters until the XMLStreamReader moves on to the
   * next event. Attempts to hold onto the character array beyond that time or 
   * modify the contents of the array are breaches of the contract for this 
   * interface.
   * @return the current text or an empty array
   * @throws java.lang.IllegalStateException if this state is not
   * a valid text state.
   */
  public char[] getTextCharacters();

  /**
   * Returns the offset into the text character array where the first character
   * (of this text event) is stored.
   * @throws java.lang.IllegalStateException if this state is not a valid text
   *   state.
   */
  public int getTextStart();

  /**
   * Returns the length of the sequence of characters for this Text event within
   * the text character array.
   * @throws java.lang.IllegalStateException if this state is not a valid text
   *   state.
   */
  public int getTextLength();

  /**
   * Return input encoding if known or <code>null</code> if unknown.
   * @return the encoding of this instance or <code>null</code>.
   * <i>Note: the source of the encoding is that that may have been provided by
   * an application to the factory.</i>
   */
  public String getEncoding();

  /**
   * Return the current location of the processor.
   * If the Location is unknown the processor should return an implementation 
   * of Location that returns -1 for the location and <code>null</code> for the 
   * publicId and systemId. The location information is only valid until next() 
   * is called.
   */
  public Location getLocation();

  /**
   * Returns the (local) name of the current event.
   * For START_ELEMENT or END_ELEMENT returns the (local) name of the current 
   * element. For ENTITY_REFERENCE it returns entity name.
   * The current event must be START_ELEMENT or END_ELEMENT, 
   * or ENTITY_REFERENCE
   * @return the localName
   * @throws IllegalStateException if this not a START_ELEMENT,
   * END_ELEMENT or ENTITY_REFERENCE
   */
  public String getLocalName();

  /**
   * If the current event is a START_ELEMENT or END_ELEMENT  this method
   * returns the URI of the prefix or the default namespace.
   * Returns <code>null</code> if the event does not have a prefix.
   * @return the URI bound to this elements prefix, the default namespace, or 
   *   <code>null</code>
   */
  public String getNamespaceURI();

  /**
   * Returns the prefix of the current event or <code>null</code> if the event 
   * does not have a prefix
   * @return the prefix or <code>null</code>
   */
  public String getPrefix();
  
  /**
   * Get the xml version declared on the xml declaration.
   * Returns <code>null</code> if none was declared.
   * @return the XML version or <code>null</code>
   */
  public String getVersion();

  /**
   * Get the standalone declaration from the xml declaration
   * @return <code>true</code> if this is standalone, or <code>false</code> 
   *   otherwise
   */
  public boolean isStandalone();

  /**
   * Checks if standalone was set in the document
   * @return <code>true</code> if standalone was set in the document, or 
   *   <code>false</code> otherwise
   */
  public boolean standaloneSet();

  /**
   * Returns the character encoding declared on the xml declaration
   * Returns <code>null</code> if none was declared
   * @return the encoding declared in the document or <code>null</code>
   */
  public String getCharacterEncodingScheme();

  /**
   * Get the target of a processing instruction
   * @return the target or <code>null</code>
   */
  public String getPITarget();

  /**
   * Get the data section of a processing instruction
   * @return the data or <code>null</code>
   */
  public String getPIData();
}
