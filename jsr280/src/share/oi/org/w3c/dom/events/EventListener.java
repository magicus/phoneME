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

/**
 *  The <code>EventListener</code> interface is the primary method for 
 * handling events. Users implement the <code>EventListener</code> interface 
 * and register their listener on an <code>EventTarget</code. The users 
 * should also remove their 
 * <code>EventListener</code> from its <code>EventTarget</code> after they 
 * have completed using the listener. 
 * <p> Copying a <code>Node</code> with the <code>Node.cloneNode</code>
 * method does not copy the event listeners attached to it. Event
 * listeners must be attached to the newly created <code>Node</code>
 * afterwards if so desired.
 * <p> Moving a <code>Node</code>, with methods <code>Document.adoptNode</code>
 * or <code>Node.appendChild</code>, does not affect the event listeners 
 * attached to it.
 * <p> The EventListener behavior follows the description in section
 * 1.2.2 of the DOM Level 3 Events specification. 
 * <p>See also the 
 * <a href='http://www.w3.org/TR/DOM-Level-3-Events'>
 * Document Object Model (DOM) Level 3 Events Specification</a> and .
 * @since DOM Level 2
 */
public interface EventListener {
    /**
     *  This method is called whenever an event occurs of the type for which 
     * the <code> EventListener</code> interface was registered. 
     * @param evt The <code>Event</code> contains contextual information 
     *   about the event.
     */
    public void handleEvent(Event evt);

}
