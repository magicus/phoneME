/*
 * @(#)Toolkit.java	1.114 06/10/10
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

/*
 * Warning :
 * Two versions of this file exist in this workspace.
 * One for Personal Basis, and one for Personal Profile.
 * Don't edit the wrong one !!!
 */

package java.awt;

import java.util.Properties;
import java.util.Map;
import java.util.WeakHashMap;
import java.util.EventListener;
import java.awt.im.InputMethodHighlight;
import java.awt.image.ImageObserver;
import java.awt.image.ImageProducer;
import java.awt.image.ColorModel;
import java.awt.datatransfer.Clipboard;
import java.awt.event.*;
import java.net.URL;
import java.io.BufferedInputStream;
import java.io.InputStream;
import java.security.AccessController;
import sun.security.action.GetPropertyAction;
import sun.io.FileIO;
import sun.io.FileIOFactory;
import java.util.ArrayList;

/* NOTE: It is no longer a requirement for <code>Toolkit</code> and AWT implementations to be peer based. However,
 the implementation of the <code>java.awt</code> components provided assume the toolkit is
 peer based. To create a toolkit to work with this implementation it is necessary to sub-class
 the <code>sun.awt.PeerBasedToolkit<code> class.*/

/**
 * This class is the abstract superclass of all actual
 * implementations of the Abstract Window Toolkit. Subclasses of
 * <code>Toolkit</code> are used to bind the various components
 * to particular native toolkit implementations.
 * <p>
 * Most applications should not call any of the methods in this
 * class directly. The methods defined by <code>Toolkit</code> are
 * the "glue" that joins the platform-independent classes in the
 * <code>java.awt</code> package with their counterparts in
 * <code>java.awt.peer</code>. Some methods defined by
 * <code>Toolkit</code> query the native operating system directly.
 *
 * @version 	1.102, 08/19/02
 * @author	Sami Shaio
 * @author	Arthur van Hoff
 * @author	Fred Ecks
 * @since       JDK1.0
 */

public abstract class  Toolkit {
    private AWTEventListener eventListener = null;
    private Map listener2SelectiveListener = new WeakHashMap();
    private AWTPermission listenToAllAWTEventsPermission = null;
    /**
     * Fills in the integer array that is supplied as an argument
     * with the current system color values.
     * <p>
     * This method is called by the method <code>updateSystemColors</code>
     * in the <code>SystemColor</code> class.
     * @param     systemColors  an integer array.
     * @since     JDK1.1
     */
    protected void loadSystemColors(int[] systemColors) {}

    /**
     * Gets the size of the screen.
     * @return    the size of this toolkit's screen, in pixels.
     * @since     JDK1.0
     */
    public abstract Dimension getScreenSize();

    /**
     * Returns the screen resolution in dots-per-inch.
     * @return    this toolkit's screen resolution, in dots-per-inch.
     * @since     JDK1.0
     */
    public abstract int getScreenResolution();


    /**
     * Gets the insets of the screen.
     * @param     gc a <code>GraphicsConfiguration</code>
     * @return    the insets of this toolkit's screen, in pixels.
     * @exception HeadlessException if GraphicsEnvironment.isHeadless()
     * returns true
     * @see       java.awt.GraphicsEnvironment#isHeadless
     * @since     1.4
     */
    public Insets getScreenInsets(GraphicsConfiguration gc)
        throws HeadlessException {
        if (this != Toolkit.getDefaultToolkit()) {
            return Toolkit.getDefaultToolkit().getScreenInsets(gc);
        } else {
            return new Insets(0, 0, 0, 0);
        }
    }

    /**
     * Determines the color model of this toolkit's screen.
     * <p>
     * <code>ColorModel</code> is an abstract class that
     * encapsulates the ability to translate between the
     * pixel values of an image and its red, green, blue,
     * and alpha components.
     * <p>
     * This toolkit method is called by the
     * <code>getColorModel</code> method
     * of the <code>Component</code> class.
     * @return    the color model of this toolkit's screen.
     * @see       java.awt.image.ColorModel
     * @see       java.awt.Component#getColorModel
     * @since     JDK1.0
     */
    public abstract ColorModel getColorModel();

    /**
     * Returns the names of the available fonts in this toolkit.<p>
     * For 1.1, the following font names are deprecated (the replacement
     * name follows):
     * <ul>
     * <li>TimesRoman (use Serif)
     * <li>Helvetica (use SansSerif)
     * <li>Courier (use Monospaced)
     * </ul><p>
     * The ZapfDingbats font is also deprecated in 1.1, but only as a
     * separate fontname.  Unicode defines the ZapfDingbat characters
     * starting at \u2700, and as of 1.1 Java supports those characters.
     * @return    the names of the available fonts in this toolkit.
     * @since     JDK1.0
     */
    public abstract String[] getFontList();

