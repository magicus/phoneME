/*
 * @(#)Container.java	1.187 06/10/10
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

import java.io.PrintStream;
import java.io.PrintWriter;
import sun.awt.peer.ComponentPeer;
import sun.awt.peer.ContainerPeer;
import java.awt.event.*;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.IOException;
import sun.awt.ConstrainableGraphics;
import java.util.Set;
import java.util.EventListener;
import java.awt.AWTEventMulticaster;

/**
 * A generic Abstract Window Toolkit(AWT) container object is a component
 * that can contain other AWT components.
 * <p>
 * Components added to a container are tracked in a list.  The order
 * of the list will define the components' front-to-back stacking order
 * within the container.  If no index is specified when adding a
 * component to a container, it will be added to the end of the list
 * (and hence to the bottom of the stacking order).
 * @version 	1.173, 08/19/02
 * @author 	Arthur van Hoff
 * @author 	Sami Shaio
 * @see       java.awt.Container#add(java.awt.Component, int)
 * @see       java.awt.Container#getComponent(int)
 * @see       java.awt.LayoutManager
 * @since     JDK1.0
 */
public class Container extends Component {
    /**
     * The number of components in this container.
     */
    int ncomponents;
    /**
     * The components in this container.
     */
    Component component[] = new Component[4];
    /**
     * Layout manager for this container.
     */
    LayoutManager layoutMgr;
    /**
     * Event router for lightweight components.  If this container
     * is native, this dispatcher takes care of forwarding and
     * retargeting the events to lightweight components contained
     * (if any).
     */
    private transient LightweightDispatcher dispatcher;
    /** Internal, cached size information */
    private Dimension maxSize;
    transient ContainerListener containerListener;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 4613797578919906343L;
    /**
     * Constructs a new Container. Containers can be extended directly,
     * but are lightweight in this case and must be contained by a parent
     * somewhere higher up in the component tree that is native.
     * (such as Frame for example).
     */
    public Container() {}

