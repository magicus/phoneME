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

import org.w3c.dom.Node;
import org.w3c.dom.Document;
import org.w3c.dom.DOMException;

import org.w3c.dom.events.Event;
import org.w3c.dom.events.MutationEvent;
import org.w3c.dom.events.EventTarget;
import org.w3c.dom.events.EventListener;
import org.w3c.dom.events.EventException;

/**
 * Implementation of DOM events propagation layer.
 */

public abstract class XNode
	extends XBase
	implements EventTarget
{
	/** Event listeners registered on the node. Each event listener represented 
	    by four objects with following offsets: 0 - EventListener object, 
	    1 - namespace URI string, 2 - event type string, 3 - use capture phase 
	    Boolean.FALSE or Boolean.TRUE object. */
	private Object[] evtlst;

	/**
	 * Constructs node object from its owner document.
	 */
	protected XNode(XDoc ownerDocument)
	{
	 	super(ownerDocument);
	}

	/**
	 * Constructs node object from other node.
	 */
	 protected XNode(XNode node, boolean deep)
	 {
		super(node, deep);
	 }

	/**
	 * Constructs node object from its qualified name and namespace URI and 
	 * its owner document.
	 */
	 protected XNode(String namespaceURI, String qName, XDoc ownerDocument)
	 {
		super(namespaceURI, qName, ownerDocument);
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public abstract short getNodeType();

	/**
	 * This method allows the registration of event listeners on the event 
	 * target. If an <code>EventListener</code> is added to an 
	 * <code>EventTarget</code> while it is processing an event, it will not 
	 * be triggered by the current actions but may be triggered during a 
	 * later stage of event flow, such as the bubbling phase. 
	 * <br> If multiple identical <code>EventListener</code>s are registered 
	 * on the same <code>EventTarget</code> with the same parameters the 
	 * duplicate instances are discarded. They do not cause the 
	 * <code>EventListener</code> to be called twice and since they are 
	 * discarded they do not need to be removed with the 
	 * <code>removeEventListener</code> method. 
	 *
	 * @param type The event type for which the user is registering
	 * @param listener The <code>listener</code> parameter takes an interface 
	 *   implemented by the user which contains the methods to be called 
	 *   when the event occurs.
	 * @param useCapture If true, <code>useCapture</code> indicates that the 
	 *   user wishes to initiate capture. After initiating capture, all 
	 *   events of the specified type will be dispatched to the registered 
	 *   <code>EventListener</code> before being dispatched to any 
	 *   <code>EventTargets</code> beneath them in the tree. Events which 
	 *   are bubbling upward through the tree will not trigger an 
	 *   <code>EventListener</code> designated to use capture.
	 */
	public void addEventListener(
			String type, EventListener listener, boolean useCapture)
	{
		addEventListenerNS(null, type, listener, useCapture);
	}

	/**
	 * This method allows the removal of event listeners from the event 
	 * target. If an <code>EventListener</code> is removed from an 
	 * <code>EventTarget</code> while it is processing an event, it will not 
	 * be triggered by the current actions. <code>EventListener</code>s can 
	 * never be invoked after being removed.
	 * <br>Calling <code>removeEventListener</code> with arguments which do 
	 * not identify any currently registered <code>EventListener</code> on 
	 * the <code>EventTarget</code> has no effect.
	 *
	 * @param type Specifies the event type of the <code>EventListener</code> 
	 *   being removed. 
	 * @param listener The <code>EventListener</code> parameter indicates the 
	 *   <code>EventListener </code> to be removed. 
	 * @param useCapture Specifies whether the <code>EventListener</code> 
	 *   being removed was registered as a capturing listener or not. If a 
	 *   listener was registered twice, one with capture and one without, 
	 *   each must be removed separately. Removal of a capturing listener 
	 *   does not affect a non-capturing version of the same listener, and 
	 *   vice versa. 
	 */
	public void removeEventListener(
			String type, EventListener listener, boolean useCapture)
	{
		removeEventListenerNS(null, type, listener, useCapture);
	}

	/**
	 *  This method allows the registration of an event listener in a 
	 * specified group or the default group and, depending on the 
	 * <code>useCapture</code> parameter, on the capture phase of the DOM 
	 * event flow or its target and bubbling phases. 
	 *
	 * @param namespaceURI  Specifies the <code>Event.namespaceURI</code> 
	 *   associated with the event for which the user is registering. 
	 * @param type  Refer to the <code>EventTarget.addEventListener()</code> 
	 *   method for a description of this parameter. 
	 * @param listener  Refer to the 
	 *   <code>EventTarget.addEventListener()</code> method for a 
	 *   description of this parameter. 
	 * @param useCapture  Refer to the 
	 *   <code>EventTarget.addEventListener()</code> method for a 
	 *   description of this parameter. 
	 *
	 * @since DOM Level 3
	 */
	public void addEventListenerNS(String namespaceURI, String type, 
			EventListener listener, boolean useCapture)
	{
		if (type == null || type.length() == 0 || listener == null)
			return;
		int idx = _getIndex(listener, namespaceURI, type, useCapture);
		if (idx >= 0)  // there is already such listener in the list
			return;
		idx = (evtlst != null)? evtlst.length: 0;
		Object list[] = new Object[idx + (1 << 2)];
		if (evtlst != null)
			System.arraycopy(evtlst, 0, list, 0, idx);
		list[idx]     = listener;
		list[idx + 1] = _getDoc()._intern(namespaceURI);
		list[idx + 2] = _getDoc()._intern(type);
		list[idx + 3] = (useCapture == true)? Boolean.TRUE: Boolean.FALSE;
		evtlst = list;
	}

	/**
	 *  This method allows the removal of an event listener, independently of 
	 * the associated event group. Calling <code>removeEventListenerNS</code>
	 *  with arguments which do not identify any currently registered 
	 * <code>EventListener</code> on the <code>EventTarget</code> has no 
	 * effect. 
	 *
	 * @param namespaceURI  Specifies the <code>Event.namespaceURI</code> 
	 *   associated with the event for which the user registered the event 
	 *   listener. 
	 * @param type  Refer to the 
	 *   <code>EventTarget.removeEventListener()</code> method for a 
	 *   description of this parameter. 
	 * @param listener  Refer to the 
	 *   <code>EventTarget.removeEventListener()</code> method for a 
	 *   description of this parameter. 
	 * @param useCapture  Refer to the 
	 *   <code>EventTarget.removeEventListener()</code> method for a 
	 *   description of this parameter.   
	 *
	 * @since DOM Level 3
	 */
	public void removeEventListenerNS(String namespaceURI, String type, 
			EventListener listener, boolean useCapture)
	{
		if (type == null || type.length() == 0 || listener == null)
			return;
		int idx = _getIndex(listener, namespaceURI, type, useCapture);
		if (idx < 0)  // there is no such listener in the list
			return;
		if (evtlst.length == (1 << 2)) {
			//		There is only one listener in the list
			evtlst = null;
			return;
		}
		int len = evtlst.length - (1 << 2);
		Object list[] = new Object[len];
		if (idx == 0) {  // the first 
			System.arraycopy(evtlst, (1 << 2), list, 0, len);
		} else if (idx == (evtlst.length << 2)) {  // the last
			System.arraycopy(evtlst, 0, list, 0, len);
		} else {
			System.arraycopy(evtlst, 0, list, 0, idx << 2);
			System.arraycopy(evtlst, ((idx + 1) << 2), list, idx << 2, 
				evtlst.length - ((idx + 1) << 2));
		}
		evtlst = list;
	}

	/**
	 *  This method allows the dispatch of events into the implementation's 
	 * event model. The event target of the event is the 
	 * <code>EventTarget</code> object on which <code>dispatchEvent</code> 
	 * is called. 
	 *
	 * @param evt  The event to be dispatched. 
	 * @return  Indicates whether any of the listeners which handled the 
	 *   event called <code>Event.preventDefault()</code>. If 
	 *   <code>Event.preventDefault()</code> was called the returned value 
	 *   is <code>false</code>, else it is <code>true</code>. 
	 * @exception EventException
	 *    UNSPECIFIED_EVENT_TYPE_ERR: Raised if the <code>Event.type</code> 
	 *   was not specified by initializing the event before 
	 *   <code>dispatchEvent</code> was called. Specification of the 
	 *   <code>Event.type</code> as <code>null</code> or an empty string 
	 *   will also trigger this exception. 
	 *   <br> DISPATCH_REQUEST_ERR: Raised if the <code>Event</code> object is 
	 *   already being dispatched. 
	 * @exception DOMException
	 *    NOT_SUPPORTED_ERR: Raised if the <code>Event</code> object has not 
	 *   been created using <code>DocumentEvent.createEvent()</code>. 
	 *   <br> INVALID_CHARACTER_ERR: Raised if <code>Event.type</code> is not 
	 *   an 
	 *   <a href='http://www.w3.org/TR/2004/REC-xml-names11-20040204/#NT-NCName'>
	 *   NCName</a> as defined in [
	 *   <a href='http://www.w3.org/TR/2004/REC-xml-names11-20040204/'>
	 *   XML Namespaces 1.1</a>].
	 *
	 * @since DOM Level 3
	 */
	public boolean dispatchEvent(Event evt)
		throws EventException, DOMException
	{
		XEvt xevt;
		try {
			xevt = (XEvt)evt;
		} catch (ClassCastException cce) {
			throw new DOMException(DOMException.NOT_SUPPORTED_ERR, "");
		}
		xevt._canDispatch(this);

		XNode._checkName(xevt.getType(), false);

		xevt._setTarget(this);
		XList list = (_getParent() != null)? _getParent()._getPropChain(): null;
		//		Capturing phase
		if (list != null && xevt._setPhase(Event.CAPTURING_PHASE)) {
			for (int idx = 0; idx < list.getLength(); idx++)
				((XNode)(list.item(idx)))._dispatchEvt(xevt);
		}
		//		Target phase
		if (xevt._setPhase(Event.AT_TARGET)) {
			xevt._setCurrentTarget(this);
			_dispatchEvt(xevt);
		}
		//		Bubbling phase
		if (list != null && xevt._setPhase(Event.BUBBLING_PHASE)) {
			for (int idx = list.getLength() - 1; idx >= 0; idx--)
				((XNode)(list.item(idx)))._dispatchEvt(xevt);
		}
		if (list != null)
			((XEvtDoc)_getDoc())._free(list);
		xevt._setPhase((short)-1);
		return !xevt.getDefaultPrevented();
	}

	/**
	 * Dispatches an event to all listeners on this node.
	 *
	 * @param xevt Event object to dispatch.
	 */
	private final void _dispatchEvt(XEvt xevt)
	{
		if (evtlst == null || xevt._isStopped())
			return;

		Object list[] = evtlst;  // keep a copy of initial set of listeners
		int    length = list.length >> 2; // number of listeners
		xevt._setCurrentTarget(this);
		//		Dispatch event to all listeners
		listeners: for (int idx = 0; idx < length; idx++) {
			int base = idx << 2;
			switch (xevt.getEventPhase()) {
			case Event.CAPTURING_PHASE:
				if (list[base + 3] == Boolean.FALSE)  // useCapture == false
					continue listeners;
				break;

			case Event.BUBBLING_PHASE:
			case Event.AT_TARGET:
				if (list[base + 3] != Boolean.FALSE)  // useCapture == true
					continue listeners;
				break;
			}
			if (xevt.getNamespaceURI() != list[base + 1] ||
				xevt.getType() != list[base + 2])
				continue listeners;

			try {
				((EventListener)(list[base])).handleEvent(xevt);
			} catch (Throwable t) {
			}
		}
	}

	/**
	 * Retrieves event propagation chain of nodes. Note, _getParent for an 
	 * attribute node returns element node. If an event is dispatched on a 
	 * disconnected subtree, the root node (#1.2.1) is the topmost node which 
	 * has not parent but not Document node.
	 *
	 * @return A list which contains event propagation chain.
	 */
	/* pkg */ final XList _getPropChain()
	{
		XList list = (_getParent() != null)? 
			_getParent()._getPropChain(): ((XEvtDoc)_getDoc())._getList();
		list._append(this);
		return list;
	}

	/**
	 * Retrieves an index of the listener in the registered listeners list. 
	 *
	 * @return An index of the listener or -1 if this listener is not registered.
	 */
	/* pkg */ final int _getIndex(EventListener listener, 
		String namespaceURI, String type, boolean useCapture)
	{
		int length = (evtlst != null)? evtlst.length >> 2: 0;
		for (int idx = 0; idx < length; idx++) {
			if (evtlst[idx << 2] == listener) {
				int base = idx << 2;
				if (!((String)(evtlst[base + 2])).equals(type))
					continue;
				if (((Boolean)(evtlst[base + 3])).booleanValue() != useCapture)
					continue;
				if (namespaceURI == null && namespaceURI != evtlst[base + 1])
					continue;
				if (namespaceURI != null && 
					!namespaceURI.equals((String)(evtlst[base + 1])))
					continue;
				return idx;
			}
		}
		return -1;
	}

	/**
	 * Notification of child addition.
	 */
	protected void _childAdded(XNode child)
	{
		XEvtDoc doc = (XEvtDoc)_getDoc();
		if ((doc.flags & XDoc.FLAG_BUILD) != 0) 
			return;
		MutationEvent evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UNINS, true, false, 
			this, null, null, null, (short)0);
		child.dispatchEvent(evt);

		evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UTREE, true, false, 
			null, null, null, null, (short)0);
		dispatchEvent(evt);
	}

	/**
	 * Notification of child removal.
	 */
	protected void _childRemoving(XNode child)
	{
		XEvtDoc doc = (XEvtDoc)_getDoc();
		if ((doc.flags & XDoc.FLAG_BUILD) != 0) 
			return;
		MutationEvent evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UNRM, true, false, 
			this, null, null, null, (short)0);
		child.dispatchEvent(evt);

		evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UTREE, true, false, 
			null, null, null, null, (short)0);
		dispatchEvent(evt);
	}

	/**
	 * Notification of a child have been removed.
	 */
	protected void _childRemoved(XNode child)
	{
	}

	/**
	 * Notification of attribute added.
	 */
	protected void _attrAdded(String name, String value)
	{
		XEvtDoc doc = (XEvtDoc)_getDoc();
		if ((doc.flags & XDoc.FLAG_BUILD) != 0) 
			return;
		MutationEvent evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UAMOD, true, false, 
			null, name, null, value, MutationEvent.ADDITION);
		dispatchEvent(evt);

		evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UTREE, true, false, 
			null, null, null, null, (short)0);
		dispatchEvent(evt);
	}

	/**
	 * Notification of attribute change.
	 */
	protected void _attrChanged(String name, String oldValue, String newValue)
	{
		XEvtDoc doc = (XEvtDoc)_getDoc();
		if ((doc.flags & XDoc.FLAG_BUILD) != 0) 
			return;
		MutationEvent evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UAMOD, true, false, 
			null, name, oldValue, newValue, MutationEvent.MODIFICATION);
		dispatchEvent(evt);

		evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UTREE, true, false, 
			null, null, null, null, (short)0);
		dispatchEvent(evt);
	}

	/**
	 * Notification of attribute removed.
	 */
	protected void _attrRemoved(String name, String value)
	{
		XEvtDoc doc = (XEvtDoc)_getDoc();
		if ((doc.flags & XDoc.FLAG_BUILD) != 0) 
			return;
		MutationEvent evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UAMOD, true, false, 
			null, name, value, null, MutationEvent.REMOVAL);
		dispatchEvent(evt);

		evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UTREE, true, false, 
			null, null, null, null, (short)0);
		dispatchEvent(evt);
	}

	/**
	 * Notification of data change.
	 */
	protected void _dataChanged(String oldData, String newData)
	{
		XEvtDoc doc = (XEvtDoc)_getDoc();
		if ((doc.flags & XDoc.FLAG_BUILD) != 0) 
			return;
		MutationEvent evt = (MutationEvent)doc.createEvent("MutationEvent");
		evt.initMutationEventNS(null, XEvtDoc.ET_UCMOD, true, false, 
			this, oldData, newData, null, (short)0);
		dispatchEvent(evt);

		if (_getParent() != null) {
			evt = (MutationEvent)doc.createEvent("MutationEvent");
			evt.initMutationEventNS(null, XEvtDoc.ET_UTREE, true, false, 
				null, null, null, null, (short)0);
			_getParent().dispatchEvent(evt);
		}
	}
}
