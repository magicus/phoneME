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
 *  The <code>KeyboardEvent</code> interface provides specific contextual 
 * information associated with keyboard devices. Each keyboard event 
 * references a key using an identifier. Keyboard events are commonly 
 * directed at the element that has the focus. 
 * <p> The <code>KeyboardEvent</code> interface provides convenient attributes 
 * for some common modifiers keys: <code>KeyboardEvent.ctrlKey</code>, 
 * <code>KeyboardEvent.shiftKey</code>, <code>KeyboardEvent.altKey</code>, 
 * <code>KeyboardEvent.metaKey</code>.  
 * <p> To create an instance of the <code>KeyboardEvent</code> interface, use 
 * the <code>DocumentEvent.createEvent("KeyboardEvent")</code> method call. 
 * <p>See also the 
 * <a href='http://www.w3.org/TR/2006/WD-DOM-Level-3-Events-20060413'>
 * Document Object Model (DOM) Level 3 Events Specification</a>.
 *
 * @since DOM Level 3
 */
public interface KeyboardEvent extends UIEvent {

    /**
     *  The key activation is not distinguished as the left or right version 
     * of the key, and did not originate from the numeric keypad (or did not 
     * originate with a virtual key corresponding to the numeric keypad). 
     * Example: the 'Q' key on a PC 101 Key US keyboard. 
     */
    public static final int DOM_KEY_LOCATION_STANDARD = 0x00;

    /**
     *  The key activated is in the left key location (there is more than one 
     * possible location for this key). Example: the left Shift key on a PC 
     * 101 Key US keyboard. 
     */
    public static final int DOM_KEY_LOCATION_LEFT     = 0x01;

    /**
     *  The key activation is in the right key location (there is more than 
     * one possible location for this key). Example: the right Shift key on 
     * a PC 101 Key US keyboard. 
     */
    public static final int DOM_KEY_LOCATION_RIGHT    = 0x02;

    /**
     *  The key activation originated on the numeric keypad or with a virtual 
     * key corresponding to the numeric keypad. Example: the '1' key on a PC 
     * 101 Key US keyboard located on the numeric pad. 
     */
    public static final int DOM_KEY_LOCATION_NUMPAD   = 0x03;

    /**
     *  <code>keyIdentifier</code> holds the identifier of the key. The key 
     * identifiers are defined in Appendix A.2 <a 
     * href="http://www.w3.org/TR/DOM-Level-3-Events/keyset.html#KeySet-Set">
     * "Key identifiers set"</a>. Implementations that are 
     * unable to identify a key must use the key identifier 
     * <code>"Unidentified"</code>. 
     */
    public String getKeyIdentifier();

    /**
     *  The <code>keyLocation</code> attribute contains an indication of the 
     * location of they key on the device, as described in <a 
     * href="http://www.w3.org/TR/DOM-Level-3-Events/events.html#ID-KeyboardEvent-KeyLocationCode">
     * Keyboard event types</a>. 
     */
    public int getKeyLocation();

    /**
     *  <code>true</code> if the control (Ctrl) key modifier is activated. 
     */
    public boolean getCtrlKey();

    /**
     *  <code>true</code> if the shift (Shift) key modifier is activated. 
     */
    public boolean getShiftKey();

    /**
     *  <code>true</code> if the alternative (Alt) key modifier is activated. 
     * <p ><b>Note:</b>  The Option key modifier on Macintosh systems must be 
     * represented using this key modifier. 
     */
    public boolean getAltKey();

    /**
     *  <code>true</code> if the meta (Meta) key modifier is activated. 
     * <p ><b>Note:</b>  The Command key modifier on Macintosh systems must be 
     * represented using this key modifier. 
     */
    public boolean getMetaKey();

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
     * @param keyLocationArg  Specifies <code>KeyboardEvent.keyLocation</code>
     *   . 
     * @param modifiersList  A 
     *   <a href='http://www.w3.org/TR/2004/REC-xml-20040204/#NT-S'>white space
     *   </a> separated list of modifier key identifiers to be activated on 
     *   this object. 
     */
    public void initKeyboardEvent(String typeArg, 
                                  boolean canBubbleArg, 
                                  boolean cancelableArg, 
                                  AbstractView viewArg, 
                                  String keyIdentifierArg, 
                                  int keyLocationArg, 
                                  String modifiersList);

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
     *   method for a description of this parameter. 
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
                                    String typeArg, 
                                    boolean canBubbleArg, 
                                    boolean cancelableArg, 
                                    AbstractView viewArg, 
                                    String keyIdentifierArg, 
                                    int keyLocationArg, 
                                    String modifiersList);
}

