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
 * The <code>MouseWheelEvent</code> interface provides specific contextual 
 * information associated with mouse wheel events.
 *
 * <p><b>Note: </b> This interface has not yet been accepted for
 * inclusion in the W3C DOM 3 Event specification. It may be removed from the
 * JSR 280 specification or added to a different namespace in a
 * subsequent version of the specification.</p>
 */
public interface MouseWheelEvent extends MouseEvent {

    /**
     * Indicates the number of "clicks" the wheel has been rotated. A positive 
     * value indicates that the wheel has been rotated away from the user 
     * (or in a right-hand manner on horizontally aligned devices) and a 
     * negative value indicates that the wheel has been rotated towards the 
     * user (or in a left-hand manner on horizontally aligned devices).
     *
     * <p>A "click" is defined to be a unit of rotation. On some devices this 
     * is a finite physical step. On devices with smooth rotation, a "click" 
     * becomes the smallest measurable amount of rotation.</p>
     */
    public int getWheelDelta();

    /**
     * The <code>initMouseWheelEventNS</code> method is used to initialize the 
     * value of a <code>MouseWheelEvent</code> object and has the same
     * behavior as <code>Event.initEventNS()</code>. 
     *
     * For <code>mousewheel</code>, <code>MouseEvent.getRelatedTarget</code>
     * must indicate the element over which the pointer is located, or
     * <code>null</code> if there is no such element (in the case where the
     * device does not have a pointer, but does have a wheel). 
     *
     * @param namespaceURIArg  Refer to the <code>Event.initEventNS()</code> 
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
     * @param screenXArg Refer to the <code>MouseEvent.initMouseEventNS()</code>
     *   method for a description of this parameter.
     * @param screenYArg Refer to the <code>MouseEvent.initMouseEventNS()</code>
     *   method for a description of this parameter.
     * @param clientXArg Refer to the <code>MouseEvent.initMouseEventNS()</code>
     *   method for a description of this parameter.
     * @param clientYArg Refer to the <code>MouseEvent.initMouseEventNS()</code>
     *   method for a description of this parameter.
     * @param buttonArg Refer to the <code>MouseEvent.initMouseEventNS()</code>
     *   method for a description of this parameter.
     * @param relatedTargetArg Refer to the <code>MouseEvent.initMouseEventNS()</code>
     *   method for a description of this parameter.
     * @param modifiersListArg Refer to the <code>MouseEvent.initMouseEventNS()</code>
     *   method for a description of this parameter.
     * @param wheelDeltaArg  A number indicating the distance in "clicks"
     *   (positive means rotated away from the user, negative means rotated
     *   towards the user). The default value of the wheelDelta attribute is 0. 
     *
     * @since DOM Level 3
     */
    public void initMouseWheelEventNS(String namespaceURIArg, 
                                      String typeArg, 
                                      boolean canBubbleArg, 
                                      boolean cancelableArg, 
                                      AbstractView viewArg, 
                                      int detailArg,
                                      int screenXArg,
                                      int screenYArg,
                                      int clientXArg,
                                      int clientYArg,
                                      short buttonArg,
                                      EventTarget relatedTargetArg,
                                      String modifiersListArg,
                                      int wheelDeltaArg);
}
