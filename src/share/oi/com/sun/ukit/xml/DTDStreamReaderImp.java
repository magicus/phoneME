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

package com.sun.ukit.xml;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.NoSuchElementException;
import java.lang.IllegalArgumentException;
import java.lang.IllegalStateException;

import javax.xml.stream.DTDStreamReader;
import javax.xml.stream.XMLStreamReader;
import javax.xml.stream.XMLResolver;
import javax.xml.stream.Location;
import javax.xml.stream.XMLStreamException;

/**
 * DTDStreamReader implementation.
 *
 * @see javax.xml.stream.DTDStreamReader
 * @see javax.xml.stream.XMLStreamReader
 */

/* pkg */ final class DTDStreamReaderImp
	implements DTDStreamReader, Location
{
	private Pair mEQhead;  // internal event queue
	private Pair mEQtail;

	private ParserStAX mStream;

	/**
	 * Constructor.
	 */
	/* pkg */ DTDStreamReaderImp(ParserStAX stream, Pair dtdevt)
	{
		if (stream == null || dtdevt == null)
			throw new NullPointerException();

		mStream = stream;

		eqAdd(dtdevt);
		mEQhead.id = START_DTD;
	}

	/**
	 * Returns next DTD parsing event.
	 *
	 * @return the integer code corresponding to the current parse event
	 * @throws NoSuchElementException if this is called when 
	 *   {@link #hasNext() hasNext()} returns <code>false</code>
	 * @throws XMLStreamException if there is an error processing the 
	 *   underlying DTD source
	 */
	public int next()
		throws XMLStreamException
	{
		if (getEventType() == END_DTD)
			throw new NoSuchElementException();

		if (mEQhead.list != null)
			mStream.del(mEQhead.list);
		//		Remove the head of the queue
		mStream.del(eqGet());
		if (mEQhead != null)
			return mEQhead.id;

		try {
			//		Event queue is empty. Get more events.
			switch (mStream.step()) {
			case Parser.EV_COMM:
				mEQhead.id = COMMENT;
				break;

			case Parser.EV_PI:
				mEQhead.id = PROCESSING_INSTRUCTION;
				break;

			case Parser.EV_PENT:
				mEQhead.id = ENTITY_DECLARATION;
				break;

			case Parser.EV_UENT:
				mEQhead.id = UNPARSED_ENTITY_DECLARATION;
				break;

			case Parser.EV_NOT:
				mEQhead.id = NOTATION_DECLARATION;
				break;

			case Parser.EV_DTDE:
				mStream.mPh = Parser.PH_DTD_MISC;
				Pair evt = mStream.pair(null);
				evt.id = END_DTD;
				eqAdd(evt);
				break;

			default:
				throw new XMLStreamException();
			}

		} catch(XMLStreamException xmlse) {
			throw xmlse;
		} catch(IOException ioe) {
			throw new XMLStreamException(ioe);
		} catch(RuntimeException rte) {
			throw rte;
		} catch(Exception e) {
			throw new XMLStreamException(e.toString());
		}

		return mEQhead.id;
	}

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
	public boolean hasNext() 
		throws XMLStreamException
	{
		return (getEventType() != END_DTD);
	}

	/**
	 * Returns an integer code that indicates the type of the event at
	 * the current cursor location.
	 *
	 * @return the integer code corresponding to the current parse event
	 */
	public int getEventType()
	{
		return (mStream.mDTD != null)? mEQhead.id: END_DTD;
	}

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
	public String getText()
	{
		switch (getEventType()) {
		case ENTITY_DECLARATION:
			if (mEQhead.num < 0)  // there is zero length array in chars
				return null;
		case COMMENT:
			if (mEQhead.value == null)
				mEQhead.value = new String(mEQhead.chars, 0, mEQhead.num);
			return mEQhead.value;

		default:
			throw new IllegalStateException();
		}
	}

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
	public char[] getTextCharacters()
	{
		switch (getEventType()) {
		case COMMENT:
		case ENTITY_DECLARATION:
			return mEQhead.chars;

		default:
			throw new IllegalStateException();
		}
	}

	/**
	 * Returns the offset into the text character array at which the first
	 * character of the data for the current event is located.
	 *
	 * @return the offset of the first character
	 * @throws java.lang.IllegalStateException if this state is not a valid
	 *   text state.
	 */
	public int getTextStart()
	{
		switch (getEventType()) {
		case COMMENT:
		case ENTITY_DECLARATION:
			return 0;

		default:
			throw new IllegalStateException();
		}
	}

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
	public int getTextLength()
	{
		switch (getEventType()) {
		case COMMENT:
		case ENTITY_DECLARATION:
			return mEQhead.num;

		default:
			throw new IllegalStateException();
		}
	}

	/**
	 * Returns the current location of the processor.
	 * If the location is unknown the processor should return an 
	 * implementation 
	 * of {@link Location Location} that returns <code>-1</code> for the 
	 * location and <code>null</code> for each of the publicId and systemId. 
	 * The location information is only valid until {@link #next() next()} is 
	 * called.
	 *
	 * @return the {@link Location Location} object
	 */
	public Location getLocation()
	{
		return this;
	}

	/**
	 * Returns the public ID of the XML
	 *
	 * @return the public ID, or null if not available
	 */
	public String getPublicId()
	{
		return mStream.getPublicId();
	}

	/**
	 * Returns the system ID of the XML
	 *
	 * @return the system ID, or null if not available
	 */
	public String getSystemId()
	{
		return mStream.getSystemId();
	}

	/**
	 * Return the line number where the current event ends,
	 * returns -1 if none is available.
	 *
	 * @return the current line number
	 */
	public int getLineNumber()
	{
		return (mStream.getEventType() == XMLStreamReader.DTD)? 
			mStream.getLineNumber(): -1;
	}

	/**
	 * Return the column number where the current event ends,
	 * returns -1 if none is available.
	 *
	 * @return the current column number
	 */
	public int getColumnNumber()
	{
		return (mStream.getEventType() == XMLStreamReader.DTD)? 
			mStream.getColumnNumber(): -1;
	}

	/**
	 * Return the byte or character offset into the input source this location
	 * is pointing to. If the input source is a file or a byte stream then 
	 * this is the byte offset into that stream, but if the input source is 
	 * a character media then the offset is the character offset. 
	 * Returns -1 if there is no offset available.
	 *
	 * @return the current offset
	 */
	public int getCharacterOffset()
	{
		return (mStream.getEventType() == XMLStreamReader.DTD)? 
			mStream.getCharacterOffset(): -1;
	}

	/**
	 * Returns the target of a processing instruction.
	 *
	 * @return the target or <code>null</code>
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getPITarget()
	{
		if (getEventType() != PROCESSING_INSTRUCTION)
			throw new IllegalStateException();
		return mEQhead.name;
	}

	/**
	 * Returns the data section of a processing instruction.
	 *
	 * @return the data or <code>null</code>
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getPIData()
	{
		if (getEventType() != PROCESSING_INSTRUCTION)
			throw new IllegalStateException();
		return mEQhead.value;
	}

	/**
	 * Returns the qualified name.
	 *
	 * @return the qualified name
	 * @throws java.lang.IllegalStateException if this state is not a valid
	 *   text state.
	 */
	public String getName()
	{
		switch (getEventType()) {
		case NOTATION_DECLARATION:
		case ENTITY_DECLARATION:
		case UNPARSED_ENTITY_DECLARATION:
		case START_DTD:
			return mEQhead.name;

		default:
			throw new IllegalStateException();
		}
	}

	/**
	 * Returns the public identifier.
	 *
	 * @return the public identifier, or null if not available
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getPublicIdentifier()
	{
		switch (getEventType()) {
		case ENTITY_DECLARATION:
			if (mEQhead.num >= 0)
				return null;
		case START_DTD:
		case NOTATION_DECLARATION:
		case UNPARSED_ENTITY_DECLARATION:
			return mEQhead.list.name;

		default:
			throw new IllegalStateException();
		}
	}

	/**
	 * Returns the system identifier.
	 *
	 * @return the system identifier, or null if not available
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getSystemIdentifier()
	{
		switch (getEventType()) {
		case ENTITY_DECLARATION:
			if (mEQhead.num >= 0)
				return null;
		case START_DTD:
		case NOTATION_DECLARATION:
		case UNPARSED_ENTITY_DECLARATION:
			return mEQhead.list.value;

		default:
			throw new IllegalStateException();
		}
	}

	/**
	 * Returns the notation name.
	 *
	 * @return the notation name, or null if not available
	 * @throws java.lang.IllegalStateException if this method is not valid in
	 * the current state.
	 */
	public String getNotationName()
	{
		switch (getEventType()) {
		case UNPARSED_ENTITY_DECLARATION:
			return mEQhead.value;

		default:
			throw new IllegalStateException();
		}
	}

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
	public void close() 
		throws XMLStreamException
	{
		while (hasNext())
			next();
	}

	/**
	 * Adds an event to the head of internal event queue.
	 *
	 * @param evt An event to add to the tail of the queue.
	 */
	/* pkg */ void eqAdd(Pair evt)
	{
		evt.next = null;
		if (mEQtail != null)
			mEQtail.next = evt;
		else
			mEQhead = evt;
		mEQtail = evt;
	}

	/**
	 * Retrieves an event from internal event queue.
	 *
	 * @return An event or <code>null</code> if queue is empty.
	 */
	/* pkg */ Pair eqGet()
	{
		Pair evt = mEQhead;
		if (evt != null) {
			mEQhead  = evt.next;
			evt.next = null;
		}
		if (mEQhead == null)
			mEQtail = null;

		return evt;
	}
}
