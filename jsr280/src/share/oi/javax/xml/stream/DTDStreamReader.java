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

import java.util.NoSuchElementException;

import javax.xml.stream.XMLStreamConstants;
import javax.xml.stream.XMLStreamReader;
import javax.xml.stream.Location;
import javax.xml.stream.XMLStreamException;

/**
 * This interface extends the Streaming API for XML (StAX) Cursor API. 
 * It enables forward, read-only access to
 * general internal and external parsed and unparsed entity declarations, 
 * notation declarations, processing instructions and comments contained in 
 * a DTD.
 * <p>
 * The DTDStreamReader instance has very specific lifecycle constraints.
 * An application can only obtain an implementation of DTDStreamReader from 
 * {@link XMLStreamReader XMLStreamReader} immediately after the method 
 * {@link XMLStreamReader#next() next()} returns a
 * {@link XMLStreamReader#DTD DTD} event, and prior to invoking any other
 * methods on {@link XMLStreamReader XMLStreamReader}. The 
 * DTDStreamReader instance is obtained by calling method 
 * {@link XMLStreamReader#getProperty getProperty} with 
 * <code>"javax.xml.stream.DTDStreamReader"</code> as the property name 
 * parameter. 
 * Multiple calls to {@link XMLStreamReader#getProperty getProperty} with 
 * a <code>"javax.xml.stream.DTDStreamReader"</code> argument will return the 
 * same instance of DTDStreamReader.
 * Once the DTDStreamReader object is instantiated by the 
 * application, its various methods can be used to access the DTD data. Its
 * processing capability extends only until the {@link #END_DTD END_DTD} event 
 * is reached;
 * at this point, any further document processing must be performed via the
 * {@link XMLStreamReader XMLStreamReader} instance.
 * </p>
 * <p>
 * If an application chooses to instantiate a DTDStreamReader,
 * the  {@link XMLStreamReader#getText() getText()} method on 
 * {@link XMLStreamReader XMLStreamReader} is invalidated
 * for the {@link XMLStreamReader#DTD DTD} event: any attempt to invoke
 * it will generate a <code>java.lang.IllegalStateException</code>. In this case, 
 * the data for the DTD event must be obtained via the DTDStreamReader
 * instance.
 * </p><p>
 *
 * After DTDStreamReader is instantiated, invocation of any of
 * the following {@link XMLStreamReader XMLStreamReader} methods
 * will change the state of 
 * DTDStreamReader implementation to {@link #END_DTD END_DTD}:
 * <ul><li>{@link XMLStreamReader#next() next()},</li> 
 * <li>{@link XMLStreamReader#hasNext() hasNext()},</li>
 * <li>{@link XMLStreamReader#nextTag() nextTag()} and</li>
 * <li>{@link XMLStreamReader#close() close()}.</li></ul>
 * Once in the {@link #END_DTD END_DTD} state, 
 * no further processing of the DTD data is possible.  
 * </p>
 * <p>
 * If no instance of DTDStreamReader is created (either the
 * {@link XMLStreamReader#getProperty getProperty} method with parameter
 * <code>"javax.xml.stream.DTDStreamReader"</code> is never invoked, 
 * or it returns <code>null</code> because it is not invoked
 * immediately after a {@link XMLStreamReader#DTD DTD} event is received),
 * the behavior of
 * {@link XMLStreamReader XMLStreamReader} is unchanged and follows the
 * specification in
 * <a href="http://www.jcp.org/en/jsr/detail?id=173&showPrint">JSR173</a>
 * for the JSR 280 subset of the API.
 * </p>
 * <p>
 * DTDStreamReader intentionally mimics 
 * {@link XMLStreamReader XMLStreamReader} in order to provide developers
 * with a familiar programming model.
 * DTDStreamReader includes several methods identical in signature 
 * and semantics to methods defined on {@link XMLStreamReader XMLStreamReader}. 
 * </p><p>
 * An object which implements DTDStreamReader uses methods {@link #next next} 
 * and {@link #getEventType getEventType} to provide the current event type. 
 * The first event is always {@link #START_DTD START_DTD}. 
 * Once an {@link #END_DTD END_DTD} event is returned by methods 
 * {@link #next next} or {@link #getEventType getEventType}, the application 
 * must continue further document processing using the
 * {@link XMLStreamReader XMLStreamReader} interface.
 * The application can use the {@link #close close} method at any time to 
 * terminate processing of any remaining DTD content and skip to the 
 * {@link #END_DTD END_DTD} event.
 * </p><p>
 * The following table describes which methods are valid in what state.
 * If a method is called in an invalid state the method will throw a
 * java.lang.IllegalStateException. Method {@link #next() next()} is an 
 * exception from this rule: it is defined to throw 
 * NoSuchElementException if it is called when {@link #hasNext() hasNext()} 
 * returns <code>false</code>, so in the case where {@link #next() next()}
 * is called in the END_DTD state it will return NoSuchElementException rather
 * than java.lang.IllegalStateException. 
 * </p>
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
 *       <td> {@link #hasNext() hasNext}, 
 *            {@link #getEventType() getEventType}, 
 *            {@link #getLocation() getLocation}, 
 *            {@link #close() close}
 *       </td>
 *     </tr>
 *     <tr>
 *       <td> {@link #START_DTD START_DTD}  </td>
 *       <td> {@link #next() next}, 
 *            {@link #getName() getName}, 
 *            {@link #getPublicIdentifier() getPublicIdentifier}, 
 *            {@link #getSystemIdentifier() getSystemIdentifier}
 *       </td>
 *     </tr>
 *     <tr>
 *       <td> {@link #END_DTD END_DTD}  </td>
 *       <td></td>
 *     </tr>
 *     <tr>
 *       <td> {@link #ENTITY_DECLARATION ENTITY_DECLARATION}  </td>
 *       <td> {@link #next() next}, 
 *            {@link #getName() getName}, 
 *            {@link #getPublicIdentifier() getPublicIdentifier}, 
 *            {@link #getSystemIdentifier() getSystemIdentifier}
 *            {@link #getText() getText}, 
 *            {@link #getTextCharacters getTextCharacters}, 
 *            {@link #getTextStart() getTextStart}, 
 *            {@link #getTextLength() getTextLength} 
 *     </tr>
 *     <tr>
 *       <td> {@link #UNPARSED_ENTITY_DECLARATION UNPARSED_ENTITY_DECLARATION}  </td>
 *       <td> {@link #next() next}, 
 *            {@link #getName() getName}, 
 *            {@link #getPublicIdentifier() getPublicIdentifier}, 
 *            {@link #getSystemIdentifier() getSystemIdentifier},
 *            {@link #getNotationName() getNotationName} 
 *     </tr>
 *     <tr>
 *       <td> {@link #NOTATION_DECLARATION NOTATION_DECLARATION}  </td>
 *       <td> {@link #next() next}, 
 *            {@link #getName() getName}, 
 *            {@link #getPublicIdentifier() getPublicIdentifier}, 
 *            {@link #getSystemIdentifier() getSystemIdentifier}
 *     </tr>
 *     </tr>
 *     <tr>
 *       <td> {@link #COMMENT COMMENT}  </td>
 *       <td> {@link #next() next}, 
 *            {@link #getText() getText}, 
 *            {@link #getTextCharacters getTextCharacters}, 
 *            {@link #getTextStart() getTextStart}, 
 *            {@link #getTextLength() getTextLength} 
 *       </td>
 *     </tr>
 *     <tr>
 *       <td> {@link #PROCESSING_INSTRUCTION PROCESSING_INSTRUCTION}  </td>
 *       <td> {@link #next() next}, 
 *            {@link #getPITarget() getPITarget}, 
 *            {@link #getPIData() getPIData} 
 *       </td>
 *     </tr>
 *  </table>
 *
 * @version 1.0
 * @author Copyright (c) 2006 by BEA Systems. All Rights Reserved.
 * @see XMLStreamConstants
 * @see XMLStreamReader
 */

