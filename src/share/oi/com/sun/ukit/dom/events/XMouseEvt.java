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

import org.w3c.dom.Node;
import org.w3c.dom.DOMException;

import org.w3c.dom.views.AbstractView;

import org.w3c.dom.events.EventTarget;
import org.w3c.dom.events.MouseEvent;
import org.w3c.dom.events.EventException;

/**
 * Basic DOM Mouse event implementation.
 *
 * @see org.w3c.dom.events.MouseEvent
 */

public abstract class XMouseEvt
	extends XUIEvt
	implements MouseEvent
{
	/** Screen coordinates */
	private int sx;
	private int sy;

	/** Client coordinates */
	private int cx;
	private int cy;

	/** Modifier flags. */
	private char modifiers;

	/** Button identifier. */
	private short button;

	/** Related target */
	private EventTarget related;

	/**
	 * Default constructor.
	 */
	protected XMouseEvt()
	{
	}

	/**
	 * The horizontal coordinate at which the event occurred relative to the 
	 * origin of the screen coordinate system. 
	 */
	public int getScreenX()
	{
		return sx;
	}

	/**
	 * The vertical coordinate at which the event occurred relative to the 
	 * origin of the screen coordinate system. 
	 */
	public int getScreenY()
	{
		return sy;
	}

	/**
	 * The horizontal coordinate at which the event occurred relative to the 
	 * DOM implementation's client area. 
	 */
	public int getClientX()
	{
		return cx;
	}

	/**
	 * The vertical coordinate at which the event occurred relative to the 
	 * DOM implementation's client area. 
	 */
	public int getClientY()
	{
		return cy;
	}

	/**
	 * <code>true</code> if the control (Ctrl) key modifier is activated. 
	 */
	public boolean getCtrlKey()
	{
		return ((modifiers & MD_CTRL) != 0);
	}

	/**
	 *  <code>true</code> if the shift (Shift) key modifier is activated. 
	 */
	public boolean getShiftKey()
	{
		return ((modifiers & MD_SHIFT) != 0);
	}

	/**
	 * <code>true</code> if the alt (alternative) key modifier is activated. 
	 * <p ><b>Note:</b>  The Option key modifier on Macintosh systems must be 
	 * represented using this key modifier. 
	 */
	public boolean getAltKey()
	{
		return ((modifiers & MD_ALT) != 0);
	}

	/**
	 * <code>true</code> if the meta (Meta) key modifier is activated. 
	 * <p ><b>Note:</b>  The Command key modifier on Macintosh system must be 
	 * represented using this meta key. 
	 */
	public boolean getMetaKey()
	{
		return ((modifiers & MD_META) != 0);
	}

	/**
	 *  During mouse events caused by the depression or release of a mouse 
	 * button, <code>button</code> is used to indicate which mouse button 
	 * changed state. <code>0</code> indicates the normal button of the 
	 * mouse (in general on the left or the one button on Macintosh mice, 
	 * used to activate a button or select text). <code>2</code> indicates 
	 * the contextual property (in general on the right, used to display a 
	 * context menu) button of the mouse if present. <code>1</code> 
	 * indicates the extra (in general in the middle and often combined with 
	 * the mouse wheel) button. Some mice may provide or simulate more 
	 * buttons, and values higher than <code>2</code> can be used to 
	 * represent such buttons. 
	 */
	public short getButton()
	{
		return button;
	}

	/**
	 *  Used to identify a secondary <code>EventTarget</code> related to a UI 
	 * event, depending on the type of event. 
	 */
	public EventTarget getRelatedTarget()
	{
		return related;
	}

	/**
	 *  The <code>initMouseEvent</code> method is used to initialize the value 
	 * of a <code>MouseEvent</code> object and has the same behavior as 
	 * <code>UIEvent.initUIEvent()</code>. 
	 * @param typeArg  Refer to the <code>UIEvent.initUIEvent()</code> method 
	 *   for a description of this parameter. 
	 * @param canBubbleArg  Refer to the <code>UIEvent.initUIEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param cancelableArg  Refer to the <code>UIEvent.initUIEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param viewArg  Refer to the <code>UIEvent.initUIEvent()</code> method 
	 *   for a description of this parameter. 
	 * @param detailArg  Refer to the <code>UIEvent.initUIEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param screenXArg  Specifies <code>MouseEvent.screenX</code>. 
	 * @param screenYArg  Specifies <code>MouseEvent.screenY</code>. 
	 * @param clientXArg  Specifies <code>MouseEvent.clientX</code>. 
	 * @param clientYArg  Specifies <code>MouseEvent.clientY</code>. 
	 * @param ctrlKeyArg  Specifies <code>MouseEvent.ctrlKey</code>. 
	 * @param altKeyArg  Specifies <code>MouseEvent.altKey</code>. 
	 * @param shiftKeyArg  Specifies <code>MouseEvent.shiftKey</code>. 
	 * @param metaKeyArg  Specifies <code>MouseEvent.metaKey</code>. 
	 * @param buttonArg  Specifies <code>MouseEvent.button</code>. 
	 * @param relatedTargetArg  Specifies 
	 *   <code>MouseEvent.relatedTarget</code>. This value may be 
	 *   <code>null</code>.   
	 */
	public void initMouseEvent(
		String typeArg, boolean canBubbleArg, boolean cancelableArg, 
		AbstractView viewArg, int detailArg, 
		int screenXArg, int screenYArg, int clientXArg, int clientYArg, 
		boolean ctrlKeyArg, boolean altKeyArg, boolean shiftKeyArg, 
		boolean metaKeyArg, short buttonArg, EventTarget relatedTargetArg)
	{
		initMouseEventNS(null, typeArg, canBubbleArg, cancelableArg, 
			viewArg, detailArg, screenXArg, screenYArg, clientXArg, clientYArg, 
			buttonArg, relatedTargetArg, null);

		if (ctrlKeyArg)
			modifiers |= MD_CTRL;
		if (altKeyArg)
			modifiers |= MD_ALT;
		if (shiftKeyArg)
			modifiers |= MD_SHIFT;
		if (metaKeyArg)
			modifiers |= MD_META;
	}

	/**
	 *  The <code>initMouseEventNS</code> method is used to initialize the 
	 * value of a <code>MouseEvent</code> object and has the same behavior 
	 * as <code>UIEvent.initUIEventNS()</code>. 
	 * @param namespaceURIArg  Refer to the <code>UIEvent.initUIEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param typeArg  Refer to the <code>UIEvent.initUIEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param canBubbleArg  Refer to the <code>UIEvent.initUIEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param cancelableArg  Refer to the <code>UIEvent.initUIEventNS()</code>
	 *    method for a description of this parameter. 
	 * @param viewArg  Refer to the <code>UIEvent.initUIEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param detailArg  Refer to the <code>UIEvent.initUIEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param screenXArg  Refer to the 
	 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
	 *   of this parameter. 
	 * @param screenYArg  Refer to the 
	 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
	 *   of this parameter. 
	 * @param clientXArg  Refer to the 
	 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
	 *   of this parameter. 
	 * @param clientYArg  Refer to the 
	 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
	 *   of this parameter. 
	 * @param buttonArg  Refer to the <code>MouseEvent.initMouseEvent()</code>
	 *    method for a description of this parameter. 
	 * @param relatedTargetArg  Refer to the 
	 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
	 *   of this parameter. 
	 * @param modifiersList  A 
	 *   <a href='http://www.w3.org/TR/2004/REC-xml-20040204/#NT-S'>
	 *   white space</a> separated list of modifier key identifiers to be 
	 *   activated on this object. As an example, <code>"Control Alt"</code> 
	 *   will activated the control and alt modifiers.
     *
	 * @since DOM Level 3
	 */
	public void initMouseEventNS(String namespaceURIArg, 
		String typeArg, boolean canBubbleArg, boolean cancelableArg, 
		AbstractView viewArg, int detailArg, 
		int screenXArg, int screenYArg, int clientXArg, int clientYArg, 
		short buttonArg, EventTarget relatedTargetArg, String modifiersList)
	{
		initUIEventNS(namespaceURIArg, typeArg, canBubbleArg, cancelableArg,
			viewArg, detailArg);

		sx = screenXArg;
		sy = screenYArg;
		cx = clientXArg;
		cy = clientYArg;

		related = relatedTargetArg;

		button = buttonArg;
		
		modifiers = 0;
		String[] list = XUIEvt._splitStr(
			(modifiersList != null)? modifiersList: "", 0, 0);
		for (int idx = 0; idx < list.length; idx++) {
			if ("Control".equals(list[idx])) {
				modifiers |= MD_CTRL;
			} else if ("Alt".equals(list[idx])) {
				modifiers |= MD_ALT;
			} else if ("Meta".equals(list[idx])) {
				modifiers |= MD_META;
			} else if ("Shift".equals(list[idx])) {
				modifiers |= MD_SHIFT;
			}
		}
	}
}
