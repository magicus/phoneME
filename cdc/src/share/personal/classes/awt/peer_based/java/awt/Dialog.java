/*
 * @(#)Dialog.java	1.19 06/10/10
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
import sun.awt.PeerBasedToolkit;
import sun.awt.peer.DialogPeer;
import sun.awt.AppContext;
import sun.awt.SunToolkit;
import sun.awt.PeerEvent;

/**
 * A Dialog is a top-level window with a title and a border
 * that is typically used to take some form of input from the user.
 *
 * The size of the dialog includes any area designated for the
 * border.  The dimensions of the border area can be obtained
 * using the <code>getInsets</code> method, however, since
 * these dimensions are platform-dependent, a valid insets
 * value cannot be obtained until the dialog is made displayable
 * by either calling <code>pack</code> or <code>show</code>.
 * Since the border area is included in the overall size of the
 * dialog, the border effectively obscures a portion of the dialog,
 * constraining the area available for rendering and/or displaying
 * subcomponents to the rectangle which has an upper-left corner
 * location of <code>(insets.left, insets.top)</code>, and has a size of
 * <code>width - (insets.left + insets.right)</code> by
 * <code>height - (insets.top + insets.bottom)</code>.
 * <p>
 * The default layout for a dialog is BorderLayout.
 * <p>
 * A dialog must have either a frame or another dialog defined as its
 * owner when it's constructed.  When the owner window of a visible dialog
 * is hidden or minimized, the dialog will automatically be hidden
 * from the user. When the owner window is subsequently re-opened, then
 * the dialog is made visible to the user again.
 * <p>
 * A dialog can be either modeless (the default) or modal.  A modal
 * dialog is one which blocks input to all other toplevel windows
 * in the app context, except for any windows created with the dialog
 * as their owner.
 * <p>
 * Dialogs are capable of generating the following window events:
 * WindowOpened, WindowClosing, WindowClosed, WindowActivated,
 * WindowDeactivated.
 *
 * <p>
 * <a name="restrictions">
 * <h4>Restrictions</h4>
 * <em>
 * Implementations of Dialog in Personal Profile exhibit
 * certain restrictions, specifically:
 * <ul>
 * <li> An implementation may limit possible Dialog sizes.  In such
 * a case, attempts to change the size of a Dialog may result in
 * a different size than what was requested.
 * See:
 * <ul>
 * <li> {@link #setSize(int, int)}
 * <li> {@link #setSize(Dimension)}
 * <li> {@link #setBounds(int, int, int, int)}
 * <li> {@link #setBounds(Rectangle)}
 * </ul>
 * <li> An implementation may prohibit resizing of Dialogs by a user.  In
 * such a case, attempts to make any Dialog resizable will fail silently.
 * See:
 * <ul>
 * <li> {@link #setResizable(boolean)}
 * </ul>
 * <li> An implementation may limit Dialog locations.  In
 * such a case, attempts to change the location of a Dialog may
 * result in a different location than what was requested.
 * See:
 * <ul>
 * <li> {@link #setLocation(int, int)}
 * <li> {@link #setLocation(Point)}
 * <li> {@link #setBounds(int, int, int, int)}
 * <li> {@link #setBounds(Rectangle)}
 * </ul>
 * <li> An implementation need not support a visible title on Dialogs.
 * In such a case, the methods {@link #setTitle} and {@link #getTitle}
 * on all Dialogs behave as normal, but no title is visible to the user.
 * See:
 * <ul>
 * <li> {@link #Dialog(Frame, String)}
 * <li> {@link #Dialog(Frame, String, boolean)}
 * <li> {@link #setTitle}
 * </ul>
 * </ul>
 * </em>
 *
 * @see WindowEvent
 * @see Window#addWindowListener
 *
 * @version 	1.8, 07/03/02
 * @author 	Sami Shaio
 * @author 	Arthur van Hoff
 * @since       JDK1.0
 */
public class Dialog extends Window {
    /**
     * A dialog's resizable property. Will be true
     * if the Dialog is to be resizable, otherwise
     * it will be false.
     *
     * @serial
     * @see setResizable()
     */
    boolean	resizable = true;

