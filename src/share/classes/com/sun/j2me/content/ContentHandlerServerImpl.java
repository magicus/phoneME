/*
 *  
 *
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

package com.sun.j2me.content;

import javax.microedition.content.ContentHandlerServer;
import javax.microedition.content.Invocation;
import javax.microedition.content.RequestListener;

/**
 * The internal structure of a registered content handler.
 */
final public class ContentHandlerServerImpl extends ContentHandlerImpl
    			implements ContentHandlerServer
{
    /** The listener to notify. */
    private RequestListener listener;
    
    private int currentBlockID;

    /**
     * Construct an empty ContentHandlerServerImpl
     * that has the same fields as the existing handler.
     * @param handler the ContentHandlerImpl to clone
     */
    public ContentHandlerServerImpl(ContentHandlerImpl handler) {
    	super(handler);
    	currentBlockID = InvocationImpl.store.allocateBlockID();
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
     *  if canceled with {@link #cancelGetRequest cancelGetRequest}
     * @see javax.microedition.content.Registry#invoke
     * @see javax.microedition.content.ContentHandlerServer#finish
     */
    public Invocation getRequest(boolean wait) {
    	Invocation result = null;
    	if(Logger.LOGGER != null)
    		Logger.LOGGER.println( "ContentHandler.getRequest(" + wait + ")" );
        // Application has tried to get a request; reset cleanup flags on all
        if (requestCalls == 0) {
        	InvocationImpl.store.setCleanupFlag(applicationID, false);
        }
        requestCalls++;

        InvocationImpl invoc =
            InvocationImpl.store.getRequest(applicationID, wait ? currentBlockID : 0);
        if (invoc != null) {
            // Keep track of number of requests delivered to the application
        	AMSGate.inst.requestForeground(invoc.invokingApp, invoc.destinationApp);
            
		    // Wrap it in an Invocation instance
            result = invoc.wrap();
        }
    	if(Logger.LOGGER != null)
    		Logger.LOGGER.println( "ContentHandler.getRequest(" + wait + ") returns " + result );
        return result;
    }

    /**
     * Cancel a pending <code>getRequest</code>.
     * This method will force a Thread blocked in a call to the
     * <code>getRequest</code> method for the same application
     * context to return early.
     * If no Thread is blocked; this call has no effect.
     */
    public void cancelGetRequest() {
		InvocationImpl.store.unblockWaitingThreads( currentBlockID );
    	currentBlockID = InvocationImpl.store.allocateBlockID();
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
    	return finish(InvocationImpl.tunnel.getInvocImpl(invoc), status);
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
		super.activateListening(listener != null);
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
