/*
 * @(#)QtGraphics.cc	1.32 06/10/25
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
#include "sun_awt_qt_QtGraphics.h"
#include "QtGraphics.h"
#include "QtComponentPeer.h"
#include "QtApplication.h"
#include "ImageRepresentation.h"
#include "QtDisposer.h"
#include <stdio.h>
#include <qapplication.h>
#include <qmap.h>
#include <qvaluelist.h>

#define max(x, y) (((int)(x) > (int)(y)) ? (x) : (y))
#define min(x, y) (((int)(x) < (int)(y)) ? (x) : (y))

#define VALUE1 32767
#define VALUE2 2000  // CR 6287713

#if (QT_VERSION >= 0x030000)
#define IS_DEFERRED_DELETE(_e) (_e->type() == QEvent::DeferredDelete)
#else
#define IS_DEFERRED_DELETE(_e) \
        (_e->type() == (QEvent::Type)QtEvent::DeferredDelete)
#endif /* QT_VERSION */

#define SCALE(val, which) \
((which == 0) ? \
 ((val > VALUE1) ? VALUE1 : ((val < -VALUE1) ? -VALUE1 : val)) : \
 ((val > VALUE2) ? VALUE2 : ((val < -VALUE2) ? -VALUE2 : val)))

/* Rescale the result of addition of val1 and val2 to VALUE if
   positive, or -VALUE if negative. It also checks for overflow. */
inline int addAndRescale(int val1, int val2 = 0, int which = 0) 
{
    if (val1 == 0) { // val2 might be zero
	if (val2 == 0) {
	    return 0;
	} else {
	    return SCALE(val2, which);
	}
    } else if (val2 == 0) { // val1 is not zero
	return SCALE(val1, which);
    } else { /* val1 and val2 is not zero, check {under/over}flow of
		their addition */
	int result = val1 + val2;
	
	if (val1 > 0 && val2 > 0 && result < 0) { // overflow
	    if (which == 0) 
		return VALUE1;
	    else
		return VALUE2;
	} else if (val1 < 0 && val2 < 0 && result > 0) { // underflow
	    if (which == 0) 
		return -VALUE1;
	    else 
		return -VALUE2;
	} else {
	    return SCALE(result, which);
	}
    }
}

/* Given a java.awt.Color object this function will return a QColor
   object corresponding to the supplied java color object. */

QColor*
QtGraphics::getColor (JNIEnv *env, jobject color)
{
    int rgb = env->CallIntMethod(color,
				 QtCachedIDs.java_awt_Color_getRGBMID);
    int alpha = (rgb >> 24) & 0xFF;
    int red = (rgb >> 16) & 0xFF;
    int green = (rgb >> 8) & 0xFF;
    int blue = rgb & 0xFF;

    QColor *qtcolor = new QColor(qRgba(red, green, blue, alpha));
     if (!qtcolor)
	env->ThrowNew (QtCachedIDs.java_awt_AWTErrorClass, 
		       "could not allocate color");

    return qtcolor;
}

inline bool
QtGraphics::isaWidget(QPaintDevice* pd) {
    return (pd == NULL
	    ? false
	    : (pd->devType() == QInternal::Widget ? true : false));
}

typedef QValueList<QtGraphics*> GraphicsList;		// List of QtGraphics objects
							// which share a physical device.

// 6176847
// Converted all Tables to pointers instead of objects
static QMap<QPaintDevice*, GraphicsList> *refTable = new QMap<QPaintDevice*, GraphicsList>();	// QPaintDevice* keys, GraphicsList values

/*
 * Instantiates a singleton widget monitor.
 *
 * This is used to manage painter cacheing by watching the widget for deletion.
 */
WidgetMonitor* QtGraphics::singletonWM = new WidgetMonitor();

/* the static members */
QPainter QtGraphics::widgetPainter;
QPainter QtGraphics::otherPainter;
QPoint QtGraphics::widgetPos;

QPainter*
QtGraphics::GetPainter(QPaintDevice* pd)
{
    if (pd == NULL) {
        return NULL;
    }

    /* To understand this part completely, you may need to look at the
       eventFilter() in QtComponentPeer.  Whenever there is a paint
       event, we call QtGraphics::EndPaintingWidget(), which in turn
       calls QPainter::end() when the widget is the same as the device
       that the painter is painting on.

       Hence, when we get to this point and orig is not NULL, we are
       sure that there is no change to the widget state that makes
       the painter invalid, and we can return widgetPainter as is.
       On the other hand, if orig != (QWidget*) pd, we are about to
       paint to a different device, so we need to call end(). */
    if (QtGraphics::isaWidget(pd)) {
	QWidget* orig = (QWidget*) QtGraphics::widgetPainter.device();
	QWidget* devWidget = (QWidget *) pd;
	QPoint devPos = devWidget->mapToGlobal(QPoint(0, 0));

	if (devWidget == orig && // orig must be not NULL
	    devPos == QtGraphics::widgetPos) { 
            return &QtGraphics::widgetPainter;
	} else if (orig != NULL) { /* if orig is NULL, we don't need
				      to call QPainter::end() */
            QtGraphics::widgetPainter.end();
	}
	
	QtGraphics::widgetPainter.begin(pd);
	QtGraphics::widgetPos = devPos;
	return &QtGraphics::widgetPainter;
    } else {
	QtGraphics::otherPainter.begin(pd);
	return &QtGraphics::otherPainter;
    }
}

// The refTable maintains a mapping from the paint device to the list of
// of existing QtGraphics objects which reference the device .
//
// The RefPaintDevice() and UnrefPaintDevice() methods utilize this
// table to manage the reference counting from QtGraphics objects
// to the physical paint device contained within.
//
// All the table accessing functions are executed within either the qt event
// loop or entered through the JNI methods.  In all cases, the qt
// mutext lock should have been grabbed so they are all thread safe.

// QtGraphics::init() is called from Java_sun_awt_qt_QtGraphics_initIDs().
/*
 * Initialization for the C++ QtGraphics class related to painter cacheing.
 */
void
QtGraphics::init()
{
    GraphicsList emptyList;
    refTable->insert((QPaintDevice*)0, emptyList);

    // The above inserts into the tables are just safeguards so that
    // all the QMap-based tables are in a well-defined state before
    // being referenced from the rest of the code.
    //
    // We have observed some SEG fault on some test case where the
    // program immediately exit after finsihing the test.  The back
    // trace from gdb points to qmap.h:499.
}

/*
 * The refTable keeps track of the list of QtGraphics objects
 * there are which reference a particular paint device.
 */
