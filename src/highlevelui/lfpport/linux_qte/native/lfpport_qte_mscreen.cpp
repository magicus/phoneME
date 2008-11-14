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

#include <kni.h>
#include <jvmspi.h>
#include <sni.h>

#include "lfpport_qte_util.h"

#include <qapplication.h>
#include <qmainwindow.h>
#include <qpainter.h>
#include <qpaintdevice.h>
#include <qpaintdevicemetrics.h>
#include <qlayout.h>
#include <qtoolbar.h>
#include <qpalette.h>
#include <qaction.h>
#include <qmessagebox.h>
#include <qstatusbar.h>
#include <qsizepolicy.h>

#include <midp_constants_data.h>
#include <keymap_input.h>
#include <midpServices.h>
#include <midpMalloc.h>
#include <midpString.h>
#include <midpEventUtil.h>
#include <lfpport_font.h>
#include <lfp_registry.h>

#include <qteapp_key.h>
#include "lfpport_qte_mscreen.h"
#include "lfpport_qte_mainwindow.h"
#include <moc_lfpport_qte_mscreen.cpp>

/**
 * @file  lfpport_qte_mscreen.cpp
 *
 * A frame-less widget that all Displayables are rendered on.
 */


jboolean PlatformMScreen::r_orientation = false;

/**
 * A frame-less widget that all Displayables are rendered on.
 */
PlatformMScreen::PlatformMScreen(QWidget *parent, const char* name) :QScrollView(parent, name) {
  vm_stopped = false;

  // MainWindow already has frame, no frame for MScreen
  setFrameStyle(QFrame::NoFrame);

  // Graphics context
  gc = new QPainter();

  force_refresh = true;
  last_pen = -1;
  last_brush = -1;
  last_dotted = 0;

  TRACE_MSC(  PlatformMScreen::MScreen..);
  connect(&vm_slicer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
  TRACE_MSC(..PlatformMScreen::MScreen);
}


/**
 * full screen mode in Qt does not work nicely with menu bar
 * so we have to fall back to the design where full screen
 * in Qt platform widget means only ticker is gone.
 */
void PlatformMScreen::init() {
    TRACE_MSC(  PlatformMScreen::init..);

    setFocusPolicy(QWidget::ClickFocus);
    
    // Always ...
    setHScrollBarMode(QScrollView::AlwaysOff);

    DISPLAY_WIDTH       = NORMALWIDTH;
    DISPLAY_HEIGHT      = NORMALHEIGHT;

    SCREEN_WIDTH        = NORMALWIDTH - VERT_SCROLLBAR_WIDTH;
    SCREEN_HEIGHT       = NORMALHEIGHT;

    DISPLAY_FULLWIDTH   = FULLWIDTH;
    DISPLAY_FULLHEIGHT  = FULLHEIGHT;

    /*
     * Use the values from constants.xml. No more dynamic querying.
     *
    // Measure without scrollbar
    setVScrollBarMode(QScrollView::AlwaysOff);
    DISPLAY_WIDTH  = visibleWidth();  // width for full and normal screen mode
    DISPLAY_HEIGHT = visibleHeight(); // height for normal mode

    // Measure with scrollbar
    setVScrollBarMode(QScrollView::AlwaysOn);
    SCREEN_WIDTH  = visibleWidth();  // width that should be used for layout
    SCREEN_HEIGHT = visibleHeight(); // height that form should return

    // Special case, see above for explanation;
    // for canvas: no scroll, no ticker
    DISPLAY_FULLWIDTH  = DISPLAY_WIDTH;
    DISPLAY_FULLHEIGHT = DISPLAY_HEIGHT +
                         MainWindow->getTicker()->sizeHint().height();

    // for canvas: no border in layout and no spacing between mscreen and ticker
    QLayout *layout = parentWidget() == NULL ? NULL : parentWidget()->layout();
    if (layout != NULL) {
      DISPLAY_FULLWIDTH  += 2*layout->margin();
      DISPLAY_FULLHEIGHT += 2*layout->margin() + layout->spacing();
    }
    */

    setVScrollBarMode(QScrollView::Auto);

    // Set the size of the midlet suites app area
    setFixedSize(getDisplayWidth(), getDisplayHeight());
    qpixmap.resize(getDisplayWidth(), getDisplayHeight());
    qpixmap.fill(); // Qt::white is default

    TRACE_MSC(..PlatformMScreen::init);
}


/**
 * Resize the buffer size (either normal or fullscreen)
 *
 * @param newSize Specify the size of the screen
 */
void PlatformMScreen::setBufferSize(BufferSize newSize)
{
    if (newSize == fullScreenSize) {
        if (gc->isActive()) {
            gc->end();
        }
        qpixmap.resize(getDisplayFullWidth(), getDisplayFullHeight());
    } else {
        qpixmap.resize(getDisplayWidth(), getDisplayHeight());
    }

    // Whether current Displayable won't repaint the entire screen on
    // resize event, the artefacts from the old screen content can appear.
    // That's why the buffer content is not preserved.
    qpixmap.fill(); // Qt::white is default
}

/**
 * Start VM by starting a time share request for it.
 */
void PlatformMScreen::startVM() {
    vm_stopped = false;
    key_press_count = 0;

    // Setup next VM time slice to happen immediately
    setNextVMTimeSlice(0);
}

/**
 * Stop VM by stopping requests for VM time slice.
 * Any leftover UI resource will be freed also.
 */
void PlatformMScreen::stopVM() {
    // Stop any further VM time slice
    setNextVMTimeSlice(-1);

    // Clean up any leftover native UI resources since VM is exiting
    MidpDeleteAllComponents();

    // Clean up any leftovers of native font resources
    lfpport_font_finalize();
}

PlatformMScreen::~PlatformMScreen()
{
    killTimers();
    delete gc;
    gc = NULL;
}

/**
 * Handle mouse press event in the VIEWPORT
 *
 * @param mouse QT's QMouseEvent
 */
void PlatformMScreen::viewportMousePressEvent(QMouseEvent *mouse)
{
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = MIDP_PEN_EVENT;
    evt.ACTION = KEYMAP_STATE_PRESSED;
    evt.X_POS = mouse->x();
    evt.Y_POS = mouse->y();

    midpStoreEventAndSignalForeground(evt);
}


/**
 * Handle mouse move event in the VIEWPORT
 *
 * @param mouse QT's QMouseEvent
 */
void PlatformMScreen::viewportMouseMoveEvent(QMouseEvent *mouse)
{
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = MIDP_PEN_EVENT;
    evt.ACTION = KEYMAP_STATE_DRAGGED;
    evt.X_POS = mouse->x();
    evt.Y_POS = mouse->y();

    midpStoreEventAndSignalForeground(evt);
}

/**
 * Handle mouse move release in the VIEWPORT
 *
 * @param mouse QT's QMouseEvent
 */
void PlatformMScreen::viewportMouseReleaseEvent(QMouseEvent *mouse)
{
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);


    evt.type = MIDP_PEN_EVENT;
    evt.ACTION = KEYMAP_STATE_RELEASED;
    evt.X_POS = mouse->x();
    evt.Y_POS = mouse->y();

    midpStoreEventAndSignalForeground(evt);
}


