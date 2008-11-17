/*
 * @(#)Dialog.java	1.62 06/10/10
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

import java.awt.event.*;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;

/**
 * A dialog component is a top level window, .  It resembles a
 * frame, but has fewer properties. It does not have an icon or
 * a settable cursor. In PersonalJava and Personal Profile
 * implementations, the display of the title is optional.
 * <p>
 * A modal dialog, when visible, prevents the user from interacting
 * with AWT components. A modeless dialog behaves more like a Frame.
 * A Personal Profile Implementation must support modal dialogs.
 * Multiple (nested)  Modal Dialogs may be implemented so that only
 * a single dialog is visible at a time: when a new modal is shown,
 * it must become visible, and when it is dismissed, the previous modal
 * dialog must be shown.
 * <p>
 * Nonmodal  dialogs are optional in a Personal Profile implementation.
 * If they are not supported, an <em> UnsupportedOperationException </em> is
 * thrown when the consturctor is called and the system property
 * <em> java.awt.dialog.SupportsNonmodal </em> is set to "false".
 * <p>
 * The modality of a dialog cannot be changed after the dialog has been created.
 * <h3>System Properties</h3>
 * The following system properties will be set to either <code>"true"</code> or <code>"false"</code>
 * indicating which optional dialog features are supported by the Personal
 * Profile implementation:
 * <li>java.awt.dialog.SupportsNonmodal
 * <li>java.awt.dialog.SupportsMultipleModal
 * <li>java.awt.dialog.SupportsResize
 * <li>java.awt.dialog.SupportsTitle
 * @see WindowEvent
 * @see Window#addWindowListener
 *
 * @version 	1.46, 03/12/01
 * @author 	Sami Shaio
 * @author 	Arthur van Hoff
 * @since       JDK1.0
 */
public class Dialog extends Window {
    boolean	resizable = true;
    /**
     * Sets to true if the Dialog is modal.  A modal
     * Dialog grabs all the input to the parent frame from the user.
     */
    boolean modal;
    /**
     * The title of the Dialog.
     */
    String title;
    private transient boolean keepBlocking = false;
    private static final String base = "dialog";
    private static int nameCounter = 0;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 5920926903803293709L;
    /**
     * Creates a dialog.
     * @param parent the non null parent of the dialog
     * @see Component#setSize
     * @see Component#setVisible
     * @exception UnsupportedOperationException If the implementation does not support
     *            modeless dialogs.
     * @exception IllegalArgumentException if the parent in null
     * @since JDK1.0
     */
    public Dialog(Frame parent) {
        this(parent, "", false);
    }

    /**
     * Creates a dialog.
     * @param parent the non null parent of the dialog
     * @param modal the modality of the dialog. If true the dialog is modal,
     *        otherwise it is modeless. If the implementation does not
     *        support modeless dialogs and modal is false, an
     *        <em>UnsupportedOperationException</em> should be thrown.
     * @see Component#setSize
     * @see Component#setVisible
     * @exception UnsupportedOperationException If the implementation does not support
     *            modeless dialogs.
     * @exception IllegalArgumentException if the parent in null
     * @since JDK1.0
     */
    public Dialog(Frame parent, boolean modal) {
        this(parent, "", modal);
    }

    /**
     * Creates a dialog.
     * @param parent the non null parent of the dialog
     * @param title the title of the dialog
     * @see Component#setSize
     * @see Component#setVisible
     * @exception UnsupportedOperationException If the implementation does not support
     *            modeless dialogs.
     * @exception IllegalArgumentException if the parent in null
     * @since JDK1.0
     */
    public Dialog(Frame parent, String title) {
        this(parent, title, false);
    }