void
QtGraphics::RefPaintDevice(QPaintDevice *pd, QtGraphics *g)
{
    if (refTable->contains(pd)) {
        (*refTable)[pd].append(g);
    } else {
        //6191478
        // This is the first reference to the QPaintDevice to create the
        // Graphics for, so we install the event filter here to monitor for
        // DeferredDeletions.
        if (QtGraphics::isaWidget(pd)) {
            // Monitors the widget for lifecycle events.
            ((QWidget*)pd)->installEventFilter(QtGraphics::singletonWM);
        }
        //6191478

       GraphicsList gList;
       gList.append(g);
       (*refTable)[pd] = gList;
    }
}

/*
 * Unreferences the paint device because the QtGraphics object is about to be
 * deleted.
 *
 * Once the reference count reaches 0, QtGraphics::CleanupPaintDevice()
 * gets called to cleanup the table entry.
 */
void
QtGraphics::UnrefPaintDevice(QPaintDevice *pd, QtGraphics* g)
{
    int count = ((*refTable)[pd]).count();
    if (count == 1) {
	// This is the very last graphics context object left
        // with reference to the passed paint device.
	QtGraphics::CleanupPaintDevice(pd);
    } else {
	((*refTable)[pd]).remove(g);
    }
}

/*
 * Unsets the paint device for the various QtGraphics objects out there
 * because the paint device is about to be deleted.
 *
 * Forcefully cleans up the table entry after notifying all the QtGraphics
 * objects about the pending deletion by setting their paint device
 * to NULL.
 */
void
QtGraphics::UnsetPaintDevice(QPaintDevice *pd)
{
    GraphicsList list = (*refTable)[pd];
    GraphicsList::Iterator it;
    QtGraphics* g;

    for (it = list.begin(); it != list.end(); ++it) {
	g = *it;
	g->paintDevice = 0;
    }


    if (QtGraphics::isaWidget(pd)) {
        QtGraphics::EndPaintingWidget(pd);
    }
    QtGraphics::CleanupPaintDevice(pd);
}

/*
 * Cleans up the table entry for the specified paint device.
 */
void
QtGraphics::CleanupPaintDevice(QPaintDevice *pd)
{
    refTable->remove(pd);
}

/*
 * Ends the painting for the widget so that it can be later revalidated.
 * This is because the paint device has changed its state, which could
 * happen if the widget has changed its size, its location, or its
 * display area has somehow become damaged; OR if the widget is to be
 * deleted, the painting also needs to end.
 */
void
QtGraphics::EndPaintingWidget(QPaintDevice *pd)
{
    if (QtGraphics::widgetPainter.device() == pd) {
        QtGraphics::widgetPainter.end();
    }
}

/*
 * This event filter is used to watch for the QEvent::DeferredDelete event
 * and to clean up the cached paintDevice(s) belonging to QtGraphics objects
 * to avoid dangling references to a deleted QObject.
 *
 * See QObject::event() and http://doc.trolltech.com/3.3/threads.html#6 for
 * details.
 */
bool 
WidgetMonitor::eventFilter( QObject *o, QEvent *e )
{
    // This member function is executed within the qt native event loop which
    // should have grabbed the qt mutext lock already.

    QWidget* w = (QWidget*) (o);
    QPaintDevice* pd = (QPaintDevice*) w;
    if ( IS_DEFERRED_DELETE(e)) {
	//printf("The qwidget 0x%x is about to be deleted!\n", (uint)(QWidget*)(o));
	QtGraphics::UnsetPaintDevice(pd);
    }

    // standard event processing
    return FALSE;
} 

QtGraphics::QtGraphics(QpWidget *widget, ImageRepresentation *imageRep) 
{
    this->initObject(widget->getQWidget(), imageRep);
}

QtGraphics::QtGraphics(QPaintDevice *paintDev, ImageRepresentation *imageRep)
{
    this->initObject(paintDev, imageRep);
}

void
QtGraphics::initObject(QPaintDevice *paintDev, ImageRepresentation *imageRep)
{
    this->paintDevice = paintDev;

    QtGraphics::RefPaintDevice(this->paintDevice, this);

    this->color = 0;
    this->xorColor = 0;
    this->painterPen = 0;
    this->painterBrush = 0;
    this->clipRect = 0;
    this->imgRep = imageRep;
    this->lineWidth = 0;
}

QtGraphics::QtGraphics(QtGraphics& src) 
{
    this->paintDevice = src.paintDevice;

    QtGraphics::RefPaintDevice(this->paintDevice, this);

    if (src.color) 
	this->color = new QColor(*(src.color));
    else
	this->color = 0;

    if (src.xorColor)
	this->xorColor = new QColor(*(src.xorColor));
    else
	this->xorColor = 0;

    this->painterPen = 0;
    this->painterBrush = 0;
    this->clipRect = 0;

    this->imgRep = src.imgRep;
    
    this->updateColor();
    this->lineWidth = 0;
}

QtGraphics::~QtGraphics() 
{
    QtGraphics::UnrefPaintDevice(this->paintDevice, this);

    this->paintDevice = 0;

    if (this->color) {
	delete this->color;
    }

    if (this->xorColor) {
	delete this->xorColor;
    }

    if (this->painterPen) {
	delete this->painterPen;
    }
    
    if (this->painterBrush) {
	delete this->painterBrush;
    }

    if (this->clipRect) {
	delete this->clipRect;
    }
}

void
QtGraphics::setLineProperties(int lineWidth, int lineCapStyle,
                              int lineJoinStyle) 
{
    this->painterPen->setWidth(lineWidth);

    switch (lineJoinStyle) {
        case 0:
            this->painterPen->setJoinStyle(Qt::MiterJoin);
            break;
        case 1:
            this->painterPen->setJoinStyle(Qt::RoundJoin);
            break;
        case 2:
            this->painterPen->setJoinStyle(Qt::BevelJoin);
            break;
    }

    switch (lineCapStyle) {
        case 0: 
            this->painterPen->setCapStyle(Qt::FlatCap);
            break;
        case 1:
            this->painterPen->setCapStyle(Qt::RoundCap);
            break;
        case 2:
            this->painterPen->setCapStyle(Qt::SquareCap);
            break;
    }
    
}

void
QtGraphics::setColor(QColor* newColor)
{
    if (newColor != 0) {
	if (this->color) {
	    delete this->color;
	}
	
	this->color = newColor;

    //6179738 : This was removed as part of fixing this bug, which 
    //          caused setLineProperties() to fail when accessing QPen
    //          so putting the code back.
    this->updateColor();
    //6179738
    }
}

QColor *
QtGraphics::getCurrentColor() {
    return this->color;
}

