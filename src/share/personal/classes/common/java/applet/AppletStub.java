/*
 * @(#)AppletStub.java	1.22 06/10/10
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
package java.applet;

import java.net.URL;

/**
 * When an applet is first created, an applet stub is attached to it
 * using the applet's <code>setStub</code> method. This stub
 * serves as the interface between the applet and the browser
 * environment or applet viewer environment in which the application
 * is running.
 *
 * @author 	Arthur van Hoff
 * @version     1.18, 08/19/02
 * @see         java.applet.Applet#setStub(java.applet.AppletStub)
 * @since       JDK1.0
 */
public interface AppletStub {
    /**
     * Determines if the applet is active. An applet is active just
     * before its <code>start</code> method is called. It becomes
     * inactive just before its <code>stop</code> method is called.
     *
     * @return  <code>true</code> if the applet is active;
     *          <code>false</code> otherwise.
     */
    boolean isActive();
    /**
     * Returns an absolute URL naming the directory of the document in which
     * the applet is embedded.  For example, suppose an applet is contained
     * within the document:
     * <blockquote><pre>
     *    http://java.sun.com/products/jdk/1.2/index.html
     * </pre></blockquote>
     * The document base is:
     * <blockquote><pre>
     *    http://java.sun.com/products/jdk/1.2/
     * </pre></blockquote>
     *
     * @return  the {@link java.net.URL} of the document that contains this
     *          applet.
     * @see     java.applet.AppletStub#getCodeBase()
     */
    URL getDocumentBase();
    /**
     * Gets the base URL.
     *
     * @return  the <code>URL</code> of the applet.
     */
    URL getCodeBase();
    /**
     * Returns the value of the named parameter in the HTML tag. For
     * example, if an applet is specified as
     * <blockquote><pre>
     * &lt;applet code="Clock" width=50 height=50&gt;
     * &lt;param name=Color value="blue"&gt;
     * &lt;/applet&gt;
     * </pre></blockquote>
     * <p>
     * then a call to <code>getParameter("Color")</code> returns the
     * value <code>"blue"</code>.
     *
     * @param   name   a parameter name.
     * @return  the value of the named parameter,
     * or <tt>null</tt> if not set.
     */
    String getParameter(String name);
    /**
     * Gets a handler to the applet's context.
     *
     * @return  the applet's context.
     */
    AppletContext getAppletContext();
    /**
     * Called when the applet wants to be resized.
     *
     * @param   width    the new requested width for the applet.
     * @param   height   the new requested height for the applet.
     */
    void appletResize(int width, int height);
}
