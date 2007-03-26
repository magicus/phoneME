/*
 *  
 *
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

package javax.microedition.content;

import com.sun.j2me.content.ContentHandlerImpl;
import com.sun.j2me.content.InvocationImpl;
import com.sun.j2me.content.RegistryImpl;

/**
 * The internal structure of a registered content handler.
 */
final class ContentHandlerServerImpl
    extends ContentHandlerImpl
    implements ContentHandlerServer
{
    /** Access restriction list. */
    String[] accesses;

    /** The reference to the RegistryImpl with the real implementation. */
    private RegistryImpl impl;

    /**
     * Construct an empty ContentHandlerServerImpl.
     */
    ContentHandlerServerImpl(RegistryImpl impl, ContentHandler handler)
                throws ContentHandlerException {
        super((ContentHandlerImpl)handler);
        impl.setServer(this);
        this.impl = impl;
        accesses = super.getAccesses();
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
        InvocationImpl invocImpl = impl.getRequest(wait);
        if (invocImpl != null) {
            // Wrap it in an Invocation instance
            return new Invocation(invocImpl);
        }
        return null;
    }

    /**
     * Cancels a pending <code>getRequest</code>.
     * This method will force all threads blocked in a call to the
     * <code>getRequest</code> method for this ContentHandlerServer
     * to return.
     * If no threads are blocked; this call has no effect.
     */
    public void cancelGetRequest() {
        impl.cancelGetRequest();
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
     *         <code>OK</code> or <code>CANCELLED</code>.
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
        return impl.finish(invoc.getInvocImpl(), status);
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
        impl.setListener(listener);
    }

    /**
     * Gets the number of IDs allowed access by the content handler.
     * The number of IDs MUST be equal to the length of the array
     * of <code>accessAllowed</code> passed to
     * {@link Registry#register Registry.register}.
     * If the number of IDs is zero then all applications and
     * content handlers are allowed access.
     *
     * @return the number of IDs allowed access
     */
    public int accessAllowedCount() {
        return accesses.length;
    }

    /**
     * Gets the ID at the specified index of an application or content
     * handler allowed access to this content handler.
     * The ID returned for each index must be the equal to the ID
     * at the same index in the <tt>accessAllowed</tt> array passed to
     * {@link Registry#register Registry.register}.
     *
     * @param index the index of the ID
     * @return the ID at the specified index
     * @exception IndexOutOfBoundsException if index is less than zero or
     *     greater than or equal to the value of the
     *     {@link #accessAllowedCount accessAllowedCount} method.
     */
    public String getAccessAllowed(int index) {
        return accesses[index];
    }

    /**
     * Determines if an ID MUST be allowed access by the content handler.
     * Access MUST be allowed if the ID has a prefix that exactly matches
     * any of the IDs returned by {@link #getAccessAllowed}.
     * The prefix comparison is equivalent to
     * <code>java.lang.String.startsWith</code>.
     *
     * @param ID the ID for which to check access
     * @return <code>true</code> if access MUST be allowed by the
     *  content handler;
     *  <code>false</code> otherwise
     * @exception NullPointerException if <code>ID</code>
     * is <code>null</code>
     */
    public boolean isAccessAllowed(String ID) {
        if (ID == null) {
            throw new NullPointerException(
                    "isAccessAllowed() argument can not be null");
        }

        for (int i=0; i < accesses.length; i++) {
            if (ID.startsWith(accesses[i])) {
                return true;
            }
        }

        return false;
    }

}
