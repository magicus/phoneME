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

/*
 * Copyright (c) 2006 World Wide Web Consortium,
 *
 * (Massachusetts Institute of Technology, European Research Consortium for
 * Informatics and Mathematics, Keio University). All Rights Reserved. This
 * work is distributed under the W3C(r) Software License [1] in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * [1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
 */

package org.w3c.dom.events;

import org.w3c.dom.DOMException;

/**
 *  The <code>EventTarget</code> interface is implemented by all 
 * <code>Nodes</code> in an implementation which supports the DOM Event 
 * Model. Therefore, this interface can be obtained by using 
 * binding-specific casting methods on an instance of the <code>Node</code> 
 * interface. The interface allows registration and removal of 
 * <code>EventListeners</code> on an <code>EventTarget</code> and dispatch 
 * of events to that <code>EventTarget</code>.
 * <p>See also the 
 * <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Events-20001113'>
 * Document Object Model (DOM) Level 2 Events Specification</a>.
 *
 * @since DOM Level 2
 */
public interface EventTarget {
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
    public void addEventListener(String type, 
                                 EventListener listener, 
                                 boolean useCapture);

    /**
     * This method allows the removal of event listeners from the event 
     * target. If an <code>EventListener</code> is removed from an 
     * <code>EventTarget</code> while it is processing an event, it will not 
     * be triggered by the current actions. <code>EventListener</code>s can 
     * never be invoked after being removed.
     * <br>Calling <code>removeEventListener</code> with arguments which do 
     * not identify any currently registered <code>EventListener</code> on 
     * the <code>EventTarget</code> has no effect.
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
    public void removeEventListener(String type, 
                                    EventListener listener, 
                                    boolean useCapture);

    /**
     *  This method allows the registration of an event listener in a 
     * specified group or the default group and, depending on the 
     * <code>useCapture</code> parameter, on the capture phase of the DOM 
     * event flow or its target and bubbling phases. 
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
     * @since DOM Level 3
     */
    public void addEventListenerNS(String namespaceURI, 
                                   String type, 
                                   EventListener listener, 
                                   boolean useCapture);

    /**
     *  This method allows the removal of an event listener, independently of 
     * the associated event group. Calling <code>removeEventListenerNS</code>
     *  with arguments which do not identify any currently registered 
     * <code>EventListener</code> on the <code>EventTarget</code> has no 
     * effect. 
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
     * @since DOM Level 3
     */
    public void removeEventListenerNS(String namespaceURI, 
                                      String type, 
                                      EventListener listener, 
                                      boolean useCapture);

    /**
     *  This method allows the dispatch of events into the implementation's 
     * event model. The event target of the event is the 
     * <code>EventTarget</code> object on which <code>dispatchEvent</code> 
     * is called. 
     * <br><i>Clarification:</i> Independently on whether the event target
     * supports the given event or not, <code>Event</code> must be dispatched 
     * to that event target. No checking on the semantic correctness of the 
     * request to dispatch the event object is performed by the 
     * implementation.
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
                                 throws EventException, DOMException;
}
