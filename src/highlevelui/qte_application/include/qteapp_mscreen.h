/*
 *  
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * This source file is specific for Qt-based configurations.
 */

#ifndef _QTEAPP_MSCREEN_H_
#define _QTEAPP_MSCREEN_H_

/**
 * @file
 * @ingroup highui_qteapp
 *
 * @brief Linux/Qte MScreen interface
 */

#include <java_types.h>
#include <qpainter.h>
#include <qtimer.h>

/**
 * The MScreen class maps to a Java platform widget-based drawing
 * widget, and is responsible for most of the drawing work for the
 * Java platform widget configuration.
 *
 * It is an abstract class.
 *
 * @see MIDPMainWindow
 */
class MScreen {
public:
    /**
     * Called upon QPE application startup to initialize MScreen resources.
     */
    virtual void init() = 0;

    /** Screen buffer size: normal or full. */
    enum BufferSize{ normalScreenSize = 0, fullScreenSize };

    /**
     * Resize the buffer size (either normal or fullscreen)
     *
     * @param newSize Specify the size of the screen
     */
    virtual void setBufferSize(BufferSize newSize) = 0;

    /**
     * Sets the JVM's next time slice to happen after the given
     * number of milliseconds.
     *
     * @param millis number of milliseconds until the next time slice
     */
    virtual void setNextVMTimeSlice(int millis) = 0;

    /**
     * Checks whether the given destination is the current painting device.
     *
     * @return true if the given destination is the current painting
     * device; false otherwise.
     */
    virtual bool isCurrentPaintDevice(QPaintDevice *dst) = 0;

    /**
     * Sets the drawing parameters in the QPainter for the graphics 
     * context.
     *
     * @param pixel_pen the pen to be set for drawing.
     * @param pixel_brush the brush to be set for drawing.
     * @param clip the clip to be set on the destination.
     * @param dst the drawing destination.
     * @param dotted the type of line (solid/dotted).
     *
     * @return pointer to the graphics context to be used for painting.
     */
    virtual QPainter *setupGC(int pixel_pen, int pixel_brush, 
			      const jshort *clip,
			      QPaintDevice *dst, int dotted) = 0;

    /**
     * Create a color from the packed pixel value.
     *
     * @param pixel structure from which to get the color to create
     *
     * @return the created Color object.
     */
    virtual QColor getColor(int pixel) = 0;

    /**
     * Return the video buffer of graphics.
     */
    virtual QPixmap* getBackBuffer() = 0;

    /**
     * Refreshes the area of the screen bounded by the given coordinates.
     *
     * @param x1 top-left x coordinate of the area to refresh
     * @param y1 top-left y coordinate of the area to refresh
     * @param x2 bottom-right x coordinate of the area to refresh
     * @param y2 bottom-right y coordinate of the area to refresh
     */
    virtual void refresh(int x1, int y1, int x2, int y2) = 0;

    /**
     * Start to give VM time slice to run.
     */
    virtual void startVM() = 0;

    /**
     * Stop VM from any further time slice.
     * Any UI leftover resource will be freed.
     */
    virtual void stopVM() = 0;

    /**
     * Suspend VM. VM will not receive time slices until resumed.
     */
    virtual void suspendVM();

    /**
     * Resume VM to normal operation.
     */
    virtual void resumeVM();

    /**
     * Resets native resources of the device when foreground is gained 
     * by a new Display.
     */
    virtual void gainedForeground() = 0;

    /**
     * Requests MIDP system to resume.
     */
    virtual void activateAll();

    /**
     * Requests MIDP system (including java applications, VM and resources)
     * to suspend.
     */
    virtual void pauseAll();

    /**
     * Returns QWidget representation of this MScreen instance.
     */
    virtual QWidget * asWidget() = 0;

    /**
     * Width of a normal screen.
     */
    virtual int getDisplayWidth() const = 0;

    /**
     * Height of a normal screen.
     */
    virtual int getDisplayHeight() const = 0;

    /**
     * Width of a full screen canvas.
     */
    virtual int getDisplayFullWidth() const = 0;

    /**
     * Height of a full screen canvas.
     */
    virtual int getDisplayFullHeight() const = 0;

    /**
     * Screen width currently available for Java.
     */
    virtual int getScreenWidth() const = 0;

    /**
     * Screen height currently available for Java.
     */
    virtual int getScreenHeight() const = 0;

    virtual jboolean reverse_orientation() = 0;

	virtual void handle_clamshell() = 0;

    virtual jboolean get_reverse_orientation() const = 0;

    virtual ~MScreen(){ };

protected:
    /** Constructs an instance. */
    MScreen();

    /**
     * Implementation of slotTimeout() shared between distinct ports.
     * The slotTimeout() methods are defined in subclasses.
     */
    void slotTimeoutImpl();

    /**
     * A flag to determine whether the VM is currently suspended
     * (should not receive time slices).
     */
    bool vm_suspended;

    /**
     * A flag to avoid performing any more timer processing after
     * JVM_TimeSlice returns -2.
     */
    bool vm_stopped;

    /**
     * Timer to request time slice for VM.
     */
    QTimer   vm_slicer;
};

/* @} */

#endif /* _MIDP_QTE_MSCREEN_H_ */
