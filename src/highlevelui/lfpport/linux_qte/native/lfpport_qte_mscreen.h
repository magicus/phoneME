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

/**
 * @file
 * MScreen contains all other native screens and control functions.
 */

#ifndef _LFPPORT_QTE_MSCREEN_H_
#define _LFPPORT_QTE_MSCREEN_H_

#include <qapplication.h>
#include <qframe.h>
#include <qlabel.h>
#include <qmainwindow.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qpaintdevice.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qmetaobject.h>
#include <qpushbutton.h>
#include <qscrollview.h>
#include <midpEvents.h>

#include <qteapp_export.h>

/**
 * PlatformMScreen class.
 *
 * Its responsibility is to provide a scrollable area
 * for all other widgets.
 */
class PlatformMScreen : public QScrollView, public MScreen {
    Q_OBJECT

public:
    /**
     * PlatformMScreen constructor.
     */
    PlatformMScreen(QWidget *parent=0, const char* name=0);

    /**
     * PlatformMScreen destructor.
     */
    ~PlatformMScreen();

    /**
     * Returns the Y coordinate of the contents which is at the top edge of the viewport.
     */
    int getScrollPosition();

    /**
     * Sets new scroll position.
     */
    void setScrollPosition(int pos);

    /**
     * Called upon QPE application startup to initialize MScreen resources.
     */
    void init();

    /**
     * Resize the buffer size (either normal or fullscreen)
     *
     * @param newSize Specify the size of the screen
     */
    void setBufferSize(BufferSize newSize);

    /**
     * Sets the JVM's next time slice to happen after the given
     * number of milliseconds.
     *
     * @param millis number of milliseconds until the next time slice
     */
    void setNextVMTimeSlice(int millis);

    /**
     * Checks whether the given destination is the current painting device.
     *
     * @return true if the given destination is the current painting
     * device; false otherwise.
     */
    bool isCurrentPaintDevice(QPaintDevice *dst);

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
    QPainter *setupGC(int pixel_pen, int pixel_brush, const jshort *clip,
		      QPaintDevice *dst, int dotted);

    /**
     * Create a color from the packed pixel value.
     *
     * @param pixel structure from which to get the color to create
     *
     * @return the created Color object.
     */
    QColor getColor(int pixel);

    /**
     * Return the video buffer of graphics.
     */
    QPixmap* getBackBuffer() { return &qpixmap; }

    /**
     * Refreshes the area of the screen bounded by the given coordinates.
     *
     * @param x1 top-left x coordinate of the area to refresh
     * @param y1 top-left y coordinate of the area to refresh
     * @param x2 bottom-right x coordinate of the area to refresh
     * @param y2 bottom-right y coordinate of the area to refresh
     */
    void refresh(int x1, int y1, int x2, int y2);

    /**
     * Start to give VM time slice to run.
     */
    void startVM();

    /**
     * Stop VM from any further time slice.
     * Any UI leftover resource will be freed.
     */
    void stopVM();

    // These methods are needed by CustomItem
    /**
     * Override QScrollView to notify Java of mouse press.
     */
    void viewportMousePressEvent(QMouseEvent *mouse);

    /**
     * Override QScrollView to notify Java of mouse move.
     */
    void viewportMouseMoveEvent(QMouseEvent *mouse);

    /**
     * Override QScrollView to notify Java of mouse release.
     */
    void viewportMouseReleaseEvent(QMouseEvent *mouse);

    /**
     * Override QScrollView to notify Java of key press.
     */
    void keyPressEvent(QKeyEvent *key);

    /**
     * Override QScrollView to notify Java of key release.
     */
    void keyReleaseEvent(QKeyEvent *key);

    /**
     * Resets native resources of the device when foreground is gained
     * by a new Display.
     */
    void gainedForeground();

    /**
     * Returns QWidget representation of this MScreen instance.
     */
    QWidget * asWidget() { return this; }

    /**
     * Size of a normal screen.
     */
    int getDisplayWidth() const;
    int getDisplayHeight() const;

    /**
     * Size of a full screen canvas.
     */
    int getDisplayFullWidth() const;
    int getDisplayFullHeight() const;

    /**
     * Size available for laying out items in a Form.
     */
    int getScreenWidth() const;
    int getScreenHeight() const;

    /**
     * Size available for Alert
     */
    int getAlertWidth() const;
    int getAlertHeight() const;

    jboolean reverse_orientation() { r_orientation = ! r_orientation; return r_orientation;}
    jboolean get_reverse_orientation() const { return r_orientation;}
    void handle_clamshell(){};

    /**
     * Returns a pointer to the single PlatformMScreen instance,
     * or NULL if it has not been created yet.
     */
    static PlatformMScreen * getMScreen() {
      return (PlatformMScreen *)qteapp_get_mscreen();
    }

public slots:
    /**
     * Invoked when a timeslice is granted.
     * Passes control to VM for this time slice and schedules
     * next invokation.
     */
    void slotTimeout();

private:

    /**
     * Repaints the current screen upon request of the given event.
     *
     * @param e system event that is causing the repainting.
     */
    void viewportPaintEvent(QPaintEvent * );

    /**
     * Handle all the key events.
     * Generate a Java key event.
     */
    void handleKeyEvent(MidpEvent evt);

    /**
     * Double buffer size.
     */
    BufferSize bufferSize;

    /**
     * The image used as the double buffer
     */
    QPixmap qpixmap;

    /**
     * graphics context for drawing
     */
    QPainter *gc;

    /**
    * Number of key press events received.
    * Unlike other widgets (Chameleon and Java), we use not a boolean state,
    * but a counter there. We need it because Zaurus SL-5500 hardware/OS
    * has defect that some hard buttons only generate key release events
    * without key press event. This counter is used to filter out those
    * events.
    */
    int key_press_count;

    /**
     * The last clip region set.
     */
    QRect    last_clip;

    /**
     * Document last_pen, last_brush, last_dotted
     */
    int      last_pen, last_brush, last_dotted;

    /**
     * Document force_refresh
     */
    bool     force_refresh;

    /**
     * Size of a normal screen.
     */
    int DISPLAY_WIDTH;
    int DISPLAY_HEIGHT;

    /**
     * Size of a full screen canvas.
     */
    int DISPLAY_FULLWIDTH;
    int DISPLAY_FULLHEIGHT;

    /**
     * Size available for laying out items in a Form.
     */
    int SCREEN_WIDTH;
    int SCREEN_HEIGHT;

    static jboolean r_orientation;
};

#endif /* _LFPPORT_QTE_MSCREEN_H_ */
