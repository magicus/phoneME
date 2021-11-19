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

import org.w3c.dom.events.CustomEvent;

/**
 * DOM custom event implementation.
 *
 * @see org.w3c.dom.events.CustomEvent
 */

/* pkg */ final class CustomEvtImp
	extends XEvt
	implements CustomEvent
{
	/** Detail object */
	private Object detail;

	/**
	 *  Specifies some detail information about the <code>Event</code>. 
	 */
	public Object getDetail()
	{
		return detail;
	}

	/**
	 *  The <code>initCustomEventNS</code> method is used to initialize the 
	 * value of a <code>CustomEvent</code> object and has the same behavior 
	 * as <code>Event.initEventNS()</code>. 
	 * @param namespaceURIArg  Refer to the <code>Event.initEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param typeArg  Refer to the <code>Event.initEventNS()</code> method 
	 *   for a description of this parameter. 
	 * @param canBubbleArg  Refer to the <code>Event.initEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param cancelableArg  Refer to the <code>Event.initEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param detailArg  Specifies <code>CustomEvent.detail</code>. This 
	 *   value may be <code>null</code>.   
	 */
	public void initCustomEventNS(String namespaceURIArg, String typeArg, 
		boolean canBubbleArg, boolean cancelableArg, Object detailArg)
	{
		initEventNS(namespaceURIArg, typeArg, canBubbleArg, cancelableArg);
		detail = detailArg;
	}
}
