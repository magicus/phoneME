/*
 * @(#)Frame.java	1.23 06/10/10
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
package java.awt;

import java.awt.event.*;
import java.util.Vector;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;

/**
 * A Frame is a top-level window with a title and a border.
 * <p>
 * The size of the frame includes any area designated for the
 * border.  The dimensions of the border area can be obtained
 * using the <code>getInsets</code> method, however, since
 * these dimensions are platform-dependent, a valid insets
 * value cannot be obtained until the frame is made displayable
 * by either calling <code>pack</code> or <code>show</code>.
 * Since the border area is included in the overall size of the
 * frame, the border effectively obscures a portion of the frame,
 * constraining the area available for rendering and/or displaying
 * subcomponents to the rectangle which has an upper-left corner
 * location of <code>(insets.left, insets.top)</code>, and has a size of
 * <code>width - (insets.left + insets.right)</code> by
 * <code>height - (insets.top + insets.bottom)</code>.
 * <p>
 * The default layout for a frame is BorderLayout.
 * <p>
 * In a multi-screen environment, you can create a <code>Frame</code>
 * on a different screen device by constructing the <code>Frame</code>
 * with {@link Frame(GraphicsConfiguration)} or
 * {@link Frame(String title, GraphicsConfiguration)}.  The
 * <code>GraphicsConfiguration</code> object is one of the
 * <code>GraphicsConfiguration</code> objects of the target screen
 * device.
 * <p>
 * In a virtual device multi-screen environment in which the desktop
 * area could span multiple physical screen devices, the bounds of all
 * configurations are relative to the virtual-coordinate system.  The
 * origin of the virtual-coordinate system is at the upper left-hand
 * corner of the primary physical screen.  Depending on the location
 * of the primary screen in the virtual device, negative coordinates
 * are possible, as shown in the following figure.
 * <p>
 * <img src="doc-files/MultiScreen.gif">
 * ALIGN=center HSPACE=10 VSPACE=7>
 * <p>
 * In such an environment, when calling <code>setLocation</code>,
 * you must pass a virtual coordinate to this method.  Similarly,
 * calling <code>getLocationOnScreen</code> on a <code>Frame</code>
 * returns virtual device coordinates.  Call the <code>getBounds</code>
 * method of a <code>GraphicsConfiguration</code> to find its origin in
 * the virtual coordinate system.
 * <p>
 * The following code sets the
 * location of the <code>Frame</code> at (10, 10) relative
 * to the origin of the physical screen of the corresponding
 * <code>GraphicsConfiguration</code>.  If the bounds of the
 * <code>GraphicsConfiguration</code> is not taken into account, the
 * <code>Frame</code> location would be set at (10, 10) relative to the
 * virtual-coordinate system and would appear on the primary physical
 * screen, which might be different from the physical screen of the
 * specified <code>GraphicsConfiguration</code>.
 *
 * <pre>
 *      Frame f = new Frame(GraphicsConfiguration gc);
 *      Rectangle bounds = gc.getBounds();
 *      f.setLocation(10 + bounds.x, 10 + bounds.y);
 * </pre>
 *
 * <p>
 * Frames are capable of generating the following types of window events:
 * WindowOpened, WindowClosing, WindowClosed, WindowIconified,
 * WindowDeiconified, WindowActivated, WindowDeactivated.
 *
 * <p>
 * <a name="restrictions">
 * <h4>Restrictions</h4>
 * <em>
 * Implementations of Frame in Personal Basis Profile exhibit
 * certain restrictionsm, specifically:
 * <ul>
 * <li> Only a single instance of Frame is permited at a time. Attempts to
 * construct a second Frame will cause the constructor to throw
 * <code>java.lang.UnsupportedOperationException</code>.  See:
 * <ul>
 * <li> {@link #Frame()}
 * <li> {@link #Frame(GraphicsConfiguration)}
 * <li> {@link #Frame(String)}
 * <li> {@link #Frame(String, GraphicsConfiguration)}
 * </ul>
 * <li> An implementation may support only a single Frame size.  In such
 * a case, attempts to change the size of the Frame will fail silently.
 * See:
 * <ul>
 * <li> {@link #setSize(int, int)}
 * <li> {@link #setSize(Dimension)}
 * <li> {@link #setBounds(int, int, int, int)}
 * <li> {@link #setBounds(Rectangle)}
 * </ul>
 * <li> An implementation may support only a single Frame location.  In
 * such a case, attempts to change the location of the Frame will fail
 * silently.  See:
 * <ul>
 * <li> {@link #setLocation(int, int)}
 * <li> {@link #setLocation(Point)}
 * <li> {@link #setBounds(int, int, int, int)}
 * <li> {@link #setBounds(Rectangle)}
 * </ul>
 * <li> An implementation may prohibit iconification.  In
 * such a case, attempts to iconify the Frame will fail silently.
 * See:
 * <ul>
 * <li> {@link #setState}
 * </ul>
 * <li> An implementation need not support a visible title.  In such a
 * case, the methods {@link #setTitle} and {@link #getTitle} behave as
 * normal, but no title is visible to the user.
 * See:
 * <ul>
 * <li> {@link #setTitle}
 * </ul>
 * </ul>
 * </em>
 *
 * @version 	1.6 11/21/01
 * @author 	Sami Shaio
 * @see WindowEvent
 * @see Window#addWindowListener
 * @since       JDK1.0
 *
 */