void
QtGraphics::setXORMode(QColor* newXorColor) 
{
    if (newXorColor != 0) {
        if (this->xorColor) {
            delete (this->xorColor);
        }
	
        this->xorColor = newXorColor;
    }
}

void
QtGraphics::setPaintMode()
{
    if (this->xorColor != 0) {
        delete this->xorColor;
        this->xorColor = 0;
     }
}

bool
QtGraphics::hasClipping()
{
    return (this->clipRect != 0);
}

bool
QtGraphics::hasPaintDevice() 
{
    if (this->paintDevice == NULL) {
	return FALSE;
    }

    return TRUE;
}

void
QtGraphics::setClip(int x, int y, int w, int h) 
{
#ifdef QWS
    /*
     * Fix for #480723
     * The graphics origin for QtWindowPeer and its subclasses is set to
     * the outer frame (i.e the title and decoration). Calls to this method
     * in response to repaint() calls passes clip with respect to the 
     * outer frame. We dont want to allow that since we could endup painting
     * the title and borders. The following code makes sure that the width
     * and height is well within the QFrame's geometry and also makes sures
     * any location (x,y) passed is within the inner frame.
     */
    if ( QtGraphics::isaWidget(this->paintDevice) ) {
        QWidget *widget = (QWidget *)this->paintDevice ;
        /* none of our native components extend from QFrame, so
         * QObject.inherits() or QObject.isA() is expensive here
         */
        if ( !strcmp(widget->className(),"QFrame") ) {
            QRect dev_rect = widget->geometry() ;
            /* w,h cannot exceed widget's width and height */
            if ( w > dev_rect.width() )
                w = dev_rect.width() ; 
            if ( h > dev_rect.height() ) 
                h = dev_rect.height() ;
            if ( x < 0 )
                x = 0 ;
            if ( y < 0 )
                y = 0 ;
        }
    }
#endif

    /* Added to check if the clip rect should be passed on to the current
     * painter (if the new clip rect is different from the old clip rect)
     */
    QRect *newRect = new QRect(x, y, w, h);

    // Fixed 6178265: QtGraphics::setClip() compare the existing clip with
    // the passed in clip using pointer arithmetic.

    /*
     * Previous versions were comparing pointers to QRect for equivalence,
     * which are wrong and resulted in the setPainterClipRect() method
     * always being called.
     */

    if (this->clipRect && *(this->clipRect) == *newRect) {
        delete newRect;   //6228131
	return;
    }


    if (this->clipRect) {
        delete this->clipRect;
    }

    this->clipRect = newRect;

    // The QtGraphics::setPainterClipRect() call is commented out
    // because the painter clip will get set on demand, so there's
    // no need doing it here.
    //
    // Also, this->getPainter() call can return a NULL pointer value
    // for a qwidget geometry with QSize(1, 1), so we don't want to
    // call setPainterClipRect() unconditionally.
/*
    if (this->getPainter()) {
	this->setPainterClipRect(this->getPainter());
    }
*/
}

void
QtGraphics::removeClip() 
{
    if (this->clipRect != 0) 
	delete this->clipRect;
    
    this->clipRect = 0;
}

QRect *
QtGraphics::getClipBounds() 
{
    return this->clipRect;
}

void QtGraphics::setPainterMode(QPainter *painter) 
{
  if (this->xorColor) 
    painter->setRasterOp(Qt::XorROP);
  else
    painter->setRasterOp(Qt::CopyROP);

  updateColor();  //6179738
}

void QtGraphics::setPainterPen(QPainter *painter) 
{
    //  if (this->actualColor) {
    //	if (painter->pen().color() != *this->actualColor ||
    //	    painter->pen().style() == Qt::NoPen) {
	    painter->setPen(*this->painterPen);
        //	}
        //  }
}

void QtGraphics::setPainterBrush(QPainter *painter) 
{
    if (this->color) {
	if (painter->brush().color() != *this->color ||
	    painter->brush().style() == Qt::NoBrush) {
	    painter->setBrush(*this->painterBrush);
	}
    }
}

void
QtGraphics::setPainterClipRect(QPainter *painter) 
{
    // Fixed 6178052: basis.DemoFrame repaint problem.
    //
    // The if (this->ClipRect) branch now checks whether
    // the existing painter clip region is equal to the
    // QtGraphics's clipRect and if they are not equal,
    // always sets the painter clip.
    //
    // In the previous version, it did not set the painter
    // clip if the original painter clip is null.  This
    // resulted in the painter's clip to be always null,
    // and the repaint problem where a simple repaint from
    // a lightweight component ends up clearing the whole
    // area.
    if (this->clipRect) {
	QRegion clipReg = painter->clipRegion();
	if (clipReg.boundingRect() == *(this->clipRect)) {
	    return;
	}

	painter->setClipRect(*(this->clipRect));

    } else {
	painter->setClipping(FALSE);
    }
}

QPainter*
QtGraphics::getPainter() 
{
    // If the paintDevice gets deleted before QtGraphics object's
    // lifecycle ends, we make sure to reset its value to NULL to
    // avoid dangling reference.  In that case, a NULL painter is
    // returned.
    //
    // See also: WidgetMonitor::eventFilter() and
    // QtGraphics::UnsetPaintDevice() for more details.

    if (this->paintDevice == NULL) {
	return NULL;
    }

    return QtGraphics::GetPainter(this->paintDevice);
}

void
QtGraphics::prepareDraw() 
{
    
    if (this->imgRep == NULL) 
        return;
    
    if (this->imgRep->isDirtyImage()) {
        this->imgRep->updatePixmap();
    }
}

void
QtGraphics::finishDraw(QPainter *painter) 
{
    /* we cache the painter for widget paint devices */

    if (QtGraphics::isaWidget(this->paintDevice)) {
        return;
    }

    if (painter) {
	painter->end();
    }
    
    if (this->imgRep != NULL) {
	this->imgRep->setDirtyPixmap(TRUE);
    }
}

void
QtGraphics::updateColor()
{
    if (this->color == 0) {
        return;
    }

    //6179738
    QColor *tempColor = 0;

    // Change the color based on xor
    if (this->xorColor != 0) {
        tempColor = this->xorColor;
    } else {
        tempColor = this->color;
    }

    // Use whichever color was set.
    if (this->painterPen == 0) {
        this->painterPen = new QPen(*(tempColor));
    } else {
        this->painterPen->setColor(*(tempColor));
    }

    if (this->painterBrush == 0) {
        this->painterBrush = new QBrush(*(tempColor));
    } else {
        painterBrush->setColor(*(tempColor));
    }
    //6179738
}

void 
QtGraphics::drawLine(int left, int top, int right, int bottom) 
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();
    if (painter == NULL) return;

    this->setPainterClipRect(painter);
    this->setPainterMode(painter);
    this->setPainterPen(painter);

    painter->drawLine(left, top, right, bottom);
    this->finishDraw(painter);
}

