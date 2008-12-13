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

import org.w3c.dom.events.EventTarget;
import org.w3c.dom.events.MutationEvent;
import org.w3c.dom.events.EventException;

/**
 * DOM mutation event implementation.
 *
 * @see org.w3c.dom.events.MutationEvent
 */

/* pkg */ final class MutEvtImp
	extends XEvt
	implements MutationEvent
{
	/** Related node. */
	private Node related;
	/** Previous value. */
	private String pvalue;
	/** New value. */
	private String nvalue;
	/** Attribute name. */
	private String attname;
	/** Attribute change type */
	private short attchid;

	/**
	 * <code>relatedNode</code> is used to identify a secondary node related 
	 * to a mutation event. For example, if a mutation event is dispatched 
	 * to a node indicating that its parent has changed, the 
	 * <code>relatedNode</code> is the changed parent. If an event is 
	 * instead dispatched to a subtree indicating a node was changed within 
	 * it, the <code>relatedNode</code> is the changed node. In the case of 
	 * the DOMAttrModified event it indicates the <code>Attr</code> node 
	 * which was modified, added, or removed. 
	 */
	public Node getRelatedNode()
	{
		return related;
	}

	/**
	 * <code>prevValue</code> indicates the previous value of the 
	 * <code>Attr</code> node in DOMAttrModified events, and of the 
	 * <code>CharacterData</code> node in DOMCharDataModified events. 
	 */
	public String getPrevValue()
	{
		return pvalue;
	}

	/**
	 * <code>newValue</code> indicates the new value of the <code>Attr</code> 
	 * node in DOMAttrModified events, and of the <code>CharacterData</code> 
	 * node in DOMCharDataModified events. 
	 */
	public String getNewValue()
	{
		return nvalue;
	}

	/**
	 * <code>attrName</code> indicates the name of the changed 
	 * <code>Attr</code> node in a DOMAttrModified event. 
	 */
	public String getAttrName()
	{
		return attname;
	}

	/**
	 * <code>attrChange</code> indicates the type of change which triggered 
	 * the DOMAttrModified event. The values can be <code>MODIFICATION</code>
	 * , <code>ADDITION</code>, or <code>REMOVAL</code>. 
	 */
	public short getAttrChange()
	{
		return attchid;
	}

	/**
	 * The <code>initMutationEvent</code> method is used to initialize the 
	 * value of a <code>MutationEvent</code> object and has the same 
	 * behavior as <code>Event.initEvent()</code>. 
	 * @param typeArg  Refer to the <code>Event.initEvent()</code> method for 
	 *   a description of this parameter. 
	 * @param canBubbleArg  Refer to the <code>Event.initEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param cancelableArg  Refer to the <code>Event.initEvent()</code> 
	 *   method for a description of this parameter. 
	 * @param relatedNodeArg  Specifies <code>MutationEvent.relatedNode</code>. 
	 * @param prevValueArg  Specifies <code>MutationEvent.prevValue</code>. 
	 *   This value may be null. 
	 * @param newValueArg  Specifies <code>MutationEvent.newValue</code>. 
	 *   This value may be null. 
	 * @param attrNameArg  Specifies <code>MutationEvent.attrname</code>. 
	 *   This value may be null. 
	 * @param attrChangeArg  Specifies <code>MutationEvent.attrChange</code>. 
	 *   This value may be null.   
	 */
	public void initMutationEvent(
		String typeArg, boolean canBubbleArg, boolean cancelableArg, 
		Node relatedNodeArg, String prevValueArg, String newValueArg, 
		String attrNameArg, short attrChangeArg)
	{
		initMutationEventNS(null, typeArg, canBubbleArg, cancelableArg,
			relatedNodeArg, prevValueArg, newValueArg, 
			attrNameArg, attrChangeArg);
	}

	/**
	 * The <code>initMutationEventNS</code> method is used to initialize the 
	 * value of a <code>MutationEvent</code> object and has the same 
	 * behavior as <code>Event.initEventNS()</code>. 
	 * @param namespaceURIArg  Refer to the <code>Event.initEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param typeArg  Refer to the <code>Event.initEventNS()</code> method 
	 *   for a description of this parameter. 
	 * @param canBubbleArg  Refer to the <code>Event.initEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param cancelableArg  Refer to the <code>Event.initEventNS()</code> 
	 *   method for a description of this parameter. 
	 * @param relatedNodeArg  Refer to the 
	 *   <code>MutationEvent.initMutationEvent()</code> method for a 
	 *   description of this parameter. 
	 * @param prevValueArg  Refer to the 
	 *   <code>MutationEvent.initMutationEvent()</code> method for a 
	 *   description of this parameter. 
	 * @param newValueArg  Refer to the 
	 *   <code>MutationEvent.initMutationEvent()</code> method for a 
	 *   description of this parameter. 
	 * @param attrNameArg  Refer to the 
	 *   <code>MutationEvent.initMutationEvent()</code> method for a 
	 *   description of this parameter. 
	 * @param attrChangeArg  Refer to the 
	 *   <code>MutationEvent.initMutationEvent()</code> method for a 
	 *   description of this parameter.
	 *
	 * @since DOM Level 3
	 */
	public void initMutationEventNS(String namespaceURIArg, 
		String typeArg, boolean canBubbleArg, boolean cancelableArg, 
		Node relatedNodeArg, String prevValueArg, String newValueArg, 
		String attrNameArg, short attrChangeArg)
	{
		initEventNS(namespaceURIArg, typeArg, canBubbleArg, cancelableArg);
		related = relatedNodeArg;
		pvalue  = prevValueArg;
		nvalue  = newValueArg;
		attname = attrNameArg;
		attchid = attrChangeArg;
	}
}