public class Frame extends Window {
    public static final int     NORMAL = 0;
    public static final int     ICONIFIED = 1;
    /**
     * This is the title of the frame.  It can be changed
     * at any time.  <code>title</code> can be null and if
     * this is the case the <code>title</code> = "".
     *
     * @serial
     * @see getTitle()
     * @see setTitle()
     */
    String 	title = "Untitled";
    /**
     * <code>icon</code> is the graphical way we can
     * represent the frame.
     * <code>icon</code> can be null, but obviously if
     * you try to set the icon image <code>icon</code>
     * cannot be null.
     *
     * @serial
     * @see getIconImage()
     * @see setIconImage()
     */
    Image  	icon;
    // ### Serialization difference
    // No field MenuBar menuBar

    /**
     * This field indicates whether the the frame is resizable
     * This property can be changed at any time.
     * <code>resizable</code> will be true if the frame is
     * resizable, otherwise it will be false.
     *
     * @serial
     * @see isResizable()
     */
    boolean	resizable = true; // 6248021

    /**
     * This field indicates whether the frame is undecorated.
     * This property can only be changed while the frame is not displayable.
     * <code>undecorated</code> will be true if the frame is
     * undecorated, otherwise it will be false.
     *
     * @serial
     * @see #setUndecorated(boolean)
     * @see #isUndecorated()
     * @see Component#isDisplayable()
     * @since 1.4
     */
    boolean undecorated = false;

    /**
     * <code>mbManagement</code> is only used by the Motif implementation.
     *
     * @serial
     */
    //boolean	mbManagement = false;	/* used only by the Motif impl.	*/

    private int state = NORMAL;
    /*
     * The Windows owned by the Frame.
     * Note: in 1.2 this has been superceded by Window.ownedWindowList
     *
     * @serial
     * @see java.awt.Window#ownedWindowList
     */
    //Vector ownedWindows;
	
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 2673458971256075116L;
    /**
     * Constructs a new instance of <code>Frame</code> that is
     * initially invisible.  The title of the <code>Frame</code>
     * is empty.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Basis Profile.</em>
     *
     * @see Component#setSize
     * @see Component#setVisible
     */
    public Frame() {
        this("", (GraphicsConfiguration) null);
    }

    /**
     * Create a <code>Frame</code> with the specified
     * <code>GraphicsConfiguration</code> of
     * a screen device.
     *
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Basis Profile.</em>
     *
     * @param gc the <code>GraphicsConfiguration</code>
     * of the target screen device. If <code>gc</code>
     * is <code>null</code>, the system default
     * <code>GraphicsConfiguration</code> is assumed.
     * @exception IllegalArgumentException if
     * <code>gc</code> is not from a screen device.
     * @since     1.3
     */
    public Frame(GraphicsConfiguration gc) {
        this("", gc);
    }

    /**
     * Constructs a new, initially invisible <code>Frame</code> object
     * with the specified title and a
     * <code>GraphicsConfiguration</code>.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Basis Profile.</em>
     *
     * @param title the title to be displayed in the frame's border.
     *              A <code>null</code> value
     *              is treated as an empty string, "".
     * @param gc the <code>GraphicsConfiguration</code>
     * of the target screen device.  If <code>gc</code> is
     * <code>null</code>, the system default
     * <code>GraphicsConfiguration</code> is assumed.
     * @exception IllegalArgumentException if <code>gc</code>
     * is not from a screen device.
     * @see java.awt.Component#setSize
     * @see java.awt.Component#setVisible
     * @see java.awt.GraphicsConfiguration#getBounds
     */
    public Frame(String title, GraphicsConfiguration gc) {
        super(gc);
        this.title = title;
        // 6206316
        setFocusTraversalPolicy(KeyboardFocusManager.
                                getCurrentKeyboardFocusManager().
                                getDefaultFocusTraversalPolicy()
                                );
    }

    /**
     * Constructs a new, initially invisible <code>Frame</code> object
     * with the specified title.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Basis Profile.</em>
     *
     * @param title the title to be displayed in the frame's border.
     *              A <code>null</code> value
     *              is treated as an empty string, "".
     * @exception IllegalArgumentException if gc is not from
     * a screen device.
     * @see java.awt.Component#setSize
     * @see java.awt.Component#setVisible
     * @see java.awt.GraphicsConfiguration#getBounds
     */
    public Frame(String title) {
        this(title, (GraphicsConfiguration) null);
    }

    /**
     * Gets the title of the frame.  The title is displayed in the
     * frame's border.
     * @return    the title of this frame, or an empty string ("")
     *                if this frame doesn't have a title.
     * @see       java.awt.Frame#setTitle
     */
    public String getTitle() {
        return title;
    }