void 
QtGraphics::drawRect(int left, int top, int width, int height, bool fill)
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();
    if (painter == NULL) return;

    this->setPainterClipRect(painter);
    this->setPainterMode(painter);

    if (fill) {
	painter->fillRect(left, top, width, height,
			  *this->painterBrush);
    } else {
	this->setPainterPen(painter);
	if (width == 0 || height == 0) {
	    painter->drawLine(left, top, left + width, top + height);
	} else {
            /* 6202390.  Call drawRect instead of drawing 4 lines,
               so that QPen's Join style will be respected.  
            * painter->drawLine(left, top, left + width - 1, top);
            * painter->drawLine(left + width, top, left + width, top + height - 1);
            * painter->drawLine(left + width, top + height, left + 1, top + height);
            * painter->drawLine(left, top + height, left, top + 1);
            **/
	    painter->setBrush(Qt::NoBrush); /* make sure we're not filling */
            /* CR 4656020, qt draws the rect 1 pixel too small too */
            painter->drawRect(left, top, width+1, height+1);
	}
    }
    this->finishDraw(painter);
}

void 
QtGraphics::clearRect(int left, int top, int width, int height, 
                      QColor *backgroundColor) 
{
    this->prepareDraw();
     QPainter *painter = this->getPainter();
    if (painter == NULL) return;

    painter->setRasterOp(Qt::CopyROP);
    this->setPainterClipRect(painter);

    painter->fillRect(left, top, width, height,
                      ((backgroundColor != 0) ? 
                       *backgroundColor :
                       painter->backgroundColor()));
    this->finishDraw(painter);
}

void 
QtGraphics::drawArc(int left, int top, int width, int height, 
		    int startAngle, int endAngle) 
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();  
    if (painter == NULL) return;

    this->setPainterClipRect(painter);
    this->setPainterMode(painter);
    this->setPainterPen(painter);

    drawArc0(left, top, width, height, startAngle, endAngle, painter);

    this->finishDraw(painter);
}

/*
 * The caller must pass in a valid (QPainter*)painter to draw an arc.
 */
void 
QtGraphics::drawArc0(int left, int top, int width, int height, 
                     int startAngle, int endAngle, QPainter* painter) 
{

#ifdef QWS
    /*
     * Fixed bug 4802007.  DrawOval doesn't work after setXORMode.
     *
     * Fix by workaround of the qt bug where QPainter::drawArc() does
     * not work in Qt::XorROP mode because of overstriking.
     */
    if (painter->rasterOp() == Qt::XorROP) {
        QPointArray pa;
        pa.makeArc(left, top, width, height, startAngle, endAngle);
        if (pa.isEmpty()) {
            return;
        }

        uint i;
        QPointArray pa0;        // Get rid of duplicate points.
        pa0.resize(pa.size());  // We know that pa is not empty.
        uint j = 0;             // Pa0 index.
        uint k = 0;             // Counts the number of duplicate points.
        pa0[j++] = pa[0];       // Always copy the first point.

        for (i = 1; i < pa.size() - 1; i++) {
            if (pa[i] != pa[i-1]) {
                pa0[j++] = pa[i];
            } else {
                k++;
            }
        }

        /* Special treatment for the end point. */
        if (pa[i] != pa[0]) {
            pa0[j++] = pa[i];
        } else {
            k++;
        }

        pa0.resize(pa.size() - k); // Truncate pa0 by k.

        painter->drawPoints(pa0);

    } else {
        painter->drawArc(left, top, width, height,
                         startAngle, endAngle);
    }
#else
    painter->drawArc(left, top, width, height,
                     startAngle, endAngle);
#endif

}

void 
QtGraphics::fillArc(int left, int top, int width, int height, int
		    startAngle, int endAngle) 
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();

    if (painter == NULL) return;

    this->setPainterClipRect(painter);
    this->setPainterMode(painter);
    this->setPainterPen(painter);

    this->setPainterBrush(painter);

    painter->drawPie(left, top, width, height,
		     startAngle, endAngle);
    this->finishDraw(painter);
}

void 
QtGraphics::drawRoundRect(int x, int y, int width, int height,
			  int arcWidth, int arcHeight) 
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();  
    if (painter == NULL) return;

    this->setPainterClipRect(painter);
    this->setPainterMode(painter);
    this->setPainterPen(painter);
    painter->setBrush(Qt::NoBrush);

    /*
     * Fixed bug 4802007.  DrawOval doesn't work after setXORMode.
     *
     * Fix by workaround of the qt bug where QPainter::drawArc() does
     * not work in Qt::XorROP mode because of overstriking.
     *
     * As a result, the following code was removed:
     *

     // Fixed bug 4800489.
     // Fixed bug 4802934 (regression) -- did not check the divisor for 0.
     
     int xRnd = (width == 0) ? 0 : (arcWidth * 99 / width);
     int yRnd = (height == 0) ? 0 : (arcHeight * 99 / height);

     painter->drawRoundRect(x, y, width, height, xRnd, yRnd);

     *
     */

    int arcRadiusWidth, arcRadiusHeight;
    int start, end, pos;

    arcWidth = abs(arcWidth);
    arcHeight = abs(arcHeight);

    if (arcWidth > width)
      arcWidth = width;

    if (arcHeight > height)
      arcHeight = height;

    arcRadiusWidth = arcWidth / 2;
    arcRadiusHeight = arcHeight / 2;

    // Calls drawArc0() because we have a valid painter to begin with.

    drawArc0(x, y,
             arcRadiusWidth*2, arcRadiusHeight*2, 90*16, 90*16, painter);
    drawArc0(x + width - arcRadiusWidth*2, y,
             arcRadiusWidth*2, arcRadiusHeight*2, 0, 90*16, painter);
    drawArc0(x, y + height - arcRadiusHeight*2,
             arcRadiusWidth*2, arcRadiusHeight*2, 180*16, 90*16, painter);
    drawArc0(x + width - arcRadiusWidth*2, y + height - arcRadiusHeight*2,
             arcRadiusWidth*2, arcRadiusHeight*2, 270*16, 90*16, painter);

    /* 6202390. There is a pixel gap between arcs and lines when
    *   the line width is over 1.  
    * start = x + arcRadiusWidth + 1;
    * end = x + width - arcRadiusWidth - 1;
    **/
    start = x + arcRadiusWidth;
    end = x + width - arcRadiusWidth;
    pos = y + height;

    painter->drawLine(start, y, end, y);
    painter->drawLine(start, pos, end, pos);

    /* 6202390. There is a pixel gap between arcs and lines when
    *  the line width is over 1.  
    * start = y + arcRadiusHeight + 1;
    * end = y + height - arcRadiusHeight - 1;
    **/
    start = y + arcRadiusHeight;
    end = y + height - arcRadiusHeight;
    pos = x + width;

    painter->drawLine(x, start, x, end);
    painter->drawLine(pos, start, pos, end);

    this->finishDraw(painter);
}

