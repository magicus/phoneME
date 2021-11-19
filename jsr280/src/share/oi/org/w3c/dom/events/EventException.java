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
 *  Event operations may throw an <code>EventException</code> as specified in 
 * their method descriptions. 
 * <p>See also the 
 * <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Events-20001113'>
 * Document Object Model (DOM) Level 2 Events Specification</a>.
 * @since DOM Level 2
 */
public class EventException extends RuntimeException {
  /**
   * Create an EventException with the specified error code and message.
   *
   * @param code the numeric exception code
   * @param message a string containing any clarifications
   */
    public EventException(short code, String message) {
       super(message);
       this.code = code;
    }
    public short   code;
    // EventExceptionCode
    /**
     *  If the <code>Event</code>'s type was not specified by initializing the 
     * event before the method was called. Specification of the Event's type 
     * as <code>null</code> or an empty string will also trigger this 
     * exception. 
     */
    public static final short UNSPECIFIED_EVENT_TYPE_ERR = 0;

    /**
     * If the <code>Event</code> object is already dispatched in the tree.
     * @since DOM Level 3
     */
    public static final short DISPATCH_REQUEST_ERR       = 1;
}