    /**
     * This field indicates whether the dialog is undecorated.
     * This property can only be changed while the dialog is not displayable.
     * <code>undecorated</code> will be true if the dialog is
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
     * Will be true if the Dialog is modal,
     * otherwise the dialog will be modeless.
     * A modal Dialog grabs all the input to
     * the owner frame from the user.
     *
     * @serial
     * @see isModal()
     * @see setModal()
     */
    boolean modal;
    /**
     * Specifies the title of the Dialog.
     * This field can be null.
     *
     * @serial
     * @see getTitle()
     * @see setTitle()
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
     * Constructs an initially invisible, non-modal Dialog with
     * an empty title and the specified owner frame.
     * @param owner the owner of the dialog
     * @exception java.lang.IllegalArgumentException if <code>owner</code>
     *            is <code>null</code>
     * @see Component#setSize
     * @see Component#setVisible
     */
    public Dialog(Frame owner) {
        this(owner, "", false);
    }

    /**
     * Constructs an initially invisible Dialog with an empty title,
     * the specified owner frame and modality.
     * @param owner the owner of the dialog
     * @param modal if true, dialog blocks input to other app windows when shown
     * @exception java.lang.IllegalArgumentException if <code>owner</code>
     *            is <code>null</code>
     */
    public Dialog(Frame owner, boolean modal) {
        this(owner, "", modal);
    }

    /**
     * Constructs an initially invisible, non-modal Dialog with
     * the specified owner frame and title.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Profile.</em>
     *
     * @param owner the owner of the dialog
     * @param title the title of the dialog. A <code>null</code> value
     *        will be accepted without causing a NullPointerException
     *        to be thrown.
     * @exception java.lang.IllegalArgumentException if <code>owner</code>
     *            is <code>null</code>
     * @see Component#setSize
     * @see Component#setVisible
     */
    public Dialog(Frame owner, String title) {
        this(owner, title, false);
    }

