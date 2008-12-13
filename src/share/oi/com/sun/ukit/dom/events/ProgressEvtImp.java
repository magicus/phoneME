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

import org.w3c.dom.events.ProgressEvent;

/**
 * DOM progress event implementation.
 *
 * @see org.w3c.dom.events.ProgressEvent
 */

/* pkg */ final class ProgressEvtImp
	extends XEvt
	implements ProgressEvent
{
	/** A number of loaded bytes. */
	private int loaded;
	/** Total amount of bytes to load or negative. */
	private int total;

	/**
	 * Specifies whether the total size of the transfer is known.
	 */
	public boolean getLengthComputable()
	{
		return (total >= 0);
	}

	/**
	 * Specifies the number of bytes downloaded since the beginning of the 
	 * download. This refers to the content, excluding headers and overhead 
	 * from the transaction,
	 * and where there is a content-encoding or transfer-encoding refers
	 * to the number of bytes to be transferred, i.e. with the relevant
	 * encodings applied. For more details on HTTP see [RFC2616].
	 */
	public int getLoaded()
	{
		return loaded;
	}

	/**
	 * Specifies the expected total number of bytes of the content 
	 * transferred in the operation. Where the size of the transfer is for
	 * some reason unknown, the value of this attribute <em>must</em> be zero.
	 */
	public int getTotal()
	{
		return (total >= 0)? total: 0;
	}

	/**
	 * The initProgressEvent method is used to initialize the value of a 
	 * progress event created through the DocumentEvent interface. 
	 * If this method is called multiple times, the final invocation takes 
	 * precedence .
	 *
	 * @param typeArg  
	 *   This <em>must</em> be one of <code>loadstart</code>, 
	 *   <code>progress</code>, <code>error</code>, <code>abort</code>,
	 *   <code>load</code>. If it is not one of those values then this
	 *   specification does not define the resulting event.
	 * @param canBubbleArg
	 *   This <em>must</em> be <var>false</var>. Where a
	 *   value of <var>true</var> is passed, implementations
	 *   <em>must</em> override that and change the value to
	 *   <var>false</var>.
	 * @param cancelableArg
	 *   This <em>must</em> be <var>false</var>. Where a
	 *   value of <var>true</var> is passed, implementations 
	 *   <em>must</em> override that and change the value to
	 *   <var>false</var>.
	 * @param lengthComputableArg
	 *   If the implementation has reliable information about
	 *   the value of <code>total</code>, then this should be <var>true</var>. 
	 *   If the implementation does not have reliable information about
	 *   the value of <code>total</code>, this should be <var>false</var>.
	 * @param loadedArg
	 *   Specifies the total number of bytes already loaded, In most cases,
	 *   this will be 0 (zero). If this value is not a non-negative number, 
	 *   the implementation <em>must</em> change it to zero.
	 * @param totalArg
	 *   Specifies the total number of bytes to be
	 *   loaded. If <code>lengthComputable</code> is <var>false</var>, 
	 *   this <em>must</em> be zero. If any other parameter is passed, and
	 *   <code>lengthComputable</code> is <var>false</var>, the implementation 
	 *   <em>must</em> override this and set the value to zero. If
	 *   <code>lengthComputable</code> is <var>true</var>, and the value 
	 *   of this parameter is not a non-negative number, the implementation 
	 *   <em>must</em> set <code>lengthComputable</code> to <var>false</var>
	 *   and the value of <code>total</code> to zero. 
	 *
	 * @since DOM Level 3
	 */
	public void initProgressEvent(String typeArg, boolean canBubbleArg, 
		boolean cancelableArg, boolean lengthComputableArg, 
		int loadedArg, int totalArg)
	{
		initProgressEventNS(null, typeArg, canBubbleArg, cancelableArg, 
			lengthComputableArg, loadedArg, totalArg);
	}

	/**
	 * The initProgressEventNS method is used to initialize the value of a 
	 * namespaced progress event created through the DocumentEvent interface.
	 * This method may only be called before the progress event has been 
	 * dispatched via the dispatchEvent method, though it may be called 
	 * multiple times during that phase if necessary. If called multiple 
	 * times, the final invocation takes precedence.
	 *
	 * @param namespaceURIArg 
	 *   Specifies the URI for the namespace of the event.
	 *   For all events defined in this specification, the
	 *   value of this parameter is <code>null</code>.
	 * @param typeArg
	 *   This must be one of <code>loadstart</code>, 
	 *   <code>progress</code>, <code>error</code>,
	 *   <code>abort</code>, <code>load</code>. If it is not one
	 *   of those values then this specification does not define 
	 *   the resulting event.
	 * @param canBubbleArg
	 *   This <em>must</em> be <var>false</var>. Where a
	 *   value of <var>true</var> is passed, implementations
	 *   <em>must</em> override that and change the value to
	 *   <var>false</var>.
	 * @param cancelableArg
	 *   This <em>must</em> be <var>false</var>. Where a
	 *   value of <var>true</var> is passed, implementations
	 *   <em>must</em> override that and change the value to
	 *   <var>false</var>.
	 * @param lengthComputableArg
	 *   If the implementation has reliable information about
	 *   the value of total, then this should be <var>true</var>. If the
	 *   implementation does not have reliable information about
	 *   the value of total, this should be <var>false</var>.
	 * @param loadedArg
	 *   This parameter specifies the total number of bytes
	 *   already loaded, In most cases, this will be 0 (zero).
	 *   If this value is not a non-negative number, the implementation 
	 *   <em>must</em> change it to zero.
	 * @param totalArg
	 *   This specifies the total number of bytes to be
	 *   loaded. If <code>lengthComputable</code> is <var>false</var>, 
	 *   this <em>must</em> be zero. If any other parameter is passed,
	 *   and <code>lengthComputable</code> is <var>false</var>, the
	 *   implementation <em>must</em> override this and set the value to
	 *   zero. If <code>lengthComputable</code> is <var>true</var>, and 
	 *   the value of this parameter is not a non-negative number, the 
	 *   implementation <em>must</em> set <code>lengthComputable</code> 
	 *   to <var>false</var> and the value of <code>total</code> to zero.
	 *
	 * @since DOM Level 3
	 */
	public void initProgressEventNS(String namespaceURIArg, String typeArg, 
		boolean canBubbleArg, boolean cancelableArg, 
		boolean lengthComputableArg, int loadedArg, int totalArg)
	{
		initEventNS(namespaceURIArg, typeArg, false, false);

		total  = (lengthComputableArg)? totalArg: -1;
		loaded = (loadedArg >= 0)? loadedArg: 0;
	}
}
