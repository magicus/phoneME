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

package com.sun.ukit.dom;

import java.util.Hashtable;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.DocumentType;

import org.w3c.dom.views.AbstractView;

/**
 * DOM document node implementation.
 *
 * @see org.w3c.dom.Document
 * @see org.w3c.dom.events.DocumentEvent
 * @see org.w3c.dom.views.DocumentView
 */

/* pkg */ final class DocImp
	extends XEvtDoc
{
	/** Default list of supported features */
	private final static String[] FEATURES = {"Core", "XML", "Events"};

	/** Hashtable of supported event types */
	private Hashtable evttyp;

	/**
	 * Constructs document object from its document type and DOM implementation 
	 * object.
	 */
	 /* pkg */ DocImp(DOMImplementation implementation)
	 {
	 	super(implementation);
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public short getNodeType()
	{
		return DOCUMENT_NODE;
	}

	/**
	 * The default <code>AbstractView</code> for this <code>Document</code>, 
	 * or <code>null</code> if none available.
	 */
	public AbstractView getDefaultView()
	{
		return null;
	}

	/**
	 * Test if the implementation can generate events of a specified type. 
	 *
	 * @param namespaceURI  Specifies the <code>Event.namespaceURI</code> of 
	 *   the event. 
	 * @param type  Specifies the <code>Event.type</code> of the event. 
	 * @return  <code>true</code> if the implementation can generate and 
	 *   dispatch this event type, <code>false</code> otherwise. 
	 *
	 * @since DOM Level 3
	 */
	public boolean canDispatch(String namespaceURI, String type)
	{
		if (namespaceURI != null || type.equals(""))
			return false;

		if (evttyp == null) {
			//		Event types defined in DOM3 Events
			evttyp = new Hashtable(32);
			evttyp.put(ET_LOAD, ET_LOAD);
			evttyp.put(ET_UNLOAD, ET_UNLOAD);
			evttyp.put(ET_ABORT, ET_ABORT);
			evttyp.put(ET_ERROR, ET_ERROR);
			evttyp.put(ET_SELECT, ET_SELECT);
			evttyp.put(ET_CHANGE, ET_CHANGE);
			evttyp.put(ET_SUBMIT, ET_SUBMIT);
			evttyp.put(ET_RESET, ET_RESET);
			//		Event types defined in DOM3 UIEvents
			evttyp.put(ET_RESIZE, ET_RESIZE);
			evttyp.put(ET_SCROLL, ET_SCROLL);
			evttyp.put(ET_FOCUS, ET_FOCUS);
			evttyp.put(ET_BLUR, ET_BLUR);
			evttyp.put(ET_ACTIVATE, ET_ACTIVATE);
			evttyp.put(ET_FOCUSIN, ET_FOCUSIN);
			evttyp.put(ET_FOCUSOUT, ET_FOCUSOUT);
			//		Event types defined in DOM3 TextEvents
			evttyp.put(ET_TXTINPUT, ET_TXTINPUT);
			//		Event types defined in DOM3 MouseEvents
			evttyp.put(ET_CLICK, ET_CLICK);
			evttyp.put(ET_MDOWN, ET_MDOWN);
			evttyp.put(ET_MUP, ET_MUP);
			evttyp.put(ET_MOVER, ET_MOVER);
			evttyp.put(ET_MMOVE, ET_MMOVE);
			evttyp.put(ET_MOUT, ET_MOUT);
			//		Event types defined in DOM3 KeyboardEvents
			evttyp.put(ET_KDOWN, ET_KDOWN);
			evttyp.put(ET_KUP, ET_KUP);
			//		Event types defined in DOM3 MutationEvents
			evttyp.put(ET_UTREE, ET_UTREE);
			evttyp.put(ET_UNINS, ET_UNINS);
			evttyp.put(ET_UNRM, ET_UNRM);
			evttyp.put(ET_UDINS, ET_UDINS);
			evttyp.put(ET_UDRM, ET_UDRM);
			evttyp.put(ET_UAMOD, ET_UAMOD);
			evttyp.put(ET_UCMOD, ET_UCMOD);
		}

		return (evttyp.get(type) != null);
	}

	/**
	 * Returns list of supported features.
	 */
	protected String[] _getFeatures()
	{
		return FEATURES;
	}
}