    /**
     * Gets the number of components in this panel.
     * @return    the number of components in this panel.
     * @see       java.awt.Container#getComponent
     * @since     JDK1.1
     */
    public int getComponentCount() {
        return countComponents();
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by getComponentCount().
     */
    public int countComponents() {
        return ncomponents;
    }

    /**
     * Gets the nth component in this container.
     * @param      n   the index of the component to get.
     * @return     the n<sup>th</sup> component in this container.
     * @exception  ArrayIndexOutOfBoundsException
     *                 if the n<sup>th</sup> value does not exist.
     * @since      JDK1.0
     */
    public Component getComponent(int n) {
        synchronized (getTreeLock()) {
            if ((n < 0) || (n >= ncomponents)) {
                throw new ArrayIndexOutOfBoundsException("No such child: " + n);
            }
            return component[n];
        }
    }

    /**
     * Gets all the components in this container.
     * @return    an array of all the components in this container.
     * @since     JDK1.0
     */
    public Component[] getComponents() {
        synchronized (getTreeLock()) {
            Component list[] = new Component[ncomponents];
            System.arraycopy(component, 0, list, 0, ncomponents);
            return list;
        }
    }

    /**
     * Determines the insets of this container, which indicate the size
     * of the container's border.
     * <p>
     * A <code>Frame</code> object, for example, has a top inset that
     * corresponds to the height of the frame's title bar.
     * @return    the insets of this container.
     * @see       java.awt.Insets
     * @see       java.awt.LayoutManager
     * @since     JDK1.1
     */
    public Insets getInsets() {
        return insets();
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>getInsets()</code>.
     */
    public Insets insets() {
        if (this.peer != null && this.peer instanceof ContainerPeer) {
            ContainerPeer peer = (ContainerPeer) this.peer;
            return (Insets) peer.getInsets().clone();
        }
        return new Insets(0, 0, 0, 0);
    }

    /**
     * Adds the specified component to the end of this container.
     * @param     comp   the component to be added.
     * @return    the component argument.
     * @since     JDK1.0
     */
    public Component add(Component comp) {
        addImpl(comp, null, -1);
        return comp;
    }

    /**
     * Adds the specified component to this container.
     * It is strongly advised to use the 1.1 method, add(Component, Object),
     * in place of this method.
     */
    public Component add(String name, Component comp) {
        addImpl(comp, name, -1);
        return comp;
    }

    /**
     * Adds the specified component to this container at the given
     * position.
     * @param     comp   the component to be added.
     * @param     index    the position at which to insert the component,
     *                   or <code>-1</code> to insert the component at the end.
     * @return    the component <code>comp</code>
     * @see	  #remove
     * @since     JDK1.0
     */
    public Component add(Component comp, int index) {
        addImpl(comp, null, index);
        return comp;
    }

    /**
     * Adds the specified component to the end of this container.
     * Also notifies the layout manager to add the component to
     * this container's layout using the specified constraints object.
     * @param     comp the component to be added
     * @param     constraints an object expressing
     *                  layout contraints for this component
     * @see       java.awt.LayoutManager
     * @since     JDK1.1
     */
    public void add(Component comp, Object constraints) {
        addImpl(comp, constraints, -1);
    }

    /**
     * Adds the specified component to this container with the specified
     * constraints at the specified index.  Also notifies the layout
     * manager to add the component to the this container's layout using
     * the specified constraints object.
     * @param comp the component to be added
     * @param constraints an object expressing layout contraints for this
     * @param index the position in the container's list at which to insert
     * the component. -1 means insert at the end.
     * component
     * @see #remove
     * @see LayoutManager
     */
    public void add(Component comp, Object constraints, int index) {
        addImpl(comp, constraints, index);
    }

    /**
     * Adds the specified component to this container at the specified
     * index. This method also notifies the layout manager to add
     * the component to this container's layout using the specified
     * constraints object.
     * <p>
     * This is the method to override if a program needs to track
     * every add request to a container. An overriding method should
     * usually include a call to the superclass's version of the method:
     * <p>
     * <blockquote>
     * <code>super.addImpl(comp, constraints, index)</code>
     * </blockquote>
     * <p>
     * @param     comp       the component to be added.
     * @param     constraints an object expressing layout contraints
     *                 for this component.
     * @param     index the position in the container's list at which to
     *                 insert the component, where <code>-1</code>
     *                 means insert at the end.
     * @see       java.awt.Container#add(java.awt.Component)
     * @see       java.awt.Container#add(java.awt.Component, int)
     * @see       java.awt.Container#add(java.awt.Component, java.lang.Object)
     * @see       java.awt.LayoutManager
     * @since     JDK1.1
     */
    protected void addImpl(Component comp, Object constraints, int index) {
        synchronized (getTreeLock()) {
            /* Check for correct arguments:  index in bounds,
             * comp cannot be one of this container's parents,
             * and comp cannot be a window.
             */
            if (index > ncomponents || (index < 0 && index != -1)) {
                throw new IllegalArgumentException(
                        "illegal component position");
            }
            if (comp instanceof Container) {
                for (Container cn = this; cn != null; cn = cn.parent) {
                    if (cn == comp) {
                        throw new IllegalArgumentException(
                                "adding container's parent to itself");
                    }
                }
            }
            if (comp instanceof Window) {
                throw new IllegalArgumentException(
                        "adding a window to a container");
            }
            /* Reparent the component and tidy up the tree's state. */
            if (comp.parent != null) {
                comp.parent.remove(comp);
                // 6262334
                if (index > ncomponents) {
                    throw new 
                    IllegalArgumentException("illegal component position");
                }
                // 6262334
            }
            /* Add component to list; allocate new array if necessary. */
            if (ncomponents == component.length) {
                Component newcomponents[] = new Component[ncomponents * 2];
                System.arraycopy(component, 0, newcomponents, 0, ncomponents);
                component = newcomponents;
            }
            if (index == -1 || index == ncomponents) {
                component[ncomponents++] = comp;
            } else {
                System.arraycopy(component, index, component,
                    index + 1, ncomponents - index);
                component[index] = comp;
                ncomponents++;
            }
            comp.parent = this;
            if (valid) {
                invalidate();
            }
            if (peer != null) {
                comp.addNotify();
            }
            /* Notify the layout manager of the added component. */
            if (layoutMgr != null) {
                if (layoutMgr instanceof LayoutManager2) {
                    ((LayoutManager2) layoutMgr).addLayoutComponent(comp, constraints);
                } else if (constraints instanceof String) {
                    layoutMgr.addLayoutComponent((String) constraints, comp);
                }
            }
            if (containerListener != null ||
                (eventMask & AWTEvent.CONTAINER_EVENT_MASK) != 0) {
                ContainerEvent e = new ContainerEvent(this,
                        ContainerEvent.COMPONENT_ADDED,
                        comp);
                processEvent(e);
            }
        }
    }

    /**
     * Removes the component, specified by <code>index</code>,
     * from this container.
     * @param     index   the index of the component to be removed.
     * @see #add
     * @since JDK1.1
     */
    public void remove(int index) {
        synchronized (getTreeLock()) {
            /* 4629242 - Check if Container contains any components or if
             * index references a null array element.
             */
            if (ncomponents == 0 || component[index] == null) {
                throw new ArrayIndexOutOfBoundsException("Invalid index : " + index + ".");
            }
            Component comp = component[index];
            if (peer != null) {
                comp.removeNotify();
            }
            if (layoutMgr != null) {
                layoutMgr.removeLayoutComponent(comp);
            }
            comp.parent = null;
            System.arraycopy(component, index + 1,
                component, index,
                ncomponents - index - 1);
            component[--ncomponents] = null;
            if (valid) {
                invalidate();
            }
            if (containerListener != null ||
                (eventMask & AWTEvent.CONTAINER_EVENT_MASK) != 0) {
                ContainerEvent e = new ContainerEvent(this,
                        ContainerEvent.COMPONENT_REMOVED,
                        comp);
                processEvent(e);
            }
            return;
        }
    }

    /**
     * Removes the specified component from this container.
     * @param comp the component to be removed
     * @see #add
     * @since JDK1.0
     */
    public void remove(Component comp) {
        synchronized (getTreeLock()) {
            if (comp.parent == this) {
                /* Search backwards, expect that more recent additions
                 * are more likely to be removed.
                 */
                Component component[] = this.component;
                for (int i = ncomponents; --i >= 0;) {
                    if (component[i] == comp) {
                        remove(i);
                    }
                }
            }
        }
    }

    /**
     * Removes all the components from this container.
     * @see #add
     * @see #remove
     * @since JDK1.0
     */
    public void removeAll() {
        synchronized (getTreeLock()) {
            while (ncomponents > 0) {
                Component comp = component[--ncomponents];
                component[ncomponents] = null;
                if (peer != null) {
                    comp.removeNotify();
                }
                if (layoutMgr != null) {
                    layoutMgr.removeLayoutComponent(comp);
                }
                comp.parent = null;
                if (containerListener != null ||
                    (eventMask & AWTEvent.CONTAINER_EVENT_MASK) != 0) {
                    ContainerEvent e = new ContainerEvent(this,
                            ContainerEvent.COMPONENT_REMOVED,
                            comp);
                    processEvent(e);
                }
            }
            if (valid) {
                invalidate();
            }
        }
    }

    /**
     * Gets the layout manager for this container.
     * @see #doLayout
     * @see #setLayout
     * @since JDK1.0
     */
    public LayoutManager getLayout() {
        return layoutMgr;
    }

    /**
     * Sets the layout manager for this container.
     * @param mgr the specified layout manager
     * @see #doLayout
     * @see #getLayout
     * @since JDK1.0
     */
    public void setLayout(LayoutManager mgr) {
        layoutMgr = mgr;
        if (valid) {
            invalidate();
        }
    }

    /**
     * Causes this container to lay out its components.  Most programs
     * should not call this method directly, but should invoke
     * the <code>validate</code> method instead.
     * @see java.awt.LayoutManager#layoutContainer
     * @see #setLayout
     * @see #validate
     * @since JDK1.1
     */
    public void doLayout() {
        layout();
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>doLayout()</code>.
     */
    public void layout() {
        LayoutManager layoutMgr = this.layoutMgr;
        if (layoutMgr != null) {
            layoutMgr.layoutContainer(this);
        }
    }

    /**
     * Invalidates the container.  The container and all parents
     * above it are marked as needing to be laid out.  This method can
     * be called often, so it needs to execute quickly.
     * @see #validate
     * @see #layout
     * @see LayoutManager
     */
    public void invalidate() {
        if (layoutMgr instanceof LayoutManager2) {
            LayoutManager2 lm = (LayoutManager2) layoutMgr;
            lm.invalidateLayout(this);
        }
        super.invalidate();
    }

    /**
     * Validates this container and all of its subcomponents.
     * <p>
     * AWT uses <code>validate</code> to cause a container to lay out
     * its subcomponents again after the components it contains
     * have been added to or modified.
     * @see #validate
     * @see Component#invalidate
     * @since JDK1.0
     */
    public void validate() {
        /* Avoid grabbing lock unless really necessary. */
        if (!valid) {
            synchronized (getTreeLock()) {
                if (!valid && peer != null) {
                    Cursor oldCursor = getCursor();
                    ContainerPeer p = null;
                    if (peer instanceof ContainerPeer) {
                        p = (ContainerPeer) peer;
                    }
                    if (p != null) {
                        p.beginValidate();
                    }
                    validateTree();
                    valid = true;
                    if (p != null) {
                        p.endValidate();
                    }
                }
            }
        }
    }

    /**
     * Recursively descends the container tree and recomputes the
     * layout for any subtrees marked as needing it (those marked as
     * invalid).  Synchronization should be provided by the method
     * that calls this one:  <code>validate</code>.
     */
    protected void validateTree() {
        if (!valid) {
            doLayout();
            Component component[] = this.component;
            for (int i = 0; i < ncomponents; ++i) {
                Component comp = component[i];
                if ((comp instanceof Container)
                    && !(comp instanceof Window)
                    && !comp.valid) {
                    ((Container) comp).validateTree();
                } else {
                    comp.validate();
                }
            }
        }
        valid = true;
    }

    /**
     * Returns the preferred size of this container.
     * @return    an instance of <code>Dimension</code> that represents
     *                the preferred size of this container.
     * @see       java.awt.Container#getMinimumSize
     * @see       java.awt.Container#getLayout
     * @see       java.awt.LayoutManager#preferredLayoutSize(java.awt.Container)
     * @see       java.awt.Component#getPreferredSize
     * @since     JDK1.0
     */
    public Dimension getPreferredSize() {
        return preferredSize();
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>getPreferredSize()</code>.
     */
    public Dimension preferredSize() {
        /* Avoid grabbing the lock if a reasonable cached size value
         * is available.
         */
        Dimension dim = prefSize;
        if (dim != null && isValid()) {
            return dim;
        }
        synchronized (getTreeLock()) {
            prefSize = (layoutMgr != null) ?
                    layoutMgr.preferredLayoutSize(this) :
                    super.preferredSize();
            return prefSize;
        }
    }

    /**
     * Returns the minimum size of this container.
     * @return    an instance of <code>Dimension</code> that represents
     *                the minimum size of this container.
     * @see       java.awt.Container#getPreferredSize
     * @see       java.awt.Container#getLayout
     * @see       java.awt.LayoutManager#minimumLayoutSize(java.awt.Container)
     * @see       java.awt.Component#getMinimumSize
     * @since     JDK1.1
     */
    public Dimension getMinimumSize() {
        return minimumSize();
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>getMinimumSize()</code>.
     */
    public Dimension minimumSize() {
        /* Avoid grabbing the lock if a reasonable cached size value
         * is available.
         */
        Dimension dim = minSize;
        if (dim != null && isValid()) {
            return dim;
        }
        synchronized (getTreeLock()) {
            minSize = (layoutMgr != null) ?
                    layoutMgr.minimumLayoutSize(this) :
                    super.minimumSize();
            return minSize;
        }
    }

    /**
     * Returns the maximum size of this container.
     * @see #getPreferredSize
     */
    public Dimension getMaximumSize() {
        /* Avoid grabbing the lock if a reasonable cached size value
         * is available.
         */
        Dimension dim = maxSize;
        if (dim != null && isValid()) {
            return dim;
        }
        if (layoutMgr instanceof LayoutManager2) {
            synchronized (getTreeLock()) {
                LayoutManager2 lm = (LayoutManager2) layoutMgr;
                maxSize = lm.maximumLayoutSize(this);
            }
        } else {
            maxSize = super.getMaximumSize();
        }
        return maxSize;
    }

    /**
     * Returns the alignment along the x axis.  This specifies how
     * the component would like to be aligned relative to other
     * components.  The value should be a number between 0 and 1
     * where 0 represents alignment along the origin, 1 is aligned
     * the furthest away from the origin, 0.5 is centered, etc.
     */
    public float getAlignmentX() {
        float xAlign;
        if (layoutMgr instanceof LayoutManager2) {
            synchronized (getTreeLock()) {
                LayoutManager2 lm = (LayoutManager2) layoutMgr;
                xAlign = lm.getLayoutAlignmentX(this);
            }
        } else {
            xAlign = super.getAlignmentX();
        }
        return xAlign;
    }

    /**
     * Returns the alignment along the y axis.  This specifies how
     * the component would like to be aligned relative to other
     * components.  The value should be a number between 0 and 1
     * where 0 represents alignment along the origin, 1 is aligned
     * the furthest away from the origin, 0.5 is centered, etc.
     */
    public float getAlignmentY() {
        float yAlign;
        if (layoutMgr instanceof LayoutManager2) {
            synchronized (getTreeLock()) {
                LayoutManager2 lm = (LayoutManager2) layoutMgr;
                yAlign = lm.getLayoutAlignmentY(this);
            }
        } else {
            yAlign = super.getAlignmentY();
        }
        return yAlign;
    }

    /**
     * Paints the container.  This forwards the paint to any lightweight components
     * that are children of this container.  If this method is reimplemented,
     * super.paint(g) should be called so that lightweight components are properly
     * rendered.  If a child component is entirely clipped by the current clipping
     * setting in g, paint() will not be forwarded to that child.
     *
     * @param g the specified Graphics window
     * @see   java.awt.Component#update(java.awt.Graphics)
     */
    public void paint(Graphics g) {
        // Fixed 6643917: calling common method for paint and paintComponents
        paintContents(g, true);
    }

    /**
     * Updates the container.  This forwards the update to any lightweight components
     * that are children of this container.  If this method is reimplemented,
     * super.update(g) should be called so that lightweight components are properly
     * rendered.  If a child component is entirely clipped by the current clipping
     * setting in g, update() will not be forwarded to that child.
     *
     * @param g the specified Graphics window
     * @see   java.awt.Component#update(java.awt.Graphics)
     */
    public void update(Graphics g) {
        if (isShowing()) {
            // Fixed 6178052: double clearing of the background program.
            //
            // The legacy code for Container.update() checked whether the update
            // is for a non-lightweight and if so would try to clear the rect
            // before calling super.update().  This resulted in a double clear
            // because the Component.update() legacy code also cleared the rect
            // for instances of Panel, Frame, Dialog, and Window, besides Canvas.
            //
            // This double clearing is undesirable.
            //
            // The fix is to call paint(g) instead of super.update(g).  See also
            // the fix in Component.update().
            if (! (peer instanceof sun.awt.peer.LightweightPeer)) {
                peer.clearBackground(g);
            }
            paint(g);
        }
    }

    /**
     * Prints the container.  This forwards the print to any lightweight components
     * that are children of this container.  If this method is reimplemented,
     * super.print(g) should be called so that lightweight components are properly
     * rendered.  If a child component is entirely clipped by the current clipping
     * setting in g, print() will not be forwarded to that child.
     *
     * @param g the specified Graphics window
     * @see   java.awt.Component#update(java.awt.Graphics)
     */
    public void print(Graphics g) {
        super.print(g);  // By default, Component.print() calls paint()
        int ncomponents = this.ncomponents;
        Component component[] = this.component;
        Rectangle clip = g.getClipRect();
        for (int i = ncomponents - 1; i >= 0; i--) {
            Component comp = component[i];
            if (comp != null &&
                comp.peer instanceof sun.awt.peer.LightweightPeer) {
                Rectangle cr = comp.getBounds();
                if ((clip == null) || cr.intersects(clip)) {
                    Graphics cg = g.create(cr.x, cr.y, cr.width, cr.height);
                    cg.setFont(comp.getFont());
                    try {
                        comp.print(cg);
                    } finally {
                        cg.dispose();
                    }
                }
            }
        }
    }

    /**
     * Paints each of the components in this container.
     * @param     g   the graphics context.
     * @see       java.awt.Component#paint
     * @see       java.awt.Component#paintAll
     * @since     JDK1.0
     */
    public void paintComponents(Graphics g) {
        // Fixed 6643917: calling common method for paint and paintComponents
        paintContents(g, false);
    }

    /**
     * 6643917 
     * Paints only lightweights or all components, depending on the
     * onlyLightweights boolean argument.
     */
    private void paintContents(Graphics g, boolean onlyLightweights) {
        if (isShowing()) {
            int ncomponents = this.ncomponents;
            Component component[] = this.component;
            Rectangle clip = g.getClipRect();
            for (int i = ncomponents - 1; i >= 0; i--) {
                Component comp = component[i];
                if (comp != null &&
                    (!onlyLightweights || comp.peer instanceof sun.awt.peer.LightweightPeer) &&
                    comp.visible == true) {
                    Rectangle cr = comp.getBounds();
                    if ((clip == null) || cr.intersects(clip)) {
                        Graphics cg = g.create();
                        if (cg instanceof ConstrainableGraphics) {
                            ((ConstrainableGraphics) cg).constrain(cr.x, cr.y, cr.width, cr.height);
                        } else {
                            cg.translate(cr.x, cr.y);
                            // Fixed 6178102.
                            // Moved the clipRect() call here, same as in Component.getGraphics().
                            cg.clipRect(0, 0, cr.width, cr.height);
                        }
                        // Fixed 6178102.
                        // This call does not belong here.
                        //cg.clipRect(0, 0, cr.width, cr.height);
                        cg.setFont(comp.getFont());
                        try {
                            if (onlyLightweights)
                                comp.paint(cg);
                            else
                                comp.paintAll(cg);
                        } finally {
                            cg.dispose();
                        }
                    }
                }
            }
        }
    }

    /**
     * Prints each of the components in this container.
     * @param     g   the graphics context.
     * @see       java.awt.Component#print
     * @see       java.awt.Component#printAll
     * @since     JDK1.0
     */
    public void printComponents(Graphics g) {
        int ncomponents = this.ncomponents;
        Component component[] = this.component;
        // Lightweight components always paint behind peered components,
        // even if they are at the top of the Z order. We emulate this
        // behavior by making two printing passes: the first for lightweights;
        // the second for heavyweights.

        for (int i = ncomponents - 1; i >= 0; i--) {
            Component comp = component[i];
            if (comp != null &&
                comp.peer instanceof sun.awt.peer.LightweightPeer) {
                printOneComponent(g, comp);
            }
        }
        for (int i = ncomponents - 1; i >= 0; i--) {
            Component comp = component[i];
            if (comp != null &&
                !(comp.peer instanceof sun.awt.peer.LightweightPeer)) {
                printOneComponent(g, comp);
            }
        }
    }

    private void printOneComponent(Graphics g, Component comp) {
        Graphics cg = g.create(comp.x, comp.y, comp.width,
                comp.height);
        cg.setFont(comp.getFont());
        try {
            comp.printAll(cg);
        } finally {
            cg.dispose();
        }
    }

    /**
     * Simulates the peer callbacks into java.awt for printing of
     * lightweight Containers.
     * @param     g   the graphics context to use for printing.
     * @see       Component#printAll
     * @see       #printComponents
     */
    void lightweightPrint(Graphics g) {
        super.lightweightPrint(g);
        printComponents(g);
    }

    /**
     * Adds the specified container listener to receive container events
     * from this container.
     * @param l the container listener
     */
    public synchronized void addContainerListener(ContainerListener l) {
        containerListener = AWTEventMulticaster.add(containerListener, l);
        newEventsOnly = true;
    }

    /**
     * Removes the specified container listener so it no longer receives
     * container events from this container.
     * @param l the container listener
     */
    public synchronized void removeContainerListener(ContainerListener l) {
        containerListener = AWTEventMulticaster.remove(containerListener, l);
    }

    /**
     * Returns an array of all the container listeners
     * registered on this container.
     *
     * @return all of this container's <code>ContainerListener</code>s
     *         or an empty array if no container
     *         listeners are currently registered
     *
     * @see #addContainerListener
     * @see #removeContainerListener
     * @since 1.4
     */
    public synchronized ContainerListener[] getContainerListeners() {
        return (ContainerListener[])AWTEventMulticaster.getListeners(
                                    (EventListener)containerListener,
                                    ContainerListener.class); 
    }
    

    // NOTE: remove when filtering is done at lower level
    boolean eventEnabled(AWTEvent e) {
        int id = e.getID();
        if (id == ContainerEvent.COMPONENT_ADDED ||
            id == ContainerEvent.COMPONENT_REMOVED) {
            if ((eventMask & AWTEvent.CONTAINER_EVENT_MASK) != 0 ||
                containerListener != null) {
                return true;
            }
            return false;
        }
        return super.eventEnabled(e);
    }

    /**
     * Processes events on this container. If the event is a ContainerEvent,
     * it invokes the processContainerEvent method, else it invokes its
     * superclass's processEvent.
     * @param e the event
     */
    protected void processEvent(AWTEvent e) {
        if (e instanceof ContainerEvent) {
            processContainerEvent((ContainerEvent) e);
            return;
        }
        super.processEvent(e);
    }

    /**
     * Processes container events occurring on this container by
     * dispatching them to any registered ContainerListener objects.
     * NOTE: This method will not be called unless container events
     * are enabled for this component; this happens when one of the
     * following occurs:
     * a) A ContainerListener object is registered via addContainerListener()
     * b) Container events are enabled via enableEvents()
     * @see Component#enableEvents
     * @param e the container event
     */
    protected void processContainerEvent(ContainerEvent e) {
        if (containerListener != null) {
            switch (e.getID()) {
            case ContainerEvent.COMPONENT_ADDED:
                containerListener.componentAdded(e);
                break;

            case ContainerEvent.COMPONENT_REMOVED:
                containerListener.componentRemoved(e);
                break;
            }
        }
    }

    /*
     * Dispatches an event to this component or one of its sub components.
     * @param e the event
     */
    void dispatchEventImpl(AWTEvent e) {
        if ((dispatcher != null) && dispatcher.dispatchEvent(e)) {
            // event was sent to a lightweight component.  The
            // native-produced event sent to the native container
            // must be properly disposed of by the peer, so it
            // gets forwarded.  If the native host has been removed
            // as a result of the sending the lightweight event,
            // the peer reference will be null.
            e.consume();
            if (peer != null) {
                peer.handleEvent(e);
            }
            return;
        }
        super.dispatchEventImpl(e);
    }

    /*
     * Dispatches an event to this component, without trying to forward
     * it to any sub components
     * @param e the event
     */
    void dispatchEventToSelf(AWTEvent e) {
        super.dispatchEventImpl(e);
    }

    /**
     * Fetchs the top-most (deepest) lightweight component that is interested
     * in receiving mouse events.
     */
    Component getMouseEventTarget(int x, int y, boolean includeSelf) {
        int ncomponents = this.ncomponents;
        Component component[] = this.component;
        for (int i = 0; i < ncomponents; i++) {
            Component comp = component[i];
            if ((comp != null) && (comp.contains(x - comp.x, y - comp.y)) &&
                (comp.peer instanceof sun.awt.peer.LightweightPeer) &&
                (comp.visible == true)) {
                // found a component that intersects the point, see if there is
                // a deeper possibility.
                if (comp instanceof Container) {
                    Container child = (Container) comp;
                    Component deeper = child.getMouseEventTarget(x - child.x, y - child.y, includeSelf);
                    if (deeper != null) {
                        return deeper;
                    }
                } else {
                    if ((comp.mouseListener != null) ||
                        ((comp.eventMask & AWTEvent.MOUSE_EVENT_MASK) != 0) ||
                        (comp.mouseMotionListener != null) ||
                        ((comp.eventMask & AWTEvent.MOUSE_MOTION_EVENT_MASK) != 0)) {
                        // there isn't a deeper target, but this component is a target
                        return comp;
                    }
                }
            }
        }
        boolean isPeerOK;
        boolean	isMouseOverMe;
        boolean	isMouseListener;
        boolean	isMotionListener;
        isPeerOK = (peer instanceof sun.awt.peer.LightweightPeer) || includeSelf;
        isMouseOverMe = contains(x, y);
        isMouseListener = (mouseListener != null) ||
                ((eventMask & AWTEvent.MOUSE_EVENT_MASK) != 0);
        isMotionListener = (mouseMotionListener != null) ||
                ((eventMask & AWTEvent.MOUSE_MOTION_EVENT_MASK) != 0);
        // didn't find a child target, return this component if it's a possible target
        if (isMouseOverMe && isPeerOK && (isMouseListener || isMotionListener)) {
            return this;
        }
        // no possible target
        return null;
    }

    /**
     * Locates the visible child component that contains the specified
     * point.  The top-most child component is returned in the case
     * where there is overlap in the components.  If the containing child
     * component is a Container, this method will continue searching for
     * the deepest nested child component.  Components which are not
     * visible are ignored during the search.<p>
     *
     * The findComponentAt method is different from getComponentAt in
     * that getComponentAt only searches the Container's immediate
     * children; if the containing component is a Container,
     * findComponentAt will search that child to find a nested component.
     *
     * @param      p   the point.
     * @return null if the component does not contain the position.
     * If there is no child component at the requested point and the
     * point is within the bounds of the container the container itself
     * is returned.
     * @see Component#contains
     * @see getComponentAt
     * @since 1.2
     */
    public Component findComponentAt(Point p) {
        return findComponentAt(p.x, p.y);
    }
    
    /**
     * Locates the visible child component that contains the specified
     * position.  The top-most child component is returned in the case
     * where there is overlap in the components.  If the containing child
     * component is a Container, this method will continue searching for
     * the deepest nested child component.  Components which are not
     * visible are ignored during the search.<p>
     *
     * The findComponentAt method is different from getComponentAt in
     * that getComponentAt only searches the Container's immediate
     * children; if the containing component is a Container,
     * findComponentAt will search that child to find a nested component.
     *
     * @param x the <i>x</i> coordinate
     * @param y the <i>y</i> coordinate
     * @return null if the component does not contain the position.
     * If there is no child component at the requested point and the
     * point is within the bounds of the container the container itself
     * is returned.
     * @see Component#contains
     * @see getComponentAt
     * @since 1.2
     */
    public Component findComponentAt(int x, int y) {
        synchronized (getTreeLock()) {
            return findComponentAt(x, y, true);
        }
    }

    /**
     * Private version of findComponentAt which has two controllable
     * behaviors. Setting 'ignoreEnabled' to 'false' bypasses disabled
     * Components during the serach. This behavior is used by the
     * lightweight cursor support in sun.awt.GlobalCursorManager.
     * Setting 'ignoreGlassPane' to 'true' bypasses the glass panes owned
     * by any JRootPanes found during the search. This behavior is
     * used by the DnD event targeting code. The cursor and DnD code
     * both call this function directly via native code.
     *
     * The addition of both of these features is temporary, pending the
     * adoption of new, public APIs which export these features (probably
     * in merlin).
     */
    final Component findComponentAt(int x, int y, boolean ignoreEnabled) {
        if (!(contains(x, y) && visible && (ignoreEnabled || enabled))) {
            return null;
        }
        int ncomponents = this.ncomponents;
        Component component[] = this.component;
        // Two passes: see comment in sun.awt.SunGraphicsCallback
        for (int i = 0; i < ncomponents; i++) {
            Component comp = component[i];
            if (comp != null &&
                !(comp.peer instanceof sun.awt.peer.LightweightPeer)) {
                if (comp instanceof Container) {
                    comp = ((Container) comp).findComponentAt(x - comp.x,
                                y - comp.y,
                                ignoreEnabled);
                } else {
                    comp = comp.getComponentAt(x - comp.x, y - comp.y);
                }
                if (comp != null && comp.visible &&
                    (ignoreEnabled || comp.enabled)) {
                    return comp;
                }
            }
        }
        for (int i = 0; i < ncomponents; i++) {
            Component comp = component[i];
            if (comp != null &&
                comp.peer instanceof sun.awt.peer.LightweightPeer) {
                if (comp instanceof Container) {
                    comp = ((Container) comp).findComponentAt(x - comp.x,
                                y - comp.y,
                                ignoreEnabled);
                } else {
                    comp = comp.getComponentAt(x - comp.x, y - comp.y);
                }
                if (comp != null && comp.visible &&
                    (ignoreEnabled || comp.enabled)) {
                    return comp;
                }
            }
        }
        return this;
    }

    /**
     * Set the cursor image to a predefined cursor.
     * @param <code>cursor</code> One of the constants defined
     *            by the <code>Cursor</code> class.
     * @see       java.awt.Component#getCursor
     * @see       java.awt.Cursor
     * @since     JDK1.1
     */
    public synchronized void setCursor(Cursor cursor) {
        if (dispatcher != null) {
            Component cursorOn = dispatcher.getCursorOn();
            // if mouse is on a lightweight component, we should not
            // call setCursor on the nativeContainer
            if (cursorOn != null) {
                this.cursor = cursor;
                // it could be that cursorOn will inherit the new cursor
                dispatcher.updateCursor(cursorOn);
                return;
            }
        }
        super.setCursor(cursor);
    }

    /**
     * Update the cursor is decided by the dispatcher
     * This method is used when setCursor on a lightweight
     */
    void updateCursor(Component comp) {
        if (dispatcher != null) {
            dispatcher.updateCursor(comp);
        }
    }

    /**
     * Fetchs the top-most (deepest) lightweight component whose cursor
     * should be displayed.
     */
    Component getCursorTarget(int x, int y) {
        int ncomponents = this.ncomponents;
        Component component[] = this.component;
        for (int i = 0; i < ncomponents; i++) {
            Component comp = component[i];
            if ((comp != null) && 
                // 6201639
                // Only examine lightweight components
                // (See related fix in LightweightDispatcher.dispatchEvent())
                (comp.peer instanceof sun.awt.peer.LightweightPeer) &&
                // 6201639
                (comp.contains(x - comp.x, y - comp.y)) &&
                (comp.visible == true)) {
                // found a component that intersects the point, see if there is
                // a deeper possibility.
                if (comp instanceof Container) {
                    Container child = (Container) comp;
                    Component deeper = child.getCursorTarget(x - child.x, y - child.y);
                    if (deeper != null) {
                        return deeper;
                    }
                } else {
                    return comp;
                }
            }
        }
        if (contains(x, y) && (peer instanceof sun.awt.peer.LightweightPeer)) {
            return this;
        }
        // no possible target
        return null;
    }

    /**
     * This is called by lightweight components that want the containing
     * windowed parent to enable some kind of events on their behalf.
     * This is needed for events that are normally only dispatched to
     * windows to be accepted so that they can be forwarded downward to
     * the lightweight component that has enabled them.
     */
    void proxyEnableEvents(long events) {
        if (peer instanceof sun.awt.peer.LightweightPeer) {
            // this container is lightweight.... continue sending it
            // upward.
            parent.proxyEnableEvents(events);
        } else {
            // This is a native container, so it needs to host
            // one of it's children.  If this function is called before
            // a peer has been created we don't yet have a dispatcher
            // because it has not yet been determined if this instance
            // is lightweight.
            if (dispatcher != null) {
                dispatcher.enableEvents(events);
            }
        }
    }

    Window getWindow() {
        Container w = this;
        while (!(w instanceof Window)) {
            w = w.getParent();
        }
        return (Window) w;
    }

    /*
     * Determines if the top-level window containing
     * this component is activated
     */
    private boolean isContainingWindowActivated() {
        Component comp = this;
        while (!(comp instanceof Window)) {
            comp = comp.getParent();
        }
        return ((Window) comp).isActive();
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>dispatchEvent(AWTEvent e)</code>
     */
    public void deliverEvent(Event e) {
        Component comp = getComponentAt(e.x, e.y);
        if ((comp != null) && (comp != this)) {
            e.translate(-comp.x, -comp.y);
            comp.deliverEvent(e);
        } else {
            postEvent(e);
        }
    }

    /**
     * Locates the component that contains the x,y position.  The
     * top-most child component is returned in the case where there
     * is overlap in the components.  This is determined by finding
     * the component closest to the index 0 that claims to contain
     * the given point via Component.contains().
     * @param x the <i>x</i> coordinate
     * @param y the <i>y</i> coordinate
     * @return null if the component does not contain the position.
     * If there is no child component at the requested point and the
     * point is within the bounds of the container the container itself
     * is returned; otherwise the top-most child is returned.
     * @see Component#contains
     * @since JDK1.1
     */
    public Component getComponentAt(int x, int y) {
        return locate(x, y);
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by <code>getComponentAt(int, int)</code>.
     */
    public Component locate(int x, int y) {
        if (!contains(x, y)) {
            return null;
        }
        int ncomponents = this.ncomponents;
        Component component[] = this.component;
        for (int i = 0; i < ncomponents; i++) {
            Component comp = component[i];
            if (comp != null) {
                if (comp.contains(x - comp.x, y - comp.y)) {
                    return comp;
                }
            }
        }
        return this;
    }

    /**
     * Gets the component that contains the specified point.
     * @param      p   the point.
     * @return     returns the component that contains the point,
     *                 or <code>null</code> if the component does
     *                 not contain the point.
     * @see        java.awt.Component#contains
     * @since      JDK1.1
     */
    public Component getComponentAt(Point p) {
        return getComponentAt(p.x, p.y);
    }

    /**
     * Notifies the container to create a peer. It will also
     * notify the components contained in this container.
     * This method should be called by <code>Container.add</code>,
     * and not by user code directly.
     * @see #removeNotify
     * @since JDK1.0
     */
    public void addNotify() {
        synchronized (getTreeLock()) {
            // addNotify() on the children may cause proxy event enabling
            // on this instance, so we first call super.addNotify() and
            // possibly create an lightweight event dispatcher before calling
            // addNotify() on the children which may be lightweight.
            super.addNotify();
            if (!(peer instanceof sun.awt.peer.LightweightPeer)) {
                dispatcher = new LightweightDispatcher(this);
            }
            int ncomponents = this.ncomponents;
            Component component[] = this.component;
            for (int i = 0; i < ncomponents; i++) {
                component[i].addNotify();
            }
        }
    }

    /**
     * Notifies this container and all of its subcomponents to remove
     * their peers.
     * This method should be invoked by the container's
     * <code>remove</code> method, and not directly by user code.
     * @see       java.awt.Container#remove(int)
     * @see       java.awt.Container#remove(java.awt.Component)
     * @since     JDK1.0
     */
    public void removeNotify() {
        synchronized (getTreeLock()) {
            int ncomponents = this.ncomponents;
            Component component[] = this.component;
            for (int i = 0; i < ncomponents; i++) {
                component[i].removeNotify();
            }
            if (dispatcher != null) {
                dispatcher.dispose();
            }
            super.removeNotify();
        }
    }

    /**
     * Checks if the component is contained in the component hierarchy of
     * this container.
     * @param c the component
     * @return     <code>true</code> if it is an ancestor;
     *             <code>true</code> otherwise.
     * @since      JDK1.1
     */
    public boolean isAncestorOf(Component c) {
        Container p;
        if (c == null || ((p = c.getParent()) == null)) {
            return false;
        }
        while (p != null) {
            if (p == this) {
                return true;
            }
            p = p.getParent();
        }
        return false;
    }

    /**
     * Returns the parameter string representing the state of this
     * container. This string is useful for debugging.
     * @return    the parameter string of this container.
     * @since     JDK1.0
     */
    protected String paramString() {
        String str = super.paramString();
        LayoutManager layoutMgr = this.layoutMgr;
        if (layoutMgr != null) {
            str += ",layout=" + layoutMgr.getClass().getName();
        }
        return str;
    }

    public void show() {
        boolean wasShowing = isShowing();
        super.show();
        if (!wasShowing && peer != null) {
            peer.clearBackground(getGraphics());
        }
    }

    /**
     * Prints a listing of this container to the specified output
     * stream. The listing starts at the specified indentation.
     * @param    out      a print stream.
     * @param    indent   the number of spaces to indent.
     * @see      java.awt.Component#list(java.io.PrintStream, int)
     * @since    JDK
     */
    public void list(PrintStream out, int indent) {
        super.list(out, indent);
        int ncomponents = this.ncomponents;
        Component component[] = this.component;
        for (int i = 0; i < ncomponents; i++) {
            Component comp = component[i];
            if (comp != null) {
                comp.list(out, indent + 1);
            }
        }
    }

    /**
     * Prints out a list, starting at the specified indention, to the specified
     * print writer.
     */
    public void list(PrintWriter out, int indent) {
        super.list(out, indent);
        int ncomponents = this.ncomponents;
        Component component[] = this.component;
        for (int i = 0; i < ncomponents; i++) {
            Component comp = component[i];
            if (comp != null) {
                comp.list(out, indent + 1);
            }
        }
    }

    void setFocusOwner(Component c) {
        Container parent = this.parent;
        if (parent != null) {
            parent.setFocusOwner(c);
        }
    }

    void preProcessKeyEvent(KeyEvent e) {
        Container parent = this.parent;
        if (parent != null) {
            parent.preProcessKeyEvent(e);
        }
    }

    void postProcessKeyEvent(KeyEvent e) {
        Container parent = this.parent;
        if (parent != null) {
            parent.postProcessKeyEvent(e);
        }
    }

    void transferFocus(Component base) {
        nextFocus(base);
    }

    boolean postsOldMouseEvents() {
        return true;
    }

    /**
     * @deprecated As of JDK version 1.1,
     * replaced by transferFocus(Component).
     */
    void nextFocus(Component base) {
        Container parent = this.parent;
        if (parent != null) {
            parent.transferFocus(base);
        }
    }
    /* Serialization support.  A Container is responsible for
     * restoring the parent fields of its component children.
     */

    private int containerSerializedDataVersion = 1;
    private void writeObject(ObjectOutputStream s)
        throws IOException {
        s.defaultWriteObject();
        AWTEventMulticaster.save(s, containerListenerK, containerListener);
        s.writeObject(null);
    }

    private void readObject(ObjectInputStream s)
        throws ClassNotFoundException, IOException {
        s.defaultReadObject();
        Component component[] = this.component;
        for (int i = 0; i < ncomponents; i++)
            component[i].parent = this;
        Object keyOrNull;
        while (null != (keyOrNull = s.readObject())) {
            String key = ((String) keyOrNull).intern();
            if (containerListenerK == key)
                addContainerListener((ContainerListener) (s.readObject()));
            else // skip value for unrecognized key
                s.readObject();
        }
    }
// Focus-related functionality added 
//-------------------------------------------------------------------------
    private transient FocusTraversalPolicy focusTraversalPolicy;
    private boolean focusCycleRoot = false;

    void initializeFocusTraversalKeys() {
        focusTraversalKeys = new Set[4];
    }

    public void setFocusTraversalKeys(int id, Set keystrokes) {
        if (id < 0 || id >= KeyboardFocusManager.TRAVERSAL_KEY_LENGTH) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        // Don't call super.setFocusTraversalKey. The Component parameter check
        // does not allow DOWN_CYCLE_TRAVERSAL_KEYS, but we do.
        setFocusTraversalKeys_NoIDCheck(id, keystrokes);
    }

    public Set getFocusTraversalKeys(int id) {
        if (id < 0 || id >= KeyboardFocusManager.TRAVERSAL_KEY_LENGTH) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        // Don't call super.getFocusTraversalKey. The Component parameter check
        // does not allow DOWN_CYCLE_TRAVERSAL_KEY, but we do.
        return getFocusTraversalKeys_NoIDCheck(id);
    }

    public boolean areFocusTraversalKeysSet(int id) {
        if (id < 0 || id >= KeyboardFocusManager.TRAVERSAL_KEY_LENGTH) {
            throw new IllegalArgumentException("invalid focus traversal key identifier");
        }
        return (focusTraversalKeys != null && focusTraversalKeys[id] != null);
    }

    public boolean isFocusCycleRoot(Container container) {
        if (isFocusCycleRoot() && container == this) {
            return true;
        } else {
            return super.isFocusCycleRoot(container);
        }
    }
 
    private Container findTraversalRoot() {
    // I potentially have two roots, myself and my root parent
    // If I am the current root, then use me
    // If none of my parents are roots, then use me
    // If my root parent is the current root, then use my root parent
    // If neither I nor my root parent is the current root, then
    // use my root parent (a guess)
    Container currentFocusCycleRoot = KeyboardFocusManager.
        getCurrentKeyboardFocusManager().getCurrentFocusCycleRoot();
    Container root;    
    if (currentFocusCycleRoot == this) {
        root = this;
    } else {
        root = getFocusCycleRootAncestor();
        if (root == null) {
            root = this;
        }
    }
    if (root != currentFocusCycleRoot) {
        KeyboardFocusManager.getCurrentKeyboardFocusManager().
            setGlobalCurrentFocusCycleRoot(root);
    }
    return root;
    }

    final boolean containsFocus() {
        synchronized (getTreeLock()) {
            Component comp = KeyboardFocusManager.
                getCurrentKeyboardFocusManager().getFocusOwner();
            while (comp != null && !(comp instanceof Window) && comp != this) {
                comp = (Component) comp.getParent();
            }
            return (comp == this);
        }
    }

    /**
     * Check if this component is the child of this container or its children.
     * Note: this function acquires treeLock
     * Note: this function traverses children tree only in one Window.
     * @param comp a component in test, must not be null
     */
    boolean isParentOf(Component comp) {
        synchronized(getTreeLock()) {
            while (comp != null && comp != this && !(comp instanceof Window)) {
                comp = comp.getParent();
            }
            return (comp == this);
        }
    }

    void clearMostRecentFocusOwnerOnHide() {
        Component comp = null;
        Container window = this;
        synchronized (getTreeLock()) {
            while (window != null && !(window instanceof Window)) {
                window = window.getParent();
            }
            if (window != null) {
                comp = KeyboardFocusManager.
                    getMostRecentFocusOwner((Window)window);
                while ((comp != null) && (comp != this) && !(comp instanceof Window)) {
                    comp = comp.getParent();
                }
            }            
        }
        if (comp == this) {
            KeyboardFocusManager.setMostRecentFocusOwner((Window)window, null);
        }
        if (window != null) {
            Window myWindow = (Window)window;
            synchronized(getTreeLock()) {
                // This synchronized should always be the second in a pair (tree lock, KeyboardFocusManager.class)
                synchronized(KeyboardFocusManager.class) {
                    Component storedComp = myWindow.getTemporaryLostComponent();
                    if (isParentOf(storedComp) || storedComp == this) {
                        myWindow.setTemporaryLostComponent(null);
                    }
                }
            }
        }
    }

    void clearCurrentFocusCycleRootOnHide() {
        KeyboardFocusManager kfm = 
            KeyboardFocusManager.getCurrentKeyboardFocusManager();
        Container cont = kfm.getCurrentFocusCycleRoot();
        synchronized (getTreeLock()) {
            while (this != cont && !(cont instanceof Window) && (cont != null)) {
                cont = cont.getParent();
            }
        }
        if (cont == this) {
            kfm.setGlobalCurrentFocusCycleRoot(null);
        }
    }

    boolean nextFocusHelper() {
        if (isFocusCycleRoot()) {
            Container root = findTraversalRoot();
            Component comp = this;
            Container anc;
            while (root != null && 
                   (anc = root.getFocusCycleRootAncestor()) != null &&
                   !(root.isShowing() && 
                     root.isFocusable() && 
                     root.isEnabled())) {
                comp = root;
                root = anc;
            }
            if (root != null) {
                FocusTraversalPolicy policy = root.getFocusTraversalPolicy();
                Component toFocus = policy.getComponentAfter(root, comp);
                if (toFocus == null) {
                    toFocus = policy.getDefaultComponent(root);
                }
                if (toFocus != null) {
                    return toFocus.requestFocus(false);
                }
            }
            return false;
        } else {
            // I only have one root, so the general case will suffice
            return super.nextFocusHelper();
        }
    }
 
    public void transferFocusBackward() {
        if (isFocusCycleRoot()) {
            Container root = findTraversalRoot();
            Component comp = this;
            while (root != null && 
                   !(root.isShowing() && 
                     root.isFocusable() && 
                     root.isEnabled())) {
                comp = root;
                root = comp.getFocusCycleRootAncestor();
            }
            if (root != null) {
                FocusTraversalPolicy policy = root.getFocusTraversalPolicy();
                Component toFocus = policy.getComponentBefore(root, comp);
                if (toFocus == null) {
                    toFocus = policy.getDefaultComponent(root);
                }
                if (toFocus != null) {
                    toFocus.requestFocus();
                }
            }
        } else {
            // I only have one root, so the general case will suffice
            super.transferFocusBackward();
        }
    }

    public void setFocusTraversalPolicy(FocusTraversalPolicy policy) {
        FocusTraversalPolicy oldPolicy;
        synchronized (this) {
            oldPolicy = this.focusTraversalPolicy;
            this.focusTraversalPolicy = policy;
        }
        firePropertyChange("focusTraversalPolicy", oldPolicy, 
                           policy);
    }

    public FocusTraversalPolicy getFocusTraversalPolicy() {
        if (!isFocusCycleRoot()) {
            return null;
        }
        FocusTraversalPolicy policy = this.focusTraversalPolicy;
        if (policy != null) {
            return policy;
        }
        Container rootAncestor = getFocusCycleRootAncestor();
        if (rootAncestor != null) {
            return rootAncestor.getFocusTraversalPolicy();
        } else {
            return KeyboardFocusManager.getCurrentKeyboardFocusManager().
            getDefaultFocusTraversalPolicy();
        }
    }

    public boolean isFocusTraversalPolicySet() {
        return (focusTraversalPolicy != null);
    }

    public void setFocusCycleRoot(boolean focusCycleRoot) {
        boolean oldFocusCycleRoot;
        synchronized (this) {
            oldFocusCycleRoot = this.focusCycleRoot;
            this.focusCycleRoot = focusCycleRoot;
        }
        firePropertyChange("focusCycleRoot", new Boolean(oldFocusCycleRoot),
               new Boolean(focusCycleRoot));
    }

    public boolean isFocusCycleRoot() {
        return focusCycleRoot;
    }

    public void transferFocusDownCycle() {
        if (isFocusCycleRoot()) {
            KeyboardFocusManager.getCurrentKeyboardFocusManager().
                setGlobalCurrentFocusCycleRoot(this);
            Component toFocus = getFocusTraversalPolicy().
                getDefaultComponent(this);
            if (toFocus != null) {
                toFocus.requestFocus();
            }
        }
    }
}
