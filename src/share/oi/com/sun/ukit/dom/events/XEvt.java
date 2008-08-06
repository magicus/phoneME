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

import org.w3c.dom.DOMException;

import org.w3c.dom.events.EventTarget;
import org.w3c.dom.events.Event;
import org.w3c.dom.events.EventException;

/**
 * Basic DOM event implementation.
 *
 * @see org.w3c.dom.events.Event
 */

public abstract class XEvt
	implements Event
{
	/** Event's time stamp */
	protected long tstamp;
	/** Event's document owner */
	/* pkg */ XDoc owner;
	/** Event's type */
	private String evttype;
	/** Event's namespace URI */
	private String evtns;
	/** Main target of the event */
	private XNode mtarget;
	/** Current target of the event */
	private XNode ctarget;
	/** Current event propagation phase */
	private short propphase;

	/** Stop propagation flag */
	protected final static char FLAG_STOPPROP   = 0x0001;
	/** Prevent default action flag */
	protected final static char FLAG_PREVDEF    = 0x0002;
	/** Support of bubbling phase flag */
	protected final static char FLAG_BUBBLES    = 0x0004;
	/** Cancelable flag */
	protected final static char FLAG_CANCELABLE = 0x0008;
	/** Event flags */
	private char flags;

	/**
	 * Default constructor.
	 */
	protected XEvt()
	{
		propphase = -1;
	}

	/**
	 * The name of the event (case-insensitive). The name must be an XML name.
	 */
	public final String getType()
	{
		return evttype;
	}

	/**
	 * Used to indicate the <code>EventTarget</code> to which the event was 
	 * originally dispatched. 
	 */
	public final EventTarget getTarget()
	{
		return mtarget;
	}

	/**
	 * Used to indicate the <code>EventTarget</code> whose 
	 * <code>EventListeners</code> are currently being processed. This is 
	 * particularly useful during capturing and bubbling. 
	 */
	public final EventTarget getCurrentTarget()
	{
		return ctarget;
	}

	/**
	 * The namespace URI associated with this event at creation time, or 
	 * <code>null</code> if it is unspecified. 
	 * <br> For events initialized with a DOM Level 2 Events method 
	 * this is always <code>null</code>. 
	 *
	 * @since DOM Level 3
	 */
	public final String getNamespaceURI()
	{
		return evtns;
	}

	/**
	 * Used to indicate which phase of event flow is currently being 
	 * evaluated. 
	 */
	public final short getEventPhase()
	{
		return propphase;
	}

	/**
	 * Used to indicate whether or not an event is a bubbling event. If the 
	 * event can bubble the value is true, else the value is false. 
	 */
	public final boolean getBubbles()
	{
		return ((flags & FLAG_BUBBLES) != 0);
	}

	/**
	 * Used to indicate whether or not an event can have its default action 
	 * prevented. If the default action can be prevented the value is true, 
	 * else the value is false. 
	 */
	public final boolean getCancelable()
	{
		return ((flags & FLAG_CANCELABLE) != 0);
	}

	/**
	 * Used to indicate whether <code>Event.preventDefault()</code> has been 
	 * called for this event. 
	 *
	 * @since DOM Level 3
	 */
	public final boolean getDefaultPrevented()
	{
		return ((flags & FLAG_PREVDEF) != 0);
	}

	/**
	 * Used to specify the time (in milliseconds relative to the epoch) at 
	 * which the event was created. Due to the fact that some systems may 
	 * not provide this information the value of <code>timeStamp</code> may 
	 * be not available for all events. When not available, a value of 0 
	 * will be returned. Examples of epoch time are the time of the system 
	 * start or 0:0:0 UTC 1st January 1970. 
	 */
	public final long getTimeStamp()
	{
		return tstamp;
	}

	/**
	 * The <code>stopPropagation</code> method is used prevent further 
	 * propagation of an event during event flow. If this method is called 
	 * by any <code>EventListener</code> the event will cease propagating 
	 * through the tree. The event will complete dispatch to all listeners 
	 * on the current <code>EventTarget</code> before event flow stops. This 
	 * method may be used during any stage of event flow.
	 */
	public final void stopPropagation()
	{
		flags |= FLAG_STOPPROP;
	}

	/**
	 * If an event is cancelable, the <code>preventDefault</code> method is 
	 * used to signify that the event is to be canceled, meaning any default 
	 * action normally taken by the implementation as a result of the event 
	 * will not occur. If, during any stage of event flow, the 
	 * <code>preventDefault</code> method is called the event is canceled. 
	 * Any default action associated with the event will not occur. Calling 
	 * this method for a non-cancelable event has no effect. Once 
	 * <code>preventDefault</code> has been called it will remain in effect 
	 * throughout the remainder of the event's propagation. This method may 
	 * be used during any stage of event flow. 
	 */
	public final void preventDefault()
	{
		if (getCancelable() == true)
			flags |= FLAG_PREVDEF;
	}

	/**
	 * The <code>initEvent</code> method is used to initialize the value of an 
	 * <code>Event</code> created through the <code>DocumentEvent</code> 
	 * interface. This method may only be called before the 
	 * <code>Event</code> has been dispatched via the 
	 * <code>dispatchEvent</code> method, though it may be called multiple 
	 * times during that phase if necessary. If called multiple times the 
	 * final invocation takes precedence. If called from a subclass of 
	 * <code>Event</code> interface only the values specified in the 
	 * <code>initEvent</code> method are modified, all other attributes are 
	 * left unchanged.
	 * @param eventTypeArg Specifies the event type. This type may be any 
	 *   event type currently defined in this specification or a new event 
	 *   type. The string must be an XML name. Any new event type must not 
	 *   begin with any upper, lower, or mixed case version of the string 
	 *   "DOM". This prefix is reserved for future DOM event sets. It is 
	 *   also strongly recommended that third parties adding their own 
	 *   events use their own prefix to avoid confusion and lessen the 
	 *   probability of conflicts with other new events.
	 * @param canBubbleArg Specifies whether or not the event can bubble.
	 * @param cancelableArg Specifies whether or not the event's default 
	 *   action can be prevented.
	 */
	public final void initEvent(
		String eventTypeArg, boolean canBubbleArg, boolean cancelableArg)
	{
		initEventNS(null, eventTypeArg, canBubbleArg, cancelableArg);
	}

	/**
	 * The <code>initEventNS</code> method is used to initialize the value of 
	 * an <code>Event</code> object and has the same behavior as 
	 * <code>Event.initEvent()</code>. 
	 * @param namespaceURIArg  Specifies <code>Event.namespaceURI</code>, the 
	 *   namespace URI associated with this event, or <code>null</code> if 
	 *   no namespace. 
	 * @param eventTypeArg  Refer to the <code>Event.initEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param canBubbleArg  Refer to the <code>Event.initEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param cancelableArg  Refer to the <code>Event.initEvent()</code> 
	 *   method for a description of this parameter.
     *
	 * @since DOM Level 3
	 */
	public final void initEventNS(String namespaceURIArg, 
		String eventTypeArg, boolean canBubbleArg, boolean cancelableArg)
	{
		if (propphase >= Event.CAPTURING_PHASE)
			return;

		evtns   = owner._intern(namespaceURIArg);
		evttype = owner._intern(eventTypeArg);

		if (canBubbleArg)
			flags |= FLAG_BUBBLES;
		else
			flags &= ~FLAG_BUBBLES;
		if (cancelableArg)
			flags |= FLAG_CANCELABLE;
		else
			flags &= ~FLAG_CANCELABLE;
	}

	/**
	 * Sets event main target.
	 */
	/* pkg */ final void _setTarget(XNode node)
	{
		mtarget = node;
	}

	/**
	 * Sets event current target.
	 */
	/* pkg */ final void _setCurrentTarget(XNode node)
	{
		ctarget = node;
	}

	/**
	 * Sets event propagation phase.
	 *
	 * @phase One of event propagation phases or -1 for event which can be 
	 *   dispatched.
	 * @return <code>true</code> if event supports this propagation phase and 
	 *   <code>false</code> otherwise.
	 */
	/* pkg */ final boolean _setPhase(short phase)
	{
		switch (phase) {
		case Event.CAPTURING_PHASE:
		case Event.AT_TARGET:
		case Event.BUBBLING_PHASE:
			if (_phaseSupported(phase)) {
				propphase = phase;
				return true;
			}
			break;

		case -1:
			propphase = phase;
			return true;

		default:
		}

		return false;
	}

	/**
	 * Retrieves stop propagation flag.
	 *
	 * @return <code>true</code> if propagation had been stopped, otherwise 
	 *   <code>false</code>.
	 */
	/* pkg */ final boolean _isStopped()
	{
		return ((flags & FLAG_STOPPROP) != 0);
	}

	/**
	 * Verifies that the node is acceptable target node for this type of event. 
	 * This method is called from <code>dispatchEvent</code> method before 
	 * start of event propagation. An event implementation may override this 
	 * method in order to apply an event specific restrictions or initialization. 
	 * However, an event implementation in this case must chain its own 
	 * implementation of this method with implementation of this method.
	 *
	 * @exception EventException
	 *   UNSPECIFIED_EVENT_TYPE_ERR: Raised if the <code>Event.type</code> 
	 *   was not specified by initializing the event before 
	 *   <code>dispatchEvent</code> was called. Specification of the 
	 *   <code>Event.type</code> as <code>null</code> or an empty string 
	 *   will also trigger this exception. 
	 *   <br/> DISPATCH_REQUEST_ERR: Raised if the <code>Event</code> object is 
	 *   already being dispatched. 
	 * @exception DOMException
	 *   NOT_SUPPORTED_ERR: Raised if the <code>Event</code> object has not 
	 *   been created using <code>DocumentEvent.createEvent()</code>. 
	 */
	protected void _canDispatch(XNode target)
		throws DOMException, EventException
	{
		if (owner == null || target._getDoc() != owner)
			throw new DOMException(DOMException.NOT_SUPPORTED_ERR, getType());

		if (propphase >= Event.CAPTURING_PHASE)
			throw new EventException(
				EventException.DISPATCH_REQUEST_ERR, getType());
	
		if (evttype == null || evttype.length() == 0)
			throw new EventException(
				EventException.UNSPECIFIED_EVENT_TYPE_ERR, getType());
	
		flags &= ~(FLAG_STOPPROP | FLAG_PREVDEF);
		tstamp = System.currentTimeMillis();
	}

	/**
	 * Controls whether event supports provided event propagation phase. If this 
	 * method returns <code>false</code>, the phase will be skipped. I.e. non 
	 * event listener registered for this phase on any node will be called. An 
	 * event implementation should override this method in order to control 
	 * event flow of particular type of event. For more details see 
	 * <a href="http://www.w3.org/TR/DOM-Level-3-Events/events.html#Events-EventTypes-complete"
	 * >DOM3 Events specification #1.5.2</a>
	 *
	 * @phase One of event propagation phases defined on <code>Event</code> 
	 *   interface.
	 * @return <code>true</code> if event supports provided propagation phase 
	 *   and <code>false</code> otherwise.
	 */
	protected boolean _phaseSupported(short phase)
	{
		if (phase != Event.BUBBLING_PHASE)
			return true;

		return getBubbles();
	}
}