    /**
     * Gets the screen metrics of the font.
     * @param     font   a font.
     * @return    the screen metrics of the specified font in this toolkit.
     * @since     JDK1.0
     */
    public abstract FontMetrics getFontMetrics(Font font);

    /**
     * Synchronizes this toolkit's graphics state. Some window systems
     * may do buffering of graphics events.
     * <p>
     * This method ensures that the display is up-to-date. It is useful
     * for animation.
     * @since     JDK1.0
     */
    public abstract void sync();
    /**
     * The default toolkit.
     */
    private static Toolkit toolkit;
    // fix for 4187686 Several class objects are used for synchronization
    private static Object classLock = new Object();
    /**
     * Gets the default toolkit.
     * <p>
     * If there is a system property named <code>"awt.toolkit"</code>,
     * that property is treated as the name of a class that is a subclass
     * of <code>Toolkit</code>.
     * <p>
     * If the system property does not exist, then the default toolkit
     * used is <code>"sun.awt.motif.MToolkit"</code> for Solaris, and
     *<code>"sun.awt.gtk.GToolkit"</code> for Linux. Both of which are
     * implementations of Abstract Window Toolkits.
     * @return    the default toolkit.
     * @exception  AWTError  if a toolkit could not be found, or
     *                 if one could not be accessed or instantiated.
     * @since     JDK1.0
     */
    public static Toolkit getDefaultToolkit() {
        // fix for 4187686 Several class objects are used for synchronization
        synchronized (classLock) {
            if (toolkit == null) {
                // JDK 1.2 wraps this block of code with a Compiler.disable/enable
                // to turn off a JIT because toolkit initilization touches many
                // classes that are only touched once. If Personal Java uses a
                // JIT in the future that kind of wrapping would make sense here.
                java.security.AccessController.doPrivileged(
                    new java.security.PrivilegedAction() {
                        public Object run() {
                            String nm = null;
                            try {
                                nm = System.getProperty("awt.toolkit");
                                toolkit = (Toolkit) Class.forName(nm).newInstance();
                            } catch (ClassNotFoundException e) {
                                throw new AWTError("Toolkit not found: " + nm);
                            } catch (InstantiationException e) {
                                throw new AWTError("Could not instantiate Toolkit: "
                                        + nm);
                            } catch (IllegalAccessException e) {
                                throw new AWTError("Could not access Toolkit: "
                                        + nm);
                            }
                            return null;
                        }
                    }
                );
            }
        }
        return toolkit;
    }

    /** 
     * Gets a Graphics object for the supplied Window. The graphics object
     * should be initialised with the foreground, background and font of
     * the window and should draw to the GraphicsDevice the Window's
     * GraphicsConfiguration belongs to. This method is package protected so
     * as to be spec compatiable. It should be abstract but this would cause
     * spec issues.
     */
    Graphics getGraphics(Window window) {
        throw new UnsupportedOperationException();
    }
	
    /**
     * Gets the GraphicsEnvironment for this toolkit. This is called from
     * GraphicsEnvironment.getLocalGraphicsEnvironment. 
     */
    GraphicsEnvironment getLocalGraphicsEnvironment() {
        throw new UnsupportedOperationException();
    }
	
    /**
     * Returns an image which gets pixel data from the specified file,
     * whose format can be either GIF, JPEG or PNG.
     * The underlying toolkit attempts to resolve multiple requests
     * with the same filename to the same returned Image.
     * Since the mechanism required to facilitate this sharing of
     * Image objects may continue to hold onto images that are no
     * longer of use for an indefinate period of time, developers
     * are encouraged to implement their own caching of images by
     * using the createImage variant wherever available.
     * @param     filename   the name of a file containing pixel data
     *                         in a recognized file format.
     * @return    an image which gets its pixel data from
     *                         the specified file.
     * @see #createImage(java.lang.String)
     */
    public abstract Image getImage(String filename);

