/*
 * @(#)GToolkit.java	1.33 06/10/10
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

package sun.awt.gtk;

import java.awt.*;
import sun.awt.peer.*;
import java.awt.image.*;
import java.net.*;
import java.util.Properties;
import java.awt.datatransfer.Clipboard;
import sun.awt.SunToolkit;
import sun.awt.im.*;
import sun.awt.AppContext;
import sun.awt.PeerBasedToolkit;
import sun.awt.image.BufferedImagePeer;

public class GToolkit extends PeerBasedToolkit implements Runnable {
    private static native void initIDs();
    private Clipboard systemClipboard;
    /** Runs the gtk_main function for event processing. */
	
    public native void run();
    static {
        java.security.AccessController.doPrivileged(
            new sun.security.action.LoadLibraryAction("gtkawt"));
        initIDs();
    }
    public GToolkit() {
        // fix for 4705956 - need to initialize SunToolkit
        super();
        systemClipboard = new GtkClipboard("system");
        // Create a thread for processing main Gtk events.
		
        Thread mainThread = new Thread(this, "AWT-Gtk");
        mainThread.setDaemon(false);  // Don't exit when main finishes.
        mainThread.start();

        // Create a thread to flush drawing events to the server
		
        FlushThread flushThread = new FlushThread (this);
		
        Runtime.getRuntime().addShutdownHook(new ShutdownHook(mainThread, flushThread));
    }
	
    static native BufferedImagePeer getBufferedImagePeer(BufferedImage image);

    static native BufferedImage createBufferedImage(BufferedImagePeer peer);
    
    /*
     * Gets the default encoding used on the current platform
     * to transfer text (character) data. 
     */
    public String getDefaultCharacterEncoding() {
        return "iso8859-1";
    }    
	
    /**
     * Creates this toolkit's implementation of <code>Button</code> using
     * the specified peer interface.
     * @param     target the button to be implemented.
     * @return    this toolkit's implementation of <code>Button</code>.
     * @see       java.awt.Button
     * @see       sun.awt.peer.ButtonPeer
     * @since     JDK1.0
     */
    public ButtonPeer createButton(Button target) {
        return new GButtonPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>TextField</code> using
     * the specified peer interface.
     * @param     target the text field to be implemented.
     * @return    this toolkit's implementation of <code>TextField</code>.
     * @see       java.awt.TextField
     * @see       sun.awt.peer.TextFieldPeer
     * @since     JDK1.0
     */
    public TextFieldPeer createTextField(TextField target) {
        return new GTextFieldPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Label</code> using
     * the specified peer interface.
     * @param     target the label to be implemented.
     * @return    this toolkit's implementation of <code>Label</code>.
     * @see       java.awt.Label
     * @see       sun.awt.peer.LabelPeer
     * @since     JDK1.0
     */
    public LabelPeer 	createLabel(Label target) {
        return new GLabelPeer(this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>List</code> using
     * the specified peer interface.
     * @param     target the list to be implemented.
     * @return    this toolkit's implementation of <code>List</code>.
     * @see       java.awt.List
     * @see       sun.awt.peer.ListPeer
     * @since     JDK1.0
     */
    public ListPeer 	createList(List target) {
        return new GListPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Checkbox</code> using
     * the specified peer interface.
     * @param     target the check box to be implemented.
     * @return    this toolkit's implementation of <code>Checkbox</code>.
     * @see       java.awt.Checkbox
     * @see       sun.awt.peer.CheckboxPeer
     * @since     JDK1.0
     */
    public CheckboxPeer 	createCheckbox(Checkbox target) {
        return new GCheckboxPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Scrollbar</code> using
     * the specified peer interface.
     * @param     target the scroll bar to be implemented.
     * @return    this toolkit's implementation of <code>Scrollbar</code>.
     * @see       java.awt.Scrollbar
     * @see       sun.awt.peer.ScrollbarPeer
     * @since     JDK1.0
     */
    public ScrollbarPeer 	createScrollbar(Scrollbar target) {
        return new GScrollbarPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>ScrollPane</code> using
     * the specified peer interface.
     * @param     target the scroll pane to be implemented.
     * @return    this toolkit's implementation of <code>ScrollPane</code>.
     * @see       java.awt.ScrollPane
     * @see       sun.awt.peer.ScrollPanePeer
     * @since     JDK1.1
     */
    public ScrollPanePeer     createScrollPane(ScrollPane target) {
        return new GScrollPanePeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>TextArea</code> using
     * the specified peer interface.
     * @param     target the text area to be implemented.
     * @return    this toolkit's implementation of <code>TextArea</code>.
     * @see       java.awt.TextArea
     * @see       sun.awt.peer.TextAreaPeer
     * @since     JDK1.0
     */
    public TextAreaPeer  	createTextArea(TextArea target) {
        return new  GTextAreaPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Choice</code> using
     * the specified peer interface.
     * @param     target the choice to be implemented.
     * @return    this toolkit's implementation of <code>Choice</code>.
     * @see       java.awt.Choice
     * @see       sun.awt.peer.ChoicePeer
     * @since     JDK1.0
     */
    public ChoicePeer	createChoice(Choice target) {
        return new GChoicePeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Frame</code> using
     * the specified peer interface.
     * @param     target the frame to be implemented.
     * @return    this toolkit's implementation of <code>Frame</code>.
     * @see       java.awt.Frame
     * @see       sun.awt.peer.FramePeer
     * @since     JDK1.0
     */
    public FramePeer  	createFrame(Frame target) {
        return new GFramePeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Canvas</code> using
     * the specified peer interface.
     * @param     target the canvas to be implemented.
     * @return    this toolkit's implementation of <code>Canvas</code>.
     * @see       java.awt.Canvas
     * @see       sun.awt.peer.CanvasPeer
     * @since     JDK1.0
     */
    public CanvasPeer 	createCanvas(Canvas target) {
        return new GCanvasPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Panel</code> using
     * the specified peer interface.
     * @param     target the panel to be implemented.
     * @return    this toolkit's implementation of <code>Panel</code>.
     * @see       java.awt.Panel
     * @see       sun.awt.peer.PanelPeer
     * @since     JDK1.0
     */
    public PanelPeer  	createPanel(Panel target) {
        return new GPanelPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Window</code> using
     * the specified peer interface.
     * @param     target the window to be implemented.
     * @return    this toolkit's implementation of <code>Window</code>.
     * @see       java.awt.Window
     * @see       sun.awt.peer.WindowPeer
     * @since     JDK1.0
     */
    public WindowPeer  	createWindow(Window target) {
        return new GWindowPeer (this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>Dialog</code> using
     * the specified peer interface.
     * @param     target the dialog to be implemented.
     * @return    this toolkit's implementation of <code>Dialog</code>.
     * @see       java.awt.Dialog
     * @see       sun.awt.peer.DialogPeer
     * @since     JDK1.0
     */
    public DialogPeer createDialog(Dialog target) {
        return new GDialogPeer(this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>MenuBar</code> using
     * the specified peer interface.
     * @param     target the menu bar to be implemented.
     * @return    this toolkit's implementation of <code>MenuBar</code>.
     * @see       java.awt.MenuBar
     * @see       sun.awt.peer.MenuBarPeer
     * @since     JDK1.0
     */
    public MenuBarPeer  	createMenuBar(MenuBar target) {
        return new GMenuBarPeer (target);
    }

    /**
     * Creates this toolkit's implementation of <code>Menu</code> using
     * the specified peer interface.
     * @param     target the menu to be implemented.
     * @return    this toolkit's implementation of <code>Menu</code>.
     * @see       java.awt.Menu
     * @see       sun.awt.peer.MenuPeer
     * @since     JDK1.0
     */
    public MenuPeer  	createMenu(Menu target) {
        return new GMenuPeer (target);
    }

    /**
     * Creates this toolkit's implementation of <code>PopupMenu</code> using
     * the specified peer interface.
     * @param     target the popup menu to be implemented.
     * @return    this toolkit's implementation of <code>PopupMenu</code>.
     * @see       java.awt.PopupMenu
     * @see       sun.awt.peer.PopupMenuPeer
     * @since     JDK1.1
     */
    public PopupMenuPeer	createPopupMenu(PopupMenu target) {
        return new GPopupMenuPeer(target);
    }

    /**
     * Creates this toolkit's implementation of <code>MenuItem</code> using
     * the specified peer interface.
     * @param     target the menu item to be implemented.
     * @return    this toolkit's implementation of <code>MenuItem</code>.
     * @see       java.awt.MenuItem
     * @see       sun.awt.peer.MenuItemPeer
     * @since     JDK1.0
     */
    public MenuItemPeer  	createMenuItem(MenuItem target) {
        return new GMenuItemPeer (target);
    }

    /**
     * Creates this toolkit's implementation of <code>FileDialog</code> using
     * the specified peer interface.
     * @param     target the file dialog to be implemented.
     * @return    this toolkit's implementation of <code>FileDialog</code>.
     * @see       java.awt.FileDialog
     * @see       sun.awt.peer.FileDialogPeer
     * @since     JDK1.0
     */
    public FileDialogPeer	createFileDialog(FileDialog target) {
        return new GFileDialogPeer(this, target);
    }

    /**
     * Creates this toolkit's implementation of <code>CheckboxMenuItem</code> using
     * the specified peer interface.
     * @param     target the checkbox menu item to be implemented.
     * @return    this toolkit's implementation of <code>CheckboxMenuItem</code>.
     * @see       java.awt.CheckboxMenuItem
     * @see       sun.awt.peer.CheckboxMenuItemPeer
     * @since     JDK1.0
     */
    public CheckboxMenuItemPeer	createCheckboxMenuItem(CheckboxMenuItem target) {
        return new GCheckboxMenuItemPeer (target);
    }

    /**
     * Creates this toolkit's implementation of <code>Font</code> using
     * the specified peer interface.
     * @param     target the font to be implemented.
     * @return    this toolkit's implementation of <code>Font</code>.
     * @see       java.awt.Font
     * @see       sun.awt.peer.FontPeer
     * @since     JDK1.0
     */
    public FontPeer getFontPeer(Font target) {
        return GFontPeer.getFontPeer(target);
    }

    /**
     * Fills in the integer array that is supplied as an argument
     * with the current system color values.
     * <p>
     * This method is called by the method <code>updateSystemColors</code>
     * in the <code>SystemColor</code> class.
     * @param     an integer array.
     * @see       java.awt.SystemColor#updateSystemColors
     * @since     JDK1.1
     */
    protected void loadSystemColors(int[] systemColors) {}

    public native int getScreenWidth();

    public native int getScreenHeight();

    /**
     * Returns the screen resolution in dots-per-inch.
     * @return    this toolkit's screen resolution, in dots-per-inch.
     * @since     JDK1.0
     */
    public int getScreenResolution() {
        return 72;
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
    public native ColorModel getColorModel();

    /**
     * Gets the screen metrics of the font.
     * @param     font   a font.
     * @return    the screen metrics of the specified font in this toolkit.
     * @since     JDK1.0
     */
    public FontMetrics getFontMetrics(Font font) {
        return (FontMetrics) this.getFontPeer(font);
    }

    /**
     * Synchronizes this toolkit's graphics state. Some window systems
     * may do buffering of graphics events.
     * <p>
     * This method ensures that the display is up-to-date. It is useful
     * for animation.
     * @since     JDK1.0
     */
    public native void sync();

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
    static boolean prepareScrImage(Image img, int w, int h, ImageObserver o) {
        if (w == 0 || h == 0) {
            return true;
        }
        if (!(img instanceof GdkImage)) {
            return false;
        }
        GdkImage ximg = (GdkImage) img;
        if (ximg.hasError()) {
            if (o != null) {
                o.imageUpdate(img, ImageObserver.ERROR | ImageObserver.ABORT,
                    -1, -1, -1, -1);
            }
            return false;
        }
        sun.awt.image.ImageRepresentation ir = ximg.getImageRep();
        return ir.prepare(o);
    }

    static int checkScrImage(Image img, int w, int h, ImageObserver o) {
        if (!(img instanceof GdkImage)) {
            return ImageObserver.ALLBITS;
        }
        GdkImage ximg = (GdkImage) img;
        int repbits;
        if (w == 0 || h == 0) {
            repbits = ImageObserver.ALLBITS;
        } else {
            repbits = ximg.getImageRep().check(o);
        }
        return ximg.check(o) | repbits;
    }

    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
        return prepareScrImage(img, w, h, o);
    }

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
    public int checkImage(Image img, int w, int h, ImageObserver o) {
        return checkScrImage(img, w, h, o);
    }

    /**
     * Creates an image with the specified image producer.
     * @param     producer the image producer to be used.
     * @return    an image with the specified image producer.
     * @see       java.awt.Image
     * @see       java.awt.image.ImageProducer
     * @see       java.awt.Component#createImage(java.awt.image.ImageProducer)
     * @since     JDK1.0
     */
    public Image createImage(ImageProducer producer) {
        return new GdkImage (producer);
    }

    /**
     * Gets a <code>PrintJob</code> object which is the result
     * of initiating a print operation on the toolkit's platform.
     * @return    a <code>PrintJob</code> object, or
     *                  <code>null</code> if the user
     *                  cancelled the print job.
     * @see       java.awt.PrintJob
     * @since     JDK1.1
     */
    //    public PrintJob getPrintJob(Frame frame, String jobtitle, Properties props) {throw new UnsupportedOperationException ();}

    /**
     * Emits an audio beep.
     * @since     JDK1.1
     */
    public native void beep();
	
    public static void postEvent(AWTEvent event) {
        // Note that this might cause some problem in the future.
        // In jdk's MToolkit, targetToAppContext() is called at a peer level
        // with the peer's target as an argument.  AWTEvent's source is not
        // always the same as the peer's target.

        postEvent(targetToAppContext(event.getSource()), event);
    }

    public static void postEvent(AppContext appContext, AWTEvent event) {
        if (appContext == null) {
            appContext = AppContext.getAppContext();
        }
        EventQueue theEventQueue =
            (EventQueue) appContext.get(AppContext.EVENT_QUEUE_KEY);
        theEventQueue.postEvent(event);
        Thread.yield();
    }

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
    public Clipboard getSystemClipboard() {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkSystemClipboardAccess();
        }        
        return systemClipboard;
    }
	
    /**
     * Returns a new input method adapter for native input methods.
     */
    public InputMethod getInputMethodAdapter()
        throws AWTException {
        throw new UnsupportedOperationException();
    }
}


/** A thread which periodically flushes any drawing calls to the server. */

class FlushThread extends Thread {
    public FlushThread (Toolkit toolkit) {
        this.toolkit = toolkit;
        start();
    }
	
    public void run() {
        while (!isInterrupted()) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {}
			
            toolkit.sync();
        }
    }
	
    private Toolkit toolkit;
}


/** The thread which is run when the system is shutting down. */

class ShutdownHook extends Thread {
    public ShutdownHook (Thread mainThread, FlushThread flushThread) {
        this.mainThread = mainThread;
        this.flushThread = flushThread;
    }
	
    public void run() {
        // Ask the flush thread to finish

        flushThread.interrupt();

        // Ask the main gtk event processing thread to finish

        gtkMainQuit();

        // Wait for both threads to complete
		
        try {
            mainThread.join(2000);
            flushThread.join(2000);
        } catch (InterruptedException e) {}
    }
	
    private native void gtkMainQuit();

    private Thread mainThread;
    private FlushThread flushThread;
}
