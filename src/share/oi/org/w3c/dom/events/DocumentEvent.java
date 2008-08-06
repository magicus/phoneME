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
 *  The <code>DocumentEvent</code> interface provides a mechanism by which the 
 * user can create an <code>Event</code> object of a type supported by the 
 * implementation. If the feature "Events" is supported by the 
 * <code>Document</code> object, the <code>DocumentEvent</code> interface 
 * must be implemented on the same object. 
 * Note that JSR 280 does not support the 3.0 Events feature, but the object
 * returned to a 2.0 Events request will include the DocumentEvent interface
 * implementation.
 * <p>See also the 
 * <a href='http://www.w3.org/TR/2006/WD-DOM-Level-3-Events-20060413'>
 * Document Object Model (DOM) Level 3 Events Specification</a>.
 *
 * @since DOM Level 2
 */

public interface DocumentEvent {

    /**
     * Create an Event.
     *
     * @param eventType  The <code>eventType</code> parameter specifies the 
     *   name of the DOM Events interface to be supported by the created 
     *   event object, e.g. <code>"Event"</code>, <code>"MouseEvent"</code>, 
     *   <code>"MutationEvent"</code> and so on. If the <code>Event</code> 
     *   is to be dispatched via the <code>EventTarget.dispatchEvent()</code>
     *   method the appropriate event init method must be called after 
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
     * <p><b>Note:</b>  JSR 280 follows the DOM 3 rule for <code>Event.type</code>
     * and considers it to be case-sensitive. This differs from DOM 2, which
     * considers it to be case-insensitive.
     * @return  The newly created event object. 
     * @exception DOMException
     *    NOT_SUPPORTED_ERR: Raised if the implementation does not support the 
     *   <code>Event</code> interface requested. 
     */
    public Event createEvent(String eventType)
                             throws DOMException;

    /**
     *  Test if the implementation can generate events of a specified type. 
     * @param namespaceURI  Specifies the <code>Event.namespaceURI</code> of 
     *   the event, may be null. 
     * @param type  Specifies the <code>Event.type</code> of the event. 
     * @return  <code>true</code> if the implementation can generate and 
     *   dispatch this event type, <code>false</code> otherwise. 
     *
     * @since DOM Level 3
     */
    public boolean canDispatch(String namespaceURI, String type);
}

