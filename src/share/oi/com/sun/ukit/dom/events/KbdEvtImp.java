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
import org.w3c.dom.events.KeyboardEvent;
import org.w3c.dom.events.EventException;

/**
 * DOM keyboard event implementation.
 *
 * @see org.w3c.dom.events.KeyboardEvent
 */

/* pkg */ final class KbdEvtImp
	extends XUIEvt
	implements KeyboardEvent
{
	/** Modifier flags. */
	private char modifiers;

	/** Key identifier. */
	private String keyidentifier;

	/** Key location. */
	private int keylocation;

	/**
	 * <code>keyIdentifier</code> holds the identifier of the key. The key 
	 * identifiers are defined in Appendix A.2 <a 
	 * href="http://www.w3.org/TR/DOM-Level-3-Events/keyset.html#KeySet-Set">
	 * "Key identifiers set"</a>. Implementations that are 
	 * unable to identify a key must use the key identifier 
	 * <code>"Unidentified"</code>. 
	 */
	public String getKeyIdentifier()
	{
		return (keyidentifier != null)? keyidentifier: "Unidentified";
	}

	/**
	 * The <code>keyLocation</code> attribute contains an indication of the 
	 * location of they key on the device, as described in <a 
	 * href="http://www.w3.org/TR/DOM-Level-3-Events/events.html#ID-KeyboardEvent-KeyLocationCode">
	 * Keyboard event types</a>. 
	 */
	public int getKeyLocation()
	{
		return keylocation;
	}

	/**
	 *  <code>true</code> if the control (Ctrl) key modifier is activated. 
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
	 *  <code>true</code> if the alternative (Alt) key modifier is activated. 
	 * <p ><b>Note:</b>  The Option key modifier on Macintosh systems must be 
	 * represented using this key modifier. 
	 */
	public boolean getAltKey()
	{
		return ((modifiers & MD_ALT) != 0);
	}

	/**
	 *  <code>true</code> if the meta (Meta) key modifier is activated. 
	 * <p ><b>Note:</b>  The Command key modifier on Macintosh systems must be 
	 * represented using this key modifier. 
	 */
	public boolean getMetaKey()
	{
		return ((modifiers & MD_META) != 0);
	}

	/**
	 *  The <code>initKeyboardEvent</code> method is used to initialize the 
	 * value of a <code>KeyboardEvent</code> object and has the same 
	 * behavior as <code>UIEvent.initUIEvent()</code>. The value of 
	 * <code>UIEvent.detail</code> remains undefined. 
	 * @param typeArg  Refer to the <code>UIEvent.initUIEvent()</code> method 
	 *   for a description of this parameter. 
	 * @param canBubbleArg  Refer to the <code>UIEvent.initUIEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param cancelableArg  Refer to the <code>UIEvent.initUIEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param viewArg  Refer to the <code>UIEvent.initUIEvent()</code> method 
	 *   for a description of this parameter. 
	 * @param keyIdentifierArg  Specifies 
	 *   <code>KeyboardEvent.keyIdentifier</code>. 
	 * @param keyLocationArg  Specifies <code>KeyboardEvent.keyLocation</code>.
	 * @param modifiersList  A 
	 *   <a href='http://www.w3.org/TR/2004/REC-xml-20040204/#NT-S'>white space
	 *   </a> separated list of modifier key identifiers to be activated on 
	 *   this object. 
	 */
	public void initKeyboardEvent(
		String typeArg, boolean canBubbleArg, boolean cancelableArg, 
		AbstractView viewArg, String keyIdentifierArg, int keyLocationArg, 
		String modifiersList)
	{
		initKeyboardEventNS(null, typeArg, canBubbleArg, cancelableArg,
			viewArg, keyIdentifierArg, keyLocationArg, modifiersList);
	}

	/**
	 *  The <code>initKeyboardEventNS</code> method is used to initialize the 
	 * value of a <code>KeyboardEvent</code> object and has the same 
	 * behavior as <code>UIEvent.initUIEventNS()</code>. The value of 
	 * <code>UIEvent.detail</code> remains undefined. 
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
	 * @param keyIdentifierArg  Refer to the 
	 *   <code>KeyboardEvent.initKeyboardEvent()</code> method for a 
	 *   description of this parameter. 
	 * @param keyLocationArg  Refer to the 
	 *   <code>KeyboardEvent.initKeyboardEvent()</code> method for a 
	 *   description of this parameter. 
	 * @param modifiersList  A 
	 *   <a href='http://www.w3.org/TR/2004/REC-xml-20040204/#NT-S'>white space
	 *   </a> separated list of modifier key identifiers to be activated on 
	 *   this object. As an example, <code>"Control Alt"</code> will activated 
	 *   the control and alt modifiers. 
	 */
	public void initKeyboardEventNS(String namespaceURIArg, 
		String typeArg, boolean canBubbleArg, boolean cancelableArg, 
		AbstractView viewArg, String keyIdentifierArg, int keyLocationArg, 
		String modifiersList)
	{
		initUIEventNS(namespaceURIArg, typeArg, canBubbleArg, cancelableArg,
			viewArg, 0);

		keyidentifier = keyIdentifierArg;

		keylocation   = keyLocationArg;

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