/**
 * Handle key press event
 *
 * @param key QT's QKeyEvent
 * @see keyReleaseEvent or <a href="http://doc.trolltech.com/qtopia/html/qwidget.html#0a4482">online menu.</a>
 */
void PlatformMScreen::keyPressEvent(QKeyEvent *key)
{
    key_press_count += 1;
#if ENABLE_MULTIPLE_ISOLATES
    if (key->key() == Qt::Key_F7 ||
        key->key() == Qt::Key_Home) {
        // F7 to display the foreground selector
        if (!key->isAutoRepeat()) {
            MidpEvent evt;
            MIDP_EVENT_INITIALIZE(evt);
            evt.intParam1 = 0;
            evt.type = SELECT_FOREGROUND_EVENT;
            midpStoreEventAndSignalAms(evt);
        }
#ifdef QT_KEYPAD_MODE
    } else if (key->key() == Qt::Key_Flip) {
#else
    } else if (key->key() == Qt::Key_F4) {
#endif
        if (!key->isAutoRepeat()) {
            MidpEvent evt;
            MIDP_EVENT_INITIALIZE(evt);
            evt.type = SELECT_FOREGROUND_EVENT;
            evt.intParam1 = 1;
            midpStoreEventAndSignalAms(evt);
        }
    }
#else
    // F7 pause or activate all Java apps
    if ((key->key() == Qt::Key_F7 ||
         key->key() == Qt::Key_Home) &&
        !key->isAutoRepeat()) {
        pauseAll();
    }
#endif
    else {
        MidpEvent evt;
        MIDP_EVENT_INITIALIZE(evt);
        if ((evt.CHR = mapKey(key)) != KEYMAP_KEY_INVALID) {
            if (evt.CHR == KEYMAP_KEY_SCREEN_ROT) {
                evt.type = ROTATION_EVENT;
            } else if (evt.CHR == KEYMAP_KEY_VIRT_KEYB) {
                evt.type = VIRTUAL_KEYBOARD_EVENT;
            } else {
                evt.type = MIDP_KEY_EVENT;
            }
            evt.ACTION = key->isAutoRepeat() ? 
                KEYMAP_STATE_REPEATED : KEYMAP_STATE_PRESSED;
            handleKeyEvent(evt);
        }
    }
}