    /**
     * Constructs an initially invisible Dialog with the
     * specified owner frame, title, and modality.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Profile.</em>
     *
     * @param owner the owner of the dialog
     * @param title the title of the dialog. A <code>null</code> value
     *        will be accepted without causing a NullPointerException
     *        to be thrown.
     * @param modal if true, dialog blocks input to other app windows when shown
     * @exception java.lang.IllegalArgumentException if <code>owner</code>
     *            is <code>null</code>
     * @see Component#setSize
     * @see Component#setVisible
     */
    public Dialog(Frame owner, String title, boolean modal) {
        super(owner);
        if (owner == null) {
            throw new IllegalArgumentException("null owner frame");
        }
        this.title = title;
        setModal(modal);
    }
    /**
     * Constructs an initially invisible Dialog with the
     * specified owner frame, title, modality, and
     * <code>GraphicsConfiguration</code>.
     * @param owner the owner of the dialog
     * @param title the title of the dialog. A <code>null</code> value
     *        will be accepted without causing a NullPointerException
     *        to be thrown.
     * @param modal if true, dialog blocks input to other app windows when shown     * @param gc the <code>GraphicsConfiguration</code>
     * of the target screen device.  If <code>gc</code> is
     * <code>null</code>, the same
     * <code>GraphicsConfiguration</code> as the owning Frame is used.
     * @exception java.lang.IllegalArgumentException if <code>owner</code>
     *            is <code>null</code>.  This exception is always thrown
     *            when GraphicsEnvironment.isHeadless() returns true
     * @see java.awt.GraphicsEnvironment#isHeadless
     * @see Component#setSize
     * @see Component#setVisible
     * @since 1.4
     */
    public Dialog(Frame owner, String title, boolean modal,
                  GraphicsConfiguration gc) {
        super(owner, gc);
       
        this.title = title;
        this.modal = modal;
        setFocusTraversalPolicy(KeyboardFocusManager.
                                getCurrentKeyboardFocusManager().
                                getDefaultFocusTraversalPolicy()
                                );
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

    public void addNotify() {
        synchronized (getTreeLock()) {
            // Bug Fix : 5012247
            // As per the spec, if the "owner" of the Dialog is not
            // displayable when Dialog.show() is called, then it should
            // be made displayable. 
            // (Note : displayable state means that native resources
            //         are assigned to the component)
            if (parent != null && parent.peer == null) { //(!displayable())
                parent.addNotify();
            }
            // Bug Fix : 5012247
            if (peer == null) {
                peer = ((PeerBasedToolkit) getToolkit()).createDialog(this);
            }
            super.addNotify();
        }
    }

    /**
     * Indicates whether the dialog is modal.
     * When a modal Dialog is made visible, user input will be
     * blocked to the other windows in the app context, except for
     * any windows created with this dialog as their owner.
     *
     * @return    <code>true</code> if this dialog window is modal;
     *            <code>false</code> otherwise.
     * @see       java.awt.Dialog#setModal
     */
    public boolean isModal() {
        return modal;
    }

    /**
     * Specifies whether this dialog should be modal.
     * @see       java.awt.Dialog#isModal
     * @since     JDK1.1
     */
    public void setModal(boolean modal) {
        this.modal = modal;
    }

    /**
     * Gets the title of the dialog. The title is displayed in the
     * dialog's border.
     * @return    the title of this dialog window. The title may be
     *            <code>null</code>.
     * @see       java.awt.Dialog#setTitle
     */
    public String getTitle() {
        return title;
    }

    /**
     * Sets the title of the Dialog.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Profile.</em>
     *
     * @param title the title displayed in the dialog's border
     * @see #getTitle
     */
    public synchronized void setTitle(String title) {
        this.title = title;
        DialogPeer peer = (DialogPeer) this.peer;
        if (peer != null) {
            peer.setTitle(title);
        }
    }
	
    /**
     * @return true if we actually showed, false if we just called toFront()
     */
    private boolean conditionalShow() {
        boolean retval;
        synchronized (getTreeLock()) {
            if (peer == null) {
                addNotify();
            }
            validate();
            if (visible) {
                toFront();
                retval = false;
            } else {
                visible = retval = true;
                peer.setVisible(true); // now guaranteed never to block
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

    // LMK app context support
    /**
    * Stores the app context on which event dispatch thread the dialog
    * is being shown. Initialized in show(), used in hideAndDisposeHandler()
    */
    transient private AppContext showAppContext;

    /**
     * Makes the Dialog visible. If the dialog and/or its owner
     * are not yet displayable, both are made displayable.  The
     * dialog will be validated prior to being made visible.
     * If the dialog is already visible, this will bring the dialog
     * to the front.
     * <p>
     * If the dialog is modal and is not already visible, this call will
     * not return until the dialog is hidden by calling <code>hide</code> or
     * <code>dispose</code>. It is permissible to show modal dialogs from
     * the event dispatching thread because the toolkit will ensure that
     * another event pump runs while the one which invoked this method
     * is blocked.
     * @see Component#hide
     * @see Component#isDisplayable
     * @see Component#validate
     * @see java.awt.Dialog#isModal
     */
    public void show() {
        beforeFirstShow = false;
        if (!isModal()) {
            conditionalShow();
        } else {
            // Set this variable before calling conditionalShow(). That
            // way, if the Dialog is hidden right after being shown, we
            // won't mistakenly block this thread.
            keepBlocking = true;

	    // LMK app context support
            // Store the app context on which this dialog is being shown.
            // Event dispatch thread of this app context will be sleeping until
            // we wake it by any event from hideAndDisposeHandler().
            showAppContext = AppContext.getAppContext();

            if (conditionalShow()) {
                // We have two mechanisms for blocking: 1. If we're on the
                // EventDispatchThread, start a new event pump. 2. If we're
                // on any other thread, call wait() on the treelock.

		// LMK added for proper focus mgmt behavior
                // keep the KeyEvents from being dispatched
                // until the focus has been transfered
                long time = Toolkit.getEventQueue().getMostRecentEventTime();
                Component predictedFocusOwner = getMostRecentFocusOwner();
                KeyboardFocusManager.getCurrentKeyboardFocusManager().
                     enqueueKeyEvents(time, predictedFocusOwner); 

                if (Toolkit.getEventQueue().isDispatchThread()) {
		    // LMK added to make sequenced events work properly
                    /*
                     * dispose SequencedEvent we are dispatching on current
                     * AppContext, to prevent us from hang.
                     *
                     * BugId 4531693 (son@sparc.spb.su)
                     */
                    SequencedEvent currentSequencedEvent = KeyboardFocusManager.                        getCurrentKeyboardFocusManager().getCurrentSequencedEvent();
                    if (currentSequencedEvent != null) {
                        System.out.println("disposing sequencedEvent");
                        currentSequencedEvent.dispose();
                    }

                    EventDispatchThread dispatchThread =
                        (EventDispatchThread) Thread.currentThread();
                    dispatchThread.pumpEvents(new Conditional() {
                            public boolean evaluate() {
                                return keepBlocking;
                            }
                        }
                    );
                } else {
                    synchronized (getTreeLock()) {
                        while (keepBlocking) {
                            try {
                                getTreeLock().wait();
                            } catch (InterruptedException e) {
                                break;
                            }
                        }
                    }
                }
		// LMK added for proper focus mgmt behavior
                KeyboardFocusManager.getCurrentKeyboardFocusManager().
                    dequeueKeyEvents(time, predictedFocusOwner);
            }
        }
    }

    // LMK app context support
    final static class WakingRunnable implements Runnable {
        public void run() {
        }
    }
	
    private void hideAndDisposeHandler() {
        if (keepBlocking) {
            synchronized (getTreeLock()) {
                keepBlocking = false;

		// LMK app context support
                if (showAppContext != null) {
                    // Wake up event dispatch thread on which the dialog was 
                    // initially shown
                    SunToolkit.postEvent(showAppContext, 
                                         new PeerEvent(this, 
                                                       new WakingRunnable(), 
                                                       PeerEvent.PRIORITY_EVENT));
                    showAppContext = null;
                }
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
     * Indicates whether this dialog is resizable by the user.
     * @return    <code>true</code> if the user can resize the dialog;
     *            <code>false</code> otherwise.
     * @see       java.awt.Dialog#setResizable
     */
    public boolean isResizable() {
        return resizable;
    }

    /**
     * Sets whether this dialog is resizable by the user.
     * <p>
     * <em>Note: This operation is subject to
     * <a href="#restrictions">restriction</a>
     * in Personal Profile.</em>
     *
     * @param     resizable <code>true</code> if the user can
     *                 resize this dialog; <code>false</code> otherwise.
     * @see       java.awt.Dialog#isResizable
     */
    public synchronized void setResizable(boolean resizable) {
        this.resizable = resizable;
        DialogPeer peer = (DialogPeer) this.peer;
        if (peer != null) {
            peer.setResizable(resizable);
        }
    }

    /**
     * Disables or enables decorations for this dialog.
     * This method can only be called while the dialog is not displayable.
     * @param  undecorated <code>true</code> if no dialog decorations are
     *         to be enabled;
     *         <code>false</code> if dialog decorations are to be enabled.
     * @throws <code>IllegalComponentStateException</code> if the dialog
     *         is displayable.
     * @see    #isUndecorated
     * @see    Component#isDisplayable
     * @since 1.4
     */
    public void setUndecorated(boolean undecorated) {
        /* Make sure we don't run in the middle of peer creation.*/
        synchronized (getTreeLock()) {
            if (isDisplayable()) {
                throw new IllegalComponentStateException("The dialog is displayable.");
            }
            this.undecorated = undecorated;
        }
    }

    /**
     * Indicates whether this dialog is undecorated.
     * By default, all dialogs are initially decorated.
     * @return    <code>true</code> if dialog is undecorated;
     *                        <code>false</code> otherwise.
     * @see       java.awt.Dialog#setUndecorated
     * @since 1.4
     */
    public boolean isUndecorated() {
        return undecorated;
    }

    /**
     * Returns the parameter string representing the state of this
     * dialog window. This string is useful for debugging.
     * @return    the parameter string of this dialog window.
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