public interface DTDStreamReader
{
	/** 
	 * Indicates an event is the start of a DTD. On this event, method 
	 * {@link #getName() getName} returns the root element's 
	 * qualified name, and methods 
	 * {@link #getPublicIdentifier() getPublicIdentifier} and 
	 * {@link #getSystemIdentifier() getSystemIdentifier} return the document's
	 * public and system identifiers.
	 */
	public static final int START_DTD = 1001;

	/** 
	 * Indicates an event is the end of a DTD. On this event, method 
	 * {@link #hasNext() hasNext} returns <code>false</code>, 
	 * and method {@link #getEventType() getEventType} returns 
	 * {@link #END_DTD END_DTD}. This event marks the end of the valid
	 * lifecycle of the DTDStreamReader instance, so any 
	 * method call other than {@link #next() next}, {@link #hasNext() hasNext},
	 * {@link #getEventType() getEventType}, {@link #close() close}, 
	 * or {@link #getLocation() getLocation} on this interface will
	 * throw java.lang.IllegalStateException.
	 */
	public static final int END_DTD = 1002;

	/** 
	 * Indicates an event is a parsed entity declaration.
	 * <p>
	 * If 
	 * the {@link #getTextLength getTextLength} method returns a negative 
	 * value, the event represents an external parsed entity: 
	 * method {@link #getName() getName} returns the 
	 * entity name, methods {@link #getPublicIdentifier getPublicIdentifier}  
	 * and {@link #getSystemIdentifier getSystemIdentifier} return the 
	 * entity's public and system identifiers, and methods
	 * {@link #getText getText} and {@link #getTextCharacters getTextCharacters} 
	 * return null and empty array, respectively.
	 * </p><p>
	 * If the {@link #getTextLength getTextLength} method returns a positive
	 * value, then the event represents an internal parsed entity:
	 * method {@link #getName() getName} returns the 
	 * entity name, methods {@link #getText getText} or 
	 * {@link #getTextCharacters getTextCharacters} return the replacement 
	 * text of an internal parsed entity, and methods 
	 * {@link #getPublicIdentifier getPublicIdentifier} and  
	 * {@link #getSystemIdentifier getSystemIdentifier} return null.
	 */
	public static final int ENTITY_DECLARATION = 
		XMLStreamConstants.ENTITY_DECLARATION;