/**
 * Handle key release event
 *
 * @param key QT's QKeyEvent
 * @see keyPressEvent or <a href="http://doc.trolltech.com/qtopia/html/qwidget.html#4b620f">online menu.</a>
 */
void PlatformMScreen::keyReleaseEvent(QKeyEvent *key)
{
    if (key_press_count == 0 || key->isAutoRepeat()) {
        // We may have a left-over keyReleaseEvent from a previous
        // invocation of the VM! Also, Zaurus SL-5500 hardware/OS
        // has defect that some hard buttons only generate key release
        // events without key press event.
        return;
    }

    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    if ((evt.CHR = mapKey(key)) != KEYMAP_KEY_INVALID) {
        evt.type = MIDP_KEY_EVENT;
        evt.ACTION = KEYMAP_STATE_RELEASED;
        handleKeyEvent(evt);
    }

    if (key_press_count > 0) {
        key_press_count -= 1;
    }
}

/**
 * Handle (all) the key events.
 *
 * @param evt MIDP event
 */
void PlatformMScreen::handleKeyEvent(MidpEvent evt)
{
  if (evt.type != MIDP_INVALID_EVENT) {
    midpStoreEventAndSignalForeground(evt);
  }
}

/**
 * Repaint current screen upon system notification
 *
 * @param e Qt paint event
 */
void PlatformMScreen::viewportPaintEvent(QPaintEvent *e)
{
    const QRect& r = e->rect();
    refresh(r.left(), r.top(), r.right(), r.bottom());
}

/**
 * Create a Color from the packed pixel value.
 *
 * @param pixel the packed pixel integer
 */
QColor PlatformMScreen::getColor(int pixel) {
    int r = (pixel >> 16) & 0xff;
    int g = (pixel >> 8)  & 0xff;
    int b = (pixel >> 0)  & 0xff;
    return QColor(r, g, b);
}

/**
 * Returns 'true' if dst is the current painting device. Otherwise,
 * return 'false'.
 *
 * @param dst Pointer to QPaintDevice structure
 */
bool
PlatformMScreen::isCurrentPaintDevice(QPaintDevice *dst) {
    return (gc->device() == dst) ? true : false;
}

/**
 * Set the drawing parameters in the QPainter
 */
QPainter *
PlatformMScreen::setupGC(int pixel_pen, int pixel_brush, const jshort *clip,
                  QPaintDevice *dst, int dotted) {

  TRACE_MSC(PlatformMScreen::setupGC);

    QPaintDevice* dev = gc->device();

    // default destination is the back buffer
    if (dst == NULL) {
      dst = (QPaintDevice*)&qpixmap;
    }

    // finish operation on old device, if changed
    if ((dev != dst) || force_refresh) {
        if (gc->isActive()) {
            gc->end();
        }
    }

    // start operation on new device, if needed
    if (!gc->isActive()) {
        gc->begin(dst);
    }

    // check if pen parameters changed
    if (((dev != dst)            ||
         (last_pen != pixel_pen) ||
         (last_dotted != dotted) ||
         force_refresh)) {

        if (pixel_pen != -1) {
            QColor color = getColor(pixel_pen);
            QPen pen = QPen(color, 0,
                            (dotted ? Qt::DotLine : Qt::SolidLine));
            gc->setPen(pen);
        } else {
            gc->setPen(Qt::NoPen);
        }
        last_pen = pixel_pen;
        last_dotted = dotted;
    }

    // check if pen parameters changed
    if (((dev != dst)            ||
         (last_brush != pixel_brush)   ||
         force_refresh)) {
        if (pixel_brush != -1) {
            gc->setBrush(getColor(pixel_brush));
        } else {
            gc->setBrush(Qt::NoBrush);
        }
        last_brush = pixel_brush;

    }


    // check if clipping region changed
    if (clip != NULL &&
        ((dev != dst)            ||
         force_refresh           ||
         (clip[0] != last_clip.left())  ||
         (clip[1] != last_clip.top())   ||
         ((clip[2] - clip[0]) != last_clip.width()) ||
         ((clip[3] - clip[1]) != last_clip.height()))) {
        QRect uclip(clip[0], clip[1],
                    clip[2] - clip[0], clip[3] - clip[1]);
        last_clip = uclip;
        gc->setClipRect(uclip);
    }

    // drop force_refresh flag after all
    force_refresh = false;

    return gc;
}

