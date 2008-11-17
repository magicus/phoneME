/*
 * @(#)QtGraphics.h	1.15 06/10/25
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
#ifndef _QTGRAPHICS_H_
#define _QTGRAPHICS_H_

#include "awt.h"
#include <qobject.h>
#include <qpainter.h>
#include <time.h>
#include <qrect.h>
#include <qpoint.h>
#include "QpWidget.h"
//#include "QtPainter.h"

class ImageRepresentation;
/*
 * This widget monitor is used by the QtGraphics class to monitor when the
 * widget is about to be deleted in order to invalidate its painter.
 *
 * On the Zaurus/Warrior/qt-emb-2.3.2 devices, it also monitors when the
 * widget's position or size has changed and revalidates its painter.
 */
class WidgetMonitor : public QObject
{
 public:
    WidgetMonitor() {}
    ~WidgetMonitor() {}

 protected:
    bool eventFilter( QObject *o, QEvent *e );
};
     
class QtGraphics
{
    friend class WidgetMonitor;
    friend class QtComponentPeer; // 6322215

 public:
    // QtGraphics::init() is called from Java_sun_awt_qt_QtGraphics_initIDs().
    static void init();
    static bool isaWidget(QPaintDevice* pd);
    static QColor *getColor(JNIEnv *env, jobject color);

    QtGraphics(QpWidget *widget, ImageRepresentation *imgRep = NULL);
    QtGraphics(QPaintDevice *paintDev, ImageRepresentation *imgRep = NULL);
    QtGraphics(QtGraphics& src);
    ~QtGraphics();

    QColor* getCurrentColor();
    void setColor(QColor* color);
    void setXORMode(QColor* xorColor);
    void setPaintMode();
    bool hasClipping();
    bool hasPaintDevice();
    void setClip(int x, int y, int w, int h);
    void removeClip();
    QRect* getClipBounds();

    void drawLine(int left, int top, int right, int bottom);
    void drawRect(int left, int top, int width, int height, bool fill);
    void clearRect(int left, int top, int width, int height,
                   QColor *backgroundColor);
    void drawArc(int left, int top, int width, int height, int
		 startAngle, int endAngle);
    void fillArc(int left, int top, int width, int height, int
		 startAngle, int endAngle);
    void drawRoundRect(int left, int top, int width, int height, int
		       arcWidth, int arcHeight);
    void fillRoundRect(int x, int y, int width, int height, int
		       arcWidth, int arcHeight);
    void drawString(QFont *font, int left, int top, QString& text, 
		    int length);
    /* drawPolyline is an un-used function, but kept as a reference.   */
    /* void drawPolyline(QPointArray& points, int index, int nPoints); */
    void drawPolygon(QPointArray& points, int nPoints, bool fill, bool close);
    void drawPixmap(int x, int y, QPixmap &pixmap, int xsrc = 0,
                    int ysrc = 0, int width = -1, int height = -1);
    void copyArea(int x, int y, int width, int height, int dx, int dy);
    void setLineProperties(int lineWidth, int joinStyle, int capStyle);

 private:
    // Note : Since we cannot call one constructor from another in C++
    //        as in java, we create this method and put the object 
    //        initialization here. QtGraphics(QpWidget,...) and 
    //        QtGraphics(QPaintDevice,...) calls this method.
    void initObject(QPaintDevice *paintDev, 
                    ImageRepresentation *imgRep = NULL);

    // These static methods facilitate painter cacheing
    static void RefPaintDevice(QPaintDevice* pd, QtGraphics* g);
    static void UnrefPaintDevice(QPaintDevice* pd, QtGraphics* g);
    static void UnsetPaintDevice(QPaintDevice* pd);
    static void CleanupPaintDevice(QPaintDevice* pd);
    static QPainter* GetPainter(QPaintDevice* pd);
    static void EndPaintingWidget(QPaintDevice* pd);

    // Variables used for painter cacheing.
    static QPainter widgetPainter;
    static QPoint widgetPos; /* The widget position when
				QPainter::begin() is called. Changes
				in widget position are not necessarily
				covered by paint events. */
    static QPainter otherPainter;

    // The widget monitor to manage the painter cache by watching the lifecycle
    // events of the widgets.
    static WidgetMonitor* singletonWM;

    void updateColor();
    void setPainterMode(QPainter *painter);
    void setPainterPen(QPainter *painter);
    void setPainterBrush(QPainter *painter);
    void setPainterClipRect(QPainter *painter);
    QPainter *getPainter();
    void prepareDraw();
    void finishDraw(QPainter *painter = NULL);
    void drawArc0(int left, int top, int width, int height, int
                  startAngle, int endAngle, QPainter* painter);

    QPaintDevice *paintDevice;
    QColor *color;
    QColor *xorColor;
    QPen *painterPen;
    QBrush *painterBrush;
    QRect *clipRect;
    ImageRepresentation *imgRep;
    QPaintDevice *lastDevice;
    float floatLineWidth;
    int lineWidth;
    Qt::PenCapStyle capStyle;
    Qt::PenJoinStyle joinStyle;
    
};

#endif