	/** 
	 * Indicates an event is an unparsed entity declaration. On this event,
	 * method {@link #getName() getName} returns the unparsed 
	 * entity name, methods {@link #getPublicIdentifier getPublicIdentifier} 
	 * and {@link #getSystemIdentifier getSystemIdentifier} return 
	 * the public identifier and the system identifier of an unparsed entity,
	 * and the 
	 * method {@link #getNotationName() getNotationName} returns the name of 
	 * the notation which identifies the format of the unparsed entity.
	 */
	public static final int UNPARSED_ENTITY_DECLARATION = 1003;

	/** 
	 * Indicates an event is a notation declaration. On this event the method 
	 * {@link #getName() getName} returns the notation name, 
	 * the methods {@link #getPublicIdentifier getPublicIdentifier} 
	 * and {@link #getSystemIdentifier getSystemIdentifier} return the
	 * public identifier and the system identifier of the notation.
	 */
	public static final int NOTATION_DECLARATION = 
		XMLStreamConstants.NOTATION_DECLARATION;

	/** 
	 * Indicates an event is a processing instruction. On this event,
	 * the method 
	 * {@link #getPITarget() getPITarget} returns the processing instruction 
	 * target and the method {@link #getPIData() getPIData} returns the
	 * processing instruction data section.
	 */
	public static final int PROCESSING_INSTRUCTION = 
		XMLStreamConstants.PROCESSING_INSTRUCTION;

	/** 
	 * Indicates an event is a comment. On this event, the methods 
	 * {@link #getText() getText} or 
	 * {@link #getTextCharacters getTextCharacters} return the character 
	 * data of the comment, and
	 * the methods {@link #getTextStart() getTextStart} and 
	 * {@link #getTextLength() getTextLength} return the index of the 
	 * first character 
	 * and the number of characters in the character array returned by 
	 * the {@link #getTextCharacters getTextCharacters} method.
	 */
	public static final int COMMENT = 
		XMLStreamConstants.COMMENT;

	/**
	 * Returns next DTD parsing event.
	 *
	 * @return the integer code corresponding to the current parse event
	 * @throws NoSuchElementException if this is called when 
	 *   {@link #hasNext() hasNext()} returns <code>false</code>
	 * @throws XMLStreamException if there is an error processing the 
	 *   underlying DTD source
	 */
	public int next() throws XMLStreamException;