    /**
     * Creates a dialog.
     * @param parent the non null parent of the dialog
     * @param title the title of the dialog
     * @param modal the modality of the dialog. If true the dialog is modal,
     *        otherwise it is modeless. If the implementation does not
     *        support modeless dialogs and modal is false, an
     *        <em>UnsupportedOperationException</em> should be thrown.
     * @see Component#setSize
     * @see Component#setVisible
     * @exception UnsupportedOperationException If the implementation does not support
     *            modeless dialogs.
     * @exception IllegalArgumentException if the parent in null
     * @since JDK1.0
     */
    public Dialog(Frame parent, String title, boolean modal) {
        super(parent);
        if (parent == null) {
            throw new IllegalArgumentException("null parent frame");
        }
        this.title = title;
        setModal(modal);
    }

    /**
     * Construct a name for this component.  Called by getName() when the
     * name is null.
     */
    String constructComponentName() {
        return base + nameCounter++;
    }

    /**
     * Creates the dialog's peer.  The peer allows us to change the appearance
     * of the frame without changing its functionality.
     * @since JDK1.0
     */

    //    public void addNotify() {
    //        synchronized (getTreeLock()) {
    //            if (peer == null) {
    //                peer = ((PeerBasedToolkit)getToolkit()).createDialog(this);
    //            }
    //            super.addNotify();
    //        }
    //    }

    /**
     * Indicates whether the dialog is modal.
     * A modal dialog grabs all input from the user.
     * @return    <code>true</code> if this dialog window is modal;
     *            <code>false</code> otherwise.
     * @see       java.awt.Dialog#setModal
     * @since     JDK1.0
     */
    public boolean isModal() {
        return modal;
    }

    /**
     * Specifies the modality of the dialog.
     * This method sets the modal property as specified by modal. A
     * Personal Profile implementation may optionally support non-modal
     * dialogs or not allow the modal property to be changed after the
     * dialog is created. isModal() may be called to verify the setting
     * of the modal property.
     * @param <code>modal</code> desired modality for the dialog
     * @see       java.awt.Dialog#isModal
     * @since     JDK1.1
     */
    public void setModal(boolean modal) {
        this.modal = modal;
        DialogXWindow xwindow = (DialogXWindow) this.xwindow;
        if (xwindow != null)
            xwindow.setModal(modal);
    }

    /**
     * Gets the title of the dialog.
     * @return    the title of this dialog window.
     * @see       java.awt.Dialog#setTitle
     * @since     JDK1.0
     */
    public String getTitle() {
        return title;
    }

    /**
     * Sets the title of the Dialog.
     * @param title the new title being given to the dialog
     * @see #getTitle
     * @since JDK1.0
     */
    public synchronized void setTitle(String title) {
        this.title = title;
        DialogXWindow xwindow = (DialogXWindow) this.xwindow;
        if (xwindow != null)
            xwindow.setTitle(title);
    }
	
    ComponentXWindow createXWindow() {
        return new DialogXWindow(this);
    }
	
    /**
     * @return true if we actually showed, false if we just called toFront()
     */
    private boolean conditionalShow() {
        boolean retval;
        synchronized (getTreeLock()) {
            if (xwindow == null) {
                addNotify();
            }
            validate();
            if (visible) {
                toFront();
                retval = false;
            } else {
                visible = retval = true;
                xwindow.map(); // now guaranteed never to block
                //		createHierarchyEvents(HierarchyEvent.HIERARCHY_CHANGED,
                //				      this, parent,
                //				      HierarchyEvent.SHOWING_CHANGED);
            }
            if (retval && (componentListener != null ||
                    (eventMask & AWTEvent.COMPONENT_EVENT_MASK) != 0)) {
                ComponentEvent e =
                    new ComponentEvent(this, ComponentEvent.COMPONENT_SHOWN);
                Toolkit.getEventQueue().postEvent(e);
            }
        }
        if (retval && (state & OPENED) == 0) {
            postWindowEvent(WindowEvent.WINDOW_OPENED);
            state |= OPENED;
        }
        return retval;
    }