    /**
     * Sets the title for this frame to the specified string.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Basis Profile.</em>
     *
     * @param    title    the title to be displayed in the frame's border
     * @param title the title to be displayed in the frame's border.
     *              A <code>null</code> value
     *              is treated as an empty string, "".
     * @see      java.awt.Frame#getTitle
     */
    public synchronized void setTitle(String title) {
        this.title = (title == null) ? "" : title;        
    }

    /**
     * Gets the image to be displayed in the minimized icon
     * for this frame.
     * @return    the icon image for this frame, or <code>null</code>
     *                    if this frame doesn't have an icon image.
     * @see       java.awt.Frame#setIconImage
     */
    public Image getIconImage() {
        return icon;
    }

    /**
     * Sets the image to displayed in the minimized icon for this frame.
     * Not all platforms support the concept of minimizing a window.
     * @param     image the icon image to be displayed.
     *            If this parameter is <code>null</code> then the
     *            icon image is set to the default image, which may vary
     *            with platform.
     * @see       java.awt.Frame#getIconImage
     */
    public synchronized void setIconImage(Image image) {
        icon = image;
    }

    /**
     * Indicates whether this frame is resizable by the user.
     * By default, all frames are initially resizable.
     * @return    <code>true</code> if the user can resize this frame;
     *                        <code>false</code> otherwise.
     * @see       java.awt.Frame#setResizable
     */
    public boolean isResizable() {
        return this.resizable; // 6248021
    }

    /**
     * Sets whether this frame is resizable by the user.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Basis Profile.</em>
     *
     * @param    resizable   <code>true</code> if this frame is resizable;
     *                       <code>false</code> otherwise.
     * @see      java.awt.Frame#isResizable
     */
    public void setResizable(boolean resizable) {
        this.resizable = resizable; // 6248021
    }

    /**
     * Disables or enables decorations for this frame.
     * This method can only be called while the frame is not displayable.
     * @param  undecorated <code>true</code> if no frame decorations are
     *         to be enabled;
     *         <code>false</code> if frame decorations are to be enabled.
     * @throws <code>IllegalComponentStateException</code> if the frame
     *         is displayable.
     * @see    #isUndecorated
     * @see    Component#isDisplayable
     * @see    javax.swing.JFrame#setDefaultLookAndFeelDecorated(boolean)
     * @since 1.4
     */
    public void setUndecorated(boolean undecorated) {
        /* Make sure we don't run in the middle of peer creation.*/
        synchronized (getTreeLock()) {
            if (isDisplayable()) {
                throw new IllegalComponentStateException("The frame is displayable.");
            }
            /* Basis frame is undecorated always - fail silently */
            // 6261403 - no longer allowed to fail silently - need to return
            // the correct value even if we don't support it
            this.undecorated = undecorated;
        }
    }

    /**
     * Indicates whether this frame is undecorated.
     * By default, all frames are initially decorated.
     * @return    <code>true</code> if frame is undecorated;
     *                        <code>false</code> otherwise.
     * @see       java.awt.Frame#setUndecorated(boolean)
     * @since 1.4
     */
    public boolean isUndecorated() {
        return undecorated;
    }
    /**
     * Sets the state of this frame.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Basis Profile.</em>
     *
     * @param  state <code>Frame.ICONIFIED</code> if this frame is in
     *           iconic state; <code>Frame.NORMAL</code> if this frame is
     *           in normal state.
     * @see      java.awt.Frame#getState
     */
    public synchronized void setState(int state) {
        getToolkit().isFrameStateSupported(state);
    }

    /**
     * Gets the state of this frame.
     * @return   <code>Frame.ICONIFIED</code> if frame in iconic state;
     *           <code>Frame.NORMAL</code> if frame is in normal state.
     * @see      java.awt.Frame#setState
     */
    public synchronized int getState() {
        return state;
    }

    /**
     * Returns the parameter String of this Frame.
     */
    protected String paramString() {
        return null;
    }
    /**
     * Frame Serialized Data Version.
     *
     * @serial
     */
    private int frameSerializedDataVersion = 1;
    /**
     * Writes default serializable fields to stream.  Writes
     * a list of serializable ItemListener(s) as optional data.
     * The non-serializable ItemListner(s) are detected and
     * no attempt is made to serialize them.
     *
     * @serialData Null terminated sequence of 0 or more pairs.
     *             The pair consists of a String and Object.
     *             The String indicates the type of object and
     *             is one of the following :
     *             itemListenerK indicating and ItemListener object.
     *
     * @see java.awt.Component.itemListenerK
     */
    private void writeObject(ObjectOutputStream s)
        throws IOException {}

    /**
     * Read the ObjectInputStream and if it isnt null
     * add a listener to receive item events fired
     * by the Frame.
     * Unrecognised keys or values will be Ignored.
     * @see removeActionListener()
     * @see addActionListener()
     */
    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {}
}
