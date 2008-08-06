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

import org.w3c.dom.views.AbstractView;

/**
 *  The <code>UIEvent</code> interface provides specific contextual 
 * information associated with User Interface events. 
 * <p> To create an instance of the <code>UIEvent</code> interface, use the 
 * <code>DocumentEvent.createEvent("UIEvent")</code> method call. 
 * <p>See also the 
 * <a href='http://www.w3.org/TR/2006/WD-DOM-Level-3-Events-20060413'>
 * Document Object Model (DOM) Level 3 Events Specification</a>.
 *
 * @since DOM Level 2
 */
public interface UIEvent extends Event {

    /**
     *  The <code>view</code> attribute identifies the 
     * <code>AbstractView</code> from which the event was generated. 
     */
    public AbstractView getView();

    /**
     *  Specifies some detail information about the <code>Event</code>, 
     * depending on the type of event. 
     */
    public int getDetail();

    /**
     *  The <code>initUIEvent</code> method is used to initialize the value of 
     * a <code>UIEvent</code> object and has the same behavior as 
     * <code>Event.initEvent()</code>. 
     * @param typeArg  Refer to the <code>Event.initEvent()</code> method for 
     *   a description of this parameter. 
     * @param canBubbleArg  Refer to the <code>Event.initEvent()</code> 
     *   method for a description of this parameter. 
     * @param cancelableArg  Refer to the <code>Event.initEvent()</code> 
     *   method for a description of this parameter. 
     * @param viewArg  Specifies <code>UIEvent.view</code>. This value may be 
     *   <code>null</code>. 
     * @param detailArg  Specifies <code>UIEvent.detail</code>.   
     */
    public void initUIEvent(String typeArg, 
                            boolean canBubbleArg, 
                            boolean cancelableArg, 
                            AbstractView viewArg, 
                            int detailArg);

    /**
     *  The <code>initUIEventNS</code> method is used to initialize the value 
     * of a <code>UIEvent</code> object and has the same behavior as 
     * <code>Event.initEventNS()</code>. 
     * @param namespaceURI  Refer to the <code>Event.initEventNS()</code> 
     *   method for a description of this parameter. 
     * @param typeArg  Refer to the <code>Event.initEventNS()</code> method 
     *   for a description of this parameter. 
     * @param canBubbleArg  Refer to the <code>Event.initEventNS()</code> 
     *   method for a description of this parameter. 
     * @param cancelableArg  Refer to the <code>Event.initEventNS()</code> 
     *   method for a description of this parameter. 
     * @param viewArg  Refer to the <code>UIEvent.initUIEvent()</code> method 
     *   for a description of this parameter. 
     * @param detailArg  Refer to the <code>UIEvent.initUIEvent()</code> 
     *   method for a description of this parameter.
     *
     * @since DOM Level 3
     */
    public void initUIEventNS(String namespaceURI, 
                              String typeArg, 
                              boolean canBubbleArg, 
                              boolean cancelableArg, 
                              AbstractView viewArg, 
                              int detailArg);
}