    /**
     * Makes this dialog visible and in front of all windows.
     * This method calls the dialog's addNotify() method,
     * validates the dialog's layout, and makes the dialog visible.
     * <p>
     * If the dialog is modal, show() will not return until the
     * dialog is no longer visible.
     * <p>
     * A Personal Profile or Personal Java implementation is not
     * required to support multiple modal dialogs. In this case, if
     * there is a previous modal dialog being shown, it can be hidden
     * until this Dialog is made invisible via setVisible() or dispose().
     * This must work recursively to allow for multiple nested modal dialogs.
     * @see Component#setVisible
     * @since JDK1.0
     */
    public void show() {
        if (!isModal()) {
            conditionalShow();
        } else {
            // Set this variable before calling conditionalShow(). That
            // way, if the Dialog is hidden right after being shown, we
            // won't mistakenly block this thread.
            keepBlocking = true;
            if (conditionalShow()) {
                // We have two mechanisms for blocking: 1. If we're on the
                // EventDispatchThread, start a new event pump. 2. If we're
                // on any other thread, call wait() on the treelock.

                if (Toolkit.getEventQueue().isDispatchThread()) {
                    EventDispatchThread dispatchThread =
                        (EventDispatchThread) Thread.currentThread();
                    dispatchThread.pumpEvents(new Conditional() {
                            public boolean evaluate() {
                                return keepBlocking && windowClosingException == null;
                            }
                        }
                    );
                } else {
                    synchronized (getTreeLock()) {
                        while (keepBlocking && windowClosingException == null) {
                            try {
                                getTreeLock().wait();
                            } catch (InterruptedException e) {
                                break;
                            }
                        }
                    }
                }
                if (windowClosingException != null) {
                    windowClosingException.fillInStackTrace();
                    throw windowClosingException;
                }
            }
        }
    }
	
    private void hideAndDisposeHandler() {
        if (keepBlocking) {
            synchronized (getTreeLock()) {
                keepBlocking = false;
                EventQueue.invokeLater(new Runnable() {
                        public void run() {}
                    }
                );
                getTreeLock().notifyAll();
            }
        }
    }

    /**
     * Hides the Dialog and then causes show() to return if it is currently
     * blocked.
     */
    public void hide() {
        super.hide();
        hideAndDisposeHandler();
    }
	
    /**
     * Disposes the Dialog and then causes show() to return if it is currently
     * blocked.
     */
    public void dispose() {
        super.dispose();
        hideAndDisposeHandler();
    }
	
    /**
     * Indicates whether this dialog window is resizable.
     * @return    <code>true</code> if the user can resize the dialog;
     *            <code>false</code> otherwise.
     * @see       java.awt.Dialog#setResizable
     * @since     JDK1.0
     */
    public boolean isResizable() {
        return resizable;
    }

    /**
     * Sets the Resizable property of the dialog.
     * This method sets the resizable property of the dialog. A
     * dilaog that is resizable may be resized by the user.  In Personal
     * Profile, this method may have no effect, it may be verified with
     * the isResizable() method.
     * @param     resizable <code>true</code> if the user can
     *                 resize this dialog; <code>false</code> otherwise.
     * @see       java.awt.Dialog#isResizable
     * @since     JDK1.0
     */
    public synchronized void setResizable(boolean resizable) {
        this.resizable = resizable;
        DialogXWindow xwindow = (DialogXWindow) this.xwindow;
        if (xwindow != null)
            xwindow.setResizable(resizable);
    }

    /**
     * Returns the parameter string representing the state of this
     * dialog window. This string is useful for debugging.
     * @return    the parameter string of this dialog window.
     * @since     JDK1.0
     */
    protected String paramString() {
        String str = super.paramString() + (modal ? ",modal" : ",modeless");
        if (title != null) {
            str += ",title=" + title;
        }
        return str;
    }

    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {
        s.defaultReadObject();
        setModal(this.modal);
    }
}