  // PBP/PP [6262540]
  // Fix for J2SE bug 6262530
    /**
     * Returns an image which gets pixel data from the specified URL.
     * The pixel data referenced by the specified URL must be in one
     * of the following formats: GIF, JPEG or PNG.
     * The underlying toolkit attempts to resolve multiple requests
     * with the same URL to the same returned Image.
     * Since the mechanism required to facilitate this sharing of
     * Image objects may continue to hold onto images that are no
     * longer of use for an indefinate period of time, developers
     * are encouraged to implement their own caching of images by
     * using the createImage variant wherever available.
     *
     * <p>
     * <em>
     * This method will throw {@link SecurityException} if the
     * caller does not have the permission obtained from
     * <code>url.openConnection.getPermission()</code>.
     * For compatibility with pre-1.2 security managers, if the permission
     * is a {@link java.io.FilePermission} or a
     * {@link java.net.SocketPermission}, then the 1.1-style
     * <code>SecurityManager.checkXXX</code> methods are called instead of
     * {@link SecurityManager#checkPermission}.
     * </em>
     *
     * <!-- PBP/PP -->
     * @throws SecurityException If the caller does not have permission to
     * access this URL.
     *
     * @param     url   the URL to use in fetching the pixel data.
     * @return    an image which gets its pixel data from
     *                         the specified URL.
     * @see #createImage(java.net.URL)
     */
    public abstract Image getImage(URL url);

    /**
     * Returns an image which gets pixel data from the specified file.
     * The returned Image is a new object which will not be shared
     * with any other caller of this method or its getImage variant.
     * @param     filename   the name of a file containing pixel data
     *                         in a recognized file format.
     * @return    an image which gets its pixel data from
     *                         the specified file.
     * @see       java.awt.Toolkit#getImage(java.lang.String)
     */

    public abstract Image createImage(String filename);

  // PBP/PP [6262540]
  // Fix for J2SE bug 6262530
    /**
     * Returns an image which gets pixel data from the specified URL.
     * The returned Image is a new object which will not be shared
     * with any other caller of this method or its getImage variant.
     *
     * <p>
     * <em>
     * This method will throw {@link SecurityException} if the
     * caller does not have the permission obtained from
     * <code>url.openConnection.getPermission()</code>.
     * For compatibility with pre-1.2 security managers, if the permission
     * is a {@link java.io.FilePermission} or a
     * {@link java.net.SocketPermission}, then the 1.1-style
     * <code>SecurityManager.checkXXX</code> methods are called instead of
     * {@link SecurityManager#checkPermission}.
     * </em>
     *
     * <!-- PBP/PP -->
     * @throws SecurityException If the caller does not have permission to
     * access this URL.
     *
     * @param     url   the URL to use in fetching the pixel data.
     * @return    an image which gets its pixel data from
     *                         the specified URL.
     * @see       java.awt.Toolkit#getImage(java.net.URL)
     */
  
    public abstract Image createImage(URL url);

    /**
     * Prepares an image for rendering.
     * <p>
     * If the values of the width and height arguments are both
     * <code>-1</code>, this method prepares the image for rendering
     * on the default screen; otherwise, this method prepares an image
     * for rendering on the default screen at the specified width and height.
     * <p>
     * The image data is downloaded asynchronously in another thread,
     * and an appropriately scaled screen representation of the image is
     * generated.
     * <p>
     * This method is called by components <code>prepareImage</code>
     * methods.
     * <p>
     * Information on the flags returned by this method can be found
     * with the definition of the <code>ImageObserver</code> interface.

     * @param     image      the image for which to prepare a
     *                           screen representation.
     * @param     width      the width of the desired screen
     *                           representation, or <code>-1</code>.
     * @param     height     the height of the desired screen
     *                           representation, or <code>-1</code>.
     * @param     observer   the <code>ImageObserver</code>
     *                           object to be notified as the
     *                           image is being prepared.
     * @return    <code>true</code> if the image has already been
     *                 fully prepared; <code>false</code> otherwise.
     * @see       java.awt.Component#prepareImage(java.awt.Image,
     *                 java.awt.image.ImageObserver)
     * @see       java.awt.Component#prepareImage(java.awt.Image,
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public abstract boolean prepareImage(Image image, int width, int height,
        ImageObserver observer);

    /**
     * Indicates the construction status of a specified image that is
     * being prepared for display.
     * <p>
     * If the values of the width and height arguments are both
     * <code>-1</code>, this method returns the construction status of
     * a screen representation of the specified image in this toolkit.
     * Otherwise, this method returns the construction status of a
     * scaled representation of the image at the specified width
     * and height.
     * <p>
     * This method does not cause the image to begin loading.
     * An application must call <code>prepareImage</code> to force
     * the loading of an image.
     * <p>
     * This method is called by the component's <code>checkImage</code>
     * methods.
     * <p>
     * Information on the flags returned by this method can be found
     * with the definition of the <code>ImageObserver</code> interface.
     * @param     image   the image whose status is being checked.
     * @param     width   the width of the scaled version whose status is
     *                 being checked, or <code>-1</code>.
     * @param     height  the height of the scaled version whose status
     *                 is being checked, or <code>-1</code>.
     * @param     observer   the <code>ImageObserver</code> object to be
     *                 notified as the image is being prepared.
     * @return    the bitwise inclusive <strong>OR</strong> of the
     *                 <code>ImageObserver</code> flags for the
     *                 image data that is currently available.
     * @see       java.awt.Toolkit#prepareImage(java.awt.Image,
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.Component#checkImage(java.awt.Image,
     *                 java.awt.image.ImageObserver)
     * @see       java.awt.Component#checkImage(java.awt.Image,
     *                 int, int, java.awt.image.ImageObserver)
     * @see       java.awt.image.ImageObserver
     * @since     JDK1.0
     */
    public abstract int checkImage(Image image, int width, int height,
        ImageObserver observer);

