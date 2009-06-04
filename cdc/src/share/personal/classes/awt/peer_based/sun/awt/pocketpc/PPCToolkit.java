/*
 * @(#)PPCToolkit.java	1.27 06/10/10
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

package sun.awt.pocketpc;

import java.awt.*;
import java.awt.image.*;
import sun.awt.peer.*;
import java.awt.datatransfer.Clipboard;
import java.net.URL;
import java.util.Properties;
import sun.awt.im.*;
import sun.awt.AppContext;
import sun.awt.SunToolkit;
import sun.awt.PeerBasedToolkit;
import sun.awt.image.BufferedImagePeer;
import sun.awt.image.ImageRepresentation;

public class PPCToolkit extends PeerBasedToolkit 
    implements Runnable  {

    private static native void initIDs();
    
    static 
    {
	java.security.AccessController.doPrivileged(new 
	    sun.security.action.LoadLibraryAction("pocketpcawt"));
        initIDs();
    }

// Debugging properties
    static final boolean enableDebug = true;

    // Print any AWT_TRACE statements
    boolean dbgTrace = false;
    private static final String propTrace = "WINAWT.Trace";

    // Verify all components are in sync with their HWND state
    boolean dbgVerify = false;
    private static final String propVerify = "WINAWT.VerifyComponents";

    // Enter debugger on assertion failure
    boolean dbgBreak = false;
    private static final String propBreak = "WINAWT.BreakOnFailure";

    // Enable heap checking
    boolean dbgHeapCheck = false;
    private static final String propHeapCheck = "WINAWT.HeapCheck";

    // System clipboard.
    PPCClipboard clipboard;

    // Some members that are useful to some components
    // TODO: make static when move to JNI
    protected int frameWidth;
    protected int frameHeight;
    protected int vsbMinHeight;
    protected int vsbWidth;
    protected int hsbMinWidth;
    protected int hsbHeight;

    public PPCToolkit() {
        InitializeDebuggingOptions();

        // Startup toolkit thread
        synchronized (this) {

            Thread t = new Thread(this, "AWT-Windows");
            t.setDaemon(false);  // Don't exit when main finishes.
            t.start();
            try {
                wait();
            }
            catch (InterruptedException x) {
            }
	    // Create a thread to flush drawing events to the server
	    
	    FlushThread flushThread = new FlushThread (this);
	    
	    Runtime.getRuntime().addShutdownHook(new ShutdownHook(t, flushThread));
        }
    }

    private void InitializeDebuggingOptions() {
        String s = Toolkit.getProperty(propTrace, "false");
        dbgTrace = Boolean.valueOf(s).booleanValue();
        s = Toolkit.getProperty(propVerify, "false");
        dbgVerify = Boolean.valueOf(s).booleanValue();
        s = Toolkit.getProperty(propBreak, "false");
        dbgBreak = Boolean.valueOf(s).booleanValue();
        s = Toolkit.getProperty(propHeapCheck, "false");
        dbgHeapCheck = Boolean.valueOf(s).booleanValue();
    }

    public native void init(Thread t);

    public void run() {
        Thread t = Thread.currentThread();
        init(t);
        synchronized (this) {
            notifyAll();
        }
        eventLoop();
    }

    /* 
     * Notifies the thread that started the server
     * to indicate that initialization is complete.
     * Then enters an infinite loop that retrieves and processes events.  
     */
    native void eventLoop();
  
    public static void postEvent(AWTEvent event) {
        postEvent(targetToAppContext(event.getSource()), event);
    }

    public static void postEvent(AppContext appContext, AWTEvent event)
    {
	if (appContext == null) {
	    appContext = AppContext.getAppContext();
	}
	
        EventQueue theEventQueue =
	    (EventQueue)appContext.get(AppContext.EVENT_QUEUE_KEY);

        theEventQueue.postEvent(event);
	Thread.yield();
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
	
    /*
     * Create peer objects.
     */

    public ButtonPeer createButton(Button target) {
	ButtonPeer peer = new PPCButtonPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public TextFieldPeer createTextField(TextField target) {
	TextFieldPeer peer = new PPCTextFieldPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public LabelPeer createLabel(Label target) {
	LabelPeer peer = new PPCLabelPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public ListPeer createList(List target) {
	ListPeer peer = new PPCListPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public CheckboxPeer createCheckbox(Checkbox target) {
	CheckboxPeer peer = new PPCCheckboxPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public ScrollbarPeer createScrollbar(Scrollbar target) {
	ScrollbarPeer peer = new PPCScrollbarPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public ScrollPanePeer createScrollPane(ScrollPane target) {
	ScrollPanePeer peer = new PPCScrollPanePeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public TextAreaPeer createTextArea(TextArea target) {
	TextAreaPeer peer = new PPCTextAreaPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    static native int getComboHeightOffset();

    public ChoicePeer createChoice(Choice target) {
	ChoicePeer peer = new PPCChoicePeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public FramePeer  createFrame(Frame target) {
	FramePeer peer = new PPCFramePeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public CanvasPeer createCanvas(Canvas target) {
	CanvasPeer peer = new PPCCanvasPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public PanelPeer createPanel(Panel target) {
	PanelPeer peer = new PPCPanelPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public WindowPeer createWindow(Window target) {
	WindowPeer peer = new PPCWindowPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public DialogPeer createDialog(Dialog target) {
	DialogPeer peer = new PPCDialogPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public FileDialogPeer createFileDialog(FileDialog target) {
	FileDialogPeer peer = new PPCFileDialogPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public MenuBarPeer createMenuBar(MenuBar target) {
	MenuBarPeer peer = new PPCMenuBarPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public MenuPeer createMenu(Menu target) {
	
	//Netscape : Changed this to 2-step construction to fix problem with 
	//creating empty top level menus.  Basically, we have to put the menupeer
	//into the peer map before we actually go out and create the peer, because
	//peer creation will try to look the peer up in the peer map.  This was 
	//causing a menus without items to be created at enormously ridculous
	//widths.

	PPCMenuPeer peer = new PPCMenuPeer(target);
	peerMap.put(target, peer);
	peer.create();
	return peer;
    }

    public PopupMenuPeer createPopupMenu(PopupMenu target) {
	PopupMenuPeer peer = new PPCPopupMenuPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public MenuItemPeer createMenuItem(MenuItem target) {
	MenuItemPeer peer = new PPCMenuItemPeer(target);
	peerMap.put(target, peer);
	return peer;
    }

    public CheckboxMenuItemPeer
    createCheckboxMenuItem(CheckboxMenuItem target) {
	CheckboxMenuItemPeer peer = new PPCCheckboxMenuItemPeer(target);
	peerMap.put(target, peer);
	return peer;
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
        return PPCFontPeer.getFontPeer(target);
    }

    static boolean prepareScrImage(Image img, int w, int h,
				   ImageObserver o) {	
	if (w == 0 || h == 0) {
	    return true;
	}
	PPCImage ximg = (PPCImage) img;
	if (ximg.hasError()) {
	    if (o != null) {
		o.imageUpdate(img, ImageObserver.ERROR|ImageObserver.ABORT,
			      -1, -1, -1, -1);
	    }
	    return false;
	}
	ImageRepresentation ir = ximg.getImageRep();
	return ir.prepare(o);	
	//return false;
    }

    static int checkScrImage(Image img, int w, int h, ImageObserver o)
    {
        if (!(img instanceof PPCImage)) {
           return ImageObserver.ALLBITS;
        } 

	PPCImage ximg = (PPCImage) img;
	int repbits;
	if (w == 0 || h == 0) {
	    repbits = ImageObserver.ALLBITS;
	}
        else {
	    repbits = ximg.getImageRep().check(o);
	}
	return ximg.check(o) | repbits;
    }

    public int checkImage(Image img, int w, int h, ImageObserver o) {
	return checkScrImage(img, w, h, o);
    }

    public boolean prepareImage(Image img, int w, int h, ImageObserver o) {
	return prepareScrImage(img, w, h, o);
    }

    public Image createImage(ImageProducer producer) {
	return new PPCImage(producer);
    }

    static native ColorModel makeColorModel();
    static ColorModel screenmodel;

    static ColorModel getStaticColorModel() {
	if (screenmodel == null) {
	    screenmodel = makeColorModel();
	}
	return screenmodel;
    }

    public ColorModel getColorModel() {
	return getStaticColorModel();
    }

    public native int getScreenResolution();
    protected native int getScreenWidth();
    protected native int getScreenHeight();

    public FontMetrics getFontMetrics(Font font) {
	return PPCFontMetrics.getFontMetrics(font);
    }
    
    public native void sync();

    public native void beep();

    public Clipboard getSystemClipboard() {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
	  security.checkSystemClipboardAccess();
	}
        if (clipboard == null) {
	    clipboard = new PPCClipboard();
	}
	return clipboard;
    }

    protected native void loadSystemColors(int[] systemColors);

    /**
     * Returns a new input method adapter for native input methods.
     */
    public InputMethod getInputMethodAdapter() {
        return null;
    }

    /**
     * Returns whether enableInputMethods should be set to true for peered
     * TextComponent instances on this platform.
     */
    public boolean enableInputMethodsForTextComponent() {
        return true;
    }

    /*
     * Get the java.awt.Component object which corresponds to the native
     * window handle argument.  If no Component is found, null is returned.
     * 
     * This method is used by the accessibility software on Win32.
     */
    public native Component getComponentFromNativeWindowHandle(int hwnd);

    /*
     * Get the native window handle which corresponds to the given peer.
     */
    private native int getNativeWindowHandleFromPeer(PPCComponentPeer peer);

    /*
     * Get the native window handle which corresponds to the
     * java.awt.Component object.  If the Component is lightweight,
     * its native container's native window handle is returned.
     * 
     * This method is used by the accessibility software on Win32.
     */
    public int getNativeWindowHandleFromComponent(Component target) {
        // Find the native component's PPCComponentPeer, and call it for this
        sun.awt.peer.ComponentPeer peer = 
	    (sun.awt.peer.ComponentPeer) targetToPeer(target);
        while ((peer != null) && !(peer instanceof PPCComponentPeer)) {
            target = target.getParent();
            if (target == null)
                return 0;   // No non-lightweight parent found
            peer = (sun.awt.peer.ComponentPeer) targetToPeer(target);
        }
        if (peer == null)
            return 0;   // No suitable peer found -- maybe not created yet
        return getNativeWindowHandleFromPeer((PPCComponentPeer)peer);
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