void PlatformMScreen::setNextVMTimeSlice(int millis) {
    if (millis < 0) {
        // A negative time means we should stop VM from getting time slice
        if (vm_slicer.isActive()) {
            vm_slicer.stop();
        }
    } else {
        if (vm_slicer.isActive()) {
            vm_slicer.changeInterval(millis);
        } else {
            vm_slicer.start(millis, TRUE);
        }
    }
}

void PlatformMScreen::slotTimeout() {
    slotTimeoutImpl();
}

/**
 * Refresh certain part of the screen.
 *
 * @param x1 absolute X coordinate of top left conner
 * @param y1 absolute Y coordinate of top left conner
 * @param x2 absolute X coordinate of lower right conner
 * @param y2 absolute Y coordinate of lower right conner
 */
void PlatformMScreen::refresh(int x1, int y1, int x2, int y2) {
    TRACE_MSC(PlatformMScreen::refresh);

    /* Only draw for canvas */
    if ((MidpCurrentScreen != NULL)
        && (MidpCurrentScreen->component.type == MIDP_CANVAS_TYPE)) {

      /* Finish last activity */
      if (gc->isActive())
          gc->end();

      bitBlt((QPaintDevice*)viewport(), x1, y1,
             &qpixmap,
             x1, y1,
             x2 - x1 + 1,
             y2 - y1 + 1);
    }

    force_refresh = true;
    last_pen = last_brush = -1;
}

int
PlatformMScreen::getScrollPosition()
{
  return (contentsY());
}

void PlatformMScreen::setScrollPosition(int pos)
{
    setContentsPos(0, pos);
}

/**
 * Resets native resources of the device when foreground is gained
 * by a new Display.
 */
void PlatformMScreen::gainedForeground() {
  force_refresh  = KNI_TRUE;
  key_press_count = 0;
}

/**
 * Width of a normal screen.
 */
 int PlatformMScreen::getDisplayWidth() const { 
    if (r_orientation) {
        return DISPLAY_HEIGHT;
    } else {
        return DISPLAY_WIDTH; 
    }
}
/**
 * Height of a normal screen.
 */
int PlatformMScreen::getDisplayHeight() const {
    if (r_orientation) {
        return DISPLAY_WIDTH;
    } else {
        return DISPLAY_HEIGHT; 
    }
}

/**
 * Width of a full screen canvas.
 */
int PlatformMScreen::getDisplayFullWidth() const {
    if (r_orientation) {
        return DISPLAY_FULLHEIGHT;
    } else {
        return DISPLAY_FULLWIDTH; 
    }
}

/**
 * Height of a full screen canvas.
 */
int PlatformMScreen::getDisplayFullHeight() const {
    if (r_orientation) {
        return DISPLAY_FULLWIDTH;
    } else {
        return DISPLAY_FULLHEIGHT;
    }
}

/**
 * Width available for laying out items in a Form.
 */
int PlatformMScreen::getScreenWidth() const {
    if (r_orientation) {
        return SCREEN_HEIGHT;
    } else {
        return SCREEN_WIDTH; 
    }
}

/**
 * Height available for laying out items in a Form.
 */
int PlatformMScreen::getScreenHeight() const {
    if (r_orientation) {
        return SCREEN_WIDTH;
    } else {
        return SCREEN_HEIGHT; 
    }
}


/**
 * Width available for Alert.
 */
int PlatformMScreen::getAlertWidth() const {
    if (r_orientation) {
        return ALERT_HEIGHT;
    } else {
        return ALERT_WIDTH;
    }

}

/**
 * Height available for Alert.
 */
int PlatformMScreen::getAlertHeight() const {
    if (r_orientation) {
        return ALERT_WIDTH;
    } else {
        return ALERT_HEIGHT; 
    }
}