    /**
     * Creates an image with the specified image producer.
     * @param     producer the image producer to be used.
     * @return    an image with the specified image producer.
     * @see       java.awt.Image
     * @see       java.awt.image.ImageProducer
     * @see       java.awt.Component#createImage(java.awt.image.ImageProducer)
     * @since     JDK1.0
     */
    public abstract Image createImage(ImageProducer producer);

    /**
     * Creates an image which decodes the image stored in the specified
     * byte array.
     * <p>
     * The data must be in some image format, such as GIF or JPEG,
     * that is supported by this toolkit.
     * @param     imagedata   an array of bytes, representing
     *                         image data in a supported image format.
     * @return    an image.
     * @since     JDK1.1
     */
    public Image createImage(byte[] imagedata) {
        return createImage(imagedata, 0, imagedata.length);
    }

    /**
     * Creates an image which decodes the image stored in the specified
     * byte array, and at the specified offset and length.
     * The data must be in some image format, such as GIF or JPEG,
     * that is supported by this toolkit.
     * @param     imagedata   an array of bytes, representing
     *                         image data in a supported image format.
     * @param     imageoffset  the offset of the beginning
     *                         of the data in the array.
     * @param     imagelength  the length of the data in the array.
     * @return    an image.
     * @since     JDK1.1
     */
    public abstract Image createImage(byte[] imagedata,
        int imageoffset,
        int imagelength);

    /**
     * Emits an audio beep.
     * @since     JDK1.1
     */
    public abstract void beep();

    /**
     * Gets an instance of the system clipboard which interfaces
     * with clipboard facilities provided by the native platform.
     * <p>
     * This clipboard enables data transfer between Java programs
     * and native applications which use native clipboard facilities.
     * @return    an instance of the system clipboard.
     * @see       java.awt.datatransfer.Clipboard
     * @since     JDK1.1
     */
    public abstract Clipboard getSystemClipboard();

    /**
     * Determines which modifier key is the appropriate accelerator
     * key for menu shortcuts.
     * <p>
     * Menu shortcuts, which are embodied in the
     * <code>MenuShortcut</code> class, are handled by the
     * <code>MenuBar</code> class.
     * <p>
     * By default, this method returns <code>Event.CTRL_MASK</code>.
     * Toolkit implementations should override this method if the
     * <b>Control</b> key isn't the correct key for accelerators.
     * @return    the modifier mask on the <code>Event</code> class
     *                 that is used for menu shortcuts on this toolkit.
     * @see       java.awt.MenuBar
     * @see       java.awt.MenuShortcut
     * @since     JDK1.1
     */
    public int getMenuShortcutKeyMask() {
        return Event.CTRL_MASK;
    }
    /* Support for I18N: any visible strings should be stored in
     * lib/awt.properties.  The Properties list is stored here, so
     * that only one copy is maintained.
     */
    private static Properties properties;
    static {
        String sep = FileIO.separator;
        String filename = (String) AccessController.doPrivileged(
                new GetPropertyAction("java.home"))
            + sep + "lib" + sep + "awt.properties";
        FileIO propsFile = FileIOFactory.newInstance(filename);
        properties = new Properties();
        try {
            InputStream in = propsFile.getInputStream();
            properties.load(new BufferedInputStream(in));
            in.close();
        } catch (Exception e) {// No properties, defaults will be used.
        }
    }
    /**
     * Gets a property with the specified key and default.
     * This method returns defaultValue if the property is not found.
     */
    public static String getProperty(String key, String defaultValue) {
        String val = properties.getProperty(key);
        return (val == null) ? defaultValue : val;
    }

