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

import org.w3c.dom.events.UIEvent;

import org.w3c.dom.views.AbstractView;

/**
 * Basic DOM UI event implementation.
 *
 * @see org.w3c.dom.events.UIEvent
 */

public abstract class XUIEvt
	extends XEvt 
	implements UIEvent
{
	/** Modifier masks */
	protected final static char MD_MASK  = 0xf000;
	protected final static char MD_CTRL  = 0x1000;
	protected final static char MD_ALT   = 0x2000;
	protected final static char MD_SHIFT = 0x4000;
	protected final static char MD_META  = 0x8000;	

	/** Event's view */
	private AbstractView view;

	/** Event's detail */
	private int detail;

	/**
	 * Default constructor.
	 */
	protected XUIEvt()
	{
	}

	/**
	 *  The <code>view</code> attribute identifies the 
	 * <code>AbstractView</code> from which the event was generated. 
	 */
	public AbstractView getView()
	{
		return view;
	}

	/**
	 *  Specifies some detail information about the <code>Event</code>, 
	 * depending on the type of event. 
	 */
	public int getDetail()
	{
		return detail;
	}

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
		boolean canBubbleArg, boolean cancelableArg, 
		AbstractView viewArg, int detailArg)
	{
		initUIEventNS(null, typeArg, canBubbleArg, cancelableArg, 
			viewArg, detailArg);
	}

	/**
	 *  The <code>initUIEventNS</code> method is used to initialize the value 
	 * of a <code>UIEvent</code> object and has the same behavior as 
	 * <code>Event.initEventNS()</code>. 
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
	 *
	 * @since DOM Level 3
	 */
	public void initUIEventNS(String namespaceURIArg, String typeArg, 
		boolean canBubbleArg, boolean cancelableArg, 
		AbstractView viewArg, int detailArg)
	{
		initEventNS(namespaceURIArg, typeArg, canBubbleArg, cancelableArg);
		view   = viewArg;
		detail = detailArg;
	}

	/**
	 * Converts a string of whitespace separated names to an array of names.
	 *
	 * @str string of whitespace separated names
	 * @start an index pointing to first character to process
	 * @index an index of a name
	 * @return an array on names
	 */
	protected static String[] _splitStr(String str, int start, int index)
	{
		//		Skip whitespace
		space: while (start < str.length()) {
			switch (str.charAt(start)) {
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				start++;
				break;

			default:
				break space;
			}
		}
		//		Read a name
		int end = start;
		name: while (end < str.length()) {
			switch (str.charAt(end)) {
			case ' ':
			case '\t':
			case '\n':
			case '\r':
				break name;

			default:
				end++;
				break;
			}
		}

		String[] list = (end < str.length())? 
			_splitStr(str, end, index + 1):
			new String[(start != end)? index + 1: index];
		if (start != end)
			list[index] = str.substring(start, end);

		return list;
	}
}
