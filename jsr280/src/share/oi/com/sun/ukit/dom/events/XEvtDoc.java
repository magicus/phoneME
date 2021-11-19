/*
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

package com.sun.ukit.dom;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.DocumentType;
import org.w3c.dom.DOMException;
import org.w3c.dom.events.Event;
import org.w3c.dom.events.DocumentEvent;
import org.w3c.dom.views.AbstractView;
import org.w3c.dom.views.DocumentView;

/**
 * DOM document node implementation.
 *
 * @see org.w3c.dom.Document
 * @see org.w3c.dom.event.DocumentEvent
 * @see org.w3c.dom.view.DocumentView
 */

public abstract class XEvtDoc
	extends XDoc
	implements DocumentEvent, DocumentView
{
	/** Event types defined in DOM3 Events */
	public final static String ET_LOAD   = "load";
	public final static String ET_UNLOAD = "unload";
	public final static String ET_ABORT  = "abort";
	public final static String ET_ERROR  = "error";
	public final static String ET_SELECT = "select";
	public final static String ET_CHANGE = "change";
	public final static String ET_SUBMIT = "submit";
	public final static String ET_RESET  = "reset";
	/** Event types defined in DOM3 UIEvents */
	public final static String ET_RESIZE   = "resize";
	public final static String ET_SCROLL   = "scroll";
	public final static String ET_FOCUS    = "focus";
	public final static String ET_BLUR     = "blur";
	public final static String ET_ACTIVATE = "DOMActivate";
	public final static String ET_FOCUSIN  = "DOMFocusIn";
	public final static String ET_FOCUSOUT = "DOMFocusOut";
	/** Event types defined in DOM3 TextEvents */
	public final static String ET_TXTINPUT = "textInput";
	/** Event types defined in DOM3 MouseEvents */
	public final static String ET_CLICK = "click";
	public final static String ET_MDOWN = "mousedown";
	public final static String ET_MUP   = "mouseup";
	public final static String ET_MOVER = "mouseover";
	public final static String ET_MMOVE = "mousemove";
	public final static String ET_MOUT  = "mouseout";
	/** Event types defined in DOM3 KeyboardEvents */
	public final static String ET_KDOWN = "keydown";
	public final static String ET_KUP   = "keyup";
	/** Event types defined in DOM3 MutationEvents */
	public final static String ET_UTREE  = "DOMSubtreeModified";
	public final static String ET_UNINS  = "DOMNodeInserted";
	public final static String ET_UNRM   = "DOMNodeRemoved";
	public final static String ET_UDINS  = "DOMNodeInsertedIntoDocument";
	public final static String ET_UDRM   = "DOMNodeRemovedFromDocument";
	public final static String ET_UAMOD  = "DOMAttrModified";
	public final static String ET_UCMOD  = "DOMCharacterDataModified";

	/** Pull of lists */
	private XList lists;

	/**
	 * Constructs document object from its document type and DOM implementation 
	 * object.
	 */
	protected XEvtDoc(DOMImplementation implementation)
	{
		super(implementation);

		//		Event types defined in DOM3 Events
		_intern(ET_LOAD);
		_intern(ET_UNLOAD);
		_intern(ET_ABORT);
		_intern(ET_ERROR);
		_intern(ET_SELECT);
		_intern(ET_CHANGE);
		_intern(ET_SUBMIT);
		_intern(ET_RESET);
		//		Event types defined in DOM3 UIEvents
		_intern(ET_RESIZE);
		_intern(ET_SCROLL);
		_intern(ET_FOCUS);
		_intern(ET_BLUR);
		_intern(ET_ACTIVATE);
		_intern(ET_FOCUSIN);
		_intern(ET_FOCUSOUT);
		//		Event types defined in DOM3 TextEvents
		_intern(ET_TXTINPUT);
		//		Event types defined in DOM3 MouseEvents
		_intern(ET_CLICK);
		_intern(ET_MDOWN);
		_intern(ET_MUP);
		_intern(ET_MOVER);
		_intern(ET_MMOVE);
		_intern(ET_MOUT);
		//		Event types defined in DOM3 KeyboardEvents
		_intern(ET_KDOWN);
		_intern(ET_KUP);
		//		Event types defined in DOM3 MutationEvents
		_intern(ET_UTREE);
		_intern(ET_UNINS);
		_intern(ET_UNRM);
		_intern(ET_UDINS);
		_intern(ET_UDRM);
		_intern(ET_UAMOD);
		_intern(ET_UCMOD);
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public abstract short getNodeType();

	/**
	 * The default <code>AbstractView</code> for this <code>Document</code>, 
	 * or <code>null</code> if none available.
	 */
	public abstract AbstractView getDefaultView();

	/**
	 *
	 * @param eventType  The <code>eventType</code> parameter specifies the 
	 *   name of the DOM Events interface to be supported by the created 
	 *   event object, e.g. <code>"Event"</code>, <code>"MouseEvent"</code>, 
	 *   <code>"MutationEvent"</code> and so on. If the <code>Event</code> 
	 *   is to be dispatched via the <code>EventTarget.dispatchEvent()</code>
	 *    method the appropriate event init method must be called after 
	 *   creation in order to initialize the <code>Event</code>'s values.  
	 *   As an example, a user wishing to synthesize some kind of 
	 *   <code>UIEvent</code> would invoke 
	 *   <code>DocumentEvent.createEvent("UIEvent")</code>. The 
	 *   <code>UIEvent.initUIEventNS()</code> method could then be called on 
	 *   the newly created <code>UIEvent</code> object to set the specific 
	 *   type of user interface event to be dispatched, DOMActivate for 
	 *   example, and set its context information, e.g. 
	 *   <code>UIEvent.detail</code> in this example. 
	 * <p><b>Note:</b>    For backward compatibility reason, "UIEvents", 
	 *   "MouseEvents", "MutationEvents", and "HTMLEvents" feature names are 
	 *   valid values for the parameter <code>eventType</code> and represent 
	 *   respectively the interfaces "UIEvent", "MouseEvent", 
	 *   "MutationEvent", and "Event". 
	 * @return  The newly created event object. 
	 * @exception DOMException
	 *    NOT_SUPPORTED_ERR: Raised if the implementation does not support the 
	 *   <code>Event</code> interface requested. 
	 */
	public final Event createEvent(String eventType)
		throws DOMException
	{
		XEvt evt  = _newEvent(eventType);
		evt.owner = this;
		return evt;
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
		//		Only final implementation of document knows what is supported
		return false;
	}

	/**
	 * Creates a new event object of specified event type. 
	 *
	 * @return  The newly created event object. 
	 * @exception DOMException
	 *    NOT_SUPPORTED_ERR: Raised if the implementation does not support the 
	 *   <code>Event</code> interface requested. 
	 */
	protected XEvt _newEvent(String eventType)
		throws DOMException
	{
		if (eventType.equals("MutationEvent") ||
			eventType.equals("MutationEvents")) {
			return new MutEvtImp();
		} else if (eventType.equals("Event") || 
			eventType.equals("HTMLEvents")) {
			return new EvtImp();
		} else if (eventType.equals("UIEvent") || 
			eventType.equals("UIEvents")) {
			return new UIEvtImp();
		} else if (eventType.equals("MouseEvent") ||
			eventType.equals("MouseEvents")) {
			return new MouseEvtImp();
		} else if (eventType.equals("MouseWheelEvent")) {
			return new MouseWheelEvtImp();
		} else if (eventType.equals("KeyboardEvent")) {
			return new KbdEvtImp();
		} else if (eventType.equals("TextEvent")) {
			return new TxtEvtImp();
		} else if (eventType.equals("ProgressEvent")) {
			return new ProgressEvtImp();
		} else if (eventType.equals("CustomEvent")) {
			return new CustomEvtImp();
		}
		throw new DOMException(DOMException.NOT_SUPPORTED_ERR, "");
	}

	/**
	 * Retrieves an empty list object. 
	 *
	 * @return An empty list object.
	 */
	protected final XList _getList()
	{
		if (lists == null)
			return new XList();

		XList list = lists;
		lists = list.next;
		return list;
	}

	/**
	 * Releases a list object. 
	 *
	 * @param list A list object to release.
	 */
	protected final void _free(XList list)
	{
		list.next = lists;
		lists = list;
		list._empty();
	}
}