    /**
     * Get the application's or applet's EventQueue instance.
     * Depending on the Toolkit implementation, different EventQueues
     * may be returned for different applets.  Applets should
     * therefore not assume that the EventQueue instance returned
     * by this method will be shared by other applets or the system.
     */
    public final EventQueue getSystemEventQueue() {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkAwtEventQueueAccess();
        }
        return getSystemEventQueueImpl();
    }

    /*
     * Get the application's or applet's EventQueue instance, without
     * checking access.  For security reasons, this can only be called
     * from a Toolkit subclass.  Implementations wishing to modify
     * the default EventQueue support should subclass this method.
     */
    protected abstract EventQueue getSystemEventQueueImpl();

    /* Accessor method for use by AWT package routines. */
    static EventQueue getEventQueue() {
        return getDefaultToolkit().getSystemEventQueueImpl();
    }
	
    /**
     * Adds an AWTEventListener to receive all AWTEvents dispatched
     * system-wide that conform to the given <code>eventMask</code>.
     * <p>
     * First, if there is a security manager, its <code>checkPermission</code>
     * method is called with an
     * <code>AWTPermission("listenToAllAWTEvents")</code> permission.
     * This may result in a SecurityException.
     * <p>
     * <code>eventMask</code> is a bitmask of event types to receive.
     * It is constructed by bitwise OR-ing together the event masks
     * defined in <code>AWTEvent</code>.
     * <p>
     * Note:  event listener use is not recommended for normal
     * application use, but are intended solely to support special
     * purpose facilities including support for accessibility,
     * event record/playback, and diagnostic tracing.
     *
     * If listener is null, no exception is thrown and no action is performed.
     *
     * @param    listener   the event listener.
     * @param    eventMask  the bitmask of event types to receive
     * @throws SecurityException
     *        if a security manager exists and its
     *        <code>checkPermission</code> method doesn't allow the operation.
     * @see      java.awt.event.AWTEventListener
     * @see      java.awt.Toolkit#addAWTEventListener
     * @see      java.awt.AWTEvent
     * @see      SecurityManager#checkPermission
     * @see      java.awt.AWTPermission
     * @since    1.2
     */
    public void addAWTEventListener(AWTEventListener listener, long eventMask) {
        AWTEventListener localL = deProxyAWTEventListener(listener);
        if (localL == null) {
            return;
        }
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            if (listenToAllAWTEventsPermission == null) {
                listenToAllAWTEventsPermission =
                        new AWTPermission("listenToAllAWTEvents");
            }
            security.checkPermission(listenToAllAWTEventsPermission);
        }
        synchronized (this) {
            SelectiveAWTEventListener selectiveListener =
                (SelectiveAWTEventListener) listener2SelectiveListener.get(localL);
            if (selectiveListener == null) {
                // Create a new selectiveListener.
                selectiveListener =
                        new SelectiveAWTEventListener(localL, eventMask);
                listener2SelectiveListener.put(localL, selectiveListener);
                eventListener = ToolkitEventMulticaster.add(eventListener,
                            selectiveListener);
            }
            // OR the eventMask into the selectiveListener's event mask.
            selectiveListener.orEventMasks(eventMask);
        }
    }

    /**
     * Removes an AWTEventListener from receiving dispatched AWTEvents.
     * <p>
     * First, if there is a security manager, its <code>checkPermission</code>
     * method is called with an
     * <code>AWTPermission("listenToAllAWTEvents")</code> permission.
     * This may result in a SecurityException.
     * <p>
     * Note:  event listener use is not recommended for normal
     * application use, but are intended solely to support special
     * purpose facilities including support for accessibility,
     * event record/playback, and diagnostic tracing.
     *
     * If listener is null, no exception is thrown and no action is performed.
     *
     * @param    listener   the event listener.
     * @throws SecurityException
     *        if a security manager exists and its
     *        <code>checkPermission</code> method doesn't allow the operation.
     * @see      java.awt.event.AWTEventListener
     * @see      java.awt.Toolkit#addAWTEventListener
     * @see      java.awt.AWTEvent
     * @see      SecurityManager#checkPermission
     * @see      java.awt.AWTPermission
     * @since    1.2
     */
    public void removeAWTEventListener(AWTEventListener listener) {
        AWTEventListener localL = deProxyAWTEventListener(listener);

        if (listener == null) {
            return;
        }
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            if (listenToAllAWTEventsPermission == null) {
                listenToAllAWTEventsPermission =
                        new AWTPermission("listenToAllAWTEvents");
            }
            security.checkPermission(listenToAllAWTEventsPermission);
        }
        synchronized (this) {
            SelectiveAWTEventListener selectiveListener =
                (SelectiveAWTEventListener) listener2SelectiveListener.get(localL);
            if (selectiveListener != null)
                listener2SelectiveListener.remove(localL);
            eventListener = ToolkitEventMulticaster.remove(eventListener,
                        (selectiveListener == null) ? localL : selectiveListener);
        }
    }

    /*
     * Extracts a "pure" AWTEventListener from a AWTEventListenerProxy,
     * if the listener is proxied.
     */
    static private AWTEventListener deProxyAWTEventListener(AWTEventListener l)
    {
        AWTEventListener localL = l;
                                                                                
        if (localL == null) {
            return null;
        }
        // if user passed in a AWTEventListenerProxy object, extract
        // the listener
        if (l instanceof AWTEventListenerProxy) {
            localL = (AWTEventListener)((AWTEventListenerProxy)l).getListener();        }
        return localL;
    }

    /**
     * Returns an array of all the <code>AWTEventListener</code>s
     * registered on this toolkit.  Listeners can be returned
     * within <code>AWTEventListenerProxy</code> objects, which also contain
     * the event mask for the given listener.
     * Note that listener objects
     * added multiple times appear only once in the returned array.
     *
     * @return all of the <code>AWTEventListener</code>s or an empty
     *         array if no listeners are currently registered
     * @throws SecurityException
     *        if a security manager exists and its
     *        <code>checkPermission</code> method doesn't allow the operation.
     * @see      #addAWTEventListener
     * @see      #removeAWTEventListener
     * @see      SecurityManager#checkPermission
     * @see      java.awt.AWTEvent
     * @see      java.awt.AWTPermission
     * @see      java.awt.event.AWTEventListener
     * @see      java.awt.event.AWTEventListenerProxy
     * @since 1.4
     */
    public AWTEventListener[] getAWTEventListeners() {
        //6242260
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            if (listenToAllAWTEventsPermission == null) {
                listenToAllAWTEventsPermission =
                    new AWTPermission("listenToAllAWTEvents");
            }
            security.checkPermission(listenToAllAWTEventsPermission);
        }
        //6242260

        synchronized (this) {
            EventListener[] la = ToolkitEventMulticaster.getListeners(eventListener,AWTEventListener.class);

            AWTEventListener[] ret = new AWTEventListener[la.length];
            for (int i = 0; i < la.length; i++) {
                SelectiveAWTEventListener sael = (SelectiveAWTEventListener)la[i];
                AWTEventListener tempL = sael.getListener();
                //assert tempL is not an AWTEventListenerProxy - we should
                // have weeded them all out
                // don't want to wrap a proxy inside a proxy
                ret[i] = new AWTEventListenerProxy(sael.getEventMask(), tempL);
            }
            return ret;
        }
    }

    /**
     * Returns an array of all the <code>AWTEventListener</code>s
     * registered on this toolkit which listen to all of the event
     * types indicates in the <code>eventMask</code> argument.
     * Listeners can be returned
     * within <code>AWTEventListenerProxy</code> objects, which also contain
     * the event mask for the given listener.
     * Note that listener objects
     * added multiple times appear only once in the returned array.
     *
     * @param  eventMask the bitmask of event types to listen for
     * @return all of the <code>AWTEventListener</code>s registered
     *         on this toolkit for the specified
     *         event types, or an empty array if no such listeners
     *         are currently registered
     * @throws SecurityException
     *        if a security manager exists and its
     *        <code>checkPermission</code> method doesn't allow the operation.
     * @see      #addAWTEventListener
     * @see      #removeAWTEventListener
     * @see      SecurityManager#checkPermission
     * @see      java.awt.AWTEvent
     * @see      java.awt.AWTPermission
     * @see      java.awt.event.AWTEventListener
     * @see      java.awt.event.AWTEventListenerProxy
     * @since 1.4
     */
    public AWTEventListener[] getAWTEventListeners(long eventMask) {
        //6242260
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            if (listenToAllAWTEventsPermission == null) {
                listenToAllAWTEventsPermission =
                    new AWTPermission("listenToAllAWTEvents");
            }
            security.checkPermission(listenToAllAWTEventsPermission);
        }
        //6242260

        synchronized (this) {
            EventListener[] la = ToolkitEventMulticaster.getListeners(eventListener,AWTEventListener.class);

            java.util.List list = new ArrayList(la.length);

            for (int i = 0; i < la.length; i++) {
                SelectiveAWTEventListener sael = (SelectiveAWTEventListener)la[i];
                if ((sael.getEventMask() & eventMask) == eventMask) {
                    //AWTEventListener tempL = sael.getListener();
                    list.add(new AWTEventListenerProxy(sael.getEventMask(),
                                                       sael.getListener()));
                }
            }
            return (AWTEventListener[])list.toArray(new AWTEventListener[0]);
        }
    }
    /*
     * This method notifies any AWTEventListeners that an event
     * is about to be dispatched.
     *
     * @param theEvent the event which will be dispatched.
     */
    void notifyAWTEventListeners(AWTEvent theEvent) {
        if (eventListener != null) {
            synchronized (this) {
                if (eventListener != null)
                    eventListener.eventDispatched(theEvent);
            }
        }
    }
    static private class ToolkitEventMulticaster extends AWTEventMulticaster
        implements AWTEventListener {
        // Implementation cloned from AWTEventMulticaster.

        ToolkitEventMulticaster(AWTEventListener a, AWTEventListener b) {
            super(a, b);
        }

        static AWTEventListener add(AWTEventListener a,
            AWTEventListener b) {
            if (a == null)  return b;
            if (b == null)  return a;
            return new ToolkitEventMulticaster(a, b);
        }

        static AWTEventListener remove(AWTEventListener l,
            AWTEventListener oldl) {
            return (AWTEventListener) removeInternal(l, oldl);
        }

        // #4178589: must overload remove(EventListener) to call our add()
        // instead of the static addInternal() so we allocate a
        // ToolkitEventMulticaster instead of an AWTEventMulticaster.
        // Note: this method is called by AWTEventListener.removeInternal(),
        // so its method signature must match AWTEventListener.remove().
        protected EventListener remove(EventListener oldl) {
            if (oldl == a)  return b;
            if (oldl == b)  return a;
            AWTEventListener a2 = (AWTEventListener) removeInternal(a, oldl);
            AWTEventListener b2 = (AWTEventListener) removeInternal(b, oldl);
            if (a2 == a && b2 == b) {
                return this;	// it's not here
            }
            return add(a2, b2);
        }

        public void eventDispatched(AWTEvent event) {
            ((AWTEventListener) a).eventDispatched(event);
            ((AWTEventListener) b).eventDispatched(event);
        }
    }

    private class SelectiveAWTEventListener implements AWTEventListener {
        AWTEventListener listener;
        private long eventMask;
        static final int LONG_BITS = 64;
        // This array contains the number of times to call the eventlistener
        // for each event type.
        int[] calls = new int[LONG_BITS];
        public AWTEventListener getListener() {return listener;}
        public long getEventMask() {return eventMask;}
        public int[] getCalls() {return calls;}

        public void orEventMasks(long mask) {
            eventMask |= mask;
            // For each event bit set in mask, increment its call count.
            for (int i = 0; i < LONG_BITS; i++) {
                // If no bits are set, break out of loop.
                if (mask == 0) {
                    break;
                }
                if ((mask & 1L) != 0) {  // Always test bit 0.
                    calls[i]++;
                }
                mask >>>= 1;  // Right shift, fill with zeros on left.
            }
        }

        SelectiveAWTEventListener(AWTEventListener l, long mask) {
            listener = l;
            eventMask = mask;
        }

        public void eventDispatched(AWTEvent event) {
            long eventBit = 0; // Used to save the bit of the event type.
            if (((eventBit = eventMask & AWTEvent.COMPONENT_EVENT_MASK) != 0 &&
                    event.id >= ComponentEvent.COMPONENT_FIRST &&
                    event.id <= ComponentEvent.COMPONENT_LAST)
                || ((eventBit = eventMask & AWTEvent.CONTAINER_EVENT_MASK) != 0 &&
                    event.id >= ContainerEvent.CONTAINER_FIRST &&
                    event.id <= ContainerEvent.CONTAINER_LAST)
                || ((eventBit = eventMask & AWTEvent.FOCUS_EVENT_MASK) != 0 &&
                    event.id >= FocusEvent.FOCUS_FIRST &&
                    event.id <= FocusEvent.FOCUS_LAST)
                || ((eventBit = eventMask & AWTEvent.KEY_EVENT_MASK) != 0 &&
                    event.id >= KeyEvent.KEY_FIRST &&
                    event.id <= KeyEvent.KEY_LAST)
                || ((eventBit = eventMask & AWTEvent.MOUSE_MOTION_EVENT_MASK) != 0 &&
                    (event.id == MouseEvent.MOUSE_MOVED ||
                        event.id == MouseEvent.MOUSE_DRAGGED))
                || ((eventBit = eventMask & AWTEvent.MOUSE_EVENT_MASK) != 0 &&
                    event.id != MouseEvent.MOUSE_MOVED &&
                    event.id != MouseEvent.MOUSE_DRAGGED &&
                    event.id >= MouseEvent.MOUSE_FIRST &&
                    event.id <= MouseEvent.MOUSE_LAST)
                || ((eventBit = eventMask & AWTEvent.WINDOW_EVENT_MASK) != 0 &&
                    event.id >= WindowEvent.WINDOW_FIRST &&
                    event.id <= WindowEvent.WINDOW_LAST)
                || ((eventBit = eventMask & AWTEvent.ACTION_EVENT_MASK) != 0 &&
                    event.id >= ActionEvent.ACTION_FIRST &&
                    event.id <= ActionEvent.ACTION_LAST)
                || ((eventBit = eventMask & AWTEvent.ADJUSTMENT_EVENT_MASK) != 0 &&
                    event.id >= AdjustmentEvent.ADJUSTMENT_FIRST &&
                    event.id <= AdjustmentEvent.ADJUSTMENT_LAST)
                || ((eventBit = eventMask & AWTEvent.ITEM_EVENT_MASK) != 0 &&
                    event.id >= ItemEvent.ITEM_FIRST &&
                    event.id <= ItemEvent.ITEM_LAST)
                || ((eventBit = eventMask & AWTEvent.TEXT_EVENT_MASK) != 0 &&
                    event.id >= TextEvent.TEXT_FIRST &&
                    event.id <= TextEvent.TEXT_LAST)
            
                /*|| ((eventBit = eventMask & AWTEvent.INPUT_METHOD_EVENT_MASK) != 0 &&
                 event.id >= InputMethodEvent.INPUT_METHOD_FIRST &&
                 event.id <= InputMethodEvent.INPUT_METHOD_LAST)
                 || ((eventBit = eventMask & AWTEvent.PAINT_EVENT_MASK) != 0 &&
                 event.id >= PaintEvent.PAINT_FIRST &&
                 event.id <= PaintEvent.PAINT_LAST)
                 || ((eventBit = eventMask & AWTEvent.INVOCATION_EVENT_MASK) != 0 &&
                 event.id >= InvocationEvent.INVOCATION_FIRST &&
                 event.id <= InvocationEvent.INVOCATION_LAST)
                 || ((eventBit = eventMask & AWTEvent.HIERARCHY_EVENT_MASK) != 0 &&
                 event.id == HierarchyEvent.HIERARCHY_CHANGED)
                 || ((eventBit = eventMask & AWTEvent.HIERARCHY_BOUNDS_EVENT_MASK) != 0 &&
                 (event.id == HierarchyEvent.ANCESTOR_MOVED ||
                 event.id == HierarchyEvent.ANCESTOR_RESIZED))*/) {
                // Get the index of the call count for this event type.
                int ci = (int) (Math.log(eventBit) / Math.log(2));
                // Call the listener as many times as it was added for this
                // event type.
                for (int i = 0; i < calls[ci]; i++) {
                    listener.eventDispatched(event);
                }
            }
        }
    }

    /**
     * Returns a map of visual attributes for the abstract level description
     * of the given input method highlight, or null if no mapping is found.
     * The style field of the input method highlight is ignored. The map
     * returned is unmodifiable.
     * @param highlight input method highlight
     * @return style attribute map, or <code>null</code>
     * @exception HeadlessException if
     *     <code>GraphicsEnvironment.isHeadless</code> returns true
     * @see       java.awt.GraphicsEnvironment#isHeadless
     * @since 1.3
     */
/*
    public abstract Map mapInputMethodHighlight(
        InputMethodHighlight highlight) throws HeadlessException;
*/

    /**
     * Returns whether Toolkit supports this state for
     * <code>Frame</code>s.  This method tells whether the <em>UI
     * concept</em> of iconification is
     * supported.  It will always return false for any state other than
     * like <code>Frame.NORMAL or Frame.ICONIFIED</code>.
     *
     * @param state one of named frame state constants.
     * @return <code>true</code> is this frame state is supported by
     *     this Toolkit implementation, <code>false</code> otherwise.
     * @exception HeadlessException
     *     if <code>GraphicsEnvironment.isHeadless()</code>
     *     returns <code>true</code>.
     */
    public boolean isFrameStateSupported(int state)
        throws HeadlessException
    {
        if (this != Toolkit.getDefaultToolkit()) {
            return Toolkit.getDefaultToolkit().
                isFrameStateSupported(state);
        } else {
            return (state == Frame.NORMAL); // others are not guaranteed
        }
    }
}