	/**
	 * Returns <code>true</code> if there are more parsing events and 
	 * <code>false</code> if there are no more events.  This method will return
	 * <code>false</code> if the current state of the DTDStreamReader is
	 * {@link #END_DTD END_DTD}
	 *
	 * @return <code>true</code> if there are more events, <code>false</code> 
	 *   otherwise
	 * @throws XMLStreamException if there is a fatal error detecting the next 
	 *   state
	 */
	public boolean hasNext() throws XMLStreamException;

	/**
	 * Returns an integer code that indicates the type of the event at
	 * the current cursor location.
	 *
	 * @return the integer code corresponding to the current parse event
	 */
	public int getEventType();

	/**
	 * Returns the current value of the parse event as a string. 
	 * This returns the value of a {@link #COMMENT COMMENT} or the replacement 
	 * value for an {@link #ENTITY_DECLARATION ENTITY_DECLARATION}. This 
	 * method returns <code>null</code> if there is no text available.
	 *
	 * @return the current text or <code>null</code> if there is no text available
	 * @throws java.lang.IllegalStateException if this state is not a valid
	 *   text state.
	 */
	public String getText();

	/**
	 * Returns an array which contains the characters from this event.
	 * This array should be treated as read-only and transient: the array 
	 * will contain the text characters only until the DTDStreamReader
	 * moves on to the next event.
	 * Attempts to hold onto the character array beyond that time or 
	 * modify the contents of the array are breaches of the contract for this 
	 * interface.
	 *
	 * @return the current text or an empty array
	 * @throws java.lang.IllegalStateException if this state is not a valid 
	 *   text state.
	 */
	public char[] getTextCharacters();

	/**
	 * Returns the offset into the text character array at which the first
	 * character of the data for the current event is located.
	 *
	 * @return the offset of the first character
	 * @throws java.lang.IllegalStateException if this state is not a valid
	 *   text state.
	 */
	public int getTextStart();

	/**
	 * Returns the length of the sequence of characters for the current event 
	 * within the text character array. This method returns <code>-1</code> 
	 * if there is no text available.
	 *
	 * @return the length of the character sequence for current event or 
	 *   <code>-1</code>
	 * @throws java.lang.IllegalStateException if this state is not a valid
	 *   text state.
	 */
	public int getTextLength();

	/**
	 * Returns the current location of the processor.
	 * If the location is unknown the processor should return an 
	 * implementation 
	 * of {@link Location Location} that returns <code>-1</code> for
	 * each of the lineNumber, columnNumber, and characterOffset,
	 * and <code>null</code> for each of the publicId and systemId. 
	 * The location information is only valid until {@link #next() next()}
	 * is called.
	 *
	 * @return the {@link Location Location} object
	 */
	public Location getLocation();

	/**
	 * Returns the target of a processing instruction.
	 *
	 * @return the target or <code>null</code>
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getPITarget();

	/**
	 * Returns the data section of a processing instruction.
	 *
	 * @return the data or <code>null</code>
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getPIData();

	/**
	 * Returns the name as a String.
	 *
	 * @return the name
	 * @throws java.lang.IllegalStateException if this state is not a valid
	 *   text state.
	 */
	public String getName();

	/**
	 * Returns the public identifier.
	 *
	 * @return the public identifier, or null if not available
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getPublicIdentifier();

	/**
	 * Returns the system identifier.
	 *
	 * @return the system identifier, or null if not available
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getSystemIdentifier();

	/**
	 * Returns the notation name.
	 *
	 * @return the notation name
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getNotationName();

	/**
	 * Terminates DTD processing, skipping all DTD related events up 
	 * to {@link #END_DTD END_DTD}. This method does not close the
	 * underlying input source.
	 * If this method is called when current state is already 
	 * {@link #END_DTD END_DTD}, this method does nothing. 
	 * Once this method has been invoked,
	 * method {@link #hasNext() hasNext()} returns <code>false</code>, 
	 * method {@link #getEventType() getEventType()} returns 
	 * {@link #END_DTD END_DTD} and any other method call except 
	 * {@link #next() next()} or {@link #getLocation() getLocation()}
	 * on this interface generates an
	 * <code>java.lang.IllegalStateException</code>.
	 *
	 * @throws XMLStreamException if there is an error processing the
	 *   underlying DTD source
	 */
	public void close() throws XMLStreamException;
}