void 
QtGraphics::fillRoundRect(int x, int y, int width, int height,
			  int arcWidth, int arcHeight) 
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();  
    if (painter == NULL) return;

    this->setPainterClipRect(painter);
    this->setPainterMode(painter);
    this->setPainterPen(painter);
    this->setPainterBrush(painter); // Fixed bug 4795797.

    if (arcWidth < 0) arcWidth = -arcWidth;
    if (arcHeight < 0) arcHeight = -arcHeight;

    if (arcWidth > width) arcWidth = width;
    if (arcHeight > height) arcHeight = height;

    if (arcWidth == width && arcHeight == height) {
       painter->drawPie(x, y, width, height,
		        0, 360*16);
       this->finishDraw(painter);
       return;
    }

    int arcRadiusWidth = arcWidth / 2;
    int arcRadiusHeight = arcHeight / 2;

    /*
     * Fixed bug 4795797.
     *
     * Use arcRadiusWidth/Height * 2 instead of arcWidth/Height to account for
     * the odd/even-ness of the respective arcWidth/Height.
     * Also notice the -1 temp. fix when drawing the 1st, 3rd, and 4th quadrants.
     * This is Qt 2.3.1 only change, without which the pies seem to overshoot
     * a little bit either x-wise and/or y-wise.
     *
     * We'll need to revisist this temporary fix if/when using newer versions of Qt.
     */

    painter->drawPie (x, y,
                      arcRadiusWidth * 2, arcRadiusHeight * 2,
                      90 * 16, 90 * 16);
    painter->drawPie (x + width - arcRadiusWidth * 2 - 1, y,
                      arcRadiusWidth * 2, arcRadiusHeight * 2,
                      0, 90 * 16);
    painter->drawPie (x, y + height - arcRadiusHeight * 2 - 1,
                      arcRadiusWidth * 2, arcRadiusHeight * 2,
                      -90 * 16, -90 * 16);
    painter->drawPie (x + width - arcRadiusWidth * 2 - 1, y + height - arcRadiusHeight * 2 - 1,
                      arcRadiusWidth * 2, arcRadiusHeight * 2,
                      0, -90 * 16);

    /*
     * Fixed bug 4795797 reopened.  When in XOR drawing mode, some strips of
     * pixels have been undrawn because they were rendered twice.
     *
     * Fixed by adjusting the widths and heights of the rectangles filled.
     */
    painter->fillRect (x + arcRadiusWidth, y,
                       width - arcRadiusWidth * 2 - 1, arcRadiusHeight,
                       *this->painterBrush);
    painter->fillRect (x, y + arcRadiusHeight,
                       width, height - arcRadiusHeight * 2 - 1,
                       *this->painterBrush);
    painter->fillRect (x + arcRadiusWidth, y + height - arcRadiusHeight - 1,
                       width - arcRadiusWidth * 2, arcRadiusHeight + 1,
                       *this->painterBrush);

    this->finishDraw(painter);
}

void 
QtGraphics::drawString(QFont *font, int left, int top, QString& text,
		       int length) 
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();
    if (painter == NULL) return;

    this->setPainterClipRect(painter);
    this->setPainterMode(painter);
    this->setPainterPen(painter);
    painter->setFont(*font);

    painter->drawText(left, top, text, length);
    this->finishDraw(painter);
}

void 
QtGraphics::drawPolygon(QPointArray& points, int nPoints, bool fill, bool close) 
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();
    if (painter == NULL) {
        return;
    }
     
    this->setPainterClipRect(painter);
    this->setPainterMode(painter);
    this->setPainterPen(painter);

    if (!fill) {
	painter->setBrush(Qt::NoBrush);
    } else {
	this->setPainterBrush(painter);
    }

    if (fill || close) {
        painter->drawPolygon(points, FALSE, 0);
    } else {
        painter->drawPolyline(points, 0, nPoints);
    }
    this->finishDraw(painter);
}

void
QtGraphics::drawPixmap(int x, int y, QPixmap &pixmap,
                       int xsrc, int ysrc, 
		       int width, int height)
{
    this->prepareDraw();
    QPainter *painter = this->getPainter();
    if (painter == NULL) {
      return;
    }

    if (width == -1) {
	width = pixmap.width() - xsrc;
    }
    
    if (height == -1) {
	height = pixmap.height() - ysrc;
    }

    if (width > 0 && height > 0) {
	if (QtGraphics::isaWidget(this->paintDevice)) {
	    QWidget *desktop = QApplication::desktop();
	    QPoint pt(x, y);
	    
	    QPoint globalPt = ((QWidget *) paintDevice)->mapToGlobal(pt);
	    
	    //whole image is offscreen
	    if (globalPt.x() > desktop->width() || 
		globalPt.y() > desktop->height() || 
		(globalPt.x() < 0 && width <= -globalPt.x()) ||
		(globalPt.x() < 0 && height <= -globalPt.y())) {
		return;
	    }
	    
	    //part/whole of image is onscreen
	    if (globalPt.x() < 0) {
		x -= globalPt.x();
		xsrc -= globalPt.x();
		width += globalPt.x();
		width = QMIN(width, desktop->width() );
	    } else {
		width = QMIN(width, (desktop->width() - globalPt.x()));
	    }
	    
	    if (globalPt.y() < 0) {
		y -= globalPt.y();
		ysrc -= globalPt.y();
		height += globalPt.y();
		height = QMIN(height, desktop->height() );
	    } else {
		height = QMIN(height, (desktop->height() - globalPt.y()));
	    }
	}
	this->setPainterMode(painter);  
    this->setPainterClipRect(painter);
	
	painter->drawPixmap(x, y, pixmap, xsrc, ysrc, width,
			    height);
	this->finishDraw(painter);
    }
}

