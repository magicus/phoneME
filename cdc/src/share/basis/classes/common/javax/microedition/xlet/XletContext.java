/*
 * @(#)XletContext.java	1.23 06/10/10
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
 *
 */

package javax.microedition.xlet;

import java.awt.Container;

/** 
 * An interface that provides methods allowing an Xlet to discover
 * information about its environment. An XletContext is passed
 * to an Xlet when the Xlet is initialized. It provides an Xlet with a
 * mechanism to retrieve properties, as well as a way to signal
 * internal state changes.
 *
 * @see javax.microedition.xlet.Xlet
 */



public interface XletContext {
    /**
     * The property key used to obtain initialization arguments for the
     * Xlet.  The call
     * <code>XletContext.getXletProperty(XletContext.ARGS)</code> will
     * return the arguments as an array of Strings.  If there are
     * no arguments, then an array of length 0 will be returned.
     *
     * @see #getXletProperty
     */
    public static final String ARGS = "javax.microedition.xlet.args";
    /**
     *
     * Used by an application to notify its manager that it
     * has entered into the
     * <i>Destroyed</i> state.  The application manager will not
     * call the Xlet's <code>destroy</code> method, and all resources
     * held by the Xlet will be considered eligible for reclamation.  
     * Before calling this method,
     * the Xlet must have performed the same operations
     * (clean up, releasing of resources etc.) it would have if the
     * <code>Xlet.destroyXlet()</code> had been called.
     *  
     */
    public void notifyDestroyed();
    /**    
     * Notifies the manager that the Xlet does not want to be active and has
     * entered the <i>Paused</i> state.  Invoking this method will
     * have no effect if the Xlet is destroyed, or if it has not
     * yet been started. <p>
     *
     * If an Xlet calls <code>notifyPaused()</code>, in the
     * future it may receive an <i>Xlet.startXlet()</i> call to request
     * it to become active, or an <i>Xlet.destroyXlet()</i> call to request
     * it to destroy itself.
     * 
     */

    public void notifyPaused();
    /**
     * Provides an Xlet with a mechanism to retrieve named
     * properties from the XletContext.
     *
     * @param key The name of the property.
     *
     * @return A reference to an object representing the property.
     * <code>null</code> is returned if no value is available for key.
     */
    public Object getXletProperty(String key);
    /** 
     * Provides the Xlet with a mechanism to indicate that it is
     * interested in entering the <i>Active</i> state. Calls to this
     * method can be used by an application manager to determine which
     * Xlets to move to <i>Active</i> state.
     * If the request is fulfilled, the application manager will
     * subsequently call <code>Xlet.startXlet()</code>
     * via a different thread than the one used to call
     * <code>resumeRequest()</code>.
     * 
     * @see Xlet#startXlet
     */
    public void resumeRequest();
    /**
     * Get the parent container for an Xlet to put its AWT components in. 
     * Xlets without a graphical representation should never call this method. 
     * If successful, this method returns an instance of java.awt.Container that
     * is initially invisible, with an arbitrary size and position. 
     
     * If this method is called multiple times on the same XletContext instance,
     * the same container will be returned each time. 

     * The methods for setting the size and position of the xlet's parent 
     * container shall attempt to change the shape of the container, but they may
     * fail silently or make platform specific approximations. To discover if
     * a request to change the size or position has succeeded, the Xlet should 
     * query the container for the result.
     *
     * @return An invisible container with an arbitrary size and position.
     *
     * @exception UnavailableContainerException - If policy or screen real estate 
     * does not permit a Container to be granted to the Xlet at this time.
     */

    public Container getContainer() throws UnavailableContainerException;

    /**
     * Returns the base class loader of the Xlet.  This class loader
     * will be dedicated exclusively to the xlet.  The xlet's classes are
     * all loaded by this classloader or by a classloader that has this
     * classloader as its ancestor.
     */
    public java.lang.ClassLoader getClassLoader();
}
