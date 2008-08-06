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

import org.w3c.dom.Node;
import org.w3c.dom.DOMException;

import org.w3c.dom.views.AbstractView;

import org.w3c.dom.events.EventTarget;
import org.w3c.dom.events.MouseWheelEvent;
import org.w3c.dom.events.EventException;

/**
 * DOM Mouse wheel event implementation.
 *
 * @see org.w3c.dom.events.MouseWheelEvent
 */

/* pkg */ final class MouseWheelEvtImp
	extends XMouseEvt
	implements MouseWheelEvent
{
	/** The number of wheel clicks. */
	private int delta;

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
	public int getWheelDelta()
	{
		return delta;
	}

	/**
	 * The <code>initWheelEventNS</code> method is used to initialize the 
	 * value of a <code>WheelEvent</code> object and has the same behavior 
	 * as <code>Event.initEventNS()</code>. 
	 *
	 * @param namespaceURIArg  Refer to the <code>Event.initUIEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param typeArg  Refer to the <code>Event.initUIEventNS()</code> method 
	 *   for a description of this parameter. 
	 * @param canBubbleArg  Refer to the <code>Event.initUIEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param cancelableArg  Refer to the <code>Event.initUIEventNS()</code> 
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
	 * @param modifiersList Refer to the <code>MouseEvent.initMouseEventNS()</code>
	 *   method for a description of this parameter.
	 * @param wheelDelta  The number of "clicks" the wheel has been rotated.
	 *
	 * @since DOM Level 3
	 */
	public void initMouseWheelEventNS(String namespaceURIArg, String typeArg, 
		boolean canBubbleArg, boolean cancelableArg, AbstractView viewArg, 
		int detailArg, int screenXArg, int screenYArg, 
		int clientXArg, int clientYArg, short buttonArg,
		EventTarget relatedTargetArg, String modifiersList, int wheelDelta)
	{
		initMouseEventNS(namespaceURIArg, typeArg, canBubbleArg, cancelableArg, 
			viewArg, detailArg, screenXArg, screenYArg, clientXArg, clientYArg, 
			buttonArg, relatedTargetArg, modifiersList);

		delta = wheelDelta;
	}
}
