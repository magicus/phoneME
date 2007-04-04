/*
 *  
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.content;

import javax.microedition.content.ContentHandlerServer;
import javax.microedition.content.ActionNameMap;
import javax.microedition.content.Invocation;
import javax.microedition.content.RequestListener;

import com.sun.j2me.content.ContentHandlerImpl;
import com.sun.j2me.content.InvocationImpl;

import java.io.EOFException;
import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

/**
 * The internal structure of a registered content handler.
 */
final class ContentHandlerServerImpl
    extends ContentHandlerImpl
    implements ContentHandlerServer
{

    /** The listener to notify. */
    private RequestListener listener;

    /**
     * Construct an empty ContentHandlerServerImpl
     * that has the same fields as the existing handler.
     * @param handler the ContentHandlerImpl to clone
     */
    ContentHandlerServerImpl(ContentHandlerImpl handler) {
	super(handler);
    }
    
    /**
     * Gets the next Invocation request pending for this
     * ContentHandlerServer. 
     * The method can be unblocked with a call to
     * {@link #cancelGetRequest cancelGetRequest}.
     * The application should process the Invocation as
     * a request to perform the <code>action</code> on the content. 
     *
     * @param wait <code>true</code> if the method must wait for
     * for an Invocation if one is not available;
     * <code>false</code> if the method MUST NOT wait.
     *
     * @return the next pending Invocation or <code>null</code>
     *  if no Invocation is available; <code>null</code>
     *  if cancelled with {@link #cancelGetRequest cancelGetRequest}
     * @see javax.microedition.content.Registry#invoke
     * @see javax.microedition.content.ContentHandlerServer#finish
     */
    public Invocation getRequest(boolean wait) {
	Invocation request = new Invocation((InvocationImpl)null);
        InvocationImpl invoc = super.getRequest(wait, request);
        if (invoc != null) {
	    // Wrap it in an Invocation instance
	    request.setInvocImpl(invoc);
	    return request;
        }
        return null;
    }

    /**
     * Finish this Invocation and set the status for the response.
     * The <code>finish</code> method may only be called when this
     * Invocation
     * has a status of <code>ACTIVE</code> or <code>HOLD</code>.
     * <p>
     * The content handler may modify the URL, type, action, or
     * arguments before invoking <code>finish</code>.
     * If the method {@link Invocation#getResponseRequired} returns
     * <code>true</code> then the modified
     * values MUST be returned to the invoking application.
     *
     * @param invoc the Invocation to finish
     * @param status the new status of the Invocation. This MUST be either
     *	 <code>OK</code> or <code>CANCELLED</code>.
     *
     * @return <code>true</code> if the MIDlet suite MUST 
     *   voluntarily exit before the response can be returned to the
     *   invoking application
     *
     * @exception IllegalArgumentException if the new
     *   <code>status</code> of the Invocation
     *    is not <code>OK</code> or <code>CANCELLED</code>
     * @exception IllegalStateException if the current
     *   <code>status</code> of the
     *   Invocation is not <code>ACTIVE</code> or <code>HOLD</code>
     * @exception NullPointerException if the invocation is <code>null</code>
     */
    public boolean finish(Invocation invoc, int status) {
	return finish(invoc.getInvocImpl(), status);
    }
    

    /**
     * Set the listener to be notified when a new request is
     * available for this content handler.  The request MUST
     * be retrieved using {@link #getRequest}.
     *
     * @param listener the listener to register;
     *   <code>null</code> to remove the listener.
     */
    public void setListener(RequestListener listener) {
	// Start/set the thread needed to monitor the InvocationStore
	this.listener = listener;
	super.setListener(listener);
    }

    /**
     * Notify the listener of a pending Request.
     */
    protected void requestNotify() {
	RequestListener l = listener;
	if (l != null) {
	    l.invocationRequestNotify(this);
	}
    }

}
