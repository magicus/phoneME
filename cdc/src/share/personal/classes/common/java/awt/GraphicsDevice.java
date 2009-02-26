/*
 * @(#)GraphicsDevice.java	1.13 06/10/10
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


package java.awt;

/**
 * The <code>GraphicsDevice</code> class describes the graphics devices
 * that might be available in a particular graphics environment.  These
 * include screen and printer devices. Note that there can be many screens
 * and many printers in an instance of {@link GraphicsEnvironment}. Each
 * graphics device has one or more {@link GraphicsConfiguration} objects
 * associated with it.  These objects specify the different configurations
 * in which the <code>GraphicsDevice</code> can be used.
 * <p>
 * In a multi-screen environment, the <code>GraphicsConfiguration</code>
 * objects can be used to render components on multiple screens.  The
 * following code sample demonstrates how to create a <code>JFrame</code>
 * object for each <code>GraphicsConfiguration</code> on each screen
 * device in the <code>GraphicsEnvironment</code>:
 * <pre>
 *   GraphicsEnvironment ge = GraphicsEnvironment.
 *   getLocalGraphicsEnvironment();
 *   GraphicsDevice[] gs = ge.getScreenDevices();
 *   for (int j = 0; j < gs.length; j++) {
 *      GraphicsDevice gd = gs[j];
 *      GraphicsConfiguration[] gc =
 * 	gd.getConfigurations();
 *      for (int i=0; i < gc.length; i++) {
 *         JFrame f = new
 *         JFrame(gs[j].getDefaultConfiguration());
 *         Canvas c = new Canvas(gc[i]);
 *         Rectangle gcBounds = gc[i].getBounds();
 *         int xoffs = gcBounds.x;
 *         int yoffs = gcBounds.y;
 *	   f.getContentPane().add(c);
 *	   f.setLocation((i*50)+xoffs, (i*60)+yoffs);
 *         f.show();
 *      }
 *   }
 * </pre>
 * @see GraphicsEnvironment
 * @see GraphicsConfiguration
 * @version 1.21, 02/09/01
 */
public abstract class GraphicsDevice {
    private Window fullScreenWindow=null;
    private Rectangle windowedModeBounds=null;

    /**
     * This is an abstract class that cannot be instantiated directly.
     * Instances must be obtained from a suitable factory or query method.
     * @see GraphicsEnvironment#getScreenDevices
     * @see GraphicsEnvironment#getDefaultScreenDevice
     * @see GraphicsConfiguration#getDevice
     */
    protected GraphicsDevice() {}
    /**
     * Device is a raster screen.
     */
    public final static int TYPE_RASTER_SCREEN = 0;
    /**
     * Device is a printer.
     */
    public final static int TYPE_PRINTER = 1;
    /**
     * Device is an image buffer.  This buffer can reside in device
     * or system memory but it is not physically viewable by the user.
     */
    public final static int TYPE_IMAGE_BUFFER = 2;
    /**
     * Returns the type of this <code>GraphicsDevice</code>.
     * @return the type of this <code>GraphicsDevice</code>, which can
     * either be TYPE_RASTER_SCREEN, TYPE_PRINTER or TYPE_IMAGE_BUFFER.
     * @see #TYPE_RASTER_SCREEN
     * @see #TYPE_PRINTER
     * @see #TYPE_IMAGE_BUFFER
     */
    public abstract int getType();

    /**
     * Returns the identification string associated with this
     * <code>GraphicsDevice</code>.
     * <p>
     * A particular program might use more than one
     * <code>GraphicsDevice</code> in a <code>GraphicsEnvironment</code>.
     * This method returns a <code>String</code> identifying a
     * particular <code>GraphicsDevice</code> in the local
     * <code>GraphicsEnvironment</code>.  Although there is
     * no public method to set this <code>String</code>, a programmer can
     * use the <code>String</code> for debugging purposes.  Vendors of
     * the Java<sup><font size=-2>TM</font></sup> Runtime Environment can
     * format the return value of the <code>String</code>.  To determine
     * how to interpret the value of the <code>String</code>, contact the
     * vendor of your Java Runtime.  To find out who the vendor is, from
     * your program, call the
     * {@link System#getProperty(String) getProperty} method of the
     * System class with "java.vendor".
     * @return a <code>String</code> that is the identification
     * of this <code>GraphicsDevice</code>.
     */
    public abstract String getIDstring();
    
    /**
     * Returns all of the <code>GraphicsConfiguration</code>
     * objects associated with this <code>GraphicsDevice</code>.
     * @return an array of <code>GraphicsConfiguration</code>
     * objects that are associated with this
     * <code>GraphicsDevice</code>.
     */
    public abstract GraphicsConfiguration[] getConfigurations();

    /**
     * Returns the default <code>GraphicsConfiguration</code>
     * associated with this <code>GraphicsDevice</code>.
     * @return the default <code>GraphicsConfiguration</code>
     * of this <code>GraphicsDevice</code>.
     */
    public abstract GraphicsConfiguration getDefaultConfiguration();
    /**
     * Returns the "best" configuration possible that passes the
     * criteria defined in the {@link GraphicsConfigTemplate}.
     * @param gct the <code>GraphicsConfigTemplate</code> object
     * used to obtain a valid <code>GraphicsConfiguration</code>
     * @return a <code>GraphicsConfiguration</code> that passes
     * the criteria defined in the specified
     * <code>GraphicsConfigTemplate</code>.
     * @see GraphicsConfigTemplate
     */

    /*
     public GraphicsConfiguration
     getBestConfiguration(GraphicsConfigTemplate gct) {
     GraphicsConfiguration[] configs = getConfigurations();
     return gct.getBestConfiguration(configs);
     }
     */

    /**
     * Returns <code>true</code> if this <code>GraphicsDevice</code>
     * supports full-screen exclusive mode.
     * @return whether full-screen exclusive mode is available for
     * this graphics device
     * @since 1.4
     */
    public abstract boolean isFullScreenSupported();

    public abstract int getAvailableAcceleratedMemory();

    /**
     * Enter full-screen mode, or return to windowed mode.
     * <p>
     * If <code>isFullScreenSupported</code> returns <code>true</code>, full
     * screen mode is considered to be <i>exclusive</i>, which implies:
     * <ul>
     * <li>Windows cannot overlap the full-screen window.  All other application
     * windows will always appear beneath the full-screen window in the Z-order.
     * <li>Input method windows are disabled.  It is advisable to call
     * <code>Component.enableInputMethods(false)</code> to make a component
     * a non-client of the input method framework.
     * </ul>
     * <p>
     * If <code>isFullScreenSupported</code> returns
     * <code>false</code>, full-screen exclusive mode is simulated by resizing
     * the window to the size of the screen and positioning it at (0,0).
     * <p>
     * When returning to windowed mode from an exclusive full-screen window, any
     * display changes made by calling <code>setDisplayMode</code> are
     * automatically restored to their original state.
     *
     * @param w a window to use as the full-screen window; <code>null</code>
     * if returning to windowed mode.
     * @see #isFullScreenSupported
     * @see #getFullScreenWindow
     * @see #setDisplayMode
     * @see Component#enableInputMethods
     * @since 1.4
     */
    public abstract void setFullScreenWindow(Window w); 

    /**
     * Returns the <code>Window</code> object representing the
     * full-screen window if the device is in full-screen mode.
     * @return the full-screen window, <code>null</code> if the device is
     * not in full-screen mode.
     * @see #setFullScreenWindow(Window)
     * @since 1.4
     */
    public abstract Window getFullScreenWindow();
}
