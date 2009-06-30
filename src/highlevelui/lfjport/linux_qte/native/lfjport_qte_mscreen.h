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
 * Definition of the MScreen widget.
 */

#ifndef MSCREEN_H
#define MSCREEN_H

#include <kni.h>

#include <qlayout.h> 
#include <qwidget.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qrect.h>
#include <qwidgetstack.h>

extern "C" {
#include <midpEvents.h>
}

#include <qteapp_mscreen.h>

/**
 * The MScreen class maps to a Java platform widget-based drawing
 * widget, and is responsible for most of the drawing work for the
 * Java platform widget configuration:
 *
 * <b>Note:</b> Even though we might someday support one MScreen
 * for each Java platform graphics context, it is not part of this
 * design. Only one MScreen should exist.
 */
class ChameleonMScreen : public QWidget, public MScreen {
    Q_OBJECT
public:
    /**
     * IMPL_NOTE:Document the MScreen constructor
     */
    ChameleonMScreen(QWidget *parent=0, const char* name=0);

    /**
     * IMPL_NOTE:Document the MScreen destructor
     */
    ~ChameleonMScreen();

    /**
     * Resets everything in the MScreen widget.
     *
     * <p><b>Implementation Note</b>: Width used in the form for
     * layout should be without scrollbar (<tt>SCREEN_WIDTH</tt>) *
     * height returned in form should be <tt>SCREEN_HEIGHT</tt>
     *
     * IMPL_NOTE:SCROLLBAR_WIDTH should not be hardcoded.
     */
    void init();

    /**
     * Sets the JVM's next time slice to happen after the given
     * number of milliseconds.
     *
     * @param millis number of milliseconds until the next time slice
     */
    void setNextVMTimeSlice(int millis);

    /**
     * Start to give VM time slice to run.
     */
    void startVM();

    /**
     * Stop VM from any further time slice.
     */
    void stopVM();

    /**
     * IMPL_NOTE:Document isCurrentPaintDevice
     */
    bool isCurrentPaintDevice(QPaintDevice *dst);

    /**
     * Sets the drawing parameters in the QPainter for the graphics context.
     *
     * IMPL_NOTE:Document the parameters for setupGC and its return value.
     */
    QPainter *setupGC(int pixel_pen, int pixel_brush, const jshort *clip,
		      QPaintDevice *dst, int dotted);

    /**
     * Create a color from the packed pixel value.
     *
     * @param pixel structure from which to get the color to create
     *
     * IMPL_NOTE:Document the return value of getColor
     */
    QColor getColor(int pixel);

    /**
     * IMPL_NOTE:Document getBackBuffer
     */
    QPixmap* getBackBuffer() { return &qpixmap; }

    /**
     * Refreshes the area of the screen bounded by the given coordinates.
     *
     * @param x1 top-left x coordinate of the area to refresh
     * @param y1 top-left y coordinate of the area to refresh
     * @param x2 bottom-right x coordinate of the area to refresh
     * @param y2 bottom-right y coordinate of the area to refresh
     * IMPL_NOTE:Document the parameters to refresh
     */
    void refresh(int x1, int y1, int x2, int y2);

    /**
     * Returns the pixmap buffersize
     *
     * IMPL_NOTE:Further explain getBufferSize, including its return value
     */
    BufferSize getBufferSize();

    /**
     * Resizes the pixmap buffer size (either normal or fullscreen)
     *
     * IMPL_NOTE:Further explain setBufferSize -- what does it do when
     * newSize is the same as the current size, for example --
     * including its parameter
     */
    void setBufferSize(BufferSize newSize);

    /**
     * IMPL_NOTE:Document keyPressEvent
     */
    void keyPressEvent(QKeyEvent *key);

    /**
     * IMPL_NOTE:Document keyReleaseEvent
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

    jboolean reverse_orientation() { r_orientation = !r_orientation; return r_orientation;}
        
    jboolean get_reverse_orientation() const { return r_orientation;}

    void handle_clamshell(){};

public slots:
    /**
     * Invoked when a timeslice is granted.
     * Passes control to VM for this time slice and schedules
     * next invocation.
     */
    void slotTimeout();

private:
    /**
     * IMPL_NOTE:Document mousePressEvent
     */
    void mousePressEvent( QMouseEvent *mouse);

    /**
     * IMPL_NOTE:Document mouseMoveEvent
     */
    void mouseMoveEvent( QMouseEvent *mouse);

    /**
     * IMPL_NOTE:Document mouseReleaseEvent
     */
    void mouseReleaseEvent(QMouseEvent *mouse);

    /**
     * IMPL_NOTE:Document resizeEvent -- what it should do if implemented,
     * and a statement that it only needs to behave that way if resize
     * is supported.
     */
    void resizeEvent(QResizeEvent *event);

    /**
     * IMPL_NOTE:Document paintEvent
     */
    void paintEvent( QPaintEvent * );

    /**
     * Document qpixmap
     */
    QPixmap qpixmap; /* double buffering of suite's screen */
    /**
     * Document gc
     */
    QPainter *gc;    /* graphics context for drawing */
    /**
     * Document painted
     */
    jboolean painted;
    /**
     * Document progressStarted
     */
    jboolean progressStarted;

    /**
     * Document seen_key_press
     */
    jboolean seen_key_press;
    /**
     * Document last_clip
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
     * Document *vbl
     */
    QVBoxLayout *vbl;

    /*
     * Define total amount of display available for MIDP.
     * It has subregions for title, scrollbar, and softbuttons.
     */
    int DISPLAY_WIDTH;
    int DISPLAY_HEIGHT;
    int DISPLAY_FULLWIDTH;
    int DISPLAY_FULLHEIGHT;
    int SCREEN_X;
    int SCREEN_Y;
    int SCREEN_WIDTH;
    int SCREEN_HEIGHT;

    static jboolean r_orientation;

    friend class ChameleonMIDPMainWindow;
};

#endif