void
QtGraphics::copyArea(int x, int y, int width, int height, int dx, int
		     dy)
{
    if (this->hasPaintDevice() && this->getPainter() != NULL) {
        this->prepareDraw();
        bitBlt (this->paintDevice, x + dx, y + dy, 
		this->paintDevice, x, y, width, height, 
		Qt::CopyROP, FALSE);
        this->finishDraw();
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_initIDs (JNIEnv *env, jclass cls)
{
    QtGraphics::init();

    QtCachedIDs.QtGraphicsClass = (jclass) env->NewGlobalRef (cls);
    GET_FIELD_ID (QtGraphics_dataFID, "data", "I");
    GET_FIELD_ID (QtGraphics_originXFID, "originX", "I");
    GET_FIELD_ID (QtGraphics_originYFID, "originY", "I");
    GET_FIELD_ID (QtGraphics_peerFID, "peer",
		  "Lsun/awt/qt/QtComponentPeer;");
    GET_FIELD_ID (QtGraphics_imageFID, "image",
		  "Lsun/awt/image/Image;");    
    GET_METHOD_ID (QtGraphics_constructWithPeerMID, "<init>", "(Lsun/awt/qt/QtComponentPeer;)V");
    cls = env->FindClass("sun/awt/qt/QtFontPeer");
    if (cls == NULL)
        return;
    GET_FIELD_ID (QtFontPeer_strikethroughFID, "strikethrough", "Z");
    GET_FIELD_ID (QtFontPeer_underlineFID, "underline", "Z");
}

JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtGraphics_createFromComponentNative (JNIEnv *env, jclass cls, jobject componentPeer)
{
    AWT_QT_LOCK;
    
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *nativePeer = QtComponentPeer::getPeerFromJNI (env, componentPeer);
    QtGraphics *graphicsNative;
    jobject graphics = NULL;
	
    if (nativePeer == NULL) {
	AWT_QT_UNLOCK;
	return NULL;
    }
	
    /* If the component is not yet visible then we return NULL. */
	
    QpWidget *drawWidget = nativePeer->getWidget();
    
    /* Create a new QtGraphics at the Java side. */
	
    graphics = env->NewObject (cls, QtCachedIDs.QtGraphics_constructWithPeerMID, componentPeer);
	
    if (graphics == NULL) {
        AWT_QT_UNLOCK;
        return NULL;
    }
    
    /* Create the QtGraphics and set it on this object. */
	
    graphicsNative = new QtGraphics(drawWidget);

    AWT_QT_UNLOCK;
    
    if (graphicsNative == NULL) {
	env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
	return NULL;
    }
    
    env->SetIntField (graphics, QtCachedIDs.QtGraphics_dataFID, (jint)graphicsNative);

    return graphics;
}

//use copy constructor
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_createFromGraphics (JNIEnv *env, jobject thisObj, jobject graphics)
{
    QtGraphics *gpx;
    QtGraphics *srcGpx;
    
    srcGpx = (QtGraphics *) env->GetIntField (graphics, QtCachedIDs.QtGraphics_dataFID);
	
    if (srcGpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
    
    /* Create the QtGraphics and set it on this object. */
    
    AWT_QT_LOCK;

    gpx = new QtGraphics(*srcGpx);
    
    AWT_QT_UNLOCK;
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
	return;
    }
    
    env->SetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID, (jint)gpx);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_createFromImage (JNIEnv *env, jobject thisObj, jobject imageRep)
{
    QtGraphics *gpx;
    ImageRepresentation *nativeImageRep;

    AWT_QT_LOCK;
      
    nativeImageRep = ImageRepresentation::getImageRepresentationFromJNI (env, imageRep, NULL);
      
    if (nativeImageRep == NULL) {
	AWT_QT_UNLOCK;
	return;
    }
      
    QPixmap *pmap = nativeImageRep->getPixmap();
    gpx = new QtGraphics(pmap, nativeImageRep);
      
    AWT_QT_UNLOCK;
      
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
	return;
    }      

    env->SetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID, (jint)gpx);      
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_disposeImpl (JNIEnv *env, jobject thisObj)
{
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
	
    if (gpx == NULL)
	return;
    
    AWT_QT_LOCK;
	
    delete gpx;

    AWT_QT_UNLOCK;

    void *nullP = NULL;
		
    env->SetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID, (jint)nullP);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_setForegroundNative (JNIEnv *env, jobject thisObj, jobject color)
{
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
	
    AWT_QT_LOCK;
    
    QColor *nativeColor = QtGraphics::getColor(env, color);
    if (nativeColor == NULL) {
	AWT_QT_UNLOCK;
	return;
    }

    gpx->setColor(nativeColor);
    
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_setPaintMode (JNIEnv *env, jobject thisObj)
{
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
	
    AWT_QT_LOCK;
    gpx->setPaintMode();
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_setXORMode (JNIEnv *env, jobject thisObj, jobject color)
{
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
	
    if (gpx == NULL || color == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
    
    AWT_QT_LOCK;

    QColor *xorColor = QtGraphics::getColor(env, color);
    if (xorColor == NULL) {
	AWT_QT_UNLOCK;
	return;
    }
		
    gpx->setXORMode(xorColor);

    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_setClipNative (JNIEnv * env, jobject thisObj,
					  jint x, jint y, jint w, jint h)
{
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj,
						       QtCachedIDs.QtGraphics_dataFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }

    AWT_QT_LOCK;

    gpx->setClip(x, y, w, h);
    
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_removeClip (JNIEnv *env, jobject thisObj)
{
    QtGraphics *gpx = (QtGraphics *)env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
	
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
	
    AWT_QT_LOCK;
    
    gpx->removeClip();
    
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_drawLineNative (JNIEnv *env, jobject thisObj, jint x1, jint y1, jint x2, jint y2)
{
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
	
    AWT_QT_LOCK;

#ifdef QWS
    gpx->drawLine(addAndRescale(originX, x1),
		  addAndRescale(originY, y1),
		  addAndRescale(originX, x2),
		  addAndRescale(originY, y2));
#else
    gpx->drawLine(originX + x1, originY + y1, originX + x2, originY + y2);
#endif

    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_drawRectNative (JNIEnv *env, 
					   jobject thisObj, 
					   jboolean fill,
					   jint x, 
					   jint y, 
					   jint width,
					   jint height)
{
    if ((width < 0) || (height < 0)) {
	return;
    }
    
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }

#ifdef QWS
    x = addAndRescale(x, originX);
    y = addAndRescale(y, originY);
    width = addAndRescale(width);
    height = addAndRescale(height);
#else 
    x += originX;
    y += originY;
#endif

    AWT_QT_LOCK;
    
    if (fill == JNI_TRUE) {
	gpx->drawRect(x, y, width, height, TRUE);
    } else {
	gpx->drawRect(x, y, width, height, FALSE);
    }

    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_clearRect (JNIEnv *env, jobject thisObj, jint x, jint y, jint width, jint height)
{
    if (width < 0 || height < 0)
	return;
    
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }

#ifdef QWS
    x = addAndRescale(originX, x);
    y = addAndRescale(originY, y);
    width = addAndRescale(width);
    height = addAndRescale(height);
#else
    x += originX;
    y += originY;
#endif
    
    AWT_QT_LOCK;
    jobject peer = env->GetObjectField (thisObj,
                                        QtCachedIDs.QtGraphics_peerFID);
    
    QColor *qtColor = NULL;
    if (peer != NULL) {
      jobject target = env->GetObjectField (peer,
                                            QtCachedIDs.QtComponentPeer_targetFID);
      jobject background = 
        env->CallObjectMethod (target,
                               QtCachedIDs.java_awt_Component_getBackgroundMID); 

      if (background != NULL) {
        qtColor = QtGraphics::getColor(env, background);
        if (qtColor == 0) {
          goto finish;
        }
      }
    } else { // an image
	jobject image = env->GetObjectField (thisObj,
					     QtCachedIDs.QtGraphics_imageFID);
	if (image != NULL) {
	    jobject imageRep = env->CallObjectMethod (image,
						      QtCachedIDs.sun_awt_image_Image_getImageRepMID);
	    if (imageRep != NULL && env->IsInstanceOf(imageRep,
						      QtCachedIDs.QtImageRepresentationClass)) {
		ImageRepresentation *nativeImageRep =
		    (ImageRepresentation*) env->GetIntField(imageRep,
							    QtCachedIDs.sun_awt_image_ImageRepresentation_pDataFID);
		if (nativeImageRep != NULL) {
		    qtColor = new
			QColor(nativeImageRep->getBackground());
		}
	    }
	}
    }

    gpx->clearRect(x, y, width, height, qtColor);

    if (qtColor) {
      delete qtColor;
    }

    finish:
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_drawArcNative (JNIEnv *env, jobject thisObj,
					  jint x,
					  jint y,
					  jint width,
					  jint height,
					  jint startAngle,
					  jint endAngle)
{
    if (width < 0 || height < 0) 
	return;
    
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (gpx == NULL) {
      env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
      return;
    }

    if (startAngle > 360) {
	startAngle = 360;
    } else if (startAngle < -360) {
	startAngle = -360;
    }
    
    if (endAngle > 360) {
	endAngle = 360;
    } else if (endAngle < -360) {
	endAngle = -360;
    }
    
    startAngle *= 16;
    endAngle *= 16;

#ifdef QWS
    x = addAndRescale(originX, x);
    y = addAndRescale(originY, y);
    width = addAndRescale(width);
    height = addAndRescale(height);
#else
    x += originX;
    y += originY;
#endif
    
    AWT_QT_LOCK;
    gpx->drawArc(x, y, width, height,
		 startAngle, endAngle);    
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_fillArcNative (JNIEnv *env, jobject thisObj,
					  jint x,
					  jint y,
					  jint width,
					  jint height,
					  jint startAngle,
					  jint endAngle)
{
    if (width < 0 || height < 0)
	return;
    
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }

    if (startAngle > 360) {
	startAngle = 360;
    } else if (startAngle < -360) {
	startAngle = -360;
    }
    
    if (endAngle > 360) {
	endAngle = 360;
    } else if (endAngle < -360) {
	endAngle = -360;
    }
    
    startAngle *= 16;
    endAngle *= 16;

#ifdef QWS
    x = addAndRescale(originX, x);
    y = addAndRescale(originY, y);
    width = addAndRescale(width, 0, 1);
    height = addAndRescale(height, 0, 1);
#else
    x += originX;
    y += originY;
#endif

    AWT_QT_LOCK;
    gpx->fillArc(x, y, width, height,
		 startAngle, endAngle);
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_drawRoundRectNative (JNIEnv *env, jobject thisObj,
						jint x,
						jint y,
						jint width,
						jint height,
						jint arcWidth,
						jint arcHeight)
{
    if (width < 0 || height < 0) 
	return;

    QtGraphics *gpx = (QtGraphics *)env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
	
    if (width < 0 || height < 0)
	return;
	
#ifdef QWS
    x = addAndRescale(originX, x);
    y = addAndRescale(originY, y);
    width = addAndRescale(width);
    height = addAndRescale(height);
    arcWidth = addAndRescale(arcWidth);
    arcHeight = addAndRescale(arcHeight);
#else
    x += originX;
    y += originY;
#endif
    
    AWT_QT_LOCK;
    gpx->drawRoundRect(x, y, width, height, arcWidth, arcHeight);
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_fillRoundRectNative (JNIEnv *env, jobject thisObj,
						jint x,
						jint y,
						jint width,
						jint height,
						jint arcWidth,
						jint arcHeight)
{
    if (width < 0 || height < 0) 
	return;
    
    QtGraphics *gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
	
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
	
    if (width < 0 || height < 0)
	return;

#ifdef QWS
    x = addAndRescale(originX, x);
    y = addAndRescale(originY, y);
    width = addAndRescale(width, 0, 1);
    height = addAndRescale(height, 0, 1);
    arcWidth = addAndRescale(arcWidth, 0, 1);
    arcHeight = addAndRescale(arcHeight, 0, 1);
#else
    x += originX;
    y += originY;
#endif
    
    AWT_QT_LOCK;
    gpx->fillRoundRect(x, y, width, height, arcWidth, arcHeight);
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_copyArea (JNIEnv *env, jobject thisObj, jint x, jint y, jint width, jint height, jint dx, jint dy)
{
    if (width < 0 || height < 0) 
	return;

    QtGraphics *gpx = (QtGraphics *)env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (gpx == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }

#ifdef QWS
    x = addAndRescale(originX, x);
    y = addAndRescale(originY, y);
    width = addAndRescale(width);
    height = addAndRescale(height);
    dx = addAndRescale(dx);
    dy = addAndRescale(dy);
#else
    x += originX;
    y += originY;
#endif
    
    AWT_QT_LOCK;
    gpx->copyArea(x, y, width, height, dx, dy);
    AWT_QT_UNLOCK;
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtGraphics_drawAttributedStringNative (JNIEnv *env, 
                                                       jobject thisObj, 
                                                       jobject fontPeer, 
                                                       jstring text, jint x, 
                                                       jint y, jobject color,
                                                       jboolean ret)
{
    QtGraphics *gpx;
    QFont *qtfont;
    QColor *curColor=NULL, *qtColor=NULL;
    int xval;
    bool strikethrough, underline;

    gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    
    if (fontPeer == NULL || gpx == NULL || text == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return 0;
    }
    
    qtfont = (QFont *) env->GetIntField (fontPeer, QtCachedIDs.QtFontPeer_dataFID);
    if (qtfont == NULL)	{
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return 0;
    }
    x += env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    y += env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    AWT_QT_LOCK;

    QString *qttext = awt_convertToQString(env, text);
    if (qttext == 0) {
        AWT_QT_UNLOCK;
        return 0;
    }
    strikethrough = 
        env->GetBooleanField(fontPeer, QtCachedIDs.QtFontPeer_strikethroughFID);
    underline =  
        env->GetBooleanField(fontPeer, QtCachedIDs.QtFontPeer_underlineFID);

    if (color != NULL) {
        qtColor = QtGraphics::getColor(env, color);
        curColor = new QColor(*(gpx->getCurrentColor()));
        if (qtColor != NULL)
            gpx->setColor(qtColor);
    }

    if (strikethrough == JNI_TRUE)
        qtfont->setStrikeOut(true);
    if (underline == JNI_TRUE)
        qtfont->setUnderline(true);
  
    if (ret == JNI_TRUE){
        QFontMetrics* fm = new QFontMetrics(*qtfont);
        xval =  fm->width(*qttext, -1);
        delete fm;
    }
    else 
        xval = -1;
    gpx->drawString(qtfont, x, y, *qttext, qttext->length());
    if (qtColor != NULL) {
        gpx->setColor(curColor);
    }

    qtfont->setStrikeOut(false);
    qtfont->setUnderline(false);
    delete qttext;

    AWT_QT_UNLOCK;
    return xval;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_drawStringNative (JNIEnv *env, jobject thisObj, 
                                             jobject fontPeer, jstring text,
                                             jint x, jint y)
{
    QtGraphics *gpx;
    QFont *qtfont;
    
    gpx = (QtGraphics *) env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
    
    if (fontPeer == NULL || gpx == NULL || text == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
    
    qtfont = (QFont *) env->GetIntField (fontPeer, QtCachedIDs.QtFontPeer_dataFID);
    
    if (qtfont == NULL)	{
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return;
    }
    
    x += env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    y += env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    AWT_QT_LOCK;

    QString *qttext = awt_convertToQString(env, text);
    if (qttext == 0) {
	AWT_QT_UNLOCK;
	return;
    }
    
    gpx->drawString(qtfont, x, y, *qttext, qttext->length());

    delete qttext;

    AWT_QT_UNLOCK;
}


/* un-used function, but kept for reference. */
/*
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_drawPolylineNative (JNIEnv *env,
					       jobject thisObj,
					       jintArray xPointsArray,
					       jintArray yPointsArray,
					       jint nPoints) 
{
    QtGraphics *gpx;
    jint *xPoints;
    jint *yPoints;
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (nPoints == 0)
        return;
    
    gpx = (QtGraphics *)env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
	
    if (gpx == NULL || xPointsArray == NULL || yPointsArray == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }
	
    if (nPoints > env->GetArrayLength (xPointsArray) ||
        nPoints > env->GetArrayLength (yPointsArray)) {
        env->ThrowNew (QtCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
        return;
    }
	
    xPoints = env->GetIntArrayElements (xPointsArray, NULL);
	
    if (xPoints == NULL)
        return;
    
    yPoints = env->GetIntArrayElements (yPointsArray, NULL);
	
    if (yPoints == NULL)
        return;
    
    AWT_QT_LOCK;
    
    QPointArray pointsArray(nPoints);
    
    for (unsigned int i = 0; ((int) i) < nPoints; i++) {
        pointsArray.setPoint(i, 
                             addAndRescale(xPoints[i], originX), 
                             addAndRescale(yPoints[i], originY));
    }

    env->ReleaseIntArrayElements (xPointsArray, xPoints, JNI_ABORT);
    env->ReleaseIntArrayElements (yPointsArray, yPoints, JNI_ABORT);
    
    gpx->drawPolyline(pointsArray, 0, nPoints);
    
    AWT_QT_UNLOCK;    
}
*/

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_drawPolygonNative (JNIEnv *env,
					      jobject thisObj,
					      jboolean filled,
					      jintArray xPointsArray,
					      jintArray yPointsArray,
					      jint nPoints,
					      jboolean close)
{
    QtGraphics *gpx;
    jint *xPoints;
    jint *yPoints;
    
    jint originX = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originXFID);
    jint originY = env->GetIntField (thisObj, QtCachedIDs.QtGraphics_originYFID);
    
    if (nPoints == 0)
        return;
    
    gpx = (QtGraphics *)env->GetIntField (thisObj, QtCachedIDs.QtGraphics_dataFID);
	
    if (gpx == NULL || xPointsArray == NULL || yPointsArray == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }
	
    if (nPoints > env->GetArrayLength (xPointsArray) ||
        nPoints > env->GetArrayLength (yPointsArray)) {
        env->ThrowNew (QtCachedIDs.ArrayIndexOutOfBoundsExceptionClass, NULL);
        return;
    }
	
    xPoints = env->GetIntArrayElements (xPointsArray, NULL);
	
    if (xPoints == NULL)
        return;
		
    yPoints = env->GetIntArrayElements (yPointsArray, NULL);
	
    if (yPoints == NULL)
        return;
    
    bool fill = (filled == JNI_TRUE) ? TRUE : FALSE;
    
    AWT_QT_LOCK;

    QPointArray pointsArray(nPoints);
    
#ifdef QWS
    for (unsigned int i = 0; ((int) i) < nPoints; i++) {
        pointsArray.setPoint(i, addAndRescale(xPoints[i], originX, 1), 
                             addAndRescale(yPoints[i], originY, 1));
    }
#else
    for (unsigned int i = 0; ((int) i) < nPoints; i++) {
        pointsArray.setPoint(i, xPoints[i] + originX, yPoints[i] + originY);
    }
#endif
	
    env->ReleaseIntArrayElements (xPointsArray, xPoints, JNI_ABORT);
    env->ReleaseIntArrayElements (yPointsArray, yPoints, JNI_ABORT);

    if (close == JNI_TRUE) 
        gpx->drawPolygon(pointsArray, nPoints, fill, TRUE);
    else
        gpx->drawPolygon(pointsArray, nPoints, fill, FALSE);
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtGraphics_setLinePropertiesNative (JNIEnv *env,
                                                    jobject thisObj,
                                                    jint lineWidth,
                                                    jint lineCapStyle,
                                                    jint lineJoinStyle)
{
    QtGraphics *gpx;

    gpx = (QtGraphics *)env->GetIntField (thisObj, 
                                          QtCachedIDs.QtGraphics_dataFID);
	
    if (gpx == NULL) {
        env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
        return;
    }
    AWT_QT_LOCK;
	gpx->setLineProperties(lineWidth, lineCapStyle, lineJoinStyle);
    AWT_QT_UNLOCK;
}



//  LocalWords:  QtGraphics
